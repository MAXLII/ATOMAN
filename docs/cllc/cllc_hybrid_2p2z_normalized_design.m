%% CLLC PSM/PFM 混合调制归一化 2P2Z 设计
% 2P2Z 输入：输出电压误差 e = Vref - Vout，单位 V。
% 2P2Z 输出：归一化混合调制指令 u，范围 0~1。
%
% 调制映射：
%
%   0 <= u < ut：PSM，固定 fs = fmax，D = 0.5*u/ut
%   ut <= u <= 1：PFM，D = 0.5，频率从 fmax 降到 fr
%
%                  u - ut
%   fs(u) = fmax - -------- * (fmax - fr)
%                  1 - ut
%
% 其中 fmax = 2*fr。u 增大时输出能力增强：PSM 占空比增大；进入 PFM
% 后开关频率下降并趋近串联谐振频率。
%
% -------------------------------------------------------------------------
% PSM/PFM 分界点 ut 的设计
% -------------------------------------------------------------------------
% 令 PSM 段与 PFM 段的平均输出电压控制灵敏度相同：
%
%   DeltaV_PSM/ut = DeltaV_PFM/(1-ut)
%
% 得到：
%
%            DeltaV_PSM
%   ut = ---------------------------
%        DeltaV_PSM + DeltaV_PFM
%
% 若 PSM 在 D=0 时输出近似为 0，则：
%
%   DeltaV_PSM = Vout(fmax)
%   DeltaV_PFM = Vout(fr) - Vout(fmax)
%   ut = Vout(fmax)/Vout(fr)
%
% 该方法让同一个归一化控制量在两个调制区间具有接近的平均静态增益。
% 实际 PSM 基波增益与相移角是非线性的，所以脚本还会画出 dVout/du；
% 最终应使用 PLECS 小信号注入或实机 FRA 校核切换点两侧的增益。
% 本设计只有一个 2P2Z 和一个无状态分段映射，不使用模式迟滞。
%
% -------------------------------------------------------------------------
% 归一化 PFM 被控对象
% -------------------------------------------------------------------------
% 在额定工作点：
%
%   dfs/du = -(fmax-fr)/(1-ut)
%   Kvf = dVout/dfs
%   Ku  = dVout/du = Kvf*dfs/du
%
%   Gp,u(s) = Ku/(1+s*tau_o)*exp(-s*Td)
%   tau_o = Rload*Cout/2
%
% 2P2Z 控制器：
%
%                         (1+s/wz)^2
%   Gc(s) = K * -------------------------------
%                    s*(1+s/wp)^2
%
% 其输出直接为 Delta u/归一化控制量，不再是 Hz。脚本使用离散实现的
% PM、GM 作为交越频率搜索约束，并输出标准 Tustin 差分系数。

clear;
clc;
close all;

%% 用户可调参数
Vin_v = 450;
Vout_reference_v = 48;
output_power_w = 6600;
Cout_f = 10000e-6;

maximum_crossover_hz = 3.5e3;
minimum_crossover_ratio = 0.2;
target_phase_margin_deg = 50;
target_gain_margin_db = 6;
maximum_pole_frequency_ratio = 0.2;
delay_cycles = 1;

% 单一离散 2P2Z 必须采用固定调用周期。如果控制器跟随变频 PWM 每周期
% 调用，则 Ts 会变化，不能继续使用一组固定离散系数。
control_update_frequency_hz = 100e3;

% 鲁棒性分析覆盖 0~1。精确切换点处，理想 PSM 基波模型的 dVout/du
% 为零，任何线性控制器都没有小信号控制权，因此仅从两侧逼近该点。
transition_guard_u = 1e-3;

% CLLC 参数
Lr_primary_h = 40e-6;
Cr_primary_f = 80e-9;
Lm_primary_h = 200e-6;
Lr_secondary_h = 0.625e-6;
Cr_secondary_f = 5.12e-6;
turns_ratio = 24/3;

