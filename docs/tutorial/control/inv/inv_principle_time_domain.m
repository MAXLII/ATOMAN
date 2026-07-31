%% 单相逆变器轻载主动阻尼与负载电流前馈仿真
% 验证多谐振电压 PR 外环、电流 PI 内环、负载电流估算前馈、
% 参考电容电流前馈和电容电流主动阻尼在不同负载下的动态表现。
clear; clc; close all;

cfg.fs = 30e3;
cfg.ts = 1 / cfg.fs;
cfg.stop_time = 1.90;
cfg.l = 440e-6;
cfg.c = 12e-6;
cfg.r_l = 0.001;
cfg.v_bus = 400;
cfg.freq_hz = 50;
cfg.v_rms = 230;
cfg.v_rms_slew_vps = 212;
cfg.load_on_time = 1.25;
cfg.load_off_time = 1.60;

% 电压 PR 恢复为低增益，负载功率主要由电流前馈提供。
cfg.pr_bandwidth_hz = 2;
cfg.volt_fc_hz = 120;
cfg.volt_pm_deg = 60;
cfg.volt_h1_kr_scale = 1;
cfg.volt_correction_limit_a = 50;
cfg.total_current_limit_a = 60;

% 电流环与 LC 谐振频率拉开，并提高相位裕度。
cfg.curr_fc_hz = 1000;
cfg.curr_pm_deg = 60;
cfg.curr_output_limit_v = 100;

% 由 iL-C*dvC/dt 估算负载电流；虚拟电阻用于电容电流主动阻尼。
cfg.load_estimator_fc_hz = 1000;
cfg.cap_current_estimator_fc_hz = 5000;
cfg.active_damping_ohm = 1.0;

load_power_cases_w = [0, 1000, 2000, 3000, 6600];
case_count = numel(load_power_cases_w);
simulations = cell(case_count, 1);

fprintf('LC resonance frequency: %.3f Hz\n', ...
    1 / (2 * pi * sqrt(cfg.l * cfg.c)));
fprintf('Current loop: fc=%.1f Hz, PM=%.1f deg; active damping=%.3f ohm\n', ...
    cfg.curr_fc_hz, cfg.curr_pm_deg, cfg.active_damping_ohm);

for case_index = 1:case_count
    simulations{case_index} = simulate_inv_case(cfg, load_power_cases_w(case_index));
end

figure('Name', 'INV multi-load active damping verification', ...
    'Color', 'w', 'Position', [80, 60, 1300, 1050]);
tiledlayout(case_count, 1, 'TileSpacing', 'compact', 'Padding', 'compact');
for case_index = 1:case_count
    sim = simulations{case_index};
    nexttile;
    plot(sim.t, sim.v_ref_rms, '--', sim.t, sim.v_cap_rms, 'LineWidth', 1.0);
    grid on;
    ylim([0, 280]);
    ylabel('RMS / V');
    title(sprintf('%g W: steady %.2f V, phase error %.3f deg, residual %.3f V RMS', ...
        sim.load_power_command_w, sim.steady_v_rms, ...
        sim.steady_phase_error_deg, sim.steady_residual_rms_v));
    xline(cfg.load_on_time, '--', 'load on', 'HandleVisibility', 'off');
    xline(cfg.load_off_time, '--', 'load off', 'HandleVisibility', 'off');
    if case_index == 1
        legend('reference', 'feedback', 'Location', 'best');
    end
end
xlabel('Time / s');

result_path = fullfile(tempdir, 'inv_principle_multi_load_result.png');
exportgraphics(gcf, result_path, 'Resolution', 150);

fprintf('\nLoad(W)  Vsteady(V)  Phase(deg)  Pactual(W)  Residual(V)  LoadMin(V)  UnloadMax(V)  PI_sat(%%)\n');
for case_index = 1:case_count
    sim = simulations{case_index};
    fprintf('%7.0f  %10.3f  %10.3f  %10.1f  %11.4f  %10.3f  %12.3f  %9.3f\n', ...
        sim.load_power_command_w, sim.steady_v_rms, ...
        sim.steady_phase_error_deg, sim.steady_power_w, ...
        sim.steady_residual_rms_v, sim.load_transient_min_v, ...
        sim.unload_transient_max_v, 100 * sim.curr_pi_saturation_ratio);
end
fprintf('Result figure: %s\n', result_path);

function sim = simulate_inv_case(cfg, load_power_w)
ts = cfg.ts;
t = (0:ts:cfg.stop_time).';
n = numel(t);
omega = 2 * pi * cfg.freq_hz;

if load_power_w > 0
    load_resistance = cfg.v_rms^2 / load_power_w;
else
    load_resistance = inf;
end
r_load = inf(n, 1);
r_load(t >= cfg.load_on_time & t < cfg.load_off_time) = load_resistance;

pr_wc = 2 * pi * cfg.pr_bandwidth_hz;
volt_wx = 2 * pi * cfg.volt_fc_hz;
volt_pm = deg2rad(cfg.volt_pm_deg);
volt_kp = cfg.c * volt_wx * sin(volt_pm) - ...
    2 * cfg.c * pr_wc * cos(volt_pm);
