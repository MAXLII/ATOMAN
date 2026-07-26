%% CLLC 2P2Z 控制器独立设计脚本
% 本脚本根据 CLLC 调频被控对象设计 2P2Z 控制器，并给出连续域参数、
% 离散域系数、开环稳定裕度和闭环波特图。脚本不依赖 Control System
% Toolbox，可直接作为参数设计和交叉验证工具使用。
%
% 设计的连续域控制器为：
%
%                         (1 + s/wz)^2
%   Gc(s) = K * ---------------------------------
%                    s * (1 + s/wp)^2
%
% 其中 wz = 2*pi*fz，wp = 2*pi*fp。它包含原点积分极点、两个重合
% 零点和两个重合高频极点。K、fz、fp 是连续域设计参数。
%
% -------------------------------------------------------------------------
% 2P2Z 参数计算公式
% -------------------------------------------------------------------------
% 设目标交越角频率 wc = 2*pi*fc，被控对象在 wc 处为：
%
%   Gp(jwc) = |Gp(jwc)| * exp(j*phi_p)
%
% 1. 零点配置
%    默认用两个零点补偿输出端主极点：
%
%       tau_o = Rload*Cout/2
%       fz = 1/(2*pi*tau_o)
%
% 2. 目标控制器相位
%
%       phi_c_req = -180 deg + PM_target - phi_p
%
% 3. 2P2Z 在 wc 处的相位
%
%       phi_c(wc) = -90 deg
%                    + 2*atan(wc/wz)
%                    - 2*atan(wc/wp)
%
%    因此每个高频极点需要提供的滞后角为：
%
%       theta_p = [-90 deg + 2*atan(wc/wz) - phi_c_req]/2
%
%    高频极点为：
%
%       wp = wc/tan(theta_p)
%       fp = fc/tan(theta_p)
%
% 4. 用交越频率处的 0 dB 条件计算增益：
%
%       B(jwc) = (1 + jwc/wz)^2 /
%                 [jwc*(1 + jwc/wp)^2]
%
%       K = 1 / (|Gp(jwc)|*|B(jwc)|)
%
% 脚本从最大目标 fc 向下搜索，选择同时满足目标相位裕度、增益裕度、
% 高频极点上限和数字控制频率约束的最高交越频率。
%
% 注意：默认被控对象是 FHA 稳态增益斜率加输出功率平衡极点的低频
% 包络模型，适合初始参数设计。最终参数需要使用 PLECS 小信号注入或
% 实机 FRA 测得的 Gp(jw) 重新设计和确认。
%
% 当前仓库 code/lib/z2p2.c 的 n2、n3 公式以及 z2p2_cal() 的误差历史
% 移位与本脚本所列标准 2P2Z/Tustin 公式不一致。本脚本输出数学上正确
% 的离散系数；在直接调用 z2p2_init() 前，应先修正并验证嵌入式实现。

clear;
clc;
close all;

%% 用户可调设计要求
maximum_crossover_hz = 3.5e3;       % 允许的最大交越频率 / Hz
minimum_crossover_ratio = 0.2;      % 最低搜索频率相对最大 fc 的比例
target_phase_margin_deg = 50;       % 目标相位裕度 / degree
target_gain_margin_db = 6;          % 目标增益裕度 / dB
minimum_phase_margin_deg = 45;      % 最终验收下限 / degree
minimum_gain_margin_db = 6;         % 最终验收下限 / dB
maximum_pole_switching_ratio = 0.2; % fp 不超过工作开关频率的比例
maximum_pole_update_ratio = 0.2;    % fp 不超过数字更新频率的比例
delay_cycles = 1;                   % 调制和计算总延迟 / 开关周期

% 默认同时设计正向额定功率和反向可达最大设计功率。
design_cases(1) = struct( ...
    'name', 'Forward', ...
    'direction', 'forward', ...
    'input_voltage_v', 450, ...
    'output_voltage_v', 48, ...
    'output_power_w', 6600, ...
    'output_capacitance_f', 10000e-6);
design_cases(2) = struct( ...
    'name', 'Reverse', ...
    'direction', 'reverse', ...
    'input_voltage_v', 48, ...
    'output_voltage_v', 450, ...
    'output_power_w', 4000, ...
    'output_capacitance_f', 1360e-6);

