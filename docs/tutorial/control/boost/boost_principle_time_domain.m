%% 两相 Boost 原理性连续时域仿真：12~60 V、阶跃、斜坡、CR/CC/CV
% 本脚本依据 code/ctrl/boost 的控制结构建立平均模型，用来检查：
%   1. 输入电压和输出参考在 12~60 V 范围内变化时的控制方向；
%   2. 输出电压阶跃与斜坡响应；
%   3. CR（恒阻）、CC（恒流）和 CV（恒压吸收端）负载切换；
%   4. 功率、输入电流和输出电流限制之间的竞争；
%   5. 两相电感电流内环和输出电压外环的连续运行。
%
% 这是开关周期平均模型，不逐个求解 MOSFET 和二极管换向，因此适合
% 验证控制器原理和低频动态，不用于判断开关纹波、器件应力和死区影响。
%
% Boost 的基本平均方程为：
%
%   L*diL/dt = Vin - (1-D)*Vout
%   C*dVout/dt = (1-D)*(iL1+iL2) - Iload
%
% code/ctrl/boost 输出的是 compare=(Vin-vL_cmd)/Vout。若把主开关占空比
% 写成 D=1-compare，就得到：
%
%   D = 1 - Vin/Vout + vL_cmd/Vout
%
% 因而电流误差为正时，电流 PI 墠大 vL_cmd，并相应提高升压占空比。

clear;
clc;
close all;

%% 1. 与 code/ctrl/boost 一致的功率级参数
phase_count = 2;
inductance_per_phase_h = 1.5e-6;
input_capacitance_f = 4*330e-6;
output_capacitance_f = 4*1000e-6;

% PLECS Boost 工程使用 100 kHz 控制中断、300 kHz PWM；慢任务为 100 us。
control_frequency_hz = 100e3;
control_period_s = 1/control_frequency_hz;
pwm_frequency_hz = 300e3;
pwm_period_s = 1/pwm_frequency_hz;
task_period_s = 100e-6;
task_divider = round(task_period_s/control_period_s);

% 平均模型的占空比保留 5% 关断时间，避免 D=1 时输出与输入完全解耦。
duty_limits = [0, 0.95];

%% 2. 根据 boost_cfg.h 的设计公式得到物理域 PI 参数
% 电压环对象：Iout -> Vout，近似为 1/(Cout*s)。
voltage_phase_margin_rad = deg2rad(60);
voltage_crossover_rad_s = 2*pi*1200;
voltage_Kp_a_per_v = sin(voltage_phase_margin_rad)* ...
    voltage_crossover_rad_s*output_capacitance_f;
voltage_Ki_a_per_vs = voltage_Kp_a_per_v* ...
    voltage_crossover_rad_s/tan(voltage_phase_margin_rad);

% 输入电压限制环对象：输入电流改变 Cin 电压，近似为 1/(Cin*s)。
input_limit_phase_margin_rad = deg2rad(45);
input_limit_crossover_rad_s = 2*pi*500;
input_limit_Kp_a_per_v = sin(input_limit_phase_margin_rad)* ...
    input_limit_crossover_rad_s*input_capacitance_f;
input_limit_Ki_a_per_vs = input_limit_Kp_a_per_v* ...
    input_limit_crossover_rad_s/tan(input_limit_phase_margin_rad);

% 每相电流环对象：vL_cmd -> iL，等于 1/(L*s)。
current_phase_margin_rad = deg2rad(45);
current_crossover_rad_s = 2*pi*6000;
current_Kp_v_per_a = sin(current_phase_margin_rad)* ...
    current_crossover_rad_s*inductance_per_phase_h;
current_Ki_v_per_as = current_Kp_v_per_a* ...
    current_crossover_rad_s/tan(current_phase_margin_rad);

%% 3. 保护和负载参数
power_limit_w = 1200;
input_current_limit_a = 80;
output_current_limit_a = 25;
input_voltage_limit_v = 12;