volt_kr_base = cfg.c * volt_wx^2 * cos(volt_pm) / (2 * pr_wc);
volt_h1_kr = cfg.volt_h1_kr_scale * volt_kr_base;
volt_pr_h1 = pr_init_state(volt_kp, volt_h1_kr, omega, pr_wc, ts, ...
    cfg.volt_correction_limit_a);
volt_pr_h3 = pr_init_state(0, volt_kr_base, 3 * omega, pr_wc, ts, ...
    cfg.volt_correction_limit_a);
volt_pr_h5 = pr_init_state(0, volt_kr_base, 5 * omega, pr_wc, ts, ...
    cfg.volt_correction_limit_a);
volt_pr_h7 = pr_init_state(0, volt_kr_base, 7 * omega, pr_wc, ts, ...
    cfg.volt_correction_limit_a);

curr_wx = 2 * pi * cfg.curr_fc_hz;
curr_pm = deg2rad(cfg.curr_pm_deg);
curr_kp = cfg.l * curr_wx * sin(curr_pm);
curr_ki = curr_kp * curr_wx / tan(curr_pm);
curr_pi = pi_init_state(curr_kp, curr_ki, ts, cfg.curr_output_limit_v);

v_ref_pk = zeros(n, 1);
v_ref = zeros(n, 1);
v_cap = zeros(n, 1);
i_l = zeros(n, 1);
i_ref = zeros(n, 1);
i_load_est = zeros(n, 1);
i_cap_est = zeros(n, 1);
curr_out = zeros(n, 1);
v_bridge = zeros(n, 1);
phase = zeros(n, 1);
curr_pi_saturated = false(n, 1);
estimator_alpha = 1 - exp(-2 * pi * cfg.load_estimator_fc_hz * ts);
cap_estimator_alpha = 1 - exp(-2 * pi * cfg.cap_current_estimator_fc_hz * ts);

for k = 1:n-1
    v_ref_pk(k + 1) = ramp_to(v_ref_pk(k), sqrt(2) * cfg.v_rms, ...
        sqrt(2) * cfg.v_rms_slew_vps * ts);
    ref_pk_slew = (v_ref_pk(k + 1) - v_ref_pk(k)) / ts;
    theta = 2 * pi * phase(k);
    v_ref(k) = v_ref_pk(k) * cos(theta);
    dv_ref_dt = ref_pk_slew * cos(theta) - omega * v_ref_pk(k) * sin(theta);
    i_cap_ref = cfg.c * dv_ref_dt;

    if k > 1
        i_cap_raw = cfg.c * (v_cap(k) - v_cap(k - 1)) / ts;
    else
        i_cap_raw = 0;
    end
    if k > 1
        i_cap_est(k) = i_cap_est(k - 1) + ...
            cap_estimator_alpha * (i_cap_raw - i_cap_est(k - 1));
    else
        i_cap_est(k) = 0;
    end
    i_load_raw = i_l(k) - i_cap_est(k);
    if k > 1
        i_load_est(k) = i_load_est(k - 1) + ...
            estimator_alpha * (i_load_raw - i_load_est(k - 1));
    else
        i_load_est(k) = 0;
    end

    [volt_pr_h1, volt_h1_out] = pr_step(volt_pr_h1, v_ref(k), v_cap(k));
    [volt_pr_h3, volt_h3_out] = pr_step(volt_pr_h3, v_ref(k), v_cap(k));
    [volt_pr_h5, volt_h5_out] = pr_step(volt_pr_h5, v_ref(k), v_cap(k));
    [volt_pr_h7, volt_h7_out] = pr_step(volt_pr_h7, v_ref(k), v_cap(k));
    volt_correction = sat(volt_h1_out + volt_h3_out + volt_h5_out + volt_h7_out, ...
        -cfg.volt_correction_limit_a, cfg.volt_correction_limit_a);
    i_ref(k) = sat(i_load_est(k) + i_cap_ref + volt_correction, ...
        -cfg.total_current_limit_a, cfg.total_current_limit_a);

    [curr_pi, curr_out(k)] = pi_step(curr_pi, i_ref(k), i_l(k));
    curr_pi_saturated(k) = abs(curr_out(k)) >= cfg.curr_output_limit_v;
    v_pwm_cmd = v_cap(k) + curr_out(k) - cfg.active_damping_ohm * i_cap_est(k);
    v_bridge(k) = sat(v_pwm_cmd, -cfg.v_bus, cfg.v_bus);

    i_l(k + 1) = i_l(k) + ts * ...
        (v_bridge(k) - v_cap(k) - cfg.r_l * i_l(k)) / cfg.l;
    if isfinite(r_load(k))
        i_load = v_cap(k) / r_load(k);
    else
        i_load = 0;
    end
    v_cap(k + 1) = v_cap(k) + ts * (i_l(k + 1) - i_load) / cfg.c;
    phase(k + 1) = mod(phase(k) + cfg.freq_hz * ts, 1);
end