% CLLC 谐振网络参数
cllc.Lr_primary = 40e-6;
cllc.Cr_primary = 80e-9;
cllc.Lm_primary = 200e-6;
cllc.Lr_secondary = 0.625e-6;
cllc.Cr_secondary = 5.12e-6;
cllc.Np = 24;
cllc.Ns = 3;
cllc.turns_ratio = cllc.Np / cllc.Ns;

% 稳态开关频率扫描用于寻找 CLLC 工作点和 dVout/dfs。
switching_frequency_hz = linspace(20e3, 260e3, 4801).';

% 控制频响扫描。最高频率还会受数字更新频率的 Nyquist 条件约束。
control_frequency_hz = logspace(-2, log10(30e3), 4001).';

%% 逐方向设计 2P2Z
case_count = numel(design_cases);
results = repmat(struct(), 1, case_count);

for case_index = 1:case_count
    design_case = design_cases(case_index);

    operating_point = build_cllc_operating_point( ...
        design_case, cllc, switching_frequency_hz);

    if ~isfinite(operating_point.switching_frequency_hz)
        error(['%s %.0f W 没有找到目标电压增益工作点。请扩大开关频率', ...
               '扫描范围、降低功率或检查输入输出电压。'], ...
              design_case.name, design_case.output_power_w);
    end

    % 默认控制器每个开关周期更新一次。若实际控制中断分频，应把这里
    % 改成真实的控制更新频率。
    update_frequency_hz = operating_point.switching_frequency_hz;
    sample_time_s = 1 / update_frequency_hz;
    nyquist_limit_hz = 0.45 * update_frequency_hz;
    valid_control_frequency = control_frequency_hz <= nyquist_limit_hz;
    design_frequency_hz = control_frequency_hz(valid_control_frequency);

    plant = build_envelope_plant( ...
        design_frequency_hz, operating_point, design_case, delay_cycles);

    zero_hz = 1 / (2*pi*operating_point.output_time_constant_s);
    maximum_pole_hz = min( ...
        maximum_pole_switching_ratio * ...
            operating_point.switching_frequency_hz, ...
        maximum_pole_update_ratio * update_frequency_hz);

    design = design_repeated_2p2z( ...
        design_frequency_hz, plant, zero_hz, ...
        maximum_crossover_hz, minimum_crossover_ratio, ...
        target_phase_margin_deg, target_gain_margin_db, ...
        maximum_pole_hz, sample_time_s);

    coefficients = calculate_z2p2_coefficients( ...
        design.K, design.zero_hz, design.pole_hz, sample_time_s);

    digital_controller = evaluate_discrete_controller( ...
        design_frequency_hz, sample_time_s, coefficients);
    digital_loop = plant .* digital_controller;
    digital_closed_loop = digital_loop ./ (1 + digital_loop);
    [digital_fc_hz, digital_pm_deg, digital_gm_db] = ...
        analyze_loop_margins(design_frequency_hz, digital_loop);

    design.digital_crossover_hz = digital_fc_hz;
    design.digital_phase_margin_deg = digital_pm_deg;
    design.digital_gain_margin_db = digital_gm_db;
    design.meets_requirements = ...
        digital_pm_deg >= minimum_phase_margin_deg && ...
        digital_gm_db >= minimum_gain_margin_db;

    results(case_index).case = design_case;
    results(case_index).operating_point = operating_point;
    results(case_index).frequency_hz = design_frequency_hz;
    results(case_index).plant = plant;
    results(case_index).design = design;
    results(case_index).sample_time_s = sample_time_s;
    results(case_index).coefficients = coefficients;
    results(case_index).digital_controller = digital_controller;
    results(case_index).digital_loop = digital_loop;
    results(case_index).digital_closed_loop = digital_closed_loop;

    print_design_result(results(case_index));
    plot_design_result(results(case_index));
end

%% 汇总表
direction = string({design_cases.name}).';
power_w = [design_cases.output_power_w].';
fsw_khz = arrayfun( ...
    @(x) x.operating_point.switching_frequency_hz / 1e3, results).';
K = arrayfun(@(x) x.design.K, results).';
fz_hz = arrayfun(@(x) x.design.zero_hz, results).';
fp_hz = arrayfun(@(x) x.design.pole_hz, results).';
fc_khz = arrayfun( ...
    @(x) x.design.digital_crossover_hz / 1e3, results).';
PM_deg = arrayfun( ...
    @(x) x.design.digital_phase_margin_deg, results).';
GM_db = arrayfun(@(x) x.design.digital_gain_margin_db, results).';
meets = arrayfun(@(x) x.design.meets_requirements, results).';

