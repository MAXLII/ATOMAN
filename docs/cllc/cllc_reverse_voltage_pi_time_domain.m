%% CLLC 反向单母线电压环连续时域原理性仿真
% 本脚本验证 cllc_reverse_voltage_pi_design.m 的单电压环。所有工况共用
% 同一个母线电压状态、同一个 PI 积分状态和同一条时间轴，中途不复位：
%   1) 0 -> 400 V 软启动；
%   2) 400 -> 500 V 斜坡，再做 500 -> 450 V 阶跃；
%   3) 450 V 下做负载阶跃和缓变；
%   4) 最后做 450 -> 400 -> 500 V 电压阶跃。
%
% 功率级采用“反向 FHA 静态曲线 + 一阶母线包络”，不模拟开关纹波。
% 它适合验证控制方向、PI 离散实现、限幅、抗饱和和连续工况切换；器件
% 应力和软开关条件仍需在 PLECS 开关模型中验证。

clear;
clc;
close all;

%% 1. 电路、额定点和控制周期
battery_voltage_v = 48;
minimum_bus_voltage_v = 400;
nominal_bus_voltage_v = 450;
maximum_bus_voltage_v = 500;
nominal_power_w = 3000;
heavy_load_power_w = 3500;
Cbus_f = 1360e-6;

Lr_primary_h = 40e-6;
Cr_primary_f = 80e-9;
Lm_primary_h = 200e-6;
Lr_secondary_h = 0.625e-6;
Cr_secondary_f = 5.12e-6;
turns_ratio = 24/3;

fr_hz = 1/(2*pi*sqrt(Lr_primary_h*Cr_primary_f));
fmin_hz = 32e3;
control_update_frequency_hz = 100e3;
sample_time_s = 1/control_update_frequency_hz;
voltage_crossover_hz = 200;
output_limits = [0, 1];

nominal_load_ohm = nominal_bus_voltage_v^2/nominal_power_w;
heavy_load_ohm = nominal_bus_voltage_v^2/heavy_load_power_w;

%% 2. 由设计公式现场计算 PI，避免手抄参数失配
frequency_scan_hz = linspace(20e3, fr_hz, 12001).';
nominal_full_voltage_v = calculate_reverse_fha_voltage( ...
    frequency_scan_hz, battery_voltage_v, nominal_load_ohm, ...
    Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);
voltage_at_fr_v = interp1(frequency_scan_hz, nominal_full_voltage_v, fr_hz);
voltage_at_fmin_v = interp1(frequency_scan_hz, nominal_full_voltage_v, fmin_hz);
transition_u = voltage_at_fr_v/voltage_at_fmin_v;
operating_frequency_hz = find_high_frequency_crossing( ...
    frequency_scan_hz, nominal_full_voltage_v, nominal_bus_voltage_v);

voltage_slope = gradient(nominal_full_voltage_v, frequency_scan_hz);
Kvf_v_per_hz = interp1(frequency_scan_hz, voltage_slope, ...
    operating_frequency_hz);
frequency_gain_hz_per_u = -(fr_hz-fmin_hz)/(1-transition_u);
Ku_v_per_u = Kvf_v_per_hz*frequency_gain_hz_per_u;
output_time_constant_s = nominal_load_ohm*Cbus_f/2;
output_pole_hz = 1/(2*pi*output_time_constant_s);
delay_s = 1/operating_frequency_hz;

wc = 2*pi*voltage_crossover_hz;
wz = 2*pi*output_pole_hz;
plant_at_crossover = Ku_v_per_u/(1+1j*wc*output_time_constant_s)* ...
    exp(-1j*wc*delay_s);
voltage_Kp = 1/(abs(plant_at_crossover)*sqrt(1+(wz/wc)^2));
voltage_Ki = wz*voltage_Kp;
voltage_ki_step = voltage_Ki*sample_time_s;

fprintf('\nReverse time-domain PI parameters\n');
fprintf('  Kp      = %.12g u/V\n', voltage_Kp);
fprintf('  Ki      = %.12g u/(V*s)\n', voltage_Ki);
fprintf('  ki_step = %.12g u/V/sample\n', voltage_ki_step);

%% 3. 建立实际调制器的 u -> Vbus 静态查表
command_grid = linspace(0, 1, 4001).';
nominal_curve_v = build_reverse_hybrid_curve( ...
    command_grid, transition_u, nominal_load_ohm, battery_voltage_v, ...
    fr_hz, fmin_hz, Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);
heavy_curve_v = build_reverse_hybrid_curve( ...
    command_grid, transition_u, heavy_load_ohm, battery_voltage_v, ...
    fr_hz, fmin_hz, Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);

if max(nominal_curve_v) < maximum_bus_voltage_v
    error('当前标称负载及频率下限无法达到 500 V。');
end
if max(heavy_curve_v) < nominal_bus_voltage_v
    error('重载曲线无法达到 450 V，负载阶跃测试不可执行。');
end