%% FHA 静态增益与混合调制范围
fr_hz = 1/(2*pi*sqrt(Lr_primary_h*Cr_primary_f));
fmax_hz = 2*fr_hz;
switching_frequency_hz = linspace(fr_hz, fmax_hz, 4001).';

load_resistance_ohm = Vout_reference_v^2/output_power_w;
ac_load_resistance_ohm = (8/pi^2)*load_resistance_ohm;

[full_psm_output_voltage_v, dc_gain] = calculate_forward_fha_voltage( ...
    switching_frequency_hz, Vin_v, ac_load_resistance_ohm, ...
    Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);

voltage_at_fr_v = full_psm_output_voltage_v(1);
voltage_at_fmax_v = full_psm_output_voltage_v(end);
psm_voltage_span_v = voltage_at_fmax_v;
pfm_voltage_span_v = voltage_at_fr_v - voltage_at_fmax_v;

if psm_voltage_span_v <= 0 || pfm_voltage_span_v <= 0
    error('当前 FHA 增益不满足 fr~2fr 单调调节条件，无法设计混合分界点。');
end

transition_u = psm_voltage_span_v / ...
    (psm_voltage_span_v + pfm_voltage_span_v);

%% 建立 u=0~1 的调制映射和静态输出曲线
normalized_command = linspace(0, 1, 2001).';
psm_region = normalized_command <= transition_u;
pfm_region = ~psm_region;

phase_shift_duty = nan(size(normalized_command));
mapped_switching_frequency_hz = nan(size(normalized_command));
static_output_voltage_v = nan(size(normalized_command));

phase_shift_duty(psm_region) = ...
    0.5*normalized_command(psm_region)/transition_u;
mapped_switching_frequency_hz(psm_region) = fmax_hz;

% 全桥移相基波幅值近似为满方波基波的 sin(pi*D) 倍。
psm_fundamental_ratio = sin(pi*phase_shift_duty(psm_region));
static_output_voltage_v(psm_region) = ...
    voltage_at_fmax_v*psm_fundamental_ratio;

phase_shift_duty(pfm_region) = 0.5;
mapped_switching_frequency_hz(pfm_region) = fmax_hz - ...
    (normalized_command(pfm_region) - transition_u) / ...
    (1 - transition_u)*(fmax_hz - fr_hz);
static_output_voltage_v(pfm_region) = interp1( ...
    switching_frequency_hz, full_psm_output_voltage_v, ...
    mapped_switching_frequency_hz(pfm_region), 'linear');

static_sensitivity_v_per_u = gradient( ...
    static_output_voltage_v, normalized_command);

%% 额定电压工作点及归一化被控对象
operating_frequency_hz = find_gain_crossing_above_resonance( ...
    switching_frequency_hz, dc_gain, ...
    Vout_reference_v/Vin_v, fr_hz);

if ~isfinite(operating_frequency_hz)
    error('fr~2fr 范围内没有找到 %.3f V 的 PFM 工作点。', ...
          Vout_reference_v);
end

operating_u = transition_u + ...
    (fmax_hz - operating_frequency_hz)/(fmax_hz - fr_hz) * ...
    (1 - transition_u);

voltage_slope_v_per_hz = gradient( ...
    full_psm_output_voltage_v, switching_frequency_hz);
Kvf_v_per_hz = interp1( ...
    switching_frequency_hz, voltage_slope_v_per_hz, ...
    operating_frequency_hz, 'linear');
frequency_gain_hz_per_u = -(fmax_hz - fr_hz)/(1 - transition_u);
Ku_v_per_u = Kvf_v_per_hz*frequency_gain_hz_per_u;

if Ku_v_per_u <= 0
    error('归一化 PFM 被控对象增益不是正值，请检查控制方向。');
end