design_summary = table( ...
    direction, power_w, fsw_khz, K, fz_hz, fp_hz, ...
    fc_khz, PM_deg, GM_db, meets, ...
    'VariableNames', { ...
        'Direction', 'Power_W', 'Fsw_kHz', 'K', 'Fz_Hz', 'Fp_Hz', ...
        'DigitalFc_kHz', 'DigitalPM_deg', 'DigitalGM_dB', 'Meets'});

fprintf('\n================ 2P2Z design summary ================\n');
disp(design_summary);

%% 局部函数
function operating_point = build_cllc_operating_point( ...
    design_case, cllc, switching_frequency_hz)
%BUILD_CLLC_OPERATING_POINT 用 FHA 求工作频率及稳态频率增益斜率。

    n = cllc.turns_ratio;
    omega = 2*pi*switching_frequency_hz;
    Zr_primary = 1j*omega*cllc.Lr_primary + ...
        1 ./ (1j*omega*cllc.Cr_primary);
    Zm_primary = 1j*omega*cllc.Lm_primary;
    Zr_secondary = 1j*omega*cllc.Lr_secondary + ...
        1 ./ (1j*omega*cllc.Cr_secondary);

    load_resistance_ohm = ...
        design_case.output_voltage_v^2 / design_case.output_power_w;
    ac_load_resistance_ohm = (8/pi^2)*load_resistance_ohm;
    resonant_frequency_hz = 1 / ...
        (2*pi*sqrt(cllc.Lr_primary*cllc.Cr_primary));

    if strcmpi(design_case.direction, 'forward')
        Zsecondary_total = Zr_secondary + ac_load_resistance_ohm;
        Zsecondary_referred = n^2*Zsecondary_total;
        Zparallel = (Zm_primary.*Zsecondary_referred) ./ ...
            (Zm_primary + Zsecondary_referred);
        Hprimary = Zparallel ./ (Zr_primary + Zparallel);
        Hsecondary = ac_load_resistance_ohm ./ Zsecondary_total;
        dc_gain = abs(Hprimary.*Hsecondary/n);
        normalized_gain = n*dc_gain;
        target_normalized_gain = ...
            n*design_case.output_voltage_v/design_case.input_voltage_v;
        operating_frequency_hz = find_gain_crossing( ...
            switching_frequency_hz, normalized_gain, ...
            target_normalized_gain, resonant_frequency_hz, 'above');
    elseif strcmpi(design_case.direction, 'reverse')
        Zprimary_total = Zr_primary + ac_load_resistance_ohm;
        Zparallel_primary = (Zm_primary.*Zprimary_total) ./ ...
            (Zm_primary + Zprimary_total);
        Zparallel_referred_secondary = Zparallel_primary/n^2;
        Hsecondary = Zparallel_referred_secondary ./ ...
            (Zr_secondary + Zparallel_referred_secondary);
        Hprimary = ac_load_resistance_ohm ./ Zprimary_total;
        dc_gain = abs(Hsecondary.*Hprimary*n);
        normalized_gain = dc_gain/n;
        target_normalized_gain = ...
            design_case.output_voltage_v / ...
            (n*design_case.input_voltage_v);
        operating_frequency_hz = find_gain_crossing( ...
            switching_frequency_hz, normalized_gain, ...
            target_normalized_gain, resonant_frequency_hz, 'below');
    else
        error('direction 必须为 ''forward'' 或 ''reverse''。');
    end

    output_voltage_v = design_case.input_voltage_v*dc_gain;
    voltage_slope_v_per_hz = gradient( ...
        output_voltage_v, switching_frequency_hz);
    Kvf_v_per_hz = interp1( ...
        switching_frequency_hz, voltage_slope_v_per_hz, ...
        operating_frequency_hz, 'linear');

    operating_point.load_resistance_ohm = load_resistance_ohm;
    operating_point.ac_load_resistance_ohm = ac_load_resistance_ohm;
    operating_point.switching_frequency_hz = operating_frequency_hz;
    operating_point.Kvf_v_per_hz = Kvf_v_per_hz;
    operating_point.output_time_constant_s = ...
        load_resistance_ohm*design_case.output_capacitance_f/2;
end

function crossing_hz = find_gain_crossing( ...
    frequency_hz, gain, target, resonant_frequency_hz, frequency_side)