% 三种电子负载均在 60 V 附近形成约 600 W 或更低的工作点。
cr_resistance_ohm = 6;       % 60 V / 6 ohm = 10 A
cc_current_a = 10;           % 恒流 10 A
cv_series_resistance_ohm = 2;% 恒压吸收端串联等效电阻

% 模式编号只用于记录和画图。
LOAD_MODE_CR = 1;
LOAD_MODE_CC = 2;
LOAD_MODE_CV = 3;

%% 4. 一次连续仿真的事件时间表
simulation_end_s = 0.650;
time_s = (0:control_period_s:simulation_end_s).';
sample_count = numel(time_s);

% 输入源电压覆盖 12~60 V：先阶跃 12->24 V，再斜坡 24->60 V。
% 在 60 V 直通边界短暂停留后阶跃回 24 V，让后续 CC/CV 测试回到
% 可调占空比的升压区，而不是一直停留在 Vin=Vout、D=0 的边界。
input_voltage_v = 12*ones(sample_count, 1);
input_voltage_v(time_s >= 0.140) = 24;
input_ramp = min(max((time_s-0.200)/(0.260-0.200), 0), 1);
input_voltage_v = input_voltage_v+36*input_ramp;
input_voltage_v(time_s >= 0.300) = 24;

% 输出参考覆盖 12~60 V，并同时包含斜坡与阶跃。
output_reference_v = 12*ones(sample_count, 1);
reference_ramp_1 = min(max((time_s-0.020)/(0.080-0.020), 0), 1);
output_reference_v = output_reference_v+24*reference_ramp_1; % 12 -> 36 V
output_reference_v(time_s >= 0.100) = 60;                    % 阶跃到 60 V
output_reference_v(time_s >= 0.320) = 48;                    % CC 下阶跃到 48 V
reference_ramp_2 = min(max((time_s-0.360)/(0.420-0.360), 0), 1);
ramp_2_window = time_s >= 0.360 & time_s < 0.440;
output_reference_v(ramp_2_window) = 48+12*reference_ramp_2(ramp_2_window);
output_reference_v(time_s >= 0.440) = 60;

% 负载模式连续切换：0~280 ms 为 CR，280~440 ms 为 CC，之后为 CV。
load_mode = LOAD_MODE_CR*ones(sample_count, 1);
load_mode(time_s >= 0.280 & time_s < 0.440) = LOAD_MODE_CC;
load_mode(time_s >= 0.440) = LOAD_MODE_CV;

% CV 负载等效为“恒压吸收端 + 串联电阻”。吸收端电压先从 24 V
% 阶跃到 36 V，再斜坡到 48 V，模拟电池或另一条直流母线变化。
cv_load_voltage_v = 24*ones(sample_count, 1);
cv_load_voltage_v(time_s >= 0.500) = 36;
cv_ramp = min(max((time_s-0.540)/(0.600-0.540), 0), 1);
cv_load_voltage_v = cv_load_voltage_v+12*cv_ramp;

%% 5. 状态、控制器存储量和记录向量
output_voltage_v = zeros(sample_count, 1);
phase_current_a = zeros(sample_count, phase_count);
input_current_a = zeros(sample_count, 1);
output_current_a = zeros(sample_count, 1);
load_power_w = zeros(sample_count, 1);
total_current_reference_a = zeros(sample_count, 1);
current_limit_reference_a = zeros(sample_count, 1);
power_current_limit_a = zeros(sample_count, 1);
input_current_path_limit_a = zeros(sample_count, 1);
output_current_path_limit_a = zeros(sample_count, 1);
input_voltage_path_limit_a = zeros(sample_count, 1);
inductor_voltage_command_v = zeros(sample_count, phase_count);
duty = zeros(sample_count, phase_count);
voltage_loop_saturated = false(sample_count, 1);
current_loop_saturated = false(sample_count, phase_count);

