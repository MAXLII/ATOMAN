%% 单相逆变器 SRFPI + 电容电流有源阻尼时域仿真
% 参考论文：
% M. Monfared, S. Golestan, J. M. Guerrero,
% "Analysis, Design, and Experimental Verification of A Synchronous
% Reference Frame Voltage Control for Single-Phase Inverters,"
% IEEE Transactions on Industrial Electronics, 2014.
% DOI: 10.1109/TIE.2013.2238878
%
% 论文方案的关键组成：
% 1. 一阶全通滤波器（APF）产生输出电压正交分量；
% 2. DQ 同步坐标系电压 PI 外环；
% 3. 电容电流比例内环，用于 LC 有源阻尼；
% 4. 输出电压参考前馈；
% 5. 3、5、7 次多谐振补偿器。
%
% 本脚本使用 base 工程的 L、C、控制频率和额定功率。控制器参数按
% 论文式 (8)、式 (13) 和式 (15) 计算。论文未给出 k3/k5/k7 的数值，
% 因此谐振增益作为本工程参数独立配置，并通过时域结果验证。
clear; clc; close all;

%% 1. 逆变器与仿真参数
cfg.fs = 30e3;
cfg.ts = 1 / cfg.fs;
cfg.stop_time = 1.90;
cfg.l = 440e-6;
cfg.c = 12e-6;
cfg.r_l = 0.001;
cfg.v_bus = 400;
cfg.freq_hz = 50;
cfg.omega = 2 * pi * cfg.freq_hz;
cfg.v_rms = 230;
cfg.v_rms_slew_vps = 212;
cfg.rated_power_w = 6600;
cfg.load_on_time = 1.25;
cfg.load_off_time = 1.60;
% 论文式 (8)、式 (13)的基础设计模型不包含数字控制延迟；延迟影响由
% 论文式 (19)另行校核。设为 1 可继续验证实际单拍延迟下的降带宽设计。
cfg.control_delay_samples = 0;
% C 侧当前 HAL 未提供独立电容电流采样，使用 C*Delta(v)/Delta(t)反馈。
cfg.use_discrete_cap_current_feedback = true;

% 论文建议电容电流环带宽取开关频率的 1/5~1/4。
cfg.inner_bandwidth_hz = 4.0e3;
% 电压环带宽位于 10*f0 与 fs/10 之间，并避开开关噪声。
cfg.voltage_bandwidth_hz = 1.3e3;
% 由 Ki < Kp*omega_f 选择稳定范围中部的积分增益。
cfg.voltage_ki_ratio = 0.55;

% DQ 电压 PI 输出是电容电流给定。
cfg.voltage_loop_limit_a = 20;
cfg.bridge_voltage_limit_v = cfg.v_bus;

% 论文只给出谐振补偿器结构，未给出 kn。以下为本工程初始整定值。
cfg.harmonic_orders = [3, 5, 7];
cfg.harmonic_gains = [0.30, 0.20, 0.15];
cfg.harmonic_output_limit_a = 8;

% 用受控谐波电流负载验证 3/5/7 次补偿效果。
cfg.harmonic_load_peak_a = [4.0, 3.0, 2.0];

%% 2. 按论文公式计算控制器参数
ctrl = design_paper_controller(cfg);

fprintf('Paper-based SRFPI capacitor-current control\n');
fprintf('LC resonance       : %.3f Hz\n', 1 / (2 * pi * sqrt(cfg.l * cfg.c)));
fprintf('Rated load         : %.6f ohm at %.0f W\n', ctrl.nominal_load_ohm, cfg.rated_power_w);
fprintf('Inner bandwidth    : %.1f Hz\n', cfg.inner_bandwidth_hz);
fprintf('Inner gain K       : %.6f V/A\n', ctrl.inner_k);
fprintf('Voltage bandwidth  : %.1f Hz\n', cfg.voltage_bandwidth_hz);
fprintf('Voltage Kp         : %.9f A/V\n', ctrl.voltage_kp);
fprintf('Voltage Ki         : %.9f A/(V*s)\n', ctrl.voltage_ki);
fprintf('Ki stability limit : %.9f A/(V*s)\n', ctrl.voltage_ki_max);
fprintf('Harmonic gains     : k3=%.3f, k5=%.3f, k7=%.3f\n\n', ...
    cfg.harmonic_gains(1), cfg.harmonic_gains(2), cfg.harmonic_gains(3));