%FIND_GAIN_CROSSING 查找最接近谐振点的指定增益交点。

    if strcmpi(frequency_side, 'above')
        valid_index = find(frequency_hz >= resonant_frequency_hz);
    elseif strcmpi(frequency_side, 'below')
        valid_index = flip(find(frequency_hz <= resonant_frequency_hz));
    else
        error('frequency_side 必须为 ''above'' 或 ''below''。');
    end

    gain_error = gain(valid_index) - target;
    crossing_pair = find( ...
        gain_error(1:end-1).*gain_error(2:end) <= 0, 1, 'first');

    if isempty(crossing_pair)
        crossing_hz = NaN;
        return;
    end

    index_1 = valid_index(crossing_pair);
    index_2 = valid_index(crossing_pair + 1);
    frequency_pair = frequency_hz([index_1, index_2]);
    gain_pair = gain([index_1, index_2]);

    crossing_hz = frequency_pair(1) + ...
        (target - gain_pair(1))*diff(frequency_pair)/diff(gain_pair);
end

function plant = build_envelope_plant( ...
    frequency_hz, operating_point, design_case, delay_cycles)
%BUILD_ENVELOPE_PLANT 建立控制量到输出电压的低频包络模型。
%
%   Gp(s) = (-Kvf)/(1 + s*tau_o) * exp(-s*Td)
%   tau_o = Rload*Cout/2
%   Td = delay_cycles/fsw
%
% 控制执行方向定义为 fs = fs_bias - u，所以 Gp 使用 -Kvf。

    s = 1j*2*pi*frequency_hz;
    delay_s = delay_cycles/operating_point.switching_frequency_hz;
    plant = (-operating_point.Kvf_v_per_hz) ./ ...
        (1 + s*operating_point.output_time_constant_s) .* ...
        exp(-s*delay_s);

    if any(~isfinite(plant)) || abs(plant(1)) == 0
        error('%s 被控对象无效，请检查工作点和频率增益斜率。', ...
              design_case.name);
    end
end

function design = design_repeated_2p2z( ...
    frequency_hz, plant, zero_hz, maximum_crossover_hz, ...
    minimum_crossover_ratio, target_pm_deg, target_gm_db, ...
    maximum_pole_hz, sample_time_s)
%DESIGN_REPEATED_2P2Z 搜索离散实现满足 PM、GM 的最高交越频率。

    crossover_candidates_hz = logspace( ...
        log10(maximum_crossover_hz), ...
        log10(maximum_crossover_hz*minimum_crossover_ratio), 81);
    selected = struct();

    for candidate_index = 1:numel(crossover_candidates_hz)
        candidate_fc_hz = crossover_candidates_hz(candidate_index);
        candidate = synthesize_at_crossover( ...
            frequency_hz, plant, zero_hz, candidate_fc_hz, ...
            target_pm_deg, maximum_pole_hz);
        [actual_fc_hz, actual_pm_deg, actual_gm_db] = ...
            analyze_loop_margins(frequency_hz, candidate.loop);

        candidate.crossover_hz = actual_fc_hz;
        candidate.phase_margin_deg = actual_pm_deg;
        candidate.gain_margin_db = actual_gm_db;

        candidate_coefficients = calculate_z2p2_coefficients( ...
            candidate.K, candidate.zero_hz, candidate.pole_hz, ...
            sample_time_s);
        candidate_digital_controller = evaluate_discrete_controller( ...
            frequency_hz, sample_time_s, candidate_coefficients);
        candidate_digital_loop = plant.*candidate_digital_controller;
        [candidate_digital_fc_hz, candidate_digital_pm_deg, ...
         candidate_digital_gm_db] = analyze_loop_margins( ...
            frequency_hz, candidate_digital_loop);
        candidate.design_digital_crossover_hz = candidate_digital_fc_hz;
        candidate.design_digital_phase_margin_deg = candidate_digital_pm_deg;
        candidate.design_digital_gain_margin_db = candidate_digital_gm_db;
        selected = candidate;

        if candidate_digital_pm_deg >= target_pm_deg && ...
           candidate_digital_gm_db >= target_gm_db
            break;
        end
    end

    design = selected;
end

function design = synthesize_at_crossover( ...
    frequency_hz, plant, zero_hz, crossover_hz, ...
    target_pm_deg, maximum_pole_hz)