% 从 Vin=Vout=12 V、CR 负载的近似平衡点开始，避免把无关的零电压
% 充电过程混入参考阶跃指标。初始负载电流为 2 A，两相各承担 1 A。
output_voltage_v(1) = 12;
initial_load_current_a = output_voltage_v(1)/cr_resistance_ohm;
phase_current_a(1, :) = initial_load_current_a/phase_count;

% 电压 PI 的积分量代表总输入电流参考；电流 PI 的积分量代表每相
% 电感电压命令。初始稳态 vL=0，因此电流环积分从 0 V 开始。
voltage_integral_a = initial_load_current_a;
input_limit_integral_a = input_current_limit_a;
current_integral_v = zeros(1, phase_count);
held_total_current_limit_a = input_current_limit_a;

%% 6. 离散闭环平均仿真
for index = 1:sample_count-1
    vin = input_voltage_v(index);
    vout = max(output_voltage_v(index), 0.1);

    % (1) 根据当前电子负载模式计算输出侧负载电流。
    switch load_mode(index)
        case LOAD_MODE_CR
            load_current = vout/cr_resistance_ohm;
        case LOAD_MODE_CC
            % 低于 2 V 时线性减载，避免理想恒流负载在零电压附近产生
            % 不符合实际的无限负阻效应。
            load_current = cc_current_a*min(vout/2, 1);
        case LOAD_MODE_CV
            load_current = max( ...
                (vout-cv_load_voltage_v(index))/cv_series_resistance_ohm, 0);
        otherwise
            error('Unknown load mode.');
    end
    output_current_a(index) = load_current;
    load_power_w(index) = vout*load_current;

    % (2) 慢任务计算四条总输入/电感电流限制，并取最小值。
    % 输入电压限制 PI 使用 error=Vin-Vin_limit：Vin 越低，其允许输入
    % 电流越小；Vin 高于阈值时输出保持在 100 A 上限附近。
    if mod(index-1, task_divider) == 0
        input_voltage_error_v = vin-input_voltage_limit_v;
        [input_voltage_limit, input_limit_integral_a, ~] = pi_step_aw( ...
            input_voltage_error_v, input_limit_integral_a, ...
            input_limit_Kp_a_per_v, input_limit_Ki_a_per_vs, ...
            task_period_s, -10, 100);

        power_limit_current = power_limit_w/max(vin, 0.1);
        input_limit_current = input_current_limit_a;
        output_limit_as_input_current = ...
            output_current_limit_a*vout/max(vin, 0.1);

        held_total_current_limit_a = min([100, input_voltage_limit, ...
            power_limit_current, input_limit_current, ...
            output_limit_as_input_current]);
        held_total_current_limit_a = max(held_total_current_limit_a, 0);
    end

    current_limit_reference_a(index) = held_total_current_limit_a;
    power_current_limit_a(index) = power_limit_w/max(vin, 0.1);
    input_current_path_limit_a(index) = input_current_limit_a;
    output_current_path_limit_a(index) = ...
        output_current_limit_a*vout/max(vin, 0.1);
    input_voltage_path_limit_a(index) = min(max( ...
        input_limit_Kp_a_per_v*(vin-input_voltage_limit_v)+ ...
        input_limit_integral_a, -10), 100);

    % (3) 输出电压 PI 产生“两相总电流参考”，动态上限来自慢任务。
    voltage_error_v = output_reference_v(index)-vout;
    [total_current_reference, voltage_integral_a, voltage_sat] = pi_step_aw( ...
        voltage_error_v, voltage_integral_a, ...
        voltage_Kp_a_per_v, voltage_Ki_a_per_vs, control_period_s, ...
        0, held_total_current_limit_a);
    total_current_reference_a(index) = total_current_reference;
    voltage_loop_saturated(index) = voltage_sat;
    phase_current_reference_a = total_current_reference/phase_count;

    % (4) 两个电流环分别输出每相电感电压命令，再由输入电压前馈换算
    % 为实际主开关占空比。两相参数相同，但保留独立状态便于以后加入
    % 电感偏差和均流误差。
    for phase = 1:phase_count
        current_error_a = phase_current_reference_a- ...
            phase_current_a(index, phase);
        [v_l_command, current_integral_v(phase), current_sat] = pi_step_aw( ...
            current_error_a, current_integral_v(phase), ...
            current_Kp_v_per_a, current_Ki_v_per_as, control_period_s, ...
            -60, 60);
        inductor_voltage_command_v(index, phase) = v_l_command;
        current_loop_saturated(index, phase) = current_sat;

        duty_command = 1-vin/vout+v_l_command/vout;
        duty(index, phase) = min(max( ...
            duty_command, duty_limits(1)), duty_limits(2));

        % 每相电感状态推进。电流不能为负，符合单向 Boost 功率级。
        actual_inductor_voltage_v = vin- ...
            (1-duty(index, phase))*vout;
        next_phase_current_a = phase_current_a(index, phase)+ ...
            control_period_s/inductance_per_phase_h* ...
            actual_inductor_voltage_v;
        phase_current_a(index+1, phase) = max(next_phase_current_a, 0);
    end

    % (5) 输出电容由两相二极管平均电流充电，并向电子负载放电。
    diode_output_current_a = sum( ...
        (1-duty(index, :)).*phase_current_a(index, :));
    output_voltage_v(index+1) = max(0, output_voltage_v(index)+ ...
        control_period_s/output_capacitance_f* ...
        (diode_output_current_a-load_current));
    input_current_a(index) = sum(phase_current_a(index, :));