% 用一组 u 工作点建立被控对象族。只跳过精确切换点，因为理想 PSM
% 基波模型在 D=50% 时局部斜率为零，但仍从两侧逼近该点。
analysis_u = unique([ ...
    linspace(0, transition_u - transition_guard_u, 31), ...
    linspace(transition_u + transition_guard_u, 1, 61), ...
    operating_u]).';
analysis_switching_frequency_hz = nan(size(analysis_u));
analysis_plant_gain_v_per_u = nan(size(analysis_u));
analysis_mode = strings(size(analysis_u));

analysis_psm = analysis_u < transition_u;
analysis_pfm = ~analysis_psm;

analysis_switching_frequency_hz(analysis_psm) = fmax_hz;
analysis_duty = 0.5*analysis_u(analysis_psm)/transition_u;
analysis_plant_gain_v_per_u(analysis_psm) = ...
    voltage_at_fmax_v*pi*cos(pi*analysis_duty)*0.5/transition_u;
analysis_mode(analysis_psm) = "PSM";

analysis_switching_frequency_hz(analysis_pfm) = fmax_hz - ...
    (analysis_u(analysis_pfm) - transition_u)/(1 - transition_u) * ...
    (fmax_hz - fr_hz);
analysis_kvf_v_per_hz = interp1( ...
    switching_frequency_hz, voltage_slope_v_per_hz, ...
    analysis_switching_frequency_hz(analysis_pfm), 'linear');
analysis_plant_gain_v_per_u(analysis_pfm) = ...
    analysis_kvf_v_per_hz*frequency_gain_hz_per_u;
analysis_mode(analysis_pfm) = "PFM";

if any(analysis_plant_gain_v_per_u <= 0)
    error('0~1 范围内出现非正小信号增益，请检查混合映射方向。');
end

output_time_constant_s = load_resistance_ohm*Cout_f/2;
sample_time_s = 1/control_update_frequency_hz;

control_frequency_hz = logspace( ...
    -4, log10(0.45*control_update_frequency_hz), 5001).';