%SYNTHESIZE_AT_CROSSOVER 根据相位条件和 0 dB 条件计算 fp、K。

    plant_phase_deg = rad2deg(unwrap(angle(plant)));
    plant_phase_at_fc_deg = interpolate_log_frequency( ...
        frequency_hz, plant_phase_deg, crossover_hz);
    required_controller_phase_deg = ...
        -180 + target_pm_deg - plant_phase_at_fc_deg;

    zero_phase_deg = 2*atand(crossover_hz/zero_hz);
    required_pole_angle_deg = ...
        (-90 + zero_phase_deg - required_controller_phase_deg)/2;

    % theta_p 必须处于 (0, 90) degree。fp >= fc 可减少交越频率附近的
    % 高频衰减，因此这里再将 theta_p 限制到不大于 45 degree。
    minimum_pole_angle_deg = atand(crossover_hz/maximum_pole_hz);
    pole_angle_deg = min(max(required_pole_angle_deg, ...
        minimum_pole_angle_deg), 45);
    pole_hz = crossover_hz/tand(pole_angle_deg);

    s = 1j*2*pi*frequency_hz;
    base_controller = (1 + s/(2*pi*zero_hz)).^2 ./ ...
        (s.*(1 + s/(2*pi*pole_hz)).^2);
    plant_gain_at_fc = interpolate_log_frequency( ...
        frequency_hz, abs(plant), crossover_hz);
    base_gain_at_fc = interpolate_log_frequency( ...
        frequency_hz, abs(base_controller), crossover_hz);
    K = 1/(plant_gain_at_fc*base_gain_at_fc);

    controller = K*base_controller;
    loop = plant.*controller;

    design.K = K;
    design.zero_hz = zero_hz;
    design.pole_hz = pole_hz;
    design.requested_crossover_hz = crossover_hz;
    design.required_controller_phase_deg = required_controller_phase_deg;
    design.pole_angle_deg = pole_angle_deg;
    design.controller = controller;
    design.loop = loop;
    design.closed_loop = loop./(1 + loop);
end

function value = interpolate_log_frequency(frequency_hz, data, query_hz)
%INTERPOLATE_LOG_FREQUENCY 在对数频率轴上线性插值。

    value = interp1(log10(frequency_hz), data, log10(query_hz), 'linear');
end

function coefficients = calculate_z2p2_coefficients(K, fz, fp, ts)
%CALCULATE_Z2P2_COEFFICIENTS 用标准双线性变换计算离散 2P2Z 系数。
%
% 差分方程：
%   u[n] = b0*e[n] + b1*e[n-1] + b2*e[n-2] + b3*e[n-3]
%          - a1*u[n-1] - a2*u[n-2] - a3*u[n-3]

% 该离散化使用 s = (2/Ts)*(1-z^-1)/(1+z^-1)。

    wz1 = 2*pi*fz;
    wz2 = wz1;
    wp1 = 2*pi*fp;
    wp2 = wp1;

    n3 = K*ts*wp1*wp2*(4 - 2*ts*wz1 - 2*ts*wz2 + ts^2*wz1*wz2);
    n2 = K*ts*wp1*wp2*(-4 - 2*ts*wz1 - 2*ts*wz2 + 3*ts^2*wz1*wz2);
    n1 = K*ts*wp1*wp2*(-4 + 2*ts*wz1 + 2*ts*wz2 + 3*ts^2*wz1*wz2);
    n0 = K*ts*wp1*wp2*(4 + 2*ts*wz1 + 2*ts*wz2 + ts^2*wz1*wz2);

    d3 = 2*(-4 + 2*ts*wp1 + 2*ts*wp2 - ts^2*wp1*wp2)*wz1*wz2;
    d2 = 2*(12 - 2*ts*wp1 - 2*ts*wp2 - ts^2*wp1*wp2)*wz1*wz2;
    d1 = 2*(-12 - 2*ts*wp1 - 2*ts*wp2 + ts^2*wp1*wp2)*wz1*wz2;
    d0 = 2*(4 + 2*ts*wp1 + 2*ts*wp2 + ts^2*wp1*wp2)*wz1*wz2;

    coefficients.a1 = d1/d0;
    coefficients.a2 = d2/d0;
    coefficients.a3 = d3/d0;
    coefficients.b0 = n0/d0;
    coefficients.b1 = n1/d0;
    coefficients.b2 = n2/d0;
    coefficients.b3 = n3/d0;
end

function controller = evaluate_discrete_controller( ...
    frequency_hz, sample_time_s, coefficients)