end

% 补齐绘图末点。
phase_current_a(end, :) = phase_current_a(end-1, :);
input_current_a(end) = sum(phase_current_a(end, :));
output_current_a(end) = output_current_a(end-1);
load_power_w(end) = output_voltage_v(end)*output_current_a(end);
total_current_reference_a(end) = total_current_reference_a(end-1);
current_limit_reference_a(end) = current_limit_reference_a(end-1);
power_current_limit_a(end) = power_current_limit_a(end-1);
input_current_path_limit_a(end) = input_current_path_limit_a(end-1);
output_current_path_limit_a(end) = output_current_path_limit_a(end-1);
input_voltage_path_limit_a(end) = input_voltage_path_limit_a(end-1);
inductor_voltage_command_v(end, :) = inductor_voltage_command_v(end-1, :);
duty(end, :) = duty(end-1, :);
voltage_loop_saturated(end) = voltage_loop_saturated(end-1);
current_loop_saturated(end, :) = current_loop_saturated(end-1, :);

%% 7. 分模式统计和自动检查
cr_window = time_s >= 0.240 & time_s < 0.275;
cc_window = time_s >= 0.420 & time_s < 0.435;
cv_window = time_s >= 0.620;

cr_metrics = interval_metrics(output_voltage_v, output_reference_v, cr_window);
cc_metrics = interval_metrics(output_voltage_v, output_reference_v, cc_window);
cv_metrics = interval_metrics(output_voltage_v, output_reference_v, cv_window);

fprintf('\nTwo-phase Boost principle time-domain simulation\n');
fprintf('------------------------------------------------\n');
fprintf('Input voltage range       : %.3f ... %.3f V\n', ...
    min(input_voltage_v), max(input_voltage_v));
fprintf('Output reference range    : %.3f ... %.3f V\n', ...
    min(output_reference_v), max(output_reference_v));
fprintf('Voltage PI Kp, Ki         : %.9g A/V, %.9g A/(V*s)\n', ...
    voltage_Kp_a_per_v, voltage_Ki_a_per_vs);
fprintf('Current PI Kp, Ki         : %.9g V/A, %.9g V/(A*s)\n', ...
    current_Kp_v_per_a, current_Ki_v_per_as);