%% 4. 在一条时间轴上定义连续事件
simulation_end_s = 2.20;
time_s = (0:sample_time_s:simulation_end_s).';
sample_count = numel(time_s);

reference_voltage_v = zeros(sample_count, 1);
% 0~0.30 s：软启动至 400 V；0.50~0.80 s：斜坡至 500 V。
reference_voltage_v = linear_segment(time_s, reference_voltage_v, ...
    0.00, 0.30, 0, minimum_bus_voltage_v);
reference_voltage_v(time_s > 0.30) = minimum_bus_voltage_v;
reference_voltage_v = linear_segment(time_s, reference_voltage_v, ...
    0.50, 0.80, minimum_bus_voltage_v, maximum_bus_voltage_v);
reference_voltage_v(time_s > 0.80) = maximum_bus_voltage_v;
% 后续为电压阶跃，状态与积分器均连续。
reference_voltage_v(time_s >= 1.00) = nominal_bus_voltage_v;
reference_voltage_v(time_s >= 1.70) = minimum_bus_voltage_v;
reference_voltage_v(time_s >= 1.95) = maximum_bus_voltage_v;

% 1.15 s 在 450 V 稳态处阶跃加重负载；1.35~1.55 s 缓变恢复。
load_resistance_ohm = nominal_load_ohm*ones(sample_count, 1);
load_resistance_ohm(time_s >= 1.15 & time_s < 1.35) = heavy_load_ohm;
ramp_mask = time_s >= 1.35 & time_s < 1.55;
load_resistance_ohm(ramp_mask) = heavy_load_ohm + ...
    (nominal_load_ohm-heavy_load_ohm).* ...
    (time_s(ramp_mask)-1.35)/(1.55-1.35);

%% 5. 状态初始化与单环离散仿真
bus_voltage_v = nan(sample_count, 1);
normalized_command = nan(sample_count, 1);
applied_command = nan(sample_count, 1);
integral_state = nan(sample_count, 1);
switching_frequency_hz = nan(sample_count, 1);
phase_shift_duty = nan(sample_count, 1);
load_power_w = nan(sample_count, 1);
saturated = false(sample_count, 1);

bus_voltage_v(1) = 0;
integrator = 0;
delay_samples = max(1, round(delay_s/sample_time_s));
command_delay_line = zeros(delay_samples, 1);

for sample_index = 1:sample_count-1
    voltage_error_v = reference_voltage_v(sample_index)- ...
        bus_voltage_v(sample_index);

    % 条件积分只在未饱和，或误差能把输出拉回范围时执行。
    raw_command = voltage_Kp*voltage_error_v+integrator;
    upper_windup = raw_command > output_limits(2) && voltage_error_v > 0;
    lower_windup = raw_command < output_limits(1) && voltage_error_v < 0;
    if ~(upper_windup || lower_windup)
        integrator = integrator+voltage_ki_step*voltage_error_v;
    end
    raw_command = voltage_Kp*voltage_error_v+integrator;
    limited_command = min(max(raw_command, output_limits(1)), output_limits(2));

    normalized_command(sample_index) = limited_command;
    integral_state(sample_index) = integrator;
    saturated(sample_index) = abs(raw_command-limited_command) > 10*eps;

    % 一个工作点开关周期的命令延时；移位寄存器不会重置。
    applied_command(sample_index) = command_delay_line(1);
    command_delay_line(1:end-1) = command_delay_line(2:end);
    command_delay_line(end) = limited_command;

    current_load = load_resistance_ohm(sample_index);
    load_blend = (nominal_load_ohm-current_load)/ ...
        (nominal_load_ohm-heavy_load_ohm);
    load_blend = min(max(load_blend, 0), 1);
    current_curve_v = (1-load_blend)*nominal_curve_v + ...
        load_blend*heavy_curve_v;
    equilibrium_voltage_v = interp1(command_grid, current_curve_v, ...
        applied_command(sample_index), 'linear');

    % 一阶精确离散：x[k+1]=xeq+(x[k]-xeq)*exp(-Ts/tau)。
    current_tau_s = current_load*Cbus_f/2;
    decay = exp(-sample_time_s/current_tau_s);
    bus_voltage_v(sample_index+1) = equilibrium_voltage_v + ...
        (bus_voltage_v(sample_index)-equilibrium_voltage_v)*decay;

    [switching_frequency_hz(sample_index), phase_shift_duty(sample_index)] = ...
        decode_reverse_modulation(applied_command(sample_index), ...
        transition_u, fr_hz, fmin_hz);
    load_power_w(sample_index) = bus_voltage_v(sample_index)^2/current_load;
end

% 补齐最后一个采样点，仅用于绘图。
normalized_command(end) = normalized_command(end-1);
applied_command(end) = applied_command(end-1);
integral_state(end) = integral_state(end-1);
switching_frequency_hz(end) = switching_frequency_hz(end-1);
phase_shift_duty(end) = phase_shift_duty(end-1);
load_power_w(end) = bus_voltage_v(end)^2/load_resistance_ohm(end);
saturated(end) = saturated(end-1);