%EVALUATE_DISCRETE_CONTROLLER 计算离散差分方程的单位圆频率响应。

    z_inverse = exp(-1j*2*pi*frequency_hz*sample_time_s);
    numerator = coefficients.b0 + ...
        coefficients.b1*z_inverse + ...
        coefficients.b2*z_inverse.^2 + ...
        coefficients.b3*z_inverse.^3;
    denominator = 1 + ...
        coefficients.a1*z_inverse + ...
        coefficients.a2*z_inverse.^2 + ...
        coefficients.a3*z_inverse.^3;
    controller = numerator./denominator;
end

function [crossover_hz, phase_margin_deg, gain_margin_db] = ...
    analyze_loop_margins(frequency_hz, loop)
%ANALYZE_LOOP_MARGINS 检查全部增益和相位交点并返回最差裕度。

    magnitude_db = 20*log10(abs(loop));
    phase_deg = rad2deg(unwrap(angle(loop)));
    valid = isfinite(magnitude_db) & isfinite(phase_deg);

    if nnz(valid) < 2
        crossover_hz = NaN;
        phase_margin_deg = NaN;
        gain_margin_db = NaN;
        return;
    end

    frequency_valid = frequency_hz(valid);
    magnitude_valid = magnitude_db(valid);
    phase_valid = phase_deg(valid);

    gain_crossings_hz = find_log_crossings( ...
        frequency_valid, magnitude_valid, 0);
    if isempty(gain_crossings_hz)
        crossover_hz = NaN;
        phase_margin_deg = NaN;
    else
        phases_at_crossings = interp1( ...
            log10(frequency_valid), phase_valid, ...
            log10(gain_crossings_hz), 'linear');
        phase_margins_deg = 180 + phases_at_crossings;
        [phase_margin_deg, worst_index] = min(phase_margins_deg);
        crossover_hz = gain_crossings_hz(worst_index);
    end

    phase_crossings_hz = find_phase_crossings( ...
        frequency_valid, phase_valid);
    if isempty(phase_crossings_hz)
        gain_margin_db = Inf;
    else
        magnitudes_at_crossings = interp1( ...
            log10(frequency_valid), magnitude_valid, ...
            log10(phase_crossings_hz), 'linear');
        gain_margin_db = min(-magnitudes_at_crossings);
    end
end

function crossings_hz = find_log_crossings(frequency_hz, data, target)
%FIND_LOG_CROSSINGS 查找数据与目标值的全部对数频率交点。

    relative_data = data - target;
    pair_index = find(relative_data(1:end-1).*relative_data(2:end) <= 0);
    crossings_hz = zeros(numel(pair_index), 1);

    for k = 1:numel(pair_index)
        index = pair_index(k);
        x_pair = log10(frequency_hz(index:index + 1));
        y_pair = relative_data(index:index + 1);
        if y_pair(1) == y_pair(2)
            crossings_hz(k) = 10^x_pair(1);
        else
            crossings_hz(k) = 10^(x_pair(1) - ...
                y_pair(1)*diff(x_pair)/diff(y_pair));
        end
    end

    crossings_hz = unique(crossings_hz);
end

function crossings_hz = find_phase_crossings(frequency_hz, phase_deg)
%FIND_PHASE_CROSSINGS 查找 -180、-540 等负实轴相位交点。

    minimum_phase = min(phase_deg);
    maximum_phase = max(phase_deg);
    first_order = ceil((-maximum_phase - 180)/360);
    last_order = floor((-minimum_phase - 180)/360);
    phase_orders = first_order:last_order;
    crossing_groups = cell(numel(phase_orders), 1);

    for order_index = 1:numel(phase_orders)
        order = phase_orders(order_index);
        target_phase = -180 - 360*order;
        crossing_groups{order_index} = find_log_crossings( ...
            frequency_hz, phase_deg, target_phase);
    end

    crossings_hz = vertcat(crossing_groups{:});
    crossings_hz = unique(crossings_hz);
end