assert(ctrl.inner_k > 0, 'Paper equation (8) produced an invalid inner-loop gain.');
assert(ctrl.voltage_kp > 0, 'Paper equation (13) produced an invalid voltage Kp.');
assert(ctrl.voltage_ki < ctrl.voltage_ki_max, ...
    'Voltage Ki violates the paper stability condition Ki < Kp*omega_f.');

%% 3. 空载与分级阻性负载投切
load_power_cases_w = [0, 1000, 2000, 3000, 6600];
case_count = numel(load_power_cases_w);
simulations = cell(case_count, 1);

for case_index = 1:case_count
    simulations{case_index} = simulate_inv_case( ...
        cfg, ctrl, load_power_cases_w(case_index), true, false);
end

%% 4. 满载谐波电流扰动下的补偿器对比
harmonic_without = simulate_inv_case( ...
    cfg, ctrl, cfg.rated_power_w, false, true);
harmonic_with = simulate_inv_case( ...
    cfg, ctrl, cfg.rated_power_w, true, true);

for case_index = 1:case_count
    assert(abs(simulations{case_index}.steady_v_rms - cfg.v_rms) < 1.0, ...
        'Steady-state voltage error is too large at %.0f W.', ...
        load_power_cases_w(case_index));
end
assert(harmonic_with.steady_thd_percent < harmonic_without.steady_thd_percent, ...
    'The 3rd/5th/7th harmonic compensator did not reduce voltage THD.');

%% 5. 结果打印
fprintf('Load(W)  Vsteady(V)  Phase(deg)  THD(%%)  Residual(V)  LoadMin(V)  UnloadMax(V)  Ipk(A)\n');
for case_index = 1:case_count
    sim = simulations{case_index};
    fprintf('%7.0f  %10.3f  %10.3f  %6.3f  %11.4f  %10.3f  %12.3f  %7.3f\n', ...
        sim.load_power_command_w, sim.steady_v_rms, ...
        sim.steady_phase_error_deg, sim.steady_thd_percent, ...
        sim.steady_residual_rms_v, sim.load_transient_min_v, ...
        sim.unload_transient_max_v, sim.max_inductor_current_a);
end

fprintf('\nHarmonic-current load at %.0f W:\n', cfg.rated_power_w);
fprintf('  HC disabled: THD=%.3f%%, residual=%.4f V RMS\n', ...
    harmonic_without.steady_thd_percent, harmonic_without.steady_residual_rms_v);
fprintf('  HC enabled : THD=%.3f%%, residual=%.4f V RMS\n', ...
    harmonic_with.steady_thd_percent, harmonic_with.steady_residual_rms_v);

%% 6. 绘图
figure('Name', 'Paper SRFPI capacitor-current control', ...
    'Color', 'w', 'Position', [80, 60, 1350, 1050]);
