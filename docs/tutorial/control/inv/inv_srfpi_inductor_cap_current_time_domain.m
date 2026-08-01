%% 单相逆变器 SRFPI 电压-电感电流-电容电流三环原理仿真
% 控制链路：
% 1. 电压 alpha-beta 量变换到同步 DQ 坐标系；
% 2. 电压 DQ PI 输出动态电容电流，并通过负载电流及 omega*C 解耦
%    得到电感电流 DQ 给定；
% 3. 电感电流 DQ PI 输出电容电流 DQ 给定；
% 4. 电容电流比例内环输出桥臂电压；
% 5. 可选的 R*iL 与 omega*L*iL 桥臂电压解耦用于验证其必要性。
%
% 原理模型显式建立 alpha-beta 两轴正交 LC 对象，使 DQ 反馈量没有
% 正交信号发生器的动态误差。实际单相代码可继续用 APF/SOGI 生成
% beta 轴，三环结构和解耦关系保持不变。
clear; clc; close all;

%% 1. 对象及工况参数
cfg.fs = 30e3;
cfg.ts = 1 / cfg.fs;
cfg.stop_time = 10.0;
cfg.l = 440e-6;
cfg.c = 12e-6;
cfg.r_l = 0.001;
cfg.v_bus = 400;
cfg.freq_hz = 50;
cfg.omega = 2 * pi * cfg.freq_hz;
cfg.v_rms = 230;
cfg.v_peak = sqrt(2) * cfg.v_rms;
cfg.v_rms_slew_vps = 212;
cfg.rated_power_w = 6600;
cfg.load_resistance_ohm = cfg.v_rms^2 / cfg.rated_power_w;
cfg.load_on_time = 3.0;
cfg.load_off_time = 7.0;
cfg.rms_settle_band_v = 0.5;
cfg.control_delay_samples = 1;

%% 2. 三环参数
% 30 kHz 控制频率下，将电容电流内环提高到采样频率的 1/5。
cfg.cap_current_bandwidth_hz = 6.0e3;
cfg.cap_current_k = design_cap_current_gain(cfg);

% 增加电感电流环后，电压环必须低于电感电流环。
cfg.inductor_current_bandwidth_hz = 2.0e3;
cfg.inductor_current_kp = cfg.l * 2 * pi * ...
    cfg.inductor_current_bandwidth_hz / cfg.cap_current_k;
cfg.inductor_current_ki = cfg.r_l * 2 * pi * ...
    cfg.inductor_current_bandwidth_hz / cfg.cap_current_k;
cfg.inductor_current_output_limit_a = 6;

cfg.voltage_bandwidth_hz = 600;
cfg.voltage_kp = cfg.c * 2 * pi * cfg.voltage_bandwidth_hz;
cfg.voltage_ki = cfg.voltage_kp * 2 * pi * 40;
cfg.voltage_output_limit_a = 25;

fprintf('SRFPI voltage-inductor-current-capacitor-current cascade\n');
fprintf('LC resonance                    : %.2f Hz\n', ...
    1 / (2 * pi * sqrt(cfg.l * cfg.c)));
fprintf('Capacitor-current gain          : %.6f V/A\n', ...
    cfg.cap_current_k);
fprintf('Inductor-current PI             : Kp=%.6f, Ki=%.6f 1/s\n', ...
    cfg.inductor_current_kp, cfg.inductor_current_ki);
fprintf('Voltage PI                      : Kp=%.9f A/V, Ki=%.9f A/(V*s)\n\n', ...
    cfg.voltage_kp, cfg.voltage_ki);
fprintf('Loop bandwidths V/I_L/I_C       : %.0f / %.0f / %.0f Hz\n', ...
    cfg.voltage_bandwidth_hz, cfg.inductor_current_bandwidth_hz, ...
    cfg.cap_current_bandwidth_hz);
fprintf('Control delay                   : %u sample(s)\n\n', ...
    cfg.control_delay_samples);

%% 3. 10 秒空载-满载-空载工况
sim = simulate_three_loop_case(cfg, true);