function print_design_result(result)
%PRINT_DESIGN_RESULT 输出可直接抄入 z2p2_init() 的设计结果。

    c = result.coefficients;
    d = result.design;
    op = result.operating_point;

    fprintf('\n================ %s %.0f W ================\n', ...
        result.case.name, result.case.output_power_w);
    fprintf('Rload = %.6g Ohm, fsw = %.3f kHz\n', ...
        op.load_resistance_ohm, op.switching_frequency_hz/1e3);
    fprintf('Kvf = %.9g V/Hz, tau_o = %.6g ms\n', ...
        op.Kvf_v_per_hz, op.output_time_constant_s*1e3);
    fprintf('2P2Z: K = %.9g, fz = %.9g Hz, fp = %.9g Hz\n', ...
        d.K, d.zero_hz, d.pole_hz);
    fprintf(['Continuous parameters: k = %.9g, fz = %.9g Hz, ', ...
             'fp = %.9g Hz, ts = %.9g s\n'], ...
        d.K, d.zero_hz, d.pole_hz, result.sample_time_s);
    fprintf('a1 = %.9g, a2 = %.9g, a3 = %.9g\n', c.a1, c.a2, c.a3);
    fprintf('b0 = %.9g, b1 = %.9g, b2 = %.9g, b3 = %.9g\n', ...
        c.b0, c.b1, c.b2, c.b3);
    fprintf('Digital loop: fc = %.3f Hz, PM = %.3f deg, GM = %.3f dB\n', ...
        d.digital_crossover_hz, d.digital_phase_margin_deg, ...
        d.digital_gain_margin_db);
end

function plot_design_result(result)
%PLOT_DESIGN_RESULT 绘制被控对象、控制器、开环和闭环波特图。

    frequency_hz = result.frequency_hz;
    plant = result.plant;
    analog_controller = result.design.controller;
    digital_controller = result.digital_controller;
    analog_loop = result.design.loop;
    digital_loop = result.digital_loop;
    analog_closed_loop = result.design.closed_loop;
    digital_closed_loop = result.digital_closed_loop;

    figure('Color', 'w', 'Name', [result.case.name, ' 2P2Z design']);
    layout = tiledlayout(4, 2, 'TileSpacing', 'compact', ...
        'Padding', 'compact');

    plot_bode_row(layout, frequency_hz, plant, [], ...
        'Plant G_p', 'Plant');
    plot_bode_row(layout, frequency_hz, analog_controller, ...
        digital_controller, '2P2Z controller', ...
        'Continuous', 'Discrete');
    plot_bode_row(layout, frequency_hz, analog_loop, digital_loop, ...
        'Open loop G_cG_p', 'Continuous', 'Discrete');
    plot_bode_row(layout, frequency_hz, analog_closed_loop, ...
        digital_closed_loop, 'Closed loop G_cG_p/(1+G_cG_p)', ...
        'Continuous', 'Discrete');

    title(layout, sprintf('%s %.0f W 2P2Z design', ...
        result.case.name, result.case.output_power_w));
end

function plot_bode_row(layout, frequency_hz, response_1, response_2, ...
    row_title, label_1, label_2)
%PLOT_BODE_ROW 绘制一行幅频和相频曲线。

    magnitude_axes = nexttile(layout);
    phase_axes = nexttile(layout);
    set(magnitude_axes, 'XScale', 'log');
    set(phase_axes, 'XScale', 'log');
    hold(magnitude_axes, 'on');
    hold(phase_axes, 'on');

    semilogx(magnitude_axes, frequency_hz, ...
        20*log10(abs(response_1)), 'LineWidth', 1.5, ...
        'DisplayName', label_1);
    semilogx(phase_axes, frequency_hz, ...
        rad2deg(unwrap(angle(response_1))), 'LineWidth', 1.5, ...
        'DisplayName', label_1);

    if ~isempty(response_2)
        semilogx(magnitude_axes, frequency_hz, ...
            20*log10(abs(response_2)), '--', 'LineWidth', 1.3, ...
            'DisplayName', label_2);
        semilogx(phase_axes, frequency_hz, ...
            rad2deg(unwrap(angle(response_2))), '--', 'LineWidth', 1.3, ...
            'DisplayName', label_2);
    end

    yline(magnitude_axes, 0, ':k', 'HandleVisibility', 'off');
    yline(phase_axes, -180, ':k', 'HandleVisibility', 'off');
    grid(magnitude_axes, 'on');
    grid(phase_axes, 'on');
    ylabel(magnitude_axes, 'Magnitude / dB');
    ylabel(phase_axes, 'Phase / degree');
    title(magnitude_axes, row_title);
    title(phase_axes, row_title);
    legend(magnitude_axes, 'Location', 'best');

    xlabel(magnitude_axes, 'Frequency / Hz');
    xlabel(phase_axes, 'Frequency / Hz');
end