print_interval_metrics('CR', cr_metrics);
print_interval_metrics('CC', cc_metrics);
print_interval_metrics('CV', cv_metrics);
fprintf('Maximum total input current: %.6f A\n', max(input_current_a));
fprintf('Maximum duty               : %.6f\n', max(duty, [], 'all'));
fprintf('Voltage-loop saturation    : %.3f %%\n', ...
    100*mean(voltage_loop_saturated));

if any(~isfinite(output_voltage_v)) || any(~isfinite(phase_current_a), 'all')
    error('Simulation generated NaN or Inf.');
end

result.time_s = time_s;
result.input_voltage_v = input_voltage_v;
result.output_reference_v = output_reference_v;
result.output_voltage_v = output_voltage_v;
result.phase_current_a = phase_current_a;
result.input_current_a = input_current_a;
result.output_current_a = output_current_a;
result.load_power_w = load_power_w;
result.total_current_reference_a = total_current_reference_a;
result.current_limit_reference_a = current_limit_reference_a;
result.duty = duty;
result.load_mode = load_mode;
result.cv_load_voltage_v = cv_load_voltage_v;
result.metrics.CR = cr_metrics;
result.metrics.CC = cc_metrics;
result.metrics.CV = cv_metrics;

%% 8. 所有工况绘制在同一个连续时间轴上
figure('Color', 'w', 'Name', 'Boost principle full-range verification', ...
    'WindowState', 'maximized');
layout = tiledlayout(7, 1, 'TileSpacing', 'compact', 'Padding', 'compact');

voltage_axes = nexttile(layout);
plot(time_s*1e3, output_reference_v, 'k--', 'LineWidth', 1.0, ...
    'DisplayName', 'Vout ref');
hold on;
plot(time_s*1e3, output_voltage_v, 'LineWidth', 1.3, ...
    'DisplayName', 'Vout');
plot(time_s*1e3, input_voltage_v, 'LineWidth', 1.1, ...
    'DisplayName', 'Vin');
grid on;
ylabel('Voltage / V');
ylim([0, 66]);
legend('Location', 'best');
title('Input/output voltage: 12~60 V step and ramp coverage');

error_axes = nexttile(layout);
plot(time_s*1e3, output_reference_v-output_voltage_v, ...
    'LineWidth', 1.1);
grid on;
ylabel('Error / V');
title('Output-voltage error');

current_axes = nexttile(layout);
plot(time_s*1e3, input_current_a, 'LineWidth', 1.2, ...
    'DisplayName', 'Iin total');
hold on;
plot(time_s*1e3, output_current_a, 'LineWidth', 1.1, ...
    'DisplayName', 'Iout/load');
plot(time_s*1e3, phase_current_a(:, 1), '--', 'LineWidth', 0.9, ...
    'DisplayName', 'IL phase A');
plot(time_s*1e3, phase_current_a(:, 2), ':', 'LineWidth', 0.9, ...
    'DisplayName', 'IL phase B');
grid on;
ylabel('Current / A');
legend('Location', 'best');
title('Two-phase inductor current and load current');

reference_axes = nexttile(layout);
plot(time_s*1e3, total_current_reference_a, 'LineWidth', 1.2, ...
    'DisplayName', 'Voltage-loop Iref');
hold on;
plot(time_s*1e3, current_limit_reference_a, '--', 'LineWidth', 1.1, ...
    'DisplayName', 'Winning current limit');
plot(time_s*1e3, power_current_limit_a, ':', 'LineWidth', 0.9, ...
    'DisplayName', 'Power limit/Vin');
plot(time_s*1e3, output_current_path_limit_a, '-.', 'LineWidth', 0.9, ...
    'DisplayName', 'Output-current equivalent limit');
grid on;
ylabel('Current / A');
ylim([0, 110]);
legend('Location', 'best');
title('Outer-loop command and slow-task limiting competition');

duty_axes = nexttile(layout);
plot(time_s*1e3, duty(:, 1), 'LineWidth', 1.1, ...
    'DisplayName', 'Duty A');