fprintf('10 s no-load/full-load/no-load simulation at %.0f W\n', ...
    cfg.rated_power_w);
fprintf('Steady voltage                  : %.3f V RMS\n', sim.steady_v_rms);
fprintf('Load minimum, one-cycle RMS     : %.3f V\n', sim.load_transient_min_v);
fprintf('Unload maximum, one-cycle RMS   : %.3f V\n', sim.unload_transient_max_v);
fprintf('Load settling, one-cycle RMS    : %.3f ms\n', sim.load_settle_time_ms);
fprintf('Unload settling, one-cycle RMS  : %.3f ms\n', sim.unload_settle_time_ms);
fprintf('Load settling, DQ magnitude RMS : %.3f ms\n', sim.load_dq_settle_time_ms);
fprintf('Unload settling, DQ magnitude   : %.3f ms\n', sim.unload_dq_settle_time_ms);
fprintf('Inductor-current D/Q RMS error  : %.4f A / %.4f A\n', ...
    sim.steady_i_d_error_rms, sim.steady_i_q_error_rms);
fprintf('Maximum inductor current        : %.3f A\n\n', ...
    sim.max_inductor_current_a);

assert(sim.is_finite, ...
    'The three-loop simulation with decoupling diverged.');
assert(abs(sim.steady_v_rms - cfg.v_rms) < 1.0, ...
    'The three-loop steady-state voltage error is too large.');
assert(sim.steady_i_d_error_rms < 0.5, ...
    'The D-axis inductor-current tracking error is too large.');
assert(sim.steady_i_q_error_rms < 0.5, ...
    'The Q-axis inductor-current tracking error is too large.');

%% 4. 全时段与 DQ 环路结果
steady_zoom = sim.t >= cfg.load_off_time - 0.08 & ...
    sim.t < cfg.load_off_time - 0.02;

figure('Name', 'SRFPI three-loop principle simulation', ...
    'Color', 'w', 'Position', [70, 45, 1450, 1050]);