v_ref(end) = v_ref_pk(end) * cos(2 * pi * phase(end));
i_ref(end) = i_ref(end - 1);
i_load_est(end) = i_load_est(end - 1);
i_cap_est(end) = i_cap_est(end - 1);
curr_out(end) = curr_out(end - 1);
v_bridge(end) = v_bridge(end - 1);
assert(all(isfinite([v_cap; i_l; v_bridge])), ...
    'INV simulation diverged at %.0f W.', load_power_w);

period_samples = round(cfg.fs / cfg.freq_hz);
v_ref_rms = sqrt(movmean(v_ref.^2, [period_samples - 1, 0]));
v_cap_rms = sqrt(movmean(v_cap.^2, [period_samples - 1, 0]));
load_current = zeros(n, 1);
load_mask = isfinite(r_load);
load_current(load_mask) = v_cap(load_mask) ./ r_load(load_mask);
load_power = v_cap .* load_current;
load_power_avg = movmean(load_power, [period_samples - 1, 0]);

steady_window = t >= cfg.load_off_time - 0.10 & t < cfg.load_off_time - 0.02;
load_transient_window = t >= cfg.load_on_time & t < cfg.load_on_time + 0.12;
unload_transient_window = t >= cfg.load_off_time & t < cfg.load_off_time + 0.12;
[steady_ref_rms, steady_ref_phase] = sine_metric(v_ref, t, steady_window, cfg.freq_hz);
[steady_v_rms, steady_v_phase, steady_residual_rms_v] = ...
    sine_metric(v_cap, t, steady_window, cfg.freq_hz);

sim.t = t;
sim.v_ref_rms = v_ref_rms;
sim.v_cap_rms = v_cap_rms;
sim.load_power_command_w = load_power_w;
sim.steady_ref_rms = steady_ref_rms;
sim.steady_v_rms = steady_v_rms;
sim.steady_phase_error_deg = wrap_to_180(rad2deg(steady_v_phase - steady_ref_phase));
sim.steady_power_w = mean(load_power_avg(steady_window));
sim.steady_residual_rms_v = steady_residual_rms_v;
sim.load_transient_min_v = min(v_cap_rms(load_transient_window));
sim.unload_transient_max_v = max(v_cap_rms(unload_transient_window));
sim.curr_pi_saturation_ratio = mean(curr_pi_saturated);
sim.max_inductor_current_a = max(abs(i_l));
end

function state = pr_init_state(kp, kr, w0, wc, ts, limit)
state.e = zeros(1, 3);
state.u = zeros(1, 3);
d0 = ts^2 * w0^2 + 4 * ts * wc + 4;
state.a1 = (2 * ts^2 * w0^2 - 8) / d0;
state.a2 = (ts^2 * w0^2 - 4 * ts * wc + 4) / d0;
state.b0 = (ts^2 * kp * w0^2 + 4 * ts * kp * wc + ...
    4 * ts * kr * wc + 4 * kp) / d0;
state.b1 = (2 * ts^2 * kp * w0^2 - 8 * kp) / d0;
state.b2 = (ts^2 * kp * w0^2 - 4 * ts * kp * wc - ...
    4 * ts * kr * wc + 4 * kp) / d0;
state.limit = limit;
end

function [state, output] = pr_step(state, ref, feedback)
e0 = ref - feedback;
raw = state.b0 * e0 + state.b1 * state.e(1) + state.b2 * state.e(2) - ...
    state.a1 * state.u(1) - state.a2 * state.u(2);
output = sat(raw, -state.limit, state.limit);
hold_state = (raw > state.limit && e0 > 0) || ...
    (raw < -state.limit && e0 < 0);
if ~hold_state
    state.e = [e0, state.e(1), state.e(2)];
    state.u = [output, state.u(1), state.u(2)];
end
end

function state = pi_init_state(kp, ki, ts, limit)
state.b0 = (ts * ki + 2 * kp) / 2;
state.b1 = (ts * ki - 2 * kp) / 2;
state.b1_inv = 1 / state.b1;
state.e1 = 0;
state.u0 = 0;
state.limit = limit;
end

function [state, output] = pi_step(state, ref, feedback)
e0 = ref - feedback;
u1 = state.u0;
state.u0 = state.b0 * e0 + state.b1 * state.e1 + u1;
state.u0 = sat(state.u0, -state.limit, state.limit);
if abs(state.u0) >= state.limit
    state.e1 = (state.u0 - state.b0 * e0 - u1) * state.b1_inv;
else
    state.e1 = e0;
end
output = state.u0;
end

function [rms_value, phase, residual_rms] = sine_metric(signal, time, window, freq)
x = 2 * pi * freq * time(window);
basis = [cos(x), sin(x), ones(sum(window), 1)];
coefficient = basis \ signal(window);
fitted = basis * coefficient;
rms_value = hypot(coefficient(1), coefficient(2)) / sqrt(2);
phase = atan2(-coefficient(2), coefficient(1));
residual_rms = sqrt(mean((signal(window) - fitted).^2));
end

function angle = wrap_to_180(angle)
angle = mod(angle + 180, 360) - 180;
end

function y = sat(x, lo, hi)
y = min(max(x, lo), hi);
end

function y = ramp_to(x, target, step)
y = x + sat(target - x, -step, step);
end