%% 6. 自动检查与连续时域图
steady_mask = time_s >= 0.90 & time_s < 0.99;
steady_500_error_v = max(abs(bus_voltage_v(steady_mask)-500));
final_mask = time_s >= 2.12;
final_500_error_v = max(abs(bus_voltage_v(final_mask)-500));
fprintf('  500 V ramp final-window max error : %.3f V\n', steady_500_error_v);
fprintf('  400->500 step final max error     : %.3f V\n', final_500_error_v);
fprintf('  Command range                     : %.4f ... %.4f\n', ...
    min(normalized_command), max(normalized_command));

figure('Name', 'CLLC reverse continuous time-domain response', 'Color', 'w');
tiledlayout(5, 1, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile;
plot(time_s, reference_voltage_v, '--', 'LineWidth', 1.2); hold on;
plot(time_s, bus_voltage_v, 'LineWidth', 1.4); grid on;
ylabel('Vbus / V'); legend('Vref', 'Vbus', 'Location', 'best');
title('Continuous reverse CLLC single-voltage-loop simulation');

nexttile;
plot(time_s, load_power_w/1e3, 'LineWidth', 1.3); grid on;
ylabel('Power / kW');

nexttile;
plot(time_s, normalized_command, 'LineWidth', 1.3); hold on;
plot(time_s, applied_command, '--', 'LineWidth', 1.0);
yline(transition_u, ':', 'PSM/PFM transition'); grid on;
ylabel('u'); legend('PI output', 'Applied u', 'Location', 'best');

nexttile;
yyaxis left;
plot(time_s, switching_frequency_hz/1e3, 'LineWidth', 1.3);
ylabel('fs / kHz');
yyaxis right;
plot(time_s, 100*phase_shift_duty, 'LineWidth', 1.1);
ylabel('Duty / %'); grid on;

nexttile;
plot(time_s, integral_state, 'LineWidth', 1.2); hold on;
stairs(time_s, double(saturated), ':', 'LineWidth', 1.0); grid on;
ylabel('PI state'); xlabel('Time / s');
legend('Integral', 'Saturated', 'Location', 'best');

sgtitle('Battery 48 V to bus 400~500 V: one uninterrupted simulation');

%% Local functions
function values = linear_segment(time_s, values, t0, t1, y0, y1)
    mask = time_s >= t0 & time_s <= t1;
    values(mask) = y0+(y1-y0)*(time_s(mask)-t0)/(t1-t0);
end

function curve_v = build_reverse_hybrid_curve(command, ut, load_ohm, ...
    battery_v, fr_hz, fmin_hz, Lrp, Crp, Lm, Lrs, Crs, n)
    curve_v = zeros(size(command));
    psm = command <= ut;
    v_fr = calculate_reverse_fha_voltage(fr_hz, battery_v, load_ohm, ...
        Lrp, Crp, Lm, Lrs, Crs, n);
    curve_v(psm) = v_fr*command(psm)/ut;
    pfm = ~psm;
    fs = fr_hz-(command(pfm)-ut)/(1-ut)*(fr_hz-fmin_hz);
    curve_v(pfm) = calculate_reverse_fha_voltage(fs, battery_v, load_ohm, ...
        Lrp, Crp, Lm, Lrs, Crs, n);
end

function [fs_hz, duty] = decode_reverse_modulation(u, ut, fr_hz, fmin_hz)
    if u <= ut
        fs_hz = fr_hz;
        duty = 0.5*u/ut;
    else
        fs_hz = fr_hz-(u-ut)/(1-ut)*(fr_hz-fmin_hz);
        duty = 0.5;
    end
end

function output_voltage_v = calculate_reverse_fha_voltage( ...
    frequency_hz, battery_voltage_v, load_resistance_ohm, ...
    Lrp_h, Crp_f, Lm_h, Lrs_h, Crs_f, turns_ratio)
    w = 2*pi*frequency_hz;
    Zrp = 1j*w*Lrp_h+1./(1j*w*Crp_f);
    Zrs = 1j*w*Lrs_h+1./(1j*w*Crs_f);
    Zm = 1j*w*Lm_h;
    Rac = (8/pi^2)*load_resistance_ohm;
    Ztotal = Zrp+Rac;
    Zparallel_primary = Zm.*Ztotal./(Zm+Ztotal);
    Zparallel_secondary = Zparallel_primary/turns_ratio^2;
    Hsecondary = Zparallel_secondary./(Zrs+Zparallel_secondary);
    Hprimary = Rac./Ztotal;
    output_voltage_v = battery_voltage_v*abs(Hsecondary.*Hprimary*turns_ratio);
end

function crossing_hz = find_high_frequency_crossing(frequency_hz, curve, target)
    index = find((curve(1:end-1)-target).*(curve(2:end)-target) <= 0, ...
        1, 'last');
    if isempty(index)
        crossing_hz = NaN;
    else
        crossing_hz = interp1(curve(index:index+1), ...
            frequency_hz(index:index+1), target);
    end
end