s = 1j*2*pi*control_frequency_hz;
plant_family = analysis_plant_gain_v_per_u.' ./ ...
    (1 + s*output_time_constant_s) .* ...
    exp(-s.*(delay_cycles./analysis_switching_frequency_hz.'));
[~, nominal_index] = min(abs(analysis_u - operating_u));
normalized_plant = plant_family(:, nominal_index);

%% 归一化 2P2Z 设计
zero_hz = 1/(2*pi*output_time_constant_s);
maximum_pole_hz = maximum_pole_frequency_ratio * ...
    min([analysis_switching_frequency_hz; control_update_frequency_hz]);

design = design_normalized_2p2z( ...
    control_frequency_hz, plant_family, zero_hz, ...
    maximum_crossover_hz, minimum_crossover_ratio, ...
    target_phase_margin_deg, target_gain_margin_db, ...
    maximum_pole_hz, sample_time_s);

coefficients = calculate_tustin_coefficients( ...
    design.K, design.zero_hz, design.pole_hz, sample_time_s);
digital_controller = evaluate_discrete_controller( ...
    control_frequency_hz, sample_time_s, coefficients);
analog_loop = normalized_plant.*design.controller;
analog_closed_loop = analog_loop./(1 + analog_loop);
digital_loop_family = plant_family.*digital_controller;
digital_loop = digital_loop_family(:, nominal_index);
digital_closed_loop = digital_loop./(1 + digital_loop);
digital_crossover_hz = nan(size(analysis_u));
digital_pm_deg = nan(size(analysis_u));
digital_gm_db = nan(size(analysis_u));
for point_index = 1:numel(analysis_u)
    [digital_crossover_hz(point_index), digital_pm_deg(point_index), ...
     digital_gm_db(point_index)] = analyze_loop_margins( ...
        control_frequency_hz, digital_loop_family(:, point_index));
end
worst_phase_margin_deg = min(digital_pm_deg);
worst_gain_margin_db = min(digital_gm_db);
nominal_crossover_hz = digital_crossover_hz(nominal_index);
nominal_pm_deg = digital_pm_deg(nominal_index);
nominal_gm_db = digital_gm_db(nominal_index);

robustness_table = table( ...
    analysis_u, analysis_mode, ...
    analysis_switching_frequency_hz/1e3, ...
    analysis_plant_gain_v_per_u, digital_crossover_hz, ...
    digital_pm_deg, digital_gm_db, ...
    'VariableNames', { ...
        'U', 'Mode', 'Fsw_kHz', 'PlantGain_V_per_U', ...
        'Crossover_Hz', 'PM_deg', 'GM_dB'});

%% 输出结果
fprintf('\n============= Normalized hybrid modulation =============\n');
fprintf('fr = %.6f kHz, fmax = %.6f kHz\n', fr_hz/1e3, fmax_hz/1e3);
fprintf('Vout(fr) = %.6f V, Vout(fmax) = %.6f V\n', ...
    voltage_at_fr_v, voltage_at_fmax_v);
fprintf('Designed transition ut = %.9f\n', transition_u);
fprintf('Single stateless mapping: no hysteresis\n');
fprintf('Rated operating point: u0 = %.9f, fs0 = %.6f kHz\n', ...
    operating_u, operating_frequency_hz/1e3);
fprintf('dfs/du = %.9g Hz/u, dVout/du = %.9g V/u\n', ...
    frequency_gain_hz_per_u, Ku_v_per_u);

fprintf('\n================ Normalized 2P2Z ========================\n');
fprintf('K = %.9g, fz = %.9g Hz, fp = %.9g Hz, Ts = %.9g s\n', ...
    design.K, design.zero_hz, design.pole_hz, sample_time_s);
fprintf('a1 = %.9g, a2 = %.9g, a3 = %.9g\n', ...
    coefficients.a1, coefficients.a2, coefficients.a3);
fprintf('b0 = %.9g, b1 = %.9g, b2 = %.9g, b3 = %.9g\n', ...
    coefficients.b0, coefficients.b1, ...
    coefficients.b2, coefficients.b3);
fprintf('Rated point: fc = %.6f kHz, PM = %.6f deg, GM = %.6f dB\n', ...
    nominal_crossover_hz/1e3, nominal_pm_deg, nominal_gm_db);
fprintf('0~1 family worst case: PM = %.6f deg, GM = %.6f dB\n', ...
    worst_phase_margin_deg, worst_gain_margin_db);
fprintf('Controller output limit: 0 <= u <= 1\n');
display_index = unique([1:10:height(robustness_table), ...
    nominal_index, height(robustness_table)]);
disp(robustness_table(display_index, :));

result.transition_u = transition_u;
result.operating_u = operating_u;
result.resonant_frequency_hz = fr_hz;
result.maximum_frequency_hz = fmax_hz;
result.operating_frequency_hz = operating_frequency_hz;
result.frequency_gain_hz_per_u = frequency_gain_hz_per_u;
result.plant_gain_v_per_u = Ku_v_per_u;
result.design = design;
result.sample_time_s = sample_time_s;
result.control_update_frequency_hz = control_update_frequency_hz;
result.coefficients = coefficients;
result.digital_crossover_hz = digital_crossover_hz;
result.digital_phase_margin_deg = digital_pm_deg;
result.digital_gain_margin_db = digital_gm_db;
result.worst_phase_margin_deg = worst_phase_margin_deg;
result.worst_gain_margin_db = worst_gain_margin_db;
result.robustness_table = robustness_table;
result.plant_family = plant_family;
result.normalized_command = normalized_command;
result.static_output_voltage_v = static_output_voltage_v;

%% 调制映射图
figure('Color', 'w', 'Name', 'Normalized hybrid modulation map');
map_layout = tiledlayout(3, 1, 'TileSpacing', 'compact', ...
    'Padding', 'compact');

frequency_axes = nexttile(map_layout);
yyaxis(frequency_axes, 'left');
plot(frequency_axes, normalized_command, ...
    mapped_switching_frequency_hz/1e3, 'LineWidth', 1.6);
ylabel(frequency_axes, 'Switching frequency / kHz');
yyaxis(frequency_axes, 'right');
plot(frequency_axes, normalized_command, phase_shift_duty, ...
    'LineWidth', 1.6);
ylabel(frequency_axes, 'PSM duty');
xline(frequency_axes, transition_u, '--k', 'u_t');
grid(frequency_axes, 'on');
title(frequency_axes, 'Normalized command to modulator');

voltage_axes = nexttile(map_layout);
plot(voltage_axes, normalized_command, static_output_voltage_v, ...
    'LineWidth', 1.6);
xline(voltage_axes, transition_u, '--k', 'u_t');
xline(voltage_axes, operating_u, ':r', 'u_0');
yline(voltage_axes, Vout_reference_v, ':', 'V_{ref}');
grid(voltage_axes, 'on');
ylabel(voltage_axes, 'Static Vout / V');
title(voltage_axes, 'FHA static output characteristic');

sensitivity_axes = nexttile(map_layout);
plot(sensitivity_axes, normalized_command, ...
    static_sensitivity_v_per_u, 'LineWidth', 1.4);
xline(sensitivity_axes, transition_u, '--k', 'u_t');
grid(sensitivity_axes, 'on');
xlabel(sensitivity_axes, 'Normalized 2P2Z output u');
ylabel(sensitivity_axes, 'dVout/du / (V/u)');
title(sensitivity_axes, 'Static control sensitivity');

title(map_layout, 'CLLC normalized PSM/PFM mapping');

%% 被控对象、2P2Z、开环与闭环波特图
figure('Color', 'w', 'Name', 'Normalized hybrid 2P2Z Bode');
bode_layout = tiledlayout(4, 2, 'TileSpacing', 'compact', ...
    'Padding', 'compact');

plot_bode_row(bode_layout, control_frequency_hz, ...
    normalized_plant, [], 'Normalized plant G_{p,u}', 'Plant');
plot_bode_row(bode_layout, control_frequency_hz, ...
    design.controller, digital_controller, ...
    'Normalized 2P2Z', 'Continuous', 'Discrete');
plot_bode_row(bode_layout, control_frequency_hz, ...
    analog_loop, digital_loop, ...
    'Open loop G_cG_{p,u}', 'Continuous', 'Discrete');
plot_bode_row(bode_layout, control_frequency_hz, ...
    analog_closed_loop, digital_closed_loop, ...
    'Closed loop G_cG_{p,u}/(1+G_cG_{p,u})', ...
    'Continuous', 'Discrete');

title(bode_layout, sprintf( ...
    'Normalized hybrid 2P2Z: u_t=%.4f, u_0=%.4f', ...
    transition_u, operating_u));

%% 单一 2P2Z 在 0~1 全范围的稳定裕度
figure('Color', 'w', 'Name', 'Single 2P2Z full range margins');
margin_layout = tiledlayout(3, 1, 'TileSpacing', 'compact', ...
    'Padding', 'compact');

bandwidth_axes = nexttile(margin_layout);
semilogy(bandwidth_axes, analysis_u, digital_crossover_hz, ...
    'o-', 'LineWidth', 1.3, 'MarkerSize', 3);
xline(bandwidth_axes, transition_u, '--k', 'u_t');
grid(bandwidth_axes, 'on');
ylabel(bandwidth_axes, 'Crossover / Hz');
title(bandwidth_axes, 'Single-controller bandwidth');

pm_axes = nexttile(margin_layout);
plot(pm_axes, analysis_u, digital_pm_deg, ...
    'o-', 'LineWidth', 1.3, 'MarkerSize', 3);
xline(pm_axes, transition_u, '--k', 'u_t');
yline(pm_axes, target_phase_margin_deg, ':r', 'PM target');
grid(pm_axes, 'on');
ylabel(pm_axes, 'PM / degree');

gm_axes = nexttile(margin_layout);
plot(gm_axes, analysis_u, digital_gm_db, ...
    'o-', 'LineWidth', 1.3, 'MarkerSize', 3);
xline(gm_axes, transition_u, '--k', 'u_t');
yline(gm_axes, target_gain_margin_db, ':r', 'GM target');
grid(gm_axes, 'on');
xlabel(gm_axes, 'Normalized 2P2Z output u');
ylabel(gm_axes, 'GM / dB');

title(margin_layout, 'One 2P2Z verified over PSM and PFM ranges');

%% 局部函数
function [output_voltage_v, dc_gain] = calculate_forward_fha_voltage( ...
    frequency_hz, Vin_v, Rac_ohm, Lrp, Crp, Lmp, Lrs, Crs, n)
%CALCULATE_FORWARD_FHA_VOLTAGE 计算满相移条件下正向 FHA 输出电压。

    omega = 2*pi*frequency_hz;
    Zr_primary = 1j*omega*Lrp + 1./(1j*omega*Crp);
    Zm_primary = 1j*omega*Lmp;
    Zr_secondary = 1j*omega*Lrs + 1./(1j*omega*Crs);
    Zsecondary_total = Zr_secondary + Rac_ohm;
    Zsecondary_referred = n^2*Zsecondary_total;
    Zparallel = (Zm_primary.*Zsecondary_referred) ./ ...
        (Zm_primary + Zsecondary_referred);
    Hprimary = Zparallel./(Zr_primary + Zparallel);
    Hsecondary = Rac_ohm./Zsecondary_total;
    dc_gain = abs(Hprimary.*Hsecondary/n);
    output_voltage_v = Vin_v*dc_gain;
end

function crossing_hz = find_gain_crossing_above_resonance( ...
    frequency_hz, gain, target_gain, resonant_frequency_hz)
%FIND_GAIN_CROSSING_ABOVE_RESONANCE 查找谐振点以上最近的目标增益交点。

    valid_index = find(frequency_hz >= resonant_frequency_hz);
    gain_error = gain(valid_index) - target_gain;
    pair = find(gain_error(1:end-1).*gain_error(2:end) <= 0, ...
        1, 'first');

    if isempty(pair)
        crossing_hz = NaN;
        return;
    end

    index_1 = valid_index(pair);
    index_2 = valid_index(pair + 1);
    frequency_pair = frequency_hz([index_1, index_2]);
    gain_pair = gain([index_1, index_2]);
    crossing_hz = frequency_pair(1) + ...
        (target_gain - gain_pair(1))*diff(frequency_pair)/diff(gain_pair);
end

function design = design_normalized_2p2z( ...
    frequency_hz, plant_family, zero_hz, maximum_crossover_hz, ...
    minimum_crossover_ratio, target_pm_deg, target_gm_db, ...
    maximum_pole_hz, sample_time_s)
%DESIGN_NORMALIZED_2P2Z 搜索整个被控对象族 PM、GM 合格的最高带宽。

    candidates_hz = logspace(log10(maximum_crossover_hz), ...
        log10(maximum_crossover_hz*minimum_crossover_ratio), 81);
    [~, design_plant_index] = max(abs(plant_family(1, :)));
    design_plant = plant_family(:, design_plant_index);

    for candidate_index = 1:numel(candidates_hz)
        candidate = synthesize_at_crossover( ...
            frequency_hz, design_plant, zero_hz, ...
            candidates_hz(candidate_index), target_pm_deg, ...
            maximum_pole_hz);
        coefficients = calculate_tustin_coefficients( ...
            candidate.K, candidate.zero_hz, candidate.pole_hz, ...
            sample_time_s);
        digital_controller = evaluate_discrete_controller( ...
            frequency_hz, sample_time_s, coefficients);
        digital_loop_family = plant_family.*digital_controller;
        family_pm_deg = nan(1, size(plant_family, 2));
        family_gm_db = nan(1, size(plant_family, 2));
        for plant_index = 1:size(plant_family, 2)
            [~, family_pm_deg(plant_index), family_gm_db(plant_index)] = ...
                analyze_loop_margins( ...
                    frequency_hz, digital_loop_family(:, plant_index));
        end

        design = candidate;
        design.family_phase_margin_deg = family_pm_deg;
        design.family_gain_margin_db = family_gm_db;
        if all(family_pm_deg >= target_pm_deg) && ...
           all(family_gm_db >= target_gm_db)
            return;
        end
    end
end

function design = synthesize_at_crossover( ...
    frequency_hz, plant, zero_hz, crossover_hz, ...
    target_pm_deg, maximum_pole_hz)
%SYNTHESIZE_AT_CROSSOVER 根据相位条件和 0 dB 条件计算 fp、K。

    plant_phase_deg = rad2deg(unwrap(angle(plant)));
    plant_phase_at_fc_deg = interp_log( ...
        frequency_hz, plant_phase_deg, crossover_hz);
    required_controller_phase_deg = ...
        -180 + target_pm_deg - plant_phase_at_fc_deg;
    zero_phase_deg = 2*atand(crossover_hz/zero_hz);
    required_pole_angle_deg = ...
        (-90 + zero_phase_deg - required_controller_phase_deg)/2;
    minimum_pole_angle_deg = atand(crossover_hz/maximum_pole_hz);
    pole_angle_deg = min(max(required_pole_angle_deg, ...
        minimum_pole_angle_deg), 45);
    pole_hz = crossover_hz/tand(pole_angle_deg);

    s = 1j*2*pi*frequency_hz;
    base_controller = (1 + s/(2*pi*zero_hz)).^2 ./ ...
        (s.*(1 + s/(2*pi*pole_hz)).^2);
    plant_gain_at_fc = interp_log( ...
        frequency_hz, abs(plant), crossover_hz);
    base_gain_at_fc = interp_log( ...
        frequency_hz, abs(base_controller), crossover_hz);
    K = 1/(plant_gain_at_fc*base_gain_at_fc);

    design.K = K;
    design.zero_hz = zero_hz;
    design.pole_hz = pole_hz;
    design.requested_crossover_hz = crossover_hz;
    design.controller = K*base_controller;
end

function value = interp_log(frequency_hz, data, query_hz)
%INTERP_LOG 在对数频率轴上线性插值。

    value = interp1(log10(frequency_hz), data, log10(query_hz), 'linear');
end

function c = calculate_tustin_coefficients(K, fz, fp, ts)
%CALCULATE_TUSTIN_COEFFICIENTS 计算标准 Tustin 2P2Z 差分系数。
%
% u[n] = b0*e[n] + b1*e[n-1] + b2*e[n-2] + b3*e[n-3]
%        -a1*u[n-1] -a2*u[n-2] -a3*u[n-3]

    wz1 = 2*pi*fz;
    wz2 = wz1;
    wp1 = 2*pi*fp;
    wp2 = wp1;

    n3 = K*ts*wp1*wp2* ...
        (4 - 2*ts*wz1 - 2*ts*wz2 + ts^2*wz1*wz2);
    n2 = K*ts*wp1*wp2* ...
        (-4 - 2*ts*wz1 - 2*ts*wz2 + 3*ts^2*wz1*wz2);
    n1 = K*ts*wp1*wp2* ...
        (-4 + 2*ts*wz1 + 2*ts*wz2 + 3*ts^2*wz1*wz2);
    n0 = K*ts*wp1*wp2* ...
        (4 + 2*ts*wz1 + 2*ts*wz2 + ts^2*wz1*wz2);

    d3 = 2*(-4 + 2*ts*wp1 + 2*ts*wp2 - ts^2*wp1*wp2)*wz1*wz2;
    d2 = 2*(12 - 2*ts*wp1 - 2*ts*wp2 - ts^2*wp1*wp2)*wz1*wz2;
    d1 = 2*(-12 - 2*ts*wp1 - 2*ts*wp2 + ts^2*wp1*wp2)*wz1*wz2;
    d0 = 2*(4 + 2*ts*wp1 + 2*ts*wp2 + ts^2*wp1*wp2)*wz1*wz2;

    c.a1 = d1/d0;
    c.a2 = d2/d0;
    c.a3 = d3/d0;
    c.b0 = n0/d0;
    c.b1 = n1/d0;
    c.b2 = n2/d0;
    c.b3 = n3/d0;
end

function controller = evaluate_discrete_controller(frequency_hz, ts, c)
%EVALUATE_DISCRETE_CONTROLLER 计算差分控制器的单位圆频率响应。

    z1 = exp(-1j*2*pi*frequency_hz*ts);
    controller = (c.b0 + c.b1*z1 + c.b2*z1.^2 + c.b3*z1.^3) ./ ...
        (1 + c.a1*z1 + c.a2*z1.^2 + c.a3*z1.^3);
end

function [crossover_hz, phase_margin_deg, gain_margin_db] = ...
    analyze_loop_margins(frequency_hz, loop)
%ANALYZE_LOOP_MARGINS 检查全部增益和负实轴相位交点。

    magnitude_db = 20*log10(abs(loop));
    phase_deg = rad2deg(unwrap(angle(loop)));
    gain_crossings_hz = find_log_crossings(frequency_hz, magnitude_db, 0);

    if isempty(gain_crossings_hz)
        crossover_hz = NaN;
        phase_margin_deg = NaN;
    else
        phases = interp1(log10(frequency_hz), phase_deg, ...
            log10(gain_crossings_hz), 'linear');
        margins = 180 + phases;
        [phase_margin_deg, index] = min(margins);
        crossover_hz = gain_crossings_hz(index);
    end

    minimum_phase = min(phase_deg);
    maximum_phase = max(phase_deg);
    first_order = ceil((-maximum_phase - 180)/360);
    last_order = floor((-minimum_phase - 180)/360);
    phase_orders = first_order:last_order;
    crossing_groups = cell(numel(phase_orders), 1);
    for order_index = 1:numel(phase_orders)
        target_phase = -180 - 360*phase_orders(order_index);
        crossing_groups{order_index} = find_log_crossings( ...
            frequency_hz, phase_deg, target_phase);
    end
    phase_crossings_hz = unique(vertcat(crossing_groups{:}));

    if isempty(phase_crossings_hz)
        gain_margin_db = Inf;
    else
        magnitudes = interp1(log10(frequency_hz), magnitude_db, ...
            log10(phase_crossings_hz), 'linear');
        gain_margin_db = min(-magnitudes);
    end
end

function crossings_hz = find_log_crossings(frequency_hz, data, target)
%FIND_LOG_CROSSINGS 查找全部对数频率交点。

    relative_data = data - target;
    pair_index = find(relative_data(1:end-1).*relative_data(2:end) <= 0);
    crossings_hz = zeros(numel(pair_index), 1);
    for index = 1:numel(pair_index)
        pair = pair_index(index);
        x = log10(frequency_hz(pair:pair + 1));
        y = relative_data(pair:pair + 1);
        crossings_hz(index) = 10^(x(1) - y(1)*diff(x)/diff(y));
    end
    crossings_hz = unique(crossings_hz);
end

function plot_bode_row(layout, frequency_hz, response_1, response_2, ...
    row_title, label_1, label_2)
%PLOT_BODE_ROW 绘制一行幅频与相频曲线。

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
    xlabel(magnitude_axes, 'Frequency / Hz');
    xlabel(phase_axes, 'Frequency / Hz');
    ylabel(magnitude_axes, 'Magnitude / dB');
    ylabel(phase_axes, 'Phase / degree');
    title(magnitude_axes, row_title);
    title(phase_axes, row_title);
    legend(magnitude_axes, 'Location', 'best');
end