tiledlayout(4, 1, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile;
hold on;
for case_index = 1:case_count
    sim = simulations{case_index};
    plot(sim.t, sim.v_cap_rms, 'LineWidth', 1.0, ...
        'DisplayName', sprintf('%g W', sim.load_power_command_w));
end
plot(simulations{1}.t, simulations{1}.v_ref_rms, 'k--', ...
    'LineWidth', 1.1, 'DisplayName', 'reference');
grid on;
ylabel('RMS / V');
ylim([0, 280]);
title('空载及分级阻性负载投切');
legend('Location', 'eastoutside');
xline(cfg.load_on_time, '--', 'load on', 'HandleVisibility', 'off');
xline(cfg.load_off_time, '--', 'load off', 'HandleVisibility', 'off');

full_load = simulations{end};
steady_zoom = full_load.t >= cfg.load_off_time - 0.06 & ...
    full_load.t < cfg.load_off_time - 0.02;
nexttile;
plot(full_load.t(steady_zoom), full_load.v_ref(steady_zoom), '--', ...
    full_load.t(steady_zoom), full_load.v_cap(steady_zoom), 'LineWidth', 1.1);
grid on;
ylabel('Voltage / V');
title('6600 W 稳态电压跟踪');
legend('reference', 'feedback', 'Location', 'best');

nexttile;
plot(full_load.t, full_load.i_cap_ref, '--', ...
    full_load.t, full_load.i_cap_feedback, 'LineWidth', 1.0);
grid on;
ylabel('Current / A');
title('电容电流给定与反馈（论文内环）');
legend('reference', 'feedback', 'Location', 'best');
xline(cfg.load_on_time, '--', 'load on', 'HandleVisibility', 'off');
xline(cfg.load_off_time, '--', 'load off', 'HandleVisibility', 'off');

harmonic_zoom = harmonic_with.t >= cfg.load_off_time - 0.06 & ...
    harmonic_with.t < cfg.load_off_time - 0.02;
nexttile;
plot(harmonic_without.t(harmonic_zoom), harmonic_without.v_cap(harmonic_zoom), ...
    harmonic_with.t(harmonic_zoom), harmonic_with.v_cap(harmonic_zoom), ...
    harmonic_with.t(harmonic_zoom), harmonic_with.v_ref(harmonic_zoom), 'k--', ...
    'LineWidth', 1.0);
grid on;
xlabel('Time / s');
ylabel('Voltage / V');
title('3/5/7 次谐波电流扰动：多谐振补偿对比');
legend('HC disabled', 'HC enabled', 'reference', 'Location', 'best');

result_path = fullfile(tempdir, 'inv_srfpi_cap_current_result.png');
exportgraphics(gcf, result_path, 'Resolution', 150);
fprintf('Result figure: %s\n', result_path);

%% 7. 满载投卸载电压瞬态
load_voltage_zoom = full_load.t >= cfg.load_on_time - 0.04 & ...
    full_load.t <= cfg.load_on_time + 0.12;
unload_voltage_zoom = full_load.t >= cfg.load_off_time - 0.04 & ...
    full_load.t <= cfg.load_off_time + 0.12;
load_relative_time_ms = 1e3 * (full_load.t - cfg.load_on_time);
unload_relative_time_ms = 1e3 * (full_load.t - cfg.load_off_time);

figure('Name', 'Paper SRFPI full-load voltage transients', ...
    'Color', 'w', 'Position', [110, 80, 1350, 850]);
tiledlayout(2, 2, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile;
plot(load_relative_time_ms(load_voltage_zoom), ...
    full_load.v_ref(load_voltage_zoom), '--', ...
    load_relative_time_ms(load_voltage_zoom), ...
    full_load.v_cap(load_voltage_zoom), 'LineWidth', 1.0);
grid on;
xline(0, '--', '投载', 'HandleVisibility', 'off');
xlabel('相对投载时间 / ms');
ylabel('电压 / V');
title('6600 W 投载瞬时电压');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(load_relative_time_ms(load_voltage_zoom), ...
    full_load.v_ref_rms(load_voltage_zoom), '--', ...
    load_relative_time_ms(load_voltage_zoom), ...
    full_load.v_cap_rms(load_voltage_zoom), 'LineWidth', 1.1);
grid on;
xline(0, '--', '投载', 'HandleVisibility', 'off');
xlabel('相对投载时间 / ms');
ylabel('单周期 RMS / V');
title(sprintf('投载电压包络，最低 %.2f V', ...
    full_load.load_transient_min_v));
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(unload_relative_time_ms(unload_voltage_zoom), ...
    full_load.v_ref(unload_voltage_zoom), '--', ...
    unload_relative_time_ms(unload_voltage_zoom), ...
    full_load.v_cap(unload_voltage_zoom), 'LineWidth', 1.0);
grid on;
xline(0, '--', '卸载', 'HandleVisibility', 'off');
xlabel('相对卸载时间 / ms');
ylabel('电压 / V');
title('6600 W 卸载瞬时电压');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(unload_relative_time_ms(unload_voltage_zoom), ...
    full_load.v_ref_rms(unload_voltage_zoom), '--', ...
    unload_relative_time_ms(unload_voltage_zoom), ...
    full_load.v_cap_rms(unload_voltage_zoom), 'LineWidth', 1.1);
grid on;
xline(0, '--', '卸载', 'HandleVisibility', 'off');
xlabel('相对卸载时间 / ms');
ylabel('单周期 RMS / V');
title(sprintf('卸载电压包络，最高 %.2f V', ...
    full_load.unload_transient_max_v));
legend('给定', '反馈', 'Location', 'best');

transient_result_path = fullfile(tempdir, ...
    'inv_srfpi_cap_current_load_transient.png');
exportgraphics(gcf, transient_result_path, 'Resolution', 150);
fprintf('Transient figure: %s\n', transient_result_path);

%% 本地函数
function ctrl = design_paper_controller(cfg)
% 按论文式 (8)、式 (13)、式 (15)计算双环参数。
ctrl.nominal_load_ohm = cfg.v_rms^2 / cfg.rated_power_w;
z = ctrl.nominal_load_ohm;
w_bi = 2 * pi * cfg.inner_bandwidth_hz;

sqrt_term = 2 * cfg.r_l * cfg.c * z * ...
    (cfg.r_l * cfg.c * z + cfg.l) + ...
    cfg.l^2 * (2 + cfg.c^2 * z^2 * w_bi^2);
ctrl.inner_k = (cfg.l + cfg.r_l * cfg.c * z + sqrt(sqrt_term)) / ...
    (cfg.c * z);

w_bv = 2 * pi * cfg.voltage_bandwidth_hz;
ctrl.voltage_kp = cfg.c * w_bv * ...
    (sqrt(2 * cfg.l^2 * w_bv^2 + ctrl.inner_k^2) - cfg.l * w_bv) / ...
    ctrl.inner_k;
ctrl.voltage_ki_max = ctrl.voltage_kp * cfg.omega;
ctrl.voltage_ki = cfg.voltage_ki_ratio * ctrl.voltage_ki_max;
end

function sim = simulate_inv_case(cfg, ctrl, load_power_w, hc_enabled, harmonic_load_enabled)
% 对论文控制器进行离散时域仿真。
t = (0:cfg.ts:cfg.stop_time).';
sample_count = numel(t);

if load_power_w > 0
    load_resistance_ohm = cfg.v_rms^2 / load_power_w;
else
    load_resistance_ohm = inf;
end

v_ref_pk = zeros(sample_count, 1);
v_ref = zeros(sample_count, 1);
v_cap = zeros(sample_count, 1);
v_cap_beta = zeros(sample_count, 1);
v_d = zeros(sample_count, 1);
v_q = zeros(sample_count, 1);
i_l = zeros(sample_count, 1);
i_load = zeros(sample_count, 1);
i_cap = zeros(sample_count, 1);
i_cap_feedback = zeros(sample_count, 1);
i_cap_ref = zeros(sample_count, 1);
harmonic_comp = zeros(sample_count, 1);
v_bridge_cmd = zeros(sample_count, 1);
v_bridge_applied = zeros(sample_count, 1);
phase = zeros(sample_count, 1);

apf = apf_init(cfg.omega, cfg.ts);
pi_d = pi_init(ctrl.voltage_kp, ctrl.voltage_ki, cfg.ts, ...
    cfg.voltage_loop_limit_a);
pi_q = pi_init(ctrl.voltage_kp, ctrl.voltage_ki, cfg.ts, ...
    cfg.voltage_loop_limit_a);

resonator_count = numel(cfg.harmonic_orders);
resonators = cell(resonator_count, 1);
for resonator_index = 1:resonator_count
    resonators{resonator_index} = resonant_init( ...
        cfg.harmonic_gains(resonator_index), ...
        cfg.harmonic_orders(resonator_index) * cfg.omega, cfg.ts);
end

delay_line = zeros(cfg.control_delay_samples + 1, 1);

for sample_index = 1:sample_count - 1
    theta = 2 * pi * phase(sample_index);
    sin_theta = sin(theta);
    cos_theta = cos(theta);

    v_ref_pk(sample_index + 1) = ramp_to( ...
        v_ref_pk(sample_index), sqrt(2) * cfg.v_rms, ...
        sqrt(2) * cfg.v_rms_slew_vps * cfg.ts);
    v_ref(sample_index) = v_ref_pk(sample_index) * cos_theta;

    load_is_on = t(sample_index) >= cfg.load_on_time && ...
        t(sample_index) < cfg.load_off_time;
    if load_is_on && isfinite(load_resistance_ohm)
        i_load(sample_index) = v_cap(sample_index) / load_resistance_ohm;
    end
    if load_is_on && harmonic_load_enabled
        for harmonic_index = 1:numel(cfg.harmonic_orders)
            order = cfg.harmonic_orders(harmonic_index);
            i_load(sample_index) = i_load(sample_index) + ...
                cfg.harmonic_load_peak_a(harmonic_index) * cos(order * theta);
        end
    end

    i_cap(sample_index) = i_l(sample_index) - i_load(sample_index);
    if cfg.use_discrete_cap_current_feedback && sample_index > 1
        i_cap_feedback(sample_index) = cfg.c * ...
            (v_cap(sample_index) - v_cap(sample_index - 1)) / cfg.ts;
    else
        i_cap_feedback(sample_index) = i_cap(sample_index);
    end

    [apf, v_cap_beta(sample_index)] = apf_step(apf, v_cap(sample_index));
    v_d(sample_index) = cos_theta * v_cap(sample_index) + ...
        sin_theta * v_cap_beta(sample_index);
    v_q(sample_index) = -sin_theta * v_cap(sample_index) + ...
        cos_theta * v_cap_beta(sample_index);

    [pi_d, i_cap_d_ref] = pi_step( ...
        pi_d, v_ref_pk(sample_index) - v_d(sample_index));
    [pi_q, i_cap_q_ref] = pi_step(pi_q, -v_q(sample_index));
    i_cap_ref(sample_index) = cos_theta * i_cap_d_ref - ...
        sin_theta * i_cap_q_ref;

    if hc_enabled
        for resonator_index = 1:resonator_count
            [resonators{resonator_index}, resonator_output] = ...
                resonant_step(resonators{resonator_index}, v_cap(sample_index));
            harmonic_comp(sample_index) = harmonic_comp(sample_index) + resonator_output;
        end
        harmonic_comp(sample_index) = sat(harmonic_comp(sample_index), ...
            -cfg.harmonic_output_limit_a, cfg.harmonic_output_limit_a);
    end

    % 谐波支路构成输出电压负反馈，用于降低对应频率的输出阻抗。
    inner_error_a = i_cap_ref(sample_index) - i_cap_feedback(sample_index) - ...
        harmonic_comp(sample_index);
    v_bridge_cmd(sample_index) = sat( ...
        v_ref(sample_index) + ctrl.inner_k * inner_error_a, ...
        -cfg.bridge_voltage_limit_v, cfg.bridge_voltage_limit_v);

    delay_line(2:end) = delay_line(1:end - 1);
    delay_line(1) = v_bridge_cmd(sample_index);
    v_bridge_applied(sample_index) = delay_line(end);

    i_l(sample_index + 1) = i_l(sample_index) + cfg.ts * ...
        (v_bridge_applied(sample_index) - v_cap(sample_index) - ...
        cfg.r_l * i_l(sample_index)) / cfg.l;
    v_cap(sample_index + 1) = v_cap(sample_index) + cfg.ts * ...
        (i_l(sample_index + 1) - i_load(sample_index)) / cfg.c;
    phase(sample_index + 1) = mod( ...
        phase(sample_index) + cfg.freq_hz * cfg.ts, 1);
end

v_ref(end) = v_ref_pk(end) * cos(2 * pi * phase(end));
i_load(end) = i_load(end - 1);
i_cap(end) = i_cap(end - 1);
i_cap_feedback(end) = i_cap_feedback(end - 1);
i_cap_ref(end) = i_cap_ref(end - 1);
harmonic_comp(end) = harmonic_comp(end - 1);
v_bridge_cmd(end) = v_bridge_cmd(end - 1);
v_bridge_applied(end) = v_bridge_applied(end - 1);

assert(all(isfinite([v_cap; i_l; v_bridge_applied])), ...
    'Paper-controller simulation diverged at %.0f W.', load_power_w);

period_samples = round(cfg.fs / cfg.freq_hz);
v_ref_rms = sqrt(movmean(v_ref.^2, [period_samples - 1, 0]));
v_cap_rms = sqrt(movmean(v_cap.^2, [period_samples - 1, 0]));
steady_window = t >= cfg.load_off_time - 0.10 & ...
    t < cfg.load_off_time - 0.02;
load_transient_window = t >= cfg.load_on_time & ...
    t < cfg.load_on_time + 0.12;
unload_transient_window = t >= cfg.load_off_time & ...
    t < cfg.load_off_time + 0.12;

[steady_ref_rms, steady_ref_phase] = sine_metric( ...
    v_ref, t, steady_window, cfg.freq_hz);
[steady_v_rms, steady_v_phase, steady_residual_rms_v, steady_thd_percent] = ...
    waveform_metric(v_cap, t, steady_window, cfg.freq_hz, 15);

sim.t = t;
sim.v_ref = v_ref;
sim.v_cap = v_cap;
sim.v_ref_rms = v_ref_rms;
sim.v_cap_rms = v_cap_rms;
sim.i_l = i_l;
sim.i_load = i_load;
sim.i_cap = i_cap;
sim.i_cap_feedback = i_cap_feedback;
sim.i_cap_ref = i_cap_ref;
sim.harmonic_comp = harmonic_comp;
sim.v_bridge_cmd = v_bridge_cmd;
sim.v_bridge_applied = v_bridge_applied;
sim.load_power_command_w = load_power_w;
sim.steady_ref_rms = steady_ref_rms;
sim.steady_v_rms = steady_v_rms;
sim.steady_phase_error_deg = wrap_to_180( ...
    rad2deg(steady_v_phase - steady_ref_phase));
sim.steady_residual_rms_v = steady_residual_rms_v;
sim.steady_thd_percent = steady_thd_percent;
sim.load_transient_min_v = min(v_cap_rms(load_transient_window));
sim.unload_transient_max_v = max(v_cap_rms(unload_transient_window));
sim.max_inductor_current_a = max(abs(i_l));
end

function state = apf_init(omega, ts)
% Tustin 离散化 H(s)=(omega-s)/(omega+s)。
denominator = omega * ts + 2;
state.b0 = (omega * ts - 2) / denominator;
state.b1 = 1;
state.a1 = state.b0;
state.x1 = 0;
state.y1 = 0;
end

function [state, output] = apf_step(state, input)
output = state.b0 * input + state.b1 * state.x1 - state.a1 * state.y1;
state.x1 = input;
state.y1 = output;
end

function state = pi_init(kp, ki, ts, limit)
state.kp = kp;
state.ki_ts_half = 0.5 * ki * ts;
state.integrator = 0;
state.error1 = 0;
state.limit = limit;
end

function [state, output] = pi_step(state, error)
integrator_candidate = state.integrator + ...
    state.ki_ts_half * (error + state.error1);
raw = state.kp * error + integrator_candidate;
output = sat(raw, -state.limit, state.limit);

hold_integrator = (raw > state.limit && error > 0) || ...
    (raw < -state.limit && error < 0);
if ~hold_integrator
    state.integrator = integrator_candidate;
end
state.error1 = error;
end

function state = resonant_init(kn, omega_n, ts)
% Tustin 离散化论文式 (17)：kn*s/(s^2+omega_n^2)。
d0 = 4 + ts^2 * omega_n^2;
state.b0 = 2 * kn * ts / d0;
state.b1 = 0;
state.b2 = -state.b0;
state.a1 = (2 * ts^2 * omega_n^2 - 8) / d0;
state.a2 = 1;
state.x1 = 0;
state.x2 = 0;
state.y1 = 0;
state.y2 = 0;
end

function [state, output] = resonant_step(state, input)
output = state.b0 * input + state.b1 * state.x1 + state.b2 * state.x2 - ...
    state.a1 * state.y1 - state.a2 * state.y2;
state.x2 = state.x1;
state.x1 = input;
state.y2 = state.y1;
state.y1 = output;
end

function [rms_value, phase] = sine_metric(signal, time, window, freq_hz)
x = 2 * pi * freq_hz * time(window);
basis = [cos(x), sin(x), ones(sum(window), 1)];
coefficient = basis \ signal(window);
rms_value = hypot(coefficient(1), coefficient(2)) / sqrt(2);
phase = atan2(-coefficient(2), coefficient(1));
end

function [rms_value, phase, residual_rms, thd_percent] = ...
    waveform_metric(signal, time, window, freq_hz, highest_harmonic)
selected_signal = signal(window);
selected_time = time(window);
sample_count = numel(selected_signal);
basis = ones(sample_count, 1);
for harmonic = 1:highest_harmonic
    angle = 2 * pi * harmonic * freq_hz * selected_time;
    basis = [basis, cos(angle), sin(angle)]; %#ok<AGROW>
end
coefficient = basis \ selected_signal;
fundamental_cos = coefficient(2);
fundamental_sin = coefficient(3);
rms_value = hypot(fundamental_cos, fundamental_sin) / sqrt(2);
phase = atan2(-fundamental_sin, fundamental_cos);

fundamental_fit = fundamental_cos * basis(:, 2) + ...
    fundamental_sin * basis(:, 3) + coefficient(1);
residual_rms = sqrt(mean((selected_signal - fundamental_fit).^2));

harmonic_rms_sq = 0;
for harmonic = 2:highest_harmonic
    cos_index = 2 * harmonic;
    sin_index = cos_index + 1;
    harmonic_rms_sq = harmonic_rms_sq + ...
        0.5 * (coefficient(cos_index)^2 + coefficient(sin_index)^2);
end
thd_percent = 100 * sqrt(harmonic_rms_sq) / max(rms_value, eps);
end

function angle = wrap_to_180(angle)
angle = mod(angle + 180, 360) - 180;
end

function y = sat(x, lower_limit, upper_limit)
y = min(max(x, lower_limit), upper_limit);
end

function y = ramp_to(x, target, step)
y = x + sat(target - x, -step, step);
end