tiledlayout(4, 2, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile([1, 2]);
plot(sim.t, sim.v_alpha_rms, ...
    sim.t, sim.v_dq_rms, ...
    sim.t, sim.v_ref_rms, 'k--', 'LineWidth', 1.0);
grid on;
xline(cfg.load_on_time, '--', '投载', 'HandleVisibility', 'off');
xline(cfg.load_off_time, '--', '卸载', 'HandleVisibility', 'off');
ylabel('单周期 RMS / V');
title('10 s 空载-6600 W 满载-空载有效值响应');
legend('单周期 RMS', 'DQ 幅值 RMS', '给定', ...
    'Location', 'eastoutside');

nexttile;
plot(sim.t(steady_zoom), sim.v_d_ref(steady_zoom), '--', ...
    sim.t(steady_zoom), sim.v_d(steady_zoom), 'LineWidth', 1.0);
grid on;
ylabel('D 轴电压 / V');
title('电压 D 轴给定与反馈');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(sim.t(steady_zoom), sim.v_q_ref(steady_zoom), '--', ...
    sim.t(steady_zoom), sim.v_q(steady_zoom), 'LineWidth', 1.0);
grid on;
ylabel('Q 轴电压 / V');
title('电压 Q 轴给定与反馈');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(sim.t(steady_zoom), sim.i_l_d_ref(steady_zoom), '--', ...
    sim.t(steady_zoom), sim.i_l_d(steady_zoom), 'LineWidth', 1.0);
grid on;
ylabel('D 轴电流 / A');
title('电感电流 D 轴给定与反馈');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(sim.t(steady_zoom), sim.i_l_q_ref(steady_zoom), '--', ...
    sim.t(steady_zoom), sim.i_l_q(steady_zoom), 'LineWidth', 1.0);
grid on;
ylabel('Q 轴电流 / A');
title('电感电流 Q 轴给定与反馈');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(sim.t(steady_zoom), sim.i_cap_d_ref(steady_zoom), '--', ...
    sim.t(steady_zoom), sim.i_cap_d(steady_zoom), 'LineWidth', 1.0);
grid on;
xlabel('时间 / s');
ylabel('D 轴电流 / A');
title('电容电流 D 轴给定与反馈');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(sim.t(steady_zoom), sim.i_cap_q_ref(steady_zoom), '--', ...
    sim.t(steady_zoom), sim.i_cap_q(steady_zoom), 'LineWidth', 1.0);
grid on;
xlabel('时间 / s');
ylabel('Q 轴电流 / A');
title('电容电流 Q 轴给定与反馈');
legend('给定', '反馈', 'Location', 'best');

result_path = fullfile(tempdir, ...
    'inv_srfpi_inductor_cap_current_result.png');
exportgraphics(gcf, result_path, 'Resolution', 150);
fprintf('\nLoop figure: %s\n', result_path);

%% 5. 满载投卸载电压瞬态
load_zoom = sim.t >= cfg.load_on_time - 0.04 & ...
    sim.t <= cfg.load_on_time + 0.12;
unload_zoom = sim.t >= cfg.load_off_time - 0.04 & ...
    sim.t <= cfg.load_off_time + 0.12;
load_time_ms = 1e3 * (sim.t - cfg.load_on_time);
unload_time_ms = 1e3 * (sim.t - cfg.load_off_time);

figure('Name', 'SRFPI three-loop load transients', ...
    'Color', 'w', 'Position', [100, 70, 1350, 850]);
tiledlayout(2, 2, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile;
plot(load_time_ms(load_zoom), sim.v_ref_alpha(load_zoom), '--', ...
    load_time_ms(load_zoom), sim.v_alpha(load_zoom), 'LineWidth', 1.0);
grid on;
xline(0, '--', '投载', 'HandleVisibility', 'off');
xlabel('相对投载时间 / ms');
ylabel('电压 / V');
title('6600 W 投载瞬时电压');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(load_time_ms(load_zoom), sim.v_ref_rms(load_zoom), '--', ...
    load_time_ms(load_zoom), sim.v_alpha_rms(load_zoom), 'LineWidth', 1.0);
hold on;
plot(load_time_ms(load_zoom), sim.v_dq_rms(load_zoom), 'LineWidth', 1.0);
grid on;
xline(0, '--', '投载', 'HandleVisibility', 'off');
xlabel('相对投载时间 / ms');
ylabel('单周期 RMS / V');
title(sprintf('投载最低 %.2f V，RMS稳定 %.2f ms', ...
    sim.load_transient_min_v, sim.load_settle_time_ms));
legend('给定', '单周期 RMS', 'DQ 幅值 RMS', 'Location', 'best');

nexttile;
plot(unload_time_ms(unload_zoom), sim.v_ref_alpha(unload_zoom), '--', ...
    unload_time_ms(unload_zoom), sim.v_alpha(unload_zoom), 'LineWidth', 1.0);
grid on;
xline(0, '--', '卸载', 'HandleVisibility', 'off');
xlabel('相对卸载时间 / ms');
ylabel('电压 / V');
title('6600 W 卸载瞬时电压');
legend('给定', '反馈', 'Location', 'best');

nexttile;
plot(unload_time_ms(unload_zoom), sim.v_ref_rms(unload_zoom), '--', ...
    unload_time_ms(unload_zoom), sim.v_alpha_rms(unload_zoom), 'LineWidth', 1.0);
hold on;
plot(unload_time_ms(unload_zoom), sim.v_dq_rms(unload_zoom), 'LineWidth', 1.0);
grid on;
xline(0, '--', '卸载', 'HandleVisibility', 'off');
xlabel('相对卸载时间 / ms');
ylabel('单周期 RMS / V');
title(sprintf('卸载最高 %.2f V，RMS稳定 %.2f ms', ...
    sim.unload_transient_max_v, sim.unload_settle_time_ms));
legend('给定', '单周期 RMS', 'DQ 幅值 RMS', 'Location', 'best');

transient_result_path = fullfile(tempdir, ...
    'inv_srfpi_inductor_cap_current_transient.png');
exportgraphics(gcf, transient_result_path, 'Resolution', 150);
fprintf('Transient figure: %s\n', transient_result_path);

%% 本地函数
function cap_current_k = design_cap_current_gain(cfg)
% 按论文式 (8)计算电容电流比例内环增益。
z = cfg.load_resistance_ohm;
w_bi = 2 * pi * cfg.cap_current_bandwidth_hz;
sqrt_term = 2 * cfg.r_l * cfg.c * z * ...
    (cfg.r_l * cfg.c * z + cfg.l) + ...
    cfg.l^2 * (2 + cfg.c^2 * z^2 * w_bi^2);
cap_current_k = (cfg.l + cfg.r_l * cfg.c * z + sqrt(sqrt_term)) / ...
    (cfg.c * z);
end

function sim = simulate_three_loop_case(cfg, enable_inductor_decoupling)
% 建立 alpha-beta 双轴对象并运行三环控制器。
t = (0:cfg.ts:cfg.stop_time).';
sample_count = numel(t);
theta = cfg.omega * t;
sin_theta_table = sin(theta);
cos_theta_table = cos(theta);

v_ref_peak = min(cfg.v_peak, sqrt(2) * cfg.v_rms_slew_vps * t);
v_ref_alpha = v_ref_peak .* cos_theta_table;
v_ref_beta = v_ref_peak .* sin_theta_table;
v_alpha = zeros(sample_count, 1);
v_beta = zeros(sample_count, 1);
i_l_alpha = zeros(sample_count, 1);
i_l_beta = zeros(sample_count, 1);
i_load_alpha = zeros(sample_count, 1);
i_load_beta = zeros(sample_count, 1);
i_cap_alpha = zeros(sample_count, 1);
i_cap_beta = zeros(sample_count, 1);
v_d_ref = zeros(sample_count, 1);
v_q_ref = zeros(sample_count, 1);
v_d = zeros(sample_count, 1);
v_q = zeros(sample_count, 1);
i_l_d_ref = zeros(sample_count, 1);
i_l_q_ref = zeros(sample_count, 1);
i_l_d = zeros(sample_count, 1);
i_l_q = zeros(sample_count, 1);
i_cap_d_ref = zeros(sample_count, 1);
i_cap_q_ref = zeros(sample_count, 1);
i_cap_d = zeros(sample_count, 1);
i_cap_q = zeros(sample_count, 1);
v_bridge_d = zeros(sample_count, 1);
v_bridge_q = zeros(sample_count, 1);

voltage_pi_d = pi_init(cfg.voltage_kp, cfg.voltage_ki, ...
    cfg.ts, cfg.voltage_output_limit_a);
voltage_pi_q = pi_init(cfg.voltage_kp, cfg.voltage_ki, ...
    cfg.ts, cfg.voltage_output_limit_a);
inductor_pi_d = pi_init(cfg.inductor_current_kp, ...
    cfg.inductor_current_ki, cfg.ts, ...
    cfg.inductor_current_output_limit_a);
inductor_pi_q = pi_init(cfg.inductor_current_kp, ...
    cfg.inductor_current_ki, cfg.ts, ...
    cfg.inductor_current_output_limit_a);
bridge_delay_alpha = zeros(cfg.control_delay_samples + 1, 1);
bridge_delay_beta = zeros(cfg.control_delay_samples + 1, 1);

for sample_index = 1:sample_count - 1
    sin_theta = sin_theta_table(sample_index);
    cos_theta = cos_theta_table(sample_index);

    load_is_on = t(sample_index) >= cfg.load_on_time && ...
        t(sample_index) < cfg.load_off_time;
    if load_is_on
        i_load_alpha(sample_index) = ...
            v_alpha(sample_index) / cfg.load_resistance_ohm;
        i_load_beta(sample_index) = ...
            v_beta(sample_index) / cfg.load_resistance_ohm;
    end

    i_cap_alpha(sample_index) = ...
        i_l_alpha(sample_index) - i_load_alpha(sample_index);
    i_cap_beta(sample_index) = ...
        i_l_beta(sample_index) - i_load_beta(sample_index);

    [v_d(sample_index), v_q(sample_index)] = alpha_beta_to_dq( ...
        v_alpha(sample_index), v_beta(sample_index), ...
        sin_theta, cos_theta);
    [i_l_d(sample_index), i_l_q(sample_index)] = alpha_beta_to_dq( ...
        i_l_alpha(sample_index), i_l_beta(sample_index), ...
        sin_theta, cos_theta);
    [i_cap_d(sample_index), i_cap_q(sample_index)] = alpha_beta_to_dq( ...
        i_cap_alpha(sample_index), i_cap_beta(sample_index), ...
        sin_theta, cos_theta);
    % 负载电流由电感电流减去电容电流得到，无需额外负载电流传感器。
    i_load_d_est = i_l_d(sample_index) - i_cap_d(sample_index);
    i_load_q_est = i_l_q(sample_index) - i_cap_q(sample_index);

    v_d_ref(sample_index) = v_ref_peak(sample_index);
    v_q_ref(sample_index) = 0;
    [voltage_pi_d, i_cap_dynamic_d] = pi_step( ...
        voltage_pi_d, v_d_ref(sample_index) - v_d(sample_index));
    [voltage_pi_q, i_cap_dynamic_q] = pi_step( ...
        voltage_pi_q, v_q_ref(sample_index) - v_q(sample_index));

    % C*dv_dq/dt = iL_dq - iload_dq - omega*C*J*v_dq。
    % 因此电压 PI 输出动态电容电流后，加负载电流及旋转项得到 iL 给定。
    i_l_d_ref(sample_index) = i_load_d_est + i_cap_dynamic_d - ...
        cfg.omega * cfg.c * v_q(sample_index);
    i_l_q_ref(sample_index) = i_load_q_est + i_cap_dynamic_q + ...
        cfg.omega * cfg.c * v_d(sample_index);

    [inductor_pi_d, i_cap_correction_d] = pi_step( ...
        inductor_pi_d, i_l_d_ref(sample_index) - i_l_d(sample_index));
    [inductor_pi_q, i_cap_correction_q] = pi_step( ...
        inductor_pi_q, i_l_q_ref(sample_index) - i_l_q(sample_index));

    % 电感电流 PI 输出等效电容电流修正量。叠加当前反馈后，电容电流
    % 比例环的误差正好形成电感电流环要求的桥臂电压修正。
    i_cap_d_ref(sample_index) = ...
        i_cap_d(sample_index) + i_cap_correction_d;
    i_cap_q_ref(sample_index) = ...
        i_cap_q(sample_index) + i_cap_correction_q;

    v_bridge_d(sample_index) = v_d_ref(sample_index) + ...
        cfg.cap_current_k * ...
        (i_cap_d_ref(sample_index) - i_cap_d(sample_index));
    v_bridge_q(sample_index) = v_q_ref(sample_index) + ...
        cfg.cap_current_k * ...
        (i_cap_q_ref(sample_index) - i_cap_q(sample_index));

    if enable_inductor_decoupling
        % omega*L 交叉项属于桥臂电压量，放在最终电压命令处量纲一致。
        v_bridge_d(sample_index) = v_bridge_d(sample_index) + ...
            cfg.r_l * i_l_d(sample_index) - ...
            cfg.omega * cfg.l * i_l_q(sample_index);
        v_bridge_q(sample_index) = v_bridge_q(sample_index) + ...
            cfg.r_l * i_l_q(sample_index) + ...
            cfg.omega * cfg.l * i_l_d(sample_index);
    end

    [v_bridge_alpha, v_bridge_beta] = dq_to_alpha_beta( ...
        v_bridge_d(sample_index), v_bridge_q(sample_index), ...
        sin_theta, cos_theta);
    bridge_magnitude = hypot(v_bridge_alpha, v_bridge_beta);
    if bridge_magnitude > cfg.v_bus
        bridge_scale = cfg.v_bus / bridge_magnitude;
        v_bridge_alpha = bridge_scale * v_bridge_alpha;
        v_bridge_beta = bridge_scale * v_bridge_beta;
    end

    bridge_delay_alpha(2:end) = bridge_delay_alpha(1:end - 1);
    bridge_delay_beta(2:end) = bridge_delay_beta(1:end - 1);
    bridge_delay_alpha(1) = v_bridge_alpha;
    bridge_delay_beta(1) = v_bridge_beta;
    v_bridge_alpha = bridge_delay_alpha(end);
    v_bridge_beta = bridge_delay_beta(end);

    i_l_alpha(sample_index + 1) = i_l_alpha(sample_index) + ...
        cfg.ts * (v_bridge_alpha - v_alpha(sample_index) - ...
        cfg.r_l * i_l_alpha(sample_index)) / cfg.l;
    i_l_beta(sample_index + 1) = i_l_beta(sample_index) + ...
        cfg.ts * (v_bridge_beta - v_beta(sample_index) - ...
        cfg.r_l * i_l_beta(sample_index)) / cfg.l;
    v_alpha(sample_index + 1) = v_alpha(sample_index) + ...
        cfg.ts * (i_l_alpha(sample_index + 1) - ...
        i_load_alpha(sample_index)) / cfg.c;
    v_beta(sample_index + 1) = v_beta(sample_index) + ...
        cfg.ts * (i_l_beta(sample_index + 1) - ...
        i_load_beta(sample_index)) / cfg.c;
end

i_load_alpha(end) = i_load_alpha(end - 1);
i_load_beta(end) = i_load_beta(end - 1);
i_cap_alpha(end) = i_l_alpha(end) - i_load_alpha(end);
i_cap_beta(end) = i_l_beta(end) - i_load_beta(end);
v_d_ref(end) = v_d_ref(end - 1);
v_q_ref(end) = v_q_ref(end - 1);
v_d(end) = v_d(end - 1);
v_q(end) = v_q(end - 1);
i_l_d_ref(end) = i_l_d_ref(end - 1);
i_l_q_ref(end) = i_l_q_ref(end - 1);
i_l_d(end) = i_l_d(end - 1);
i_l_q(end) = i_l_q(end - 1);
i_cap_d_ref(end) = i_cap_d_ref(end - 1);
i_cap_q_ref(end) = i_cap_q_ref(end - 1);
i_cap_d(end) = i_cap_d(end - 1);
i_cap_q(end) = i_cap_q(end - 1);

period_samples = round(cfg.fs / cfg.freq_hz);
v_ref_rms = sqrt(movmean(v_ref_alpha.^2, [period_samples - 1, 0]));
v_alpha_rms = sqrt(movmean(v_alpha.^2, [period_samples - 1, 0]));
v_dq_rms = hypot(v_d, v_q) / sqrt(2);
steady_window = t >= cfg.load_off_time - 0.10 & ...
    t < cfg.load_off_time - 0.02;
load_window = t >= cfg.load_on_time & ...
    t < cfg.load_on_time + 0.12;
unload_window = t >= cfg.load_off_time & ...
    t < cfg.load_off_time + 0.12;

sim.t = t;
sim.v_ref_alpha = v_ref_alpha;
sim.v_ref_beta = v_ref_beta;
sim.v_alpha = v_alpha;
sim.v_beta = v_beta;
sim.v_ref_rms = v_ref_rms;
sim.v_alpha_rms = v_alpha_rms;
sim.v_dq_rms = v_dq_rms;
sim.v_d_ref = v_d_ref;
sim.v_q_ref = v_q_ref;
sim.v_d = v_d;
sim.v_q = v_q;
sim.i_l_d_ref = i_l_d_ref;
sim.i_l_q_ref = i_l_q_ref;
sim.i_l_d = i_l_d;
sim.i_l_q = i_l_q;
sim.i_cap_d_ref = i_cap_d_ref;
sim.i_cap_q_ref = i_cap_q_ref;
sim.i_cap_d = i_cap_d;
sim.i_cap_q = i_cap_q;
sim.v_bridge_d = v_bridge_d;
sim.v_bridge_q = v_bridge_q;
sim.steady_v_rms = mean(v_alpha_rms(steady_window));
sim.load_transient_min_v = min(v_alpha_rms(load_window));
sim.unload_transient_max_v = max(v_alpha_rms(unload_window));
sim.load_settle_time_ms = rms_settle_time_ms( ...
    v_alpha_rms, t, cfg.load_on_time, cfg.v_rms, ...
    cfg.rms_settle_band_v, period_samples);
sim.unload_settle_time_ms = rms_settle_time_ms( ...
    v_alpha_rms, t, cfg.load_off_time, cfg.v_rms, ...
    cfg.rms_settle_band_v, period_samples);
sim.load_dq_settle_time_ms = rms_settle_time_ms( ...
    v_dq_rms, t, cfg.load_on_time, cfg.v_rms, ...
    cfg.rms_settle_band_v, period_samples);
sim.unload_dq_settle_time_ms = rms_settle_time_ms( ...
    v_dq_rms, t, cfg.load_off_time, cfg.v_rms, ...
    cfg.rms_settle_band_v, period_samples);
sim.steady_i_d_error_rms = sqrt(mean( ...
    (i_l_d_ref(steady_window) - i_l_d(steady_window)).^2));
sim.steady_i_q_error_rms = sqrt(mean( ...
    (i_l_q_ref(steady_window) - i_l_q(steady_window)).^2));
sim.max_inductor_current_a = max(hypot(i_l_alpha, i_l_beta));
sim.is_finite = all(isfinite([v_alpha; v_beta; i_l_alpha; i_l_beta]));
end

function state = pi_init(kp, ki, ts, limit)
% 初始化带条件积分抗饱和的 Tustin PI。
state.kp = kp;
state.ki_ts_half = 0.5 * ki * ts;
state.integrator = 0;
state.error1 = 0;
state.limit = limit;
end

function [state, output] = pi_step(state, error)
% 执行一次 Tustin PI 计算。
integrator_candidate = state.integrator + ...
    state.ki_ts_half * (error + state.error1);
raw_output = state.kp * error + integrator_candidate;
output = sat(raw_output, -state.limit, state.limit);
hold_integrator = (raw_output > state.limit && error > 0) || ...
    (raw_output < -state.limit && error < 0);
if ~hold_integrator
    state.integrator = integrator_candidate;
end
state.error1 = error;
end

function [d_axis, q_axis] = alpha_beta_to_dq( ...
    alpha_axis, beta_axis, sin_theta, cos_theta)
% 使用与单相 SRFPI 控制器一致的 Park 变换。
d_axis = cos_theta * alpha_axis + sin_theta * beta_axis;
q_axis = -sin_theta * alpha_axis + cos_theta * beta_axis;
end

function [alpha_axis, beta_axis] = dq_to_alpha_beta( ...
    d_axis, q_axis, sin_theta, cos_theta)
% 执行逆 Park 变换。
alpha_axis = cos_theta * d_axis - sin_theta * q_axis;
beta_axis = sin_theta * d_axis + cos_theta * q_axis;
end

function settle_time_ms = rms_settle_time_ms( ...
    rms_signal, time, event_time, reference, band, hold_samples)
% 计算事件后进入误差带并持续一个基波周期的最早时刻。
inside_band = abs(rms_signal - reference) <= band;
inside_count = movsum(double(inside_band), [0, hold_samples - 1]);
settled_index = find(time >= event_time & ...
    inside_count >= hold_samples, 1, 'first');
if isempty(settled_index)
    settle_time_ms = NaN;
else
    settle_time_ms = 1e3 * (time(settled_index) - event_time);
end
end

function y = sat(x, lower_limit, upper_limit)
% 对标量进行上下限饱和。
y = min(max(x, lower_limit), upper_limit);
end