hold on;
plot(time_s*1e3, duty(:, 2), '--', 'LineWidth', 1.0, ...
    'DisplayName', 'Duty B');
grid on;
ylabel('Duty');
ylim([-0.05, 1]);
legend('Location', 'best');
title('Main-switch duty after Vin/Vout feedforward');

power_axes = nexttile(layout);
plot(time_s*1e3, load_power_w, 'LineWidth', 1.2, ...
    'DisplayName', 'Load power');
hold on;
yline(power_limit_w, '--k', 'Power limit', 'DisplayName', 'Power limit');
grid on;
ylabel('Power / W');
legend('Location', 'best');
title('Load power');

mode_axes = nexttile(layout);
stairs(time_s*1e3, load_mode, 'k', 'LineWidth', 1.2, ...
    'DisplayName', 'Load mode');
hold on;
plot(time_s*1e3, cv_load_voltage_v/24, ':', 'LineWidth', 1.0, ...
    'DisplayName', 'CV sink voltage / 24');
ylim([0.7, 3.3]);
yticks([1, 2, 3]);
yticklabels({'CR', 'CC', 'CV'});
grid on;
ylabel('Mode');
xlabel('Continuous simulation time / ms');
legend('Location', 'best');
title('Continuous CR, CC and CV load sequence');

all_axes = [voltage_axes, error_axes, current_axes, reference_axes, ...
    duty_axes, power_axes, mode_axes];
event_times_s = [0.020, 0.080, 0.100, 0.140, 0.200, 0.260, ...
    0.280, 0.300, 0.320, 0.360, 0.420, 0.440, 0.500, 0.540, 0.600];
for axes_index = 1:numel(all_axes)
    for event_index = 1:numel(event_times_s)
        xline(all_axes(axes_index), event_times_s(event_index)*1e3, ':', ...
            'HandleVisibility', 'off');
    end
end
linkaxes(all_axes, 'x');
xlim(voltage_axes, [0, simulation_end_s*1e3]);
title(layout, ...
    'Two-phase Boost continuous principle simulation: step, ramp, CR, CC and CV');

%% 局部函数
function [output, integral, saturated] = pi_step_aw( ...
    error, integral, Kp, Ki, Ts, lower_limit, upper_limit)
%PI_STEP_AW 带条件积分抗饱和的离散 PI。
% 先尝试积分，再判断输出是否越界；若误差会把饱和推得更深，则撤销本次
% 积分。这样负载或参考离开限制区后，控制器可以快速恢复。

    previous_integral = integral;
    integral = integral+Ki*Ts*error;
    raw_output = Kp*error+integral;
    output = min(max(raw_output, lower_limit), upper_limit);
    saturated = output ~= raw_output;

    pushes_upper = raw_output > upper_limit && error > 0;
    pushes_lower = raw_output < lower_limit && error < 0;
    if pushes_upper || pushes_lower
        integral = previous_integral;
        raw_output = Kp*error+integral;
        output = min(max(raw_output, lower_limit), upper_limit);
    end
end

function metrics = interval_metrics(voltage, reference, window)
%INTERVAL_METRICS 统计指定稳态窗口的平均误差和峰峰值。

    selected_voltage = voltage(window);
    selected_reference = reference(window);
    metrics.mean_voltage_v = mean(selected_voltage);
    metrics.mean_reference_v = mean(selected_reference);
    metrics.mean_error_v = mean(selected_reference-selected_voltage);
    metrics.peak_to_peak_v = max(selected_voltage)-min(selected_voltage);
end

function print_interval_metrics(name, metrics)
%PRINT_INTERVAL_METRICS 打印 CR、CC 或 CV 的统一指标。

    fprintf(['%-3s: Vref=%8.4f V, Vout=%8.4f V, error=%+9.5f V, ' ...
        'Vpp=%8.5f V\n'], name, metrics.mean_reference_v, ...
        metrics.mean_voltage_v, metrics.mean_error_v, ...
        metrics.peak_to_peak_v);
end
