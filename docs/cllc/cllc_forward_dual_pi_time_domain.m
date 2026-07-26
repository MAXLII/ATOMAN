%% CLLC 正向电压/电流双环竞争 PI 原理性时域仿真
% 学习目标：本文件展示在没有搭建完整开关电路时，如何用 MATLAB 建立
% “静态非线性功率级 + 慢速输出包络 + 离散控制器”的平均模型，快速
% 验证控制方向、PI 参数、限幅、竞争接管和软启动。建议按以下顺序阅读：
%
%   1. 定义电路、PI 和仿真事件；
%   2. 用 FHA 建立不同负载下的 u->Vout 静态查表曲线；
%   3. 设置功率级状态、共享积分器和一个开关周期的控制延时；
%   4. 在固定采样周期内逐点执行与嵌入式 C 一致的竞争 PI；
%   5. 用一阶差分方程推进输出电压；
%   6. 自动提取阶跃指标，并绘制可解释的内部状态；
%   7. 用独立工况验证 0~24 V 软启动和 24~72 V 全范围斜坡。
%
% “原理性”的含义：这里不逐个开关周期求解 MOSFET、谐振电流和整流
% 换向，而是保留控制设计最关键的低频关系。它非常适合先发现控制符号、
% 积分更新、限幅和模式切换问题，但不能代替 PLECS 开关模型对器件应力、
% ZVS/ZCS、死区和纹波的验证。
%
% MATLAB 语法提示：长度为 sample_count 的列向量保存整个时间序列；
% for 循环每次只计算一个采样点。逻辑索引（例如 time_s>=事件时刻）
% 用来一次性生成给定和负载序列，不需要为每个时刻编写 if 语句。
%
% 本脚本使用 cllc_forward_dual_pi_design.m 的参数，并逐步骤复现
% code/lib/pi_dual_compete.c 的 MIN 竞争与共享积分算法：
%
%   out_v = Kp_v*(Vref-Vout) + i_share
%   out_i = Kp_i*(Ilimit-Iload) + i_share
%   u = min(out_v, out_i)
%
% 只有获胜通道使用自己的 ki_step*error 更新 i_share。输出饱和时，
% i_share 被回算到饱和边界，实现与 C 模块一致的抗积分饱和。
%
% 功率级采用 FHA 静态 u->Vout 曲线和 tau=Rload*Cout/2 的平均包络。
% 该模型用于验证控制方向、竞争接管、共积分连续性和阶跃响应，不包含
% 开关纹波、谐振电流应力、同步整流时序和死区。

clear;
clc;
close all;

%% 电路和控制参数
% 这里的网络参数必须与设计脚本一致。时域脚本直接写入已经设计好的 PI
% 数值，使它可以单独运行；如果重新调整设计脚本，应同步更新这四个参数。
nominal_output_voltage_v = 48;
minimum_output_voltage_v = 24;
maximum_output_voltage_v = 72;
bus_voltage_gain = 8;
bus_voltage_offset_v = 25;
minimum_bus_voltage_limit_v = 370;
bus_time_constant_s = 5e-3;
minimum_bus_voltage_v = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*minimum_output_voltage_v + bus_voltage_offset_v);
nominal_bus_voltage_v = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*nominal_output_voltage_v + bus_voltage_offset_v);
maximum_bus_voltage_v = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*maximum_output_voltage_v + bus_voltage_offset_v);
Cout_f = 10e-3;

nominal_power_w = 6600;
rated_current_a = 150;
overload_power_w = 9000;
nominal_load_ohm = nominal_output_voltage_v^2/nominal_power_w;
minimum_power_w = minimum_output_voltage_v*rated_current_a;
minimum_load_ohm = minimum_output_voltage_v/rated_current_a;
maximum_load_ohm = maximum_output_voltage_v^2/nominal_power_w;
overload_load_ohm = nominal_output_voltage_v^2/overload_power_w;

% 使用 R=V^2/P 把恒功率工况转换成电阻负载。注意：这只表示每个负载
% 阶段内的等效电阻，不是严格的瞬时恒功率负载模型。

Lr_primary_h = 40e-6;
Cr_primary_f = 80e-9;
Lm_primary_h = 200e-6;
Lr_secondary_h = 0.625e-6;
Cr_secondary_f = 5.12e-6;
turns_ratio = 24/3;

fr_hz = 1/(2*pi*sqrt(Lr_primary_h*Cr_primary_f));
fmax_hz = 2*fr_hz;

% cllc_forward_dual_pi_design.m 的双竞争 PI 参数
% 连续 Ki 先保留为人容易理解的 u/(单位*s)，随后乘 Ts 得到 C 程序每次
% 调用使用的 ki_step。Kp 不需要乘 Ts。
voltage_Kp = 0.0390560696419;      % u/V
voltage_Ki = 22.3758732323;        % u/(V*s)
current_Kp = 0.136341056236;       % u/A
current_Ki = 78.1120634685;        % u/(A*s)
% 150 A 是正常额定电流；竞争电流环作为过流保护，阈值取 110%=165 A。
current_limit_a = 1.10*rated_current_a;

control_update_frequency_hz = 100e3;
sample_time_s = 1/control_update_frequency_hz;
voltage_ki_step = voltage_Ki*sample_time_s;
current_ki_step = current_Ki*sample_time_s;

% 最终执行量 u 必须位于 0~1。u=0 表示最高频率下零相移，u=1 表示
% 谐振频率下满相移。限幅不仅保护调制器，也要求积分器做回算防饱和。
output_limits = [0, 1];

%% 全电压范围阶跃仿真事件
% 第一列按 24 -> 48 -> 72 -> 48 -> 24 V 遍历完整输出范围。每个稳态点
% 同时按额定包络计算负载：P=min(6600,150*Vref)。因此 24 V 点为
% 3600 W/150 A，48 V 和 72 V 点均为 6600 W。
simulation_end_s = 0.280;
step_24_to_48_s = 0.040;
step_48_to_72_s = 0.100;
step_72_to_48_s = 0.160;
step_48_to_24_s = 0.220;

time_s = (0:sample_time_s:simulation_end_s).';
sample_count = numel(time_s);

% 先用常数向量初始化，再通过逻辑掩码修改指定时间区间。
% 这种写法比逐点判断更直观，也方便画出理想给定波形。
reference_voltage_v = minimum_output_voltage_v*ones(sample_count, 1);
reference_voltage_v(time_s >= step_24_to_48_s & ...
    time_s < step_48_to_72_s) = nominal_output_voltage_v;
reference_voltage_v(time_s >= step_48_to_72_s & ...
    time_s < step_72_to_48_s) = maximum_output_voltage_v;
reference_voltage_v(time_s >= step_72_to_48_s & ...
    time_s < step_48_to_24_s) = nominal_output_voltage_v;

power_reference_w = min(nominal_power_w, ...
    rated_current_a*reference_voltage_v);
load_resistance_ohm = nan(sample_count, 1);

%% 全范围三个工作点及过载点的非线性静态调制曲线
% command_grid 是归一化执行量 u 的查表横轴。对每个负载分别计算静态
% Vout(u)，因为负载变化会改变谐振腔阻尼和增益。4001 点能兼顾插值
% 精度与计算速度；它不是控制采样点数。
command_grid = linspace(0, 1, 4001).';
nominal_curve = build_hybrid_static_curve( ...
    command_grid, nominal_load_ohm, nominal_bus_voltage_v, ...
    nominal_output_voltage_v, ...
    fr_hz, fmax_hz, Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);
minimum_curve = build_hybrid_static_curve( ...
    command_grid, minimum_load_ohm, minimum_bus_voltage_v, ...
    minimum_output_voltage_v, ...
    fr_hz, fmax_hz, Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);
maximum_curve = build_hybrid_static_curve( ...
    command_grid, maximum_load_ohm, maximum_bus_voltage_v, ...
    maximum_output_voltage_v, ...
    fr_hz, fmax_hz, Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);

% CC 全范围阶跃使用 91.67 A，使最高电压 72 V 时恰好为 6600 W。
% 在三个代表电压点分别建立恒流负载的等效 FHA 曲线，斜坡/阶跃过渡时
% 再按参考电压插值。CC 负载的等效电阻为 R=V/Icc。
cc_load_current_a = nominal_power_w/maximum_output_voltage_v;
cc_minimum_load_ohm = minimum_output_voltage_v/cc_load_current_a;
cc_nominal_load_ohm = nominal_output_voltage_v/cc_load_current_a;
cc_maximum_load_ohm = maximum_output_voltage_v/cc_load_current_a;
cc_minimum_curve = build_hybrid_static_curve( ...
    command_grid, cc_minimum_load_ohm, minimum_bus_voltage_v, ...
    minimum_output_voltage_v, fr_hz, fmax_hz, ...
    Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);
cc_nominal_curve = build_hybrid_static_curve( ...
    command_grid, cc_nominal_load_ohm, nominal_bus_voltage_v, ...
    nominal_output_voltage_v, fr_hz, fmax_hz, ...
    Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);
cc_maximum_curve = build_hybrid_static_curve( ...
    command_grid, cc_maximum_load_ohm, maximum_bus_voltage_v, ...
    maximum_output_voltage_v, fr_hz, fmax_hz, ...
    Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);
overload_curve = build_hybrid_static_curve( ...
    command_grid, overload_load_ohm, nominal_bus_voltage_v, ...
    nominal_output_voltage_v, ...
    fr_hz, fmax_hz, Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);

transition_u = nominal_curve.transition_u;

% interp1 在额定负载的 Vout(u) 曲线上反查 48 V 对应的 u0。
% 该值同时是正常稳态的执行量和共享积分初值，因为稳态电压误差为零，
% 比例项也为零。这样可从稳态开始仿真，避免人为的上电暂态干扰阶跃指标。
initial_u = interp1(minimum_curve.output_voltage_v, command_grid, ...
    minimum_output_voltage_v, 'linear');
if ~isfinite(initial_u)
    error('150 A 负载静态曲线中没有找到 24 V 工作点。');
end

nominal_u = interp1(nominal_curve.output_voltage_v, command_grid, ...
    nominal_output_voltage_v, 'linear');
maximum_u = interp1(maximum_curve.output_voltage_v, command_grid, ...
    maximum_output_voltage_v, 'linear');
if ~isfinite(nominal_u) || ~isfinite(maximum_u)
    error('静态曲线无法覆盖 48 V 或 72 V，请检查母线、变比和频率范围。');
end

cc_initial_u = interp1(cc_minimum_curve.output_voltage_v, command_grid, ...
    minimum_output_voltage_v, 'linear');
if ~isfinite(cc_initial_u)
    error('CC 静态曲线中没有找到 24 V 工作点。');
end

delay_samples = max(1, round( ...
    (1/minimum_curve.operating_frequency_hz)/sample_time_s));

% 用移位寄存器模拟采样、计算、PWM 更新造成的纯延时。当前参数约为一个
% 开关周期，并至少保留一个采样延时。command_delay_line(1) 是本次真正
% 施加到功率级的旧命令，最新 command 从数组末端进入。
command_delay_line = initial_u*ones(delay_samples, 1);

%% 状态与稳态初始化
% nan 预分配有两个作用：提高循环速度，并让“某个元素忘记赋值”的错误
% 在检查 isfinite 或绘图时立刻暴露。布尔量 saturated 用 false 初始化。
output_voltage_v = nan(sample_count, 1);
bus_voltage_v = nan(sample_count, 1);
bus_reference_v = nan(sample_count, 1);
load_current_a = nan(sample_count, 1);
shared_integral = nan(sample_count, 1);
voltage_candidate = nan(sample_count, 1);
current_candidate = nan(sample_count, 1);
normalized_command = nan(sample_count, 1);
applied_command = nan(sample_count, 1);
switching_frequency_hz = nan(sample_count, 1);
phase_shift_duty = nan(sample_count, 1);
active_channel = nan(sample_count, 1); % 1=voltage, 2=current
saturated = false(sample_count, 1);

output_voltage_v(1) = minimum_output_voltage_v;
bus_voltage_v(1) = minimum_bus_voltage_v;
bus_reference_v(1) = minimum_bus_voltage_v;
load_current_a(1) = output_voltage_v(1)/minimum_load_ohm;

% 只有一个积分状态 i_share。两个 PI 候选共享它，但每次仅由获胜通道
% 更新；这正是竞争切换时保持输出连续的关键。
i_share = initial_u;

%% 离散闭环仿真：与 pi_dual_compete.c 相同的计算顺序
% 每次循环代表一次控制中断。计算顺序不能随意交换，因为“先选获胜者、
% 再由获胜者积分、最后限幅回算”决定了与嵌入式实现是否一致。
for sample_index = 1:sample_count - 1
    % (1) 由当前电压和当前负载计算反馈电流。
    % 额定输出包络：低压区恒流 150 A，高压区恒功率 6600 W。
    load_current_a(sample_index) = min(rated_current_a, ...
        nominal_power_w/max(output_voltage_v(sample_index), eps));
    current_load_ohm = output_voltage_v(sample_index)/ ...
        load_current_a(sample_index);
    load_resistance_ohm(sample_index) = current_load_ohm;

    voltage_error_v = reference_voltage_v(sample_index) - ...
        output_voltage_v(sample_index);
    current_error_a = current_limit_a - load_current_a(sample_index);

    % (2) 两个误差采用不同量纲和增益，但都换算成同一归一化 u 域。
    %     电流误差定义为 Ilimit-Iload：正常时为正，过流时为负。
    voltage_p = voltage_Kp*voltage_error_v;
    current_p = current_Kp*current_error_a;
    voltage_raw = voltage_p + i_share;
    current_raw = current_p + i_share;

    % (3) MIN 竞争。正常时 current_raw 很大，voltage_raw 较小，电压环
    %     获胜；过流后 current_raw 下降，电流环获胜并主动降低 u。
    if voltage_raw <= current_raw
        active_channel(sample_index) = 1;
        active_error = voltage_error_v;
        active_ki_step = voltage_ki_step;
        active_p = voltage_p;
    else
        active_channel(sample_index) = 2;
        active_error = current_error_a;
        active_ki_step = current_ki_step;
        active_p = current_p;
    end

    % (4) 只有获胜通道更新共享积分。ki_step 已经包含 Ts，所以这里直接
    %     乘误差，不能再乘 sample_time_s。
    i_share = i_share + active_ki_step*active_error;
    command = active_p + i_share;

    % (5) 输出饱和时反算 i_share=u_limit-active_p。这样下一次离开饱和
    %     边界时不需要先消除积累的大量积分，属于 back-calculation
    %     抗积分饱和，并与 pi_dual_compete.c 的行为一致。
    if command > output_limits(2)
        command = output_limits(2);
        i_share = output_limits(2) - active_p;
        saturated(sample_index) = true;
    elseif command < output_limits(1)
        command = output_limits(1);
        i_share = output_limits(1) - active_p;
        saturated(sample_index) = true;
    end
    i_share = min(max(i_share, ...
        output_limits(1) - active_p), output_limits(2) - active_p);

    % 使用积分后的最新状态重建两个候选值，仅用于记录和绘图。
    % 实际 command 仍由本次获胜通道产生。
    voltage_candidate(sample_index) = voltage_p + i_share;
    current_candidate(sample_index) = current_p + i_share;
    shared_integral(sample_index) = i_share;
    normalized_command(sample_index) = command;

    % (6) 命令先经过离散延时，再由混合调制器映射为频率和相移占空比。
    applied_command(sample_index) = command_delay_line(1);
    command_delay_line(1:end-1) = command_delay_line(2:end);
    command_delay_line(end) = command;
    [switching_frequency_hz(sample_index), ...
     phase_shift_duty(sample_index)] = hybrid_modulator( ...
        applied_command(sample_index), transition_u, fr_hz, fmax_hz);

    % (7) 在当前负载的 Vout(u) 表中插值得到静态目标电压，再用显式欧拉法
    %     离散一阶包络：
    %       V[k+1]=V[k]+Ts/tau*(Vtarget[k]-V[k])
    %     当 Ts 远小于 tau 时，该离散近似稳定且足够准确。
    % FHA 输出与母线电压成正比。静态曲线在对应稳态母线下建立，再乘
    % Vbus_actual/Vbus_curve，即可加入动态母线而无需每步重算整个谐振网络。
    voltage_target_v = interpolate_full_range_target( ...
        applied_command(sample_index), reference_voltage_v(sample_index), ...
        bus_voltage_v(sample_index), minimum_output_voltage_v, ...
        nominal_output_voltage_v, maximum_output_voltage_v, ...
        minimum_curve, nominal_curve, maximum_curve, command_grid);
    output_time_constant_s = current_load_ohm*Cout_f/2;
    output_voltage_v(sample_index + 1) = ...
        output_voltage_v(sample_index) + ...
        sample_time_s/output_time_constant_s * ...
        (voltage_target_v - output_voltage_v(sample_index));

    % 前级母线参考为 max(370,8*Vref+25)，实际母线具有一阶滞后：
    % dVbus/dt=(Vbus_ref-Vbus)/tau_bus。
    bus_reference_v(sample_index) = max(minimum_bus_voltage_limit_v, ...
        bus_voltage_gain*reference_voltage_v(sample_index) + ...
        bus_voltage_offset_v);
    bus_voltage_v(sample_index + 1) = bus_voltage_v(sample_index) + ...
        sample_time_s/bus_time_constant_s * ...
        (bus_reference_v(sample_index) - bus_voltage_v(sample_index));
end

% 补齐绘图末点
% for 循环只推进到 sample_count-1，最后一个状态已有 Vout，但没有执行
% 控制计算。这里复制前一控制量，确保所有向量长度一致、绘图末端不为 NaN。
load_current_a(end) = min(rated_current_a, ...
    nominal_power_w/max(output_voltage_v(end), eps));
load_resistance_ohm(end) = output_voltage_v(end)/load_current_a(end);
bus_reference_v(end) = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*reference_voltage_v(end) + bus_voltage_offset_v);
shared_integral(end) = shared_integral(end - 1);
voltage_candidate(end) = voltage_candidate(end - 1);
current_candidate(end) = current_candidate(end - 1);
normalized_command(end) = normalized_command(end - 1);
applied_command(end) = applied_command(end - 1);
switching_frequency_hz(end) = switching_frequency_hz(end - 1);
phase_shift_duty(end) = phase_shift_duty(end - 1);
active_channel(end) = active_channel(end - 1);
saturated(end) = saturated(end - 1);

%% 阶跃指标和控制方向检查
% 自动计算指标比只凭肉眼看波形更可靠：参考阶跃提取上升/稳定/超调，
% 负载阶跃提取最大电压偏差和恢复时间，过载工况检查峰值与稳态限流值。
step_metrics(1) = calculate_reference_step_metrics( ...
    time_s, output_voltage_v, step_24_to_48_s, ...
    step_48_to_72_s, 24, 48);
step_metrics(2) = calculate_reference_step_metrics( ...
    time_s, output_voltage_v, step_48_to_72_s, ...
    step_72_to_48_s, 48, 72);
step_metrics(3) = calculate_reference_step_metrics( ...
    time_s, output_voltage_v, step_72_to_48_s, ...
    step_48_to_24_s, 72, 48);
step_metrics(4) = calculate_reference_step_metrics( ...
    time_s, output_voltage_v, step_48_to_24_s, ...
    simulation_end_s + sample_time_s, 48, 24);
step_peak_current_a = max(load_current_a);
step_current_limit_active = any(active_channel == 2);

fprintf('\n================ Dual-compete time-domain verification ============\n');
fprintf('MIN competition, initial shared integral = %.9f\n', initial_u);
fprintf('Voltage PI: Kp = %.9g, ki_step = %.9g\n', ...
    voltage_Kp, voltage_ki_step);
fprintf('Current PI: Kp = %.9g, ki_step = %.9g\n', ...
    current_Kp, current_ki_step);
step_labels = {'24 -> 48 V', '48 -> 72 V', ...
    '72 -> 48 V', '48 -> 24 V'};
for step_index = 1:numel(step_metrics)
    fprintf(['%s: rise = %.6f ms, settling = %.6f ms, ', ...
             'overshoot = %.6f V\n'], ...
        step_labels{step_index}, ...
        step_metrics(step_index).rise_time_s*1e3, ...
        step_metrics(step_index).settling_time_s*1e3, ...
        step_metrics(step_index).overshoot_v);
end
fprintf(['Full-range step peak current = %.6f A, ', ...
         'current-limit takeover = %d\n'], ...
    step_peak_current_a, step_current_limit_active);
fprintf('Saturated samples = %d\n', nnz(saturated));

simulation.time_s = time_s;
% 将全部波形和指标打包进 simulation 结构体。脚本运行结束后可以在
% Command Window 输入 simulation.step_metrics 或用字段画自定义图，
% 也可在自动化测试中用 assert 检查结果。
simulation.reference_voltage_v = reference_voltage_v;
simulation.output_voltage_v = output_voltage_v;
simulation.bus_voltage_v = bus_voltage_v;
simulation.bus_reference_v = bus_reference_v;
simulation.load_current_a = load_current_a;
simulation.current_limit_a = current_limit_a;
simulation.normalized_command = normalized_command;
simulation.applied_command = applied_command;
simulation.switching_frequency_hz = switching_frequency_hz;
simulation.phase_shift_duty = phase_shift_duty;
simulation.shared_integral = shared_integral;
simulation.voltage_candidate = voltage_candidate;
simulation.current_candidate = current_candidate;
simulation.active_channel = active_channel;
simulation.load_resistance_ohm = load_resistance_ohm;
simulation.power_reference_w = power_reference_w;
simulation.step_metrics = step_metrics;
simulation.step_peak_current_a = step_peak_current_a;
simulation.step_current_limit_active = step_current_limit_active;

%% CC 模式下的 24~72 V 全范围电压阶跃
% 使用与上一节完全相同的 Vref 和母线动态，只把负载改为固定电流
% Icc=6600/72=91.67 A。这样整个 24~72 V 范围都不超过额定功率。
% 两种负载模式使用独立控制状态，绘图时在同一阶跃时间段叠加比较。
cc_output_voltage_v = nan(sample_count, 1);
cc_bus_voltage_v = nan(sample_count, 1);
cc_bus_reference_v = nan(sample_count, 1);
cc_shared_integral = nan(sample_count, 1);
cc_normalized_command = nan(sample_count, 1);
cc_applied_command = nan(sample_count, 1);
cc_switching_frequency_hz = nan(sample_count, 1);
cc_active_channel = nan(sample_count, 1);
cc_saturated = false(sample_count, 1);
cc_command_delay_line = cc_initial_u*ones(delay_samples, 1);
cc_i_share = cc_initial_u;
cc_output_voltage_v(1) = minimum_output_voltage_v;
cc_bus_voltage_v(1) = minimum_bus_voltage_v;
cc_bus_reference_v(1) = minimum_bus_voltage_v;

for sample_index = 1:sample_count - 1
    voltage_error_v = reference_voltage_v(sample_index) - ...
        cc_output_voltage_v(sample_index);
    current_error_a = current_limit_a - cc_load_current_a;
    voltage_p = voltage_Kp*voltage_error_v;
    current_p = current_Kp*current_error_a;

    if voltage_p + cc_i_share <= current_p + cc_i_share
        cc_active_channel(sample_index) = 1;
        active_error = voltage_error_v;
        active_ki_step = voltage_ki_step;
        active_p = voltage_p;
    else
        cc_active_channel(sample_index) = 2;
        active_error = current_error_a;
        active_ki_step = current_ki_step;
        active_p = current_p;
    end

    cc_i_share = cc_i_share + active_ki_step*active_error;
    command = active_p + cc_i_share;
    if command > output_limits(2)
        command = output_limits(2);
        cc_i_share = output_limits(2) - active_p;
        cc_saturated(sample_index) = true;
    elseif command < output_limits(1)
        command = output_limits(1);
        cc_i_share = output_limits(1) - active_p;
        cc_saturated(sample_index) = true;
    end
    cc_i_share = min(max(cc_i_share, ...
        output_limits(1) - active_p), output_limits(2) - active_p);

    cc_shared_integral(sample_index) = cc_i_share;
    cc_normalized_command(sample_index) = command;
    cc_applied_command(sample_index) = cc_command_delay_line(1);
    cc_command_delay_line(1:end-1) = cc_command_delay_line(2:end);
    cc_command_delay_line(end) = command;
    [cc_switching_frequency_hz(sample_index), ~] = hybrid_modulator( ...
        cc_applied_command(sample_index), transition_u, fr_hz, fmax_hz);

    voltage_target_v = interpolate_full_range_target( ...
        cc_applied_command(sample_index), ...
        reference_voltage_v(sample_index), ...
        cc_bus_voltage_v(sample_index), minimum_output_voltage_v, ...
        nominal_output_voltage_v, maximum_output_voltage_v, ...
        cc_minimum_curve, cc_nominal_curve, cc_maximum_curve, ...
        command_grid);
    cc_load_ohm = max(cc_output_voltage_v(sample_index), 1)/ ...
        cc_load_current_a;
    output_time_constant_s = cc_load_ohm*Cout_f/2;
    cc_output_voltage_v(sample_index + 1) = ...
        cc_output_voltage_v(sample_index) + ...
        sample_time_s/output_time_constant_s * ...
        (voltage_target_v - cc_output_voltage_v(sample_index));

    cc_bus_reference_v(sample_index) = max( ...
        minimum_bus_voltage_limit_v, ...
        bus_voltage_gain*reference_voltage_v(sample_index) + ...
        bus_voltage_offset_v);
    cc_bus_voltage_v(sample_index + 1) = ...
        cc_bus_voltage_v(sample_index) + ...
        sample_time_s/bus_time_constant_s * ...
        (cc_bus_reference_v(sample_index) - ...
         cc_bus_voltage_v(sample_index));
end

cc_bus_reference_v(end) = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*reference_voltage_v(end) + bus_voltage_offset_v);
cc_shared_integral(end) = cc_shared_integral(end - 1);
cc_normalized_command(end) = cc_normalized_command(end - 1);
cc_applied_command(end) = cc_applied_command(end - 1);
cc_switching_frequency_hz(end) = cc_switching_frequency_hz(end - 1);
cc_active_channel(end) = cc_active_channel(end - 1);
cc_saturated(end) = cc_saturated(end - 1);

step_events_s = [step_24_to_48_s, step_48_to_72_s, ...
    step_72_to_48_s, step_48_to_24_s];
step_window_ends_s = [step_48_to_72_s, step_72_to_48_s, ...
    step_48_to_24_s, simulation_end_s + sample_time_s];
step_initial_v = [24, 48, 72, 48];
step_final_v = [48, 72, 48, 24];
empty_step_metrics = struct('rise_time_s', NaN, ...
    'settling_time_s', NaN, 'overshoot_v', NaN);
cc_step_metrics = repmat(empty_step_metrics, 1, 4);
for step_index = 1:4
    cc_step_metrics(step_index) = calculate_reference_step_metrics( ...
        time_s, cc_output_voltage_v, step_events_s(step_index), ...
        step_window_ends_s(step_index), step_initial_v(step_index), ...
        step_final_v(step_index));
end

fprintf('\n================ CC full-range voltage steps =====================\n');
fprintf('CC load current = %.6f A, maximum power at 72 V = %.6f W\n', ...
    cc_load_current_a, cc_load_current_a*maximum_output_voltage_v);
for step_index = 1:numel(cc_step_metrics)
    fprintf(['CC %s: rise = %.6f ms, settling = %.6f ms, ', ...
             'overshoot = %.6f V\n'], ...
        step_labels{step_index}, ...
        cc_step_metrics(step_index).rise_time_s*1e3, ...
        cc_step_metrics(step_index).settling_time_s*1e3, ...
        cc_step_metrics(step_index).overshoot_v);
end

simulation.cc_step.load_current_a = cc_load_current_a;
simulation.cc_step.output_voltage_v = cc_output_voltage_v;
simulation.cc_step.bus_voltage_v = cc_bus_voltage_v;
simulation.cc_step.bus_reference_v = cc_bus_reference_v;
simulation.cc_step.normalized_command = cc_normalized_command;
simulation.cc_step.applied_command = cc_applied_command;
simulation.cc_step.switching_frequency_hz = cc_switching_frequency_hz;
simulation.cc_step.shared_integral = cc_shared_integral;
simulation.cc_step.active_channel = cc_active_channel;
simulation.cc_step.step_metrics = cc_step_metrics;

%% 电压软启动与 24~72 V 全范围斜坡
% 第二个仿真先从 0 V 软启动到最低输出 24 V，再执行 24->72->24 V
% 斜坡，从而同时验证上电过程和全范围缓变跟踪。
soft_start_delay_s = 5e-3;
soft_start_ramp_s = 30e-3;
range_ramp_up_start_s = 50e-3;
range_ramp_time_s = 60e-3;
range_ramp_down_start_s = 130e-3;
soft_start_end_s = 210e-3;
soft_time_s = (0:sample_time_s:soft_start_end_s).';
soft_sample_count = numel(soft_time_s);
soft_reference_voltage_v = minimum_output_voltage_v*min(max( ...
    (soft_time_s - soft_start_delay_s)/soft_start_ramp_s, 0), 1);

range_ramp_up_progress = min(max( ...
    (soft_time_s - range_ramp_up_start_s)/range_ramp_time_s, 0), 1);
soft_reference_voltage_v = soft_reference_voltage_v + ...
    (maximum_output_voltage_v - minimum_output_voltage_v)* ...
    range_ramp_up_progress;
range_ramp_down_progress = min(max( ...
    (soft_time_s - range_ramp_down_start_s)/range_ramp_time_s, 0), 1);
soft_reference_voltage_v = soft_reference_voltage_v - ...
    (maximum_output_voltage_v - minimum_output_voltage_v)* ...
    range_ramp_down_progress;

soft_load_resistance_ohm = minimum_load_ohm*ones(soft_sample_count, 1);
soft_in_regulated_range = ...
    soft_reference_voltage_v >= minimum_output_voltage_v;
soft_power_reference_w = min(nominal_power_w, ...
    rated_current_a*soft_reference_voltage_v(soft_in_regulated_range));
soft_load_resistance_ohm(soft_in_regulated_range) = ...
    soft_reference_voltage_v(soft_in_regulated_range).^2 ./ ...
    soft_power_reference_w;

% min(max(...,0),1) 把标准化斜坡限制在 0~1，再乘 48 V。改变
% soft_start_ramp_s 就能直接比较不同软启动斜率下的跟踪误差和峰值电流。

soft_output_voltage_v = zeros(soft_sample_count, 1);
soft_bus_voltage_v = minimum_bus_voltage_limit_v*ones(soft_sample_count, 1);
soft_bus_reference_v = minimum_bus_voltage_limit_v*ones(soft_sample_count, 1);
soft_load_current_a = zeros(soft_sample_count, 1);
soft_shared_integral = zeros(soft_sample_count, 1);
soft_normalized_command = zeros(soft_sample_count, 1);
soft_applied_command = zeros(soft_sample_count, 1);
soft_switching_frequency_hz = fmax_hz*ones(soft_sample_count, 1);
soft_phase_shift_duty = zeros(soft_sample_count, 1);
soft_active_channel = ones(soft_sample_count, 1); % 1=voltage, 2=current
soft_saturated = false(soft_sample_count, 1);
soft_command_delay_line = zeros(delay_samples, 1);
soft_i_share = 0;

% 与稳态阶跃仿真不同，软启动从 Vout=0、u=0、i_share=0 开始；因此
% 初始频率为 fmax、相移为零，能够真实展示单一 u 从 PSM 进入 PFM。

for sample_index = 1:soft_sample_count - 1
    % 以下循环刻意保持与主仿真相同的竞争、积分、限幅、延时和功率级
    % 更新顺序。重复写出是为了学习时可以独立跟踪软启动每一步状态。
    if soft_reference_voltage_v(sample_index) < minimum_output_voltage_v
        current_load_ohm = minimum_load_ohm;
        soft_load_current_a(sample_index) = ...
            soft_output_voltage_v(sample_index)/current_load_ohm;
    else
        soft_load_current_a(sample_index) = min(rated_current_a, ...
            nominal_power_w/max(soft_output_voltage_v(sample_index), eps));
        current_load_ohm = soft_output_voltage_v(sample_index)/ ...
            soft_load_current_a(sample_index);
        soft_load_resistance_ohm(sample_index) = current_load_ohm;
    end
    voltage_error_v = soft_reference_voltage_v(sample_index) - ...
        soft_output_voltage_v(sample_index);
    current_error_a = current_limit_a - ...
        soft_load_current_a(sample_index);
    voltage_p = voltage_Kp*voltage_error_v;
    current_p = current_Kp*current_error_a;

    if voltage_p + soft_i_share <= current_p + soft_i_share
        soft_active_channel(sample_index) = 1;
        active_error = voltage_error_v;
        active_ki_step = voltage_ki_step;
        active_p = voltage_p;
    else
        soft_active_channel(sample_index) = 2;
        active_error = current_error_a;
        active_ki_step = current_ki_step;
        active_p = current_p;
    end

    soft_i_share = soft_i_share + active_ki_step*active_error;
    command = active_p + soft_i_share;
    if command > output_limits(2)
        command = output_limits(2);
        soft_i_share = output_limits(2) - active_p;
        soft_saturated(sample_index) = true;
    elseif command < output_limits(1)
        command = output_limits(1);
        soft_i_share = output_limits(1) - active_p;
        soft_saturated(sample_index) = true;
    end
    soft_i_share = min(max(soft_i_share, ...
        output_limits(1) - active_p), output_limits(2) - active_p);

    soft_shared_integral(sample_index) = soft_i_share;
    soft_normalized_command(sample_index) = command;
    soft_applied_command(sample_index) = soft_command_delay_line(1);
    soft_command_delay_line(1:end-1) = soft_command_delay_line(2:end);
    soft_command_delay_line(end) = command;
    [soft_switching_frequency_hz(sample_index), ...
     soft_phase_shift_duty(sample_index)] = hybrid_modulator( ...
        soft_applied_command(sample_index), transition_u, fr_hz, fmax_hz);

    reference_for_curve_v = min(max( ...
        soft_reference_voltage_v(sample_index), ...
        minimum_output_voltage_v), maximum_output_voltage_v);
    voltage_target_v = interpolate_full_range_target( ...
        soft_applied_command(sample_index), reference_for_curve_v, ...
        soft_bus_voltage_v(sample_index), ...
        minimum_output_voltage_v, nominal_output_voltage_v, ...
        maximum_output_voltage_v, minimum_curve, nominal_curve, ...
        maximum_curve, command_grid);
    output_time_constant_s = current_load_ohm*Cout_f/2;
    soft_output_voltage_v(sample_index + 1) = ...
        soft_output_voltage_v(sample_index) + ...
        sample_time_s/output_time_constant_s * ...
        (voltage_target_v - soft_output_voltage_v(sample_index));

    soft_bus_reference_v(sample_index) = max( ...
        minimum_bus_voltage_limit_v, ...
        bus_voltage_gain*soft_reference_voltage_v(sample_index) + ...
        bus_voltage_offset_v);
    soft_bus_voltage_v(sample_index + 1) = ...
        soft_bus_voltage_v(sample_index) + ...
        sample_time_s/bus_time_constant_s * ...
        (soft_bus_reference_v(sample_index) - ...
         soft_bus_voltage_v(sample_index));
end

soft_load_current_a(end) = min(rated_current_a, ...
    nominal_power_w/max(soft_output_voltage_v(end), eps));
soft_load_resistance_ohm(end) = ...
    soft_output_voltage_v(end)/soft_load_current_a(end);
soft_bus_reference_v(end) = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*soft_reference_voltage_v(end) + ...
    bus_voltage_offset_v);
soft_shared_integral(end) = soft_shared_integral(end - 1);
soft_normalized_command(end) = soft_normalized_command(end - 1);
soft_applied_command(end) = soft_applied_command(end - 1);
soft_switching_frequency_hz(end) = soft_switching_frequency_hz(end - 1);
soft_phase_shift_duty(end) = soft_phase_shift_duty(end - 1);
soft_active_channel(end) = soft_active_channel(end - 1);
soft_saturated(end) = soft_saturated(end - 1);

soft_ramp_end_s = soft_start_delay_s + soft_start_ramp_s;
soft_post_ramp = soft_time_s >= soft_ramp_end_s & ...
    soft_time_s < range_ramp_up_start_s;

% 斜坡期间 Vout 允许正常跟随误差；稳定时间从斜坡结束时开始计算，
% 误差带取额定电压的 1%。同时记录全程超调、峰值电流和电流环是否接管。
soft_start_settling_s = find_settling_time( ...
    soft_time_s(soft_post_ramp), ...
    soft_output_voltage_v(soft_post_ramp), minimum_output_voltage_v, ...
    0.01*minimum_output_voltage_v, soft_ramp_end_s);
soft_start_overshoot_v = max(0, ...
    max(soft_output_voltage_v(soft_post_ramp)) - minimum_output_voltage_v);
soft_start_peak_current_a = max(soft_load_current_a);
soft_start_current_limited = any(soft_active_channel == 2);
range_ramp_window = soft_time_s >= range_ramp_up_start_s;
range_ramp_maximum_error_v = max(abs( ...
    soft_reference_voltage_v(range_ramp_window) - ...
    soft_output_voltage_v(range_ramp_window)));

fprintf('\n================ Voltage soft-start verification ================\n');
fprintf('Vref soft start: 0 -> %.1f V, delay = %.3f ms, ramp = %.3f ms\n', ...
    minimum_output_voltage_v, soft_start_delay_s*1e3, ...
    soft_start_ramp_s*1e3);
fprintf(['Settling after ramp = %.6f ms, overshoot = %.6f V, ', ...
         'peak current = %.6f A\n'], ...
    soft_start_settling_s*1e3, soft_start_overshoot_v, ...
    soft_start_peak_current_a);
fprintf('Current-limit takeover = %d, saturated samples = %d\n', ...
    soft_start_current_limited, nnz(soft_saturated));
fprintf('24 -> 72 -> 24 V ramp maximum tracking error = %.6f V\n', ...
    range_ramp_maximum_error_v);

simulation.soft_start.time_s = soft_time_s;
simulation.soft_start.reference_voltage_v = soft_reference_voltage_v;
simulation.soft_start.output_voltage_v = soft_output_voltage_v;
simulation.soft_start.bus_voltage_v = soft_bus_voltage_v;
simulation.soft_start.bus_reference_v = soft_bus_reference_v;
simulation.soft_start.load_current_a = soft_load_current_a;
simulation.soft_start.normalized_command = soft_normalized_command;
simulation.soft_start.applied_command = soft_applied_command;
simulation.soft_start.switching_frequency_hz = ...
    soft_switching_frequency_hz;
simulation.soft_start.phase_shift_duty = soft_phase_shift_duty;
simulation.soft_start.shared_integral = soft_shared_integral;
simulation.soft_start.active_channel = soft_active_channel;
simulation.soft_start.ramp_delay_s = soft_start_delay_s;
simulation.soft_start.ramp_time_s = soft_start_ramp_s;
simulation.soft_start.settling_after_ramp_s = soft_start_settling_s;
simulation.soft_start.overshoot_v = soft_start_overshoot_v;
simulation.soft_start.peak_current_a = soft_start_peak_current_a;
simulation.soft_start.current_limited = soft_start_current_limited;
simulation.soft_start.range_ramp_time_s = range_ramp_time_s;
simulation.soft_start.range_ramp_maximum_error_v = ...
    range_ramp_maximum_error_v;
simulation.soft_start.load_resistance_ohm = soft_load_resistance_ohm;

%% 阶跃限流与缓变限流比较
% 第三个仿真比较“限流给定如何从非限制状态进入 165 A”。两种方式面对
% 完全相同的 6600 W -> 9000 W 负载阶跃：
%   1. Step：Ilimit 从 200 A 立即阶跃到 165 A；
%   2. Ramp：Ilimit 在 10 ms 内从 200 A 线性下降到 165 A。
% 200 A 高于 9000 W、48 V 时的 187.5 A，因此事件前不会误触发电流环。
% 该比较可以观察快速限流的电压跌落与缓变限流的电流下降斜率差异。
limit_comparison_end_s = 90e-3;
limit_load_step_s = 20e-3;
limit_load_release_s = 70e-3;
limit_ramp_time_s = 10e-3;
limit_initial_a = 200;
limit_final_a = current_limit_a;
limit_time_s = (0:sample_time_s:limit_comparison_end_s).';
limit_sample_count = numel(limit_time_s);
limit_mode_count = 2; % 第1列=阶跃限流，第2列=缓变限流

limit_load_resistance_ohm = nominal_load_ohm*ones(limit_sample_count, 1);
limit_overload_window = limit_time_s >= limit_load_step_s & ...
    limit_time_s < limit_load_release_s;
limit_load_resistance_ohm(limit_overload_window) = overload_load_ohm;

limit_reference_a = limit_initial_a*ones(limit_sample_count, limit_mode_count);
limit_reference_a(limit_time_s >= limit_load_step_s, 1) = limit_final_a;
limit_ramp_progress = min(max( ...
    (limit_time_s - limit_load_step_s)/limit_ramp_time_s, 0), 1);
limit_reference_a(:, 2) = limit_initial_a - ...
    (limit_initial_a - limit_final_a)*limit_ramp_progress;

limit_output_voltage_v = nominal_output_voltage_v*ones( ...
    limit_sample_count, limit_mode_count);
limit_bus_voltage_v = nominal_bus_voltage_v*ones( ...
    limit_sample_count, limit_mode_count);
limit_bus_reference_v = nominal_bus_voltage_v*ones( ...
    limit_sample_count, limit_mode_count);
limit_load_current_a = zeros(limit_sample_count, limit_mode_count);
limit_shared_integral = zeros(limit_sample_count, limit_mode_count);
limit_normalized_command = zeros(limit_sample_count, limit_mode_count);
limit_applied_command = zeros(limit_sample_count, limit_mode_count);
limit_switching_frequency_hz = zeros(limit_sample_count, limit_mode_count);
limit_active_channel = ones(limit_sample_count, limit_mode_count);
limit_saturated = false(limit_sample_count, limit_mode_count);
limit_i_share = nominal_u*ones(1, limit_mode_count);
limit_command_delay_line = nominal_u*ones(delay_samples, limit_mode_count);

% 两列状态在同一个时间循环中并行推进。外层是采样时刻，内层是阶跃和
% 缓变两种限流方式；除 Ilimit 给定不同外，其余模型和 PI 参数完全相同。
for sample_index = 1:limit_sample_count - 1
    current_load_ohm = limit_load_resistance_ohm(sample_index);
    if current_load_ohm == nominal_load_ohm
        active_curve = nominal_curve;
    else
        active_curve = overload_curve;
    end

    for mode_index = 1:limit_mode_count
        limit_load_current_a(sample_index, mode_index) = ...
            limit_output_voltage_v(sample_index, mode_index)/ ...
            current_load_ohm;
        voltage_error_v = nominal_output_voltage_v - ...
            limit_output_voltage_v(sample_index, mode_index);
        current_error_a = limit_reference_a(sample_index, mode_index) - ...
            limit_load_current_a(sample_index, mode_index);
        voltage_p = voltage_Kp*voltage_error_v;
        current_p = current_Kp*current_error_a;

        if voltage_p + limit_i_share(mode_index) <= ...
                current_p + limit_i_share(mode_index)
            limit_active_channel(sample_index, mode_index) = 1;
            active_error = voltage_error_v;
            active_ki_step = voltage_ki_step;
            active_p = voltage_p;
        else
            limit_active_channel(sample_index, mode_index) = 2;
            active_error = current_error_a;
            active_ki_step = current_ki_step;
            active_p = current_p;
        end

        limit_i_share(mode_index) = limit_i_share(mode_index) + ...
            active_ki_step*active_error;
        command = active_p + limit_i_share(mode_index);
        if command > output_limits(2)
            command = output_limits(2);
            limit_i_share(mode_index) = output_limits(2) - active_p;
            limit_saturated(sample_index, mode_index) = true;
        elseif command < output_limits(1)
            command = output_limits(1);
            limit_i_share(mode_index) = output_limits(1) - active_p;
            limit_saturated(sample_index, mode_index) = true;
        end
        limit_i_share(mode_index) = min(max( ...
            limit_i_share(mode_index), output_limits(1) - active_p), ...
            output_limits(2) - active_p);

        limit_shared_integral(sample_index, mode_index) = ...
            limit_i_share(mode_index);
        limit_normalized_command(sample_index, mode_index) = command;
        limit_applied_command(sample_index, mode_index) = ...
            limit_command_delay_line(1, mode_index);
        limit_command_delay_line(1:end-1, mode_index) = ...
            limit_command_delay_line(2:end, mode_index);
        limit_command_delay_line(end, mode_index) = command;
        [limit_switching_frequency_hz(sample_index, mode_index), ~] = ...
            hybrid_modulator( ...
                limit_applied_command(sample_index, mode_index), ...
                transition_u, fr_hz, fmax_hz);

        voltage_target_v = interp1(command_grid, ...
            active_curve.output_voltage_v, ...
            limit_applied_command(sample_index, mode_index), 'linear') * ...
            limit_bus_voltage_v(sample_index, mode_index)/ ...
            active_curve.input_voltage_v;
        output_time_constant_s = current_load_ohm*Cout_f/2;
        limit_output_voltage_v(sample_index + 1, mode_index) = ...
            limit_output_voltage_v(sample_index, mode_index) + ...
            sample_time_s/output_time_constant_s * ...
            (voltage_target_v - ...
             limit_output_voltage_v(sample_index, mode_index));

        limit_bus_reference_v(sample_index, mode_index) = max( ...
            minimum_bus_voltage_limit_v, ...
            bus_voltage_gain*nominal_output_voltage_v + ...
            bus_voltage_offset_v);
        limit_bus_voltage_v(sample_index + 1, mode_index) = ...
            limit_bus_voltage_v(sample_index, mode_index) + ...
            sample_time_s/bus_time_constant_s * ...
            (limit_bus_reference_v(sample_index, mode_index) - ...
             limit_bus_voltage_v(sample_index, mode_index));
    end
end

limit_load_current_a(end, :) = ...
    limit_output_voltage_v(end, :)/limit_load_resistance_ohm(end);
limit_bus_reference_v(end, :) = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*nominal_output_voltage_v + bus_voltage_offset_v);
limit_shared_integral(end, :) = limit_shared_integral(end - 1, :);
limit_normalized_command(end, :) = limit_normalized_command(end - 1, :);
limit_applied_command(end, :) = limit_applied_command(end - 1, :);
limit_switching_frequency_hz(end, :) = ...
    limit_switching_frequency_hz(end - 1, :);
limit_active_channel(end, :) = limit_active_channel(end - 1, :);
limit_saturated(end, :) = limit_saturated(end - 1, :);

limit_metric_window = limit_time_s >= limit_load_release_s - 5e-3 & ...
    limit_time_s < limit_load_release_s;
limit_settled_current_a = mean( ...
    limit_load_current_a(limit_metric_window, :), 1);
limit_minimum_voltage_v = min( ...
    limit_output_voltage_v(limit_overload_window, :), [], 1);
limit_peak_current_a = max( ...
    limit_load_current_a(limit_overload_window, :), [], 1);

fprintf('\n================ Step/ramp current-limit comparison =============\n');
fprintf('Ilimit: %.1f -> %.1f A, ramp time = %.3f ms\n', ...
    limit_initial_a, limit_final_a, limit_ramp_time_s*1e3);
fprintf(['Step limit: peak = %.6f A, settled = %.6f A, ', ...
         'minimum Vout = %.6f V\n'], ...
    limit_peak_current_a(1), limit_settled_current_a(1), ...
    limit_minimum_voltage_v(1));
fprintf(['Ramp limit: peak = %.6f A, settled = %.6f A, ', ...
         'minimum Vout = %.6f V\n'], ...
    limit_peak_current_a(2), limit_settled_current_a(2), ...
    limit_minimum_voltage_v(2));

simulation.current_limit_comparison.time_s = limit_time_s;
simulation.current_limit_comparison.reference_a = limit_reference_a;
simulation.current_limit_comparison.output_voltage_v = ...
    limit_output_voltage_v;
simulation.current_limit_comparison.bus_voltage_v = limit_bus_voltage_v;
simulation.current_limit_comparison.bus_reference_v = ...
    limit_bus_reference_v;
simulation.current_limit_comparison.load_current_a = limit_load_current_a;
simulation.current_limit_comparison.normalized_command = ...
    limit_normalized_command;
simulation.current_limit_comparison.applied_command = ...
    limit_applied_command;
simulation.current_limit_comparison.switching_frequency_hz = ...
    limit_switching_frequency_hz;
simulation.current_limit_comparison.shared_integral = ...
    limit_shared_integral;
simulation.current_limit_comparison.active_channel = ...
    limit_active_channel;
simulation.current_limit_comparison.load_resistance_ohm = ...
    limit_load_resistance_ohm;
simulation.current_limit_comparison.step_time_s = limit_load_step_s;
simulation.current_limit_comparison.ramp_time_s = limit_ramp_time_s;
simulation.current_limit_comparison.settled_current_a = ...
    limit_settled_current_a;
simulation.current_limit_comparison.minimum_voltage_v = ...
    limit_minimum_voltage_v;
simulation.current_limit_comparison.peak_current_a = limit_peak_current_a;

%% 单次连续运行：软启动、斜坡、CR/额定包络阶跃、CC 阶跃和两种限流
% 这一节是最终时域验证。所有事件共用同一个时间向量、输出电压状态、
% 母线状态、共享积分器和控制延时线，中途没有重新初始化或拼接波形。
continuous_end_s = 1.000;
continuous_time_s = (0:sample_time_s:continuous_end_s).';
continuous_sample_count = numel(continuous_time_s);

% 连续事件时间表
event.soft_start_begin_s = 0.005;
event.soft_start_end_s = 0.035;
event.ramp_up_begin_s = 0.050;
event.ramp_up_end_s = 0.110;
event.ramp_down_begin_s = 0.130;
event.ramp_down_end_s = 0.190;
event.envelope_step_48_s = 0.220;
event.envelope_step_72_s = 0.280;
event.envelope_step_48_down_s = 0.340;
event.envelope_step_24_s = 0.400;
event.cc_transition_begin_s = 0.460;
event.cc_transition_end_s = 0.480;
event.cc_step_48_s = 0.500;
event.cc_step_72_s = 0.560;
event.cc_step_48_down_s = 0.620;
event.cc_step_24_s = 0.680;
event.prepare_48_begin_s = 0.740;
event.prepare_48_end_s = 0.770;
event.nominal_load_begin_s = 0.790;
event.step_limit_begin_s = 0.820;
event.step_limit_end_s = 0.870;
event.ramp_limit_begin_s = 0.900;
event.ramp_limit_end_s = 0.960;
event.limit_ramp_time_s = 0.010;

% 先建立完整的连续电压给定。每一段都从上一段终点继续，不发生跳回零。
soft_progress = min(max((continuous_time_s - event.soft_start_begin_s)/ ...
    (event.soft_start_end_s - event.soft_start_begin_s), 0), 1);
continuous_reference_voltage_v = minimum_output_voltage_v*soft_progress;

ramp_up_progress = min(max((continuous_time_s - event.ramp_up_begin_s)/ ...
    (event.ramp_up_end_s - event.ramp_up_begin_s), 0), 1);
continuous_reference_voltage_v = continuous_reference_voltage_v + ...
    (maximum_output_voltage_v - minimum_output_voltage_v)*ramp_up_progress;
ramp_down_progress = min(max((continuous_time_s - event.ramp_down_begin_s)/ ...
    (event.ramp_down_end_s - event.ramp_down_begin_s), 0), 1);
continuous_reference_voltage_v = continuous_reference_voltage_v - ...
    (maximum_output_voltage_v - minimum_output_voltage_v)*ramp_down_progress;

continuous_reference_voltage_v(continuous_time_s >= ...
    event.envelope_step_48_s) = 48;
continuous_reference_voltage_v(continuous_time_s >= ...
    event.envelope_step_72_s) = 72;
continuous_reference_voltage_v(continuous_time_s >= ...
    event.envelope_step_48_down_s) = 48;
continuous_reference_voltage_v(continuous_time_s >= ...
    event.envelope_step_24_s) = 24;
continuous_reference_voltage_v(continuous_time_s >= ...
    event.cc_step_48_s) = 48;
continuous_reference_voltage_v(continuous_time_s >= ...
    event.cc_step_72_s) = 72;
continuous_reference_voltage_v(continuous_time_s >= ...
    event.cc_step_48_down_s) = 48;
continuous_reference_voltage_v(continuous_time_s >= ...
    event.cc_step_24_s) = 24;

prepare_progress = min(max((continuous_time_s - event.prepare_48_begin_s)/ ...
    (event.prepare_48_end_s - event.prepare_48_begin_s), 0), 1);
prepare_window = continuous_time_s >= event.prepare_48_begin_s & ...
    continuous_time_s < event.prepare_48_end_s;
continuous_reference_voltage_v(prepare_window) = ...
    24 + (48 - 24)*prepare_progress(prepare_window);
continuous_reference_voltage_v(continuous_time_s >= ...
    event.prepare_48_end_s) = 48;

% 限流参考也在同一个时间轴上连续定义。阶跃限流直接 200->165 A；
% 缓变限流在 10 ms 内从 200 A 下降到 165 A。
continuous_current_limit_a = current_limit_a*ones(continuous_sample_count, 1);
continuous_current_limit_a(continuous_time_s >= ...
    event.nominal_load_begin_s) = 200;
continuous_current_limit_a(continuous_time_s >= event.step_limit_begin_s & ...
    continuous_time_s < event.step_limit_end_s) = current_limit_a;
ramp_limit_progress = min(max((continuous_time_s - ...
    event.ramp_limit_begin_s)/event.limit_ramp_time_s, 0), 1);
ramp_limit_window = continuous_time_s >= event.ramp_limit_begin_s & ...
    continuous_time_s < event.ramp_limit_end_s;
continuous_current_limit_a(ramp_limit_window) = 200 - ...
    (200 - current_limit_a)*ramp_limit_progress(ramp_limit_window);

% 单一连续状态向量
continuous_output_voltage_v = zeros(continuous_sample_count, 1);
continuous_bus_voltage_v = ...
    minimum_bus_voltage_limit_v*ones(continuous_sample_count, 1);
continuous_bus_reference_v = ...
    minimum_bus_voltage_limit_v*ones(continuous_sample_count, 1);
continuous_load_current_a = zeros(continuous_sample_count, 1);
continuous_shared_integral = zeros(continuous_sample_count, 1);
continuous_voltage_candidate = zeros(continuous_sample_count, 1);
continuous_current_candidate = zeros(continuous_sample_count, 1);
continuous_normalized_command = zeros(continuous_sample_count, 1);
continuous_applied_command = zeros(continuous_sample_count, 1);
continuous_switching_frequency_hz = fmax_hz*ones(continuous_sample_count, 1);
continuous_active_channel = ones(continuous_sample_count, 1);
continuous_saturated = false(continuous_sample_count, 1);
continuous_command_delay_line = zeros(delay_samples, 1);
continuous_i_share = 0;

for sample_index = 1:continuous_sample_count - 1
    current_time_s = continuous_time_s(sample_index);
    current_output_v = continuous_output_voltage_v(sample_index);
    current_reference_v = continuous_reference_voltage_v(sample_index);

    % 负载模式沿时间连续切换：额定包络 -> CC -> 额定负载/过载。
    if current_time_s < event.cc_transition_begin_s
        if current_reference_v < minimum_output_voltage_v
            load_current = current_output_v/minimum_load_ohm;
        else
            load_current = min(rated_current_a, ...
                nominal_power_w/max(current_output_v, eps));
        end
        curve_mode = 1; % 额定包络曲线
        curve_blend = 0;
    elseif current_time_s < event.cc_transition_end_s
        transition_progress = (current_time_s - ...
            event.cc_transition_begin_s)/(event.cc_transition_end_s - ...
            event.cc_transition_begin_s);
        envelope_current = min(rated_current_a, ...
            nominal_power_w/max(current_output_v, eps));
        load_current = envelope_current + transition_progress* ...
            (cc_load_current_a - envelope_current);
        curve_mode = 3; % 额定包络与 CC 曲线混合
        curve_blend = transition_progress;
    elseif current_time_s < event.nominal_load_begin_s
        if current_time_s < event.prepare_48_end_s
            load_current = cc_load_current_a;
            curve_blend = 0;
        else
            load_transition = min(max((current_time_s - ...
                event.prepare_48_end_s)/(event.nominal_load_begin_s - ...
                event.prepare_48_end_s), 0), 1);
            nominal_current = current_output_v/nominal_load_ohm;
            load_current = cc_load_current_a + load_transition* ...
                (nominal_current - cc_load_current_a);
            curve_blend = load_transition;
        end
        curve_mode = 2; % CC 曲线，末端向额定 48 V 曲线过渡
    else
        overload_active = ...
            (current_time_s >= event.step_limit_begin_s && ...
             current_time_s < event.step_limit_end_s) || ...
            (current_time_s >= event.ramp_limit_begin_s && ...
             current_time_s < event.ramp_limit_end_s);
        if overload_active
            load_current = current_output_v/overload_load_ohm;
            curve_mode = 4;
        else
            load_current = current_output_v/nominal_load_ohm;
            curve_mode = 5;
        end
        curve_blend = 0;
    end
    continuous_load_current_a(sample_index) = load_current;

    voltage_error_v = current_reference_v - current_output_v;
    current_error_a = continuous_current_limit_a(sample_index) - load_current;
    voltage_p = voltage_Kp*voltage_error_v;
    current_p = current_Kp*current_error_a;
    voltage_raw = voltage_p + continuous_i_share;
    current_raw = current_p + continuous_i_share;

    if voltage_raw <= current_raw
        continuous_active_channel(sample_index) = 1;
        active_error = voltage_error_v;
        active_ki_step = voltage_ki_step;
        active_p = voltage_p;
    else
        continuous_active_channel(sample_index) = 2;
        active_error = current_error_a;
        active_ki_step = current_ki_step;
        active_p = current_p;
    end

    continuous_i_share = continuous_i_share + active_ki_step*active_error;
    command = active_p + continuous_i_share;
    if command > output_limits(2)
        command = output_limits(2);
        continuous_i_share = output_limits(2) - active_p;
        continuous_saturated(sample_index) = true;
    elseif command < output_limits(1)
        command = output_limits(1);
        continuous_i_share = output_limits(1) - active_p;
        continuous_saturated(sample_index) = true;
    end
    continuous_i_share = min(max(continuous_i_share, ...
        output_limits(1) - active_p), output_limits(2) - active_p);

    continuous_shared_integral(sample_index) = continuous_i_share;
    continuous_voltage_candidate(sample_index) = voltage_p + continuous_i_share;
    continuous_current_candidate(sample_index) = current_p + continuous_i_share;
    continuous_normalized_command(sample_index) = command;
    continuous_applied_command(sample_index) = ...
        continuous_command_delay_line(1);
    continuous_command_delay_line(1:end-1) = ...
        continuous_command_delay_line(2:end);
    continuous_command_delay_line(end) = command;
    [continuous_switching_frequency_hz(sample_index), ~] = ...
        hybrid_modulator(continuous_applied_command(sample_index), ...
            transition_u, fr_hz, fmax_hz);

    reference_for_curve_v = min(max(current_reference_v, ...
        minimum_output_voltage_v), maximum_output_voltage_v);
    envelope_target_v = interpolate_full_range_target( ...
        continuous_applied_command(sample_index), reference_for_curve_v, ...
        continuous_bus_voltage_v(sample_index), minimum_output_voltage_v, ...
        nominal_output_voltage_v, maximum_output_voltage_v, ...
        minimum_curve, nominal_curve, maximum_curve, command_grid);
    cc_target_v = interpolate_full_range_target( ...
        continuous_applied_command(sample_index), reference_for_curve_v, ...
        continuous_bus_voltage_v(sample_index), minimum_output_voltage_v, ...
        nominal_output_voltage_v, maximum_output_voltage_v, ...
        cc_minimum_curve, cc_nominal_curve, cc_maximum_curve, command_grid);

    if curve_mode == 1
        voltage_target_v = envelope_target_v;
    elseif curve_mode == 2
        nominal_target_v = interp1(command_grid, ...
            nominal_curve.output_voltage_v, ...
            continuous_applied_command(sample_index), 'linear') * ...
            continuous_bus_voltage_v(sample_index)/ ...
            nominal_curve.input_voltage_v;
        voltage_target_v = cc_target_v + curve_blend* ...
            (nominal_target_v - cc_target_v);
    elseif curve_mode == 3
        voltage_target_v = envelope_target_v + curve_blend* ...
            (cc_target_v - envelope_target_v);
    elseif curve_mode == 4
        voltage_target_v = interp1(command_grid, ...
            overload_curve.output_voltage_v, ...
            continuous_applied_command(sample_index), 'linear') * ...
            continuous_bus_voltage_v(sample_index)/ ...
            overload_curve.input_voltage_v;
    else
        voltage_target_v = interp1(command_grid, ...
            nominal_curve.output_voltage_v, ...
            continuous_applied_command(sample_index), 'linear') * ...
            continuous_bus_voltage_v(sample_index)/ ...
            nominal_curve.input_voltage_v;
    end

    effective_load_ohm = max(current_output_v, 1)/max(load_current, 1);
    output_time_constant_s = effective_load_ohm*Cout_f/2;
    continuous_output_voltage_v(sample_index + 1) = current_output_v + ...
        sample_time_s/output_time_constant_s * ...
        (voltage_target_v - current_output_v);

    continuous_bus_reference_v(sample_index) = max( ...
        minimum_bus_voltage_limit_v, ...
        bus_voltage_gain*current_reference_v + bus_voltage_offset_v);
    continuous_bus_voltage_v(sample_index + 1) = ...
        continuous_bus_voltage_v(sample_index) + ...
        sample_time_s/bus_time_constant_s * ...
        (continuous_bus_reference_v(sample_index) - ...
         continuous_bus_voltage_v(sample_index));
end

continuous_load_current_a(end) = continuous_load_current_a(end - 1);
continuous_shared_integral(end) = continuous_shared_integral(end - 1);
continuous_voltage_candidate(end) = continuous_voltage_candidate(end - 1);
continuous_current_candidate(end) = continuous_current_candidate(end - 1);
continuous_normalized_command(end) = continuous_normalized_command(end - 1);
continuous_applied_command(end) = continuous_applied_command(end - 1);
continuous_switching_frequency_hz(end) = ...
    continuous_switching_frequency_hz(end - 1);
continuous_active_channel(end) = continuous_active_channel(end - 1);
continuous_saturated(end) = continuous_saturated(end - 1);
continuous_bus_reference_v(end) = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*continuous_reference_voltage_v(end) + ...
    bus_voltage_offset_v);

simulation.continuous.time_s = continuous_time_s;
simulation.continuous.reference_voltage_v = continuous_reference_voltage_v;
simulation.continuous.output_voltage_v = continuous_output_voltage_v;
simulation.continuous.load_current_a = continuous_load_current_a;
simulation.continuous.current_limit_a = continuous_current_limit_a;
simulation.continuous.normalized_command = continuous_normalized_command;
simulation.continuous.shared_integral = continuous_shared_integral;
simulation.continuous.voltage_candidate = continuous_voltage_candidate;
simulation.continuous.current_candidate = continuous_current_candidate;
simulation.continuous.active_channel = continuous_active_channel;
simulation.continuous.switching_frequency_hz = ...
    continuous_switching_frequency_hz;
simulation.continuous.bus_voltage_v = continuous_bus_voltage_v;
simulation.continuous.bus_reference_v = continuous_bus_reference_v;
simulation.continuous.saturated = continuous_saturated;
simulation.continuous.events = event;

%% 三个验证阶段使用同一个显示时间轴
% 三段仿真本身仍保持独立初始条件，但绘图时按时间依次拼接：
%   1. 软启动及 24~72~24 V 全范围斜坡；
%   2. 24~48~72~48~24 V 全范围阶跃；
%   3. 阶跃限流与缓变限流叠加比较。
% stage_gap_s 在阶段之间留出空白，明确表示此处切换了独立验证工况，
% 避免把状态重置误解为真实连续暂态。五个子图共享同一个全局横轴。
stage_gap_s = 20e-3;
soft_display_time_s = soft_time_s;
step_display_offset_s = soft_display_time_s(end) + stage_gap_s;
step_display_time_s = step_display_offset_s + time_s;
limit_display_offset_s = step_display_time_s(end) + stage_gap_s;
limit_display_time_s = limit_display_offset_s + limit_time_s;

% 旧的独立工况叠加图默认关闭；保留开关仅供逐段调试。正式结果只显示
% 后面的单次 continuous 仿真，不使用这些带独立初始条件的数据绘图。
show_independent_diagnostic_figure = ...
    strcmp(getenv('CLLC_SHOW_INDEPENDENT'), '1');
if show_independent_diagnostic_figure
figure('Color', 'w', 'Name', 'CLLC three time-domain simulations', ...
    'WindowState', 'maximized');
layout = tiledlayout(5, 1, 'TileSpacing', 'compact', ...
    'Padding', 'compact');

voltage_axes = nexttile(layout, 1);
plot(voltage_axes, step_display_time_s*1e3, reference_voltage_v, '--', ...
    'LineWidth', 1.2, 'DisplayName', 'Vref');
hold(voltage_axes, 'on');
plot(voltage_axes, step_display_time_s*1e3, output_voltage_v, ...
    'LineWidth', 1.5, 'DisplayName', 'Vout');
grid(voltage_axes, 'on');
ylabel(voltage_axes, 'Voltage / V');
legend(voltage_axes, 'Location', 'best');
title(voltage_axes, 'Output-voltage response of all verification stages');

current_axes = nexttile(layout, 2);
plot(current_axes, step_display_time_s*1e3, load_current_a, ...
    'LineWidth', 1.5, 'DisplayName', 'Iload');
hold(current_axes, 'on');
yline(current_axes, current_limit_a, '--r', 'Ilimit', ...
    'DisplayName', 'Ilimit');
grid(current_axes, 'on');
ylabel(current_axes, 'Current / A');
legend(current_axes, 'Location', 'best');
title(current_axes, 'Load-current limiting response');

command_axes = nexttile(layout, 3);
plot(command_axes, step_display_time_s*1e3, normalized_command, ...
    'LineWidth', 1.5, 'DisplayName', 'u');
hold(command_axes, 'on');
plot(command_axes, step_display_time_s*1e3, shared_integral, '--', ...
    'LineWidth', 1.1, 'DisplayName', 'i_{share}');
yline(command_axes, transition_u, ':k', 'u_t', ...
    'DisplayName', 'transition');
grid(command_axes, 'on');
ylabel(command_axes, 'Normalized');
ylim(command_axes, [-0.05, 1.15]);
legend(command_axes, 'Location', 'best');
title(command_axes, 'Competition output and shared integral');

competition_axes = nexttile(layout, 4);
yyaxis(competition_axes, 'left');
plot(competition_axes, step_display_time_s*1e3, voltage_candidate, ...
    'LineWidth', 1.1, 'DisplayName', 'Voltage candidate');
hold(competition_axes, 'on');
plot(competition_axes, step_display_time_s*1e3, current_candidate, ...
    'LineWidth', 1.1, 'DisplayName', 'Current candidate');
ylabel(competition_axes, 'Raw candidate u');
yyaxis(competition_axes, 'right');
stairs(competition_axes, step_display_time_s*1e3, active_channel, ...
    'k', 'LineWidth', 1.0, 'DisplayName', 'Active channel');
ylim(competition_axes, [0.8, 2.2]);
yticks(competition_axes, [1, 2]);
yticklabels(competition_axes, {'Voltage', 'Current'});
ylabel(competition_axes, 'Winner');
grid(competition_axes, 'on');
title(competition_axes, 'MIN competition and active channel');

modulation_axes = nexttile(layout, 5);
yyaxis(modulation_axes, 'left');
plot(modulation_axes, step_display_time_s*1e3, ...
    switching_frequency_hz/1e3, ...
    'LineWidth', 1.4, 'DisplayName', 'fs');
ylabel(modulation_axes, 'Frequency / kHz');
yyaxis(modulation_axes, 'right');
plot(modulation_axes, step_display_time_s*1e3, bus_reference_v, '--', ...
    'LineWidth', 1.1, 'DisplayName', 'Vbus ref');
hold(modulation_axes, 'on');
plot(modulation_axes, step_display_time_s*1e3, bus_voltage_v, ...
    'LineWidth', 1.2, 'DisplayName', 'Vbus');
ylabel(modulation_axes, 'Bus voltage / V');
legend(modulation_axes, 'Location', 'best');
grid(modulation_axes, 'on');
xlabel(modulation_axes, 'Time / ms');
title(modulation_axes, 'Switching frequency and lagged bus voltage');

event_times_ms = 1e3*(step_display_offset_s + ...
    [step_24_to_48_s, step_48_to_72_s, ...
     step_72_to_48_s, step_48_to_24_s]);
all_axes = [voltage_axes, current_axes, command_axes, ...
    competition_axes, modulation_axes];
for axes_index = 1:numel(all_axes)
    for event_index = 1:numel(event_times_ms)
        xline(all_axes(axes_index), event_times_ms(event_index), ':', ...
            'HandleVisibility', 'off');
    end
end
%% 将软启动/斜坡阶段叠加到统一坐标轴
soft_voltage_axes = voltage_axes;
hold(soft_voltage_axes, 'on');
plot(soft_voltage_axes, soft_display_time_s*1e3, ...
    soft_reference_voltage_v, '--', ...
    'LineWidth', 1.2, 'DisplayName', 'Vref ramp');
hold(soft_voltage_axes, 'on');
plot(soft_voltage_axes, soft_display_time_s*1e3, soft_output_voltage_v, ...
    'LineWidth', 1.5, 'DisplayName', 'Vout');
grid(soft_voltage_axes, 'on');
ylabel(soft_voltage_axes, 'Voltage / V');
legend(soft_voltage_axes, 'Location', 'best');
title(soft_voltage_axes, 'Output-voltage response of all stages');

soft_current_axes = current_axes;
hold(soft_current_axes, 'on');
plot(soft_current_axes, soft_display_time_s*1e3, soft_load_current_a, ...
    'LineWidth', 1.5, 'DisplayName', 'Iload');
hold(soft_current_axes, 'on');
yline(soft_current_axes, current_limit_a, '--r', 'Ilimit', ...
    'DisplayName', 'Ilimit');
grid(soft_current_axes, 'on');
ylabel(soft_current_axes, 'Current / A');
legend(soft_current_axes, 'Location', 'best');
title(soft_current_axes, 'Load current and current-limit references');

soft_command_axes = command_axes;
hold(soft_command_axes, 'on');
plot(soft_command_axes, soft_display_time_s*1e3, ...
    soft_normalized_command, ...
    'LineWidth', 1.5, 'DisplayName', 'u');
hold(soft_command_axes, 'on');
plot(soft_command_axes, soft_display_time_s*1e3, ...
    soft_shared_integral, '--', ...
    'LineWidth', 1.1, 'DisplayName', 'i_{share}');
yline(soft_command_axes, transition_u, ':k', 'u_t', ...
    'DisplayName', 'transition');
grid(soft_command_axes, 'on');
ylabel(soft_command_axes, 'Normalized');
ylim(soft_command_axes, [-0.05, 1.05]);
legend(soft_command_axes, 'Location', 'best');
title(soft_command_axes, 'Competition output and shared integral');

soft_winner_axes = competition_axes;
yyaxis(soft_winner_axes, 'right');
hold(soft_winner_axes, 'on');
stairs(soft_winner_axes, soft_display_time_s*1e3, soft_active_channel, ...
    'k', 'LineWidth', 1.1);
ylim(soft_winner_axes, [0.8, 2.2]);
yticks(soft_winner_axes, [1, 2]);
yticklabels(soft_winner_axes, {'Voltage', 'Current'});
grid(soft_winner_axes, 'on');
ylabel(soft_winner_axes, 'Winner');
title(soft_winner_axes, 'Active competition channel');

soft_modulation_axes = modulation_axes;
yyaxis(soft_modulation_axes, 'left');
hold(soft_modulation_axes, 'on');
plot(soft_modulation_axes, soft_display_time_s*1e3, ...
    soft_switching_frequency_hz/1e3, 'LineWidth', 1.4, ...
    'DisplayName', 'fs');
ylabel(soft_modulation_axes, 'Frequency / kHz');
yyaxis(soft_modulation_axes, 'right');
plot(soft_modulation_axes, soft_display_time_s*1e3, ...
    soft_bus_reference_v, '--', 'LineWidth', 1.1, ...
    'DisplayName', 'Vbus ref');
hold(soft_modulation_axes, 'on');
plot(soft_modulation_axes, soft_display_time_s*1e3, ...
    soft_bus_voltage_v, 'LineWidth', 1.2, 'DisplayName', 'Vbus');
ylabel(soft_modulation_axes, 'Bus voltage / V');
legend(soft_modulation_axes, 'Location', 'best');
grid(soft_modulation_axes, 'on');
xlabel(soft_modulation_axes, 'Time / ms');
title(soft_modulation_axes, 'Frequency and lagged bus voltage');

soft_event_times_ms = 1e3*[soft_start_delay_s, soft_ramp_end_s, ...
    range_ramp_up_start_s, range_ramp_up_start_s + range_ramp_time_s, ...
    range_ramp_down_start_s, range_ramp_down_start_s + range_ramp_time_s];
soft_all_axes = [soft_voltage_axes, soft_current_axes, ...
    soft_command_axes, soft_winner_axes, soft_modulation_axes];
for axes_index = 1:numel(soft_all_axes)
    for event_index = 1:numel(soft_event_times_ms)
        xline(soft_all_axes(axes_index), ...
            soft_event_times_ms(event_index), ':', ...
            'HandleVisibility', 'off');
    end
end

%% 阶跃/缓变限流比较波形
limit_voltage_axes = voltage_axes;
hold(limit_voltage_axes, 'on');
plot(limit_voltage_axes, limit_display_time_s*1e3, ...
    limit_output_voltage_v(:, 1), 'LineWidth', 1.3, ...
    'DisplayName', 'Step limit');
hold(limit_voltage_axes, 'on');
plot(limit_voltage_axes, limit_display_time_s*1e3, ...
    limit_output_voltage_v(:, 2), '--', 'LineWidth', 1.3, ...
    'DisplayName', 'Ramp limit');
yline(limit_voltage_axes, nominal_output_voltage_v, ':k', ...
    'HandleVisibility', 'off');
grid(limit_voltage_axes, 'on');
ylabel(limit_voltage_axes, 'Voltage / V');
legend(limit_voltage_axes, 'Location', 'best');
title(limit_voltage_axes, 'Output-voltage response of all stages');

limit_current_axes = current_axes;
hold(limit_current_axes, 'on');
plot(limit_current_axes, limit_display_time_s*1e3, ...
    limit_load_current_a(:, 1), 'LineWidth', 1.3, ...
    'DisplayName', 'Iload step');
hold(limit_current_axes, 'on');
plot(limit_current_axes, limit_display_time_s*1e3, ...
    limit_load_current_a(:, 2), '--', 'LineWidth', 1.3, ...
    'DisplayName', 'Iload ramp');
plot(limit_current_axes, limit_display_time_s*1e3, ...
    limit_reference_a(:, 1), ':', 'LineWidth', 1.1, ...
    'DisplayName', 'Ilimit step');
plot(limit_current_axes, limit_display_time_s*1e3, ...
    limit_reference_a(:, 2), '-.', 'LineWidth', 1.1, ...
    'DisplayName', 'Ilimit ramp');
grid(limit_current_axes, 'on');
ylabel(limit_current_axes, 'Current / A');
legend(limit_current_axes, 'Location', 'best');
title(limit_current_axes, 'Load current and current-limit references');

limit_command_axes = command_axes;
hold(limit_command_axes, 'on');
plot(limit_command_axes, limit_display_time_s*1e3, ...
    limit_normalized_command(:, 1), 'LineWidth', 1.3, ...
    'DisplayName', 'u step');
hold(limit_command_axes, 'on');
plot(limit_command_axes, limit_display_time_s*1e3, ...
    limit_normalized_command(:, 2), '--', 'LineWidth', 1.3, ...
    'DisplayName', 'u ramp');
plot(limit_command_axes, limit_display_time_s*1e3, ...
    limit_shared_integral(:, 1), ':', 'LineWidth', 1.0, ...
    'DisplayName', 'i_{share} step');
plot(limit_command_axes, limit_display_time_s*1e3, ...
    limit_shared_integral(:, 2), '-.', 'LineWidth', 1.0, ...
    'DisplayName', 'i_{share} ramp');
grid(limit_command_axes, 'on');
ylabel(limit_command_axes, 'Normalized');
ylim(limit_command_axes, [-0.05, 1.15]);
legend(limit_command_axes, 'Location', 'best');
title(limit_command_axes, 'Command and shared integral');

limit_winner_axes = competition_axes;
yyaxis(limit_winner_axes, 'right');
hold(limit_winner_axes, 'on');
stairs(limit_winner_axes, limit_display_time_s*1e3, ...
    limit_active_channel(:, 1), 'LineWidth', 1.1, ...
    'DisplayName', 'Step limit');
hold(limit_winner_axes, 'on');
stairs(limit_winner_axes, limit_display_time_s*1e3, ...
    limit_active_channel(:, 2) + 0.05, '--', 'LineWidth', 1.1, ...
    'DisplayName', 'Ramp limit (+0.05)');
ylim(limit_winner_axes, [0.8, 2.2]);
yticks(limit_winner_axes, [1, 2]);
yticklabels(limit_winner_axes, {'Voltage', 'Current'});
grid(limit_winner_axes, 'on');
ylabel(limit_winner_axes, 'Winner');
legend(limit_winner_axes, 'Location', 'best');
title(limit_winner_axes, 'Competition takeover');

limit_modulation_axes = modulation_axes;
yyaxis(limit_modulation_axes, 'left');
hold(limit_modulation_axes, 'on');
plot(limit_modulation_axes, limit_display_time_s*1e3, ...
    limit_switching_frequency_hz(:, 1)/1e3, 'LineWidth', 1.3, ...
    'DisplayName', 'fs step');
hold(limit_modulation_axes, 'on');
plot(limit_modulation_axes, limit_display_time_s*1e3, ...
    limit_switching_frequency_hz(:, 2)/1e3, '--', 'LineWidth', 1.3, ...
    'DisplayName', 'fs ramp');
ylabel(limit_modulation_axes, 'Frequency / kHz');
yyaxis(limit_modulation_axes, 'right');
plot(limit_modulation_axes, limit_display_time_s*1e3, ...
    limit_bus_voltage_v(:, 1), ':', 'LineWidth', 1.1, ...
    'DisplayName', 'Vbus step');
hold(limit_modulation_axes, 'on');
plot(limit_modulation_axes, limit_display_time_s*1e3, ...
    limit_bus_voltage_v(:, 2), '-.', 'LineWidth', 1.1, ...
    'DisplayName', 'Vbus ramp');
ylabel(limit_modulation_axes, 'Bus voltage / V');
grid(limit_modulation_axes, 'on');
xlabel(limit_modulation_axes, 'Time / ms');
legend(limit_modulation_axes, 'Location', 'best');
title(limit_modulation_axes, 'Frequency and bus response');

limit_event_times_ms = 1e3*(limit_display_offset_s + ...
    [limit_load_step_s, limit_load_release_s]);
limit_all_axes = [limit_voltage_axes, limit_current_axes, ...
    limit_command_axes, limit_winner_axes, limit_modulation_axes];
for axes_index = 1:numel(limit_all_axes)
    for event_index = 1:numel(limit_event_times_ms)
        xline(limit_all_axes(axes_index), ...
            limit_event_times_ms(event_index), ':', ...
            'HandleVisibility', 'off');
    end
end

% 用阶段起点标识独立工况切换，并让五个子图严格共享同一个横轴。
stage_start_times_ms = 1e3*[step_display_offset_s, limit_display_offset_s];
for axes_index = 1:numel(all_axes)
    xline(all_axes(axes_index), stage_start_times_ms(1), '--k', ...
        'Step stage', 'HandleVisibility', 'off');
    xline(all_axes(axes_index), stage_start_times_ms(2), '--k', ...
        'Current-limit stage', 'HandleVisibility', 'off');
end
linkaxes(all_axes, 'x');
xlim(all_axes(1), [0, limit_display_time_s(end)*1e3]);

title(layout, ...
    'CLLC shared-integral MIN competition on one common time axis');
end

%% 最终显示：单次连续仿真的统一时间轴
% 前面的独立工况保留用于参数对照和指标计算；最终窗口只显示 continuous
% 仿真。该窗口中的所有曲线来自同一个控制状态轨迹，不存在拼接或复位。
figure('Color', 'w', 'Name', 'CLLC continuous full verification', ...
    'WindowState', 'maximized');
continuous_layout = tiledlayout(6, 1, 'TileSpacing', 'compact', ...
    'Padding', 'compact');

continuous_voltage_axes = nexttile(continuous_layout);
plot(continuous_voltage_axes, continuous_time_s*1e3, ...
    continuous_reference_voltage_v, '--', 'LineWidth', 1.1, ...
    'DisplayName', 'Vref');
hold(continuous_voltage_axes, 'on');
plot(continuous_voltage_axes, continuous_time_s*1e3, ...
    continuous_output_voltage_v, 'LineWidth', 1.4, ...
    'DisplayName', 'Vout');
grid(continuous_voltage_axes, 'on');
ylabel(continuous_voltage_axes, 'Voltage / V');
legend(continuous_voltage_axes, 'Location', 'best');
title(continuous_voltage_axes, 'Continuous output-voltage verification');

continuous_current_axes = nexttile(continuous_layout);
plot(continuous_current_axes, continuous_time_s*1e3, ...
    continuous_load_current_a, 'LineWidth', 1.3, ...
    'DisplayName', 'Iload');
hold(continuous_current_axes, 'on');
plot(continuous_current_axes, continuous_time_s*1e3, ...
    continuous_current_limit_a, '--', 'LineWidth', 1.1, ...
    'DisplayName', 'Ilimit');
yline(continuous_current_axes, rated_current_a, ':k', ...
    '150 A rated', 'DisplayName', 'Irated');
grid(continuous_current_axes, 'on');
ylabel(continuous_current_axes, 'Current / A');
legend(continuous_current_axes, 'Location', 'best');
title(continuous_current_axes, 'Continuous load and limiting current');

continuous_command_axes = nexttile(continuous_layout);
plot(continuous_command_axes, continuous_time_s*1e3, ...
    continuous_normalized_command, 'LineWidth', 1.3, ...
    'DisplayName', 'u');
hold(continuous_command_axes, 'on');
plot(continuous_command_axes, continuous_time_s*1e3, ...
    continuous_shared_integral, '--', 'LineWidth', 1.1, ...
    'DisplayName', 'i_{share}');
yline(continuous_command_axes, transition_u, ':k', ...
    'u_t', 'DisplayName', 'PSM/PFM transition');
grid(continuous_command_axes, 'on');
ylabel(continuous_command_axes, 'Normalized');
ylim(continuous_command_axes, [-0.05, 1.15]);
legend(continuous_command_axes, 'Location', 'best');
title(continuous_command_axes, 'Command and uninterrupted shared integral');

continuous_compete_axes = nexttile(continuous_layout);
yyaxis(continuous_compete_axes, 'left');
plot(continuous_compete_axes, continuous_time_s*1e3, ...
    continuous_voltage_candidate, 'LineWidth', 1.0, ...
    'DisplayName', 'Voltage candidate');
hold(continuous_compete_axes, 'on');
plot(continuous_compete_axes, continuous_time_s*1e3, ...
    continuous_current_candidate, '--', 'LineWidth', 1.0, ...
    'DisplayName', 'Current candidate');
ylabel(continuous_compete_axes, 'Raw candidate u');
yyaxis(continuous_compete_axes, 'right');
stairs(continuous_compete_axes, continuous_time_s*1e3, ...
    continuous_active_channel, 'k', 'LineWidth', 1.0, ...
    'DisplayName', 'Winner');
ylim(continuous_compete_axes, [0.8, 2.2]);
yticks(continuous_compete_axes, [1, 2]);
yticklabels(continuous_compete_axes, {'Voltage', 'Current'});
ylabel(continuous_compete_axes, 'Winner');
grid(continuous_compete_axes, 'on');
title(continuous_compete_axes, 'MIN competition without state reset');

continuous_modulation_axes = nexttile(continuous_layout);
plot(continuous_modulation_axes, continuous_time_s*1e3, ...
    continuous_switching_frequency_hz/1e3, 'LineWidth', 1.2, ...
    'DisplayName', 'fs');
ylabel(continuous_modulation_axes, 'Frequency / kHz');
grid(continuous_modulation_axes, 'on');
legend(continuous_modulation_axes, 'Location', 'best');
title(continuous_modulation_axes, 'Switching-frequency response');

continuous_bus_axes = nexttile(continuous_layout);
plot(continuous_bus_axes, continuous_time_s*1e3, ...
    continuous_bus_reference_v, '--', 'LineWidth', 1.0, ...
    'DisplayName', 'Vbus ref');
hold(continuous_bus_axes, 'on');
plot(continuous_bus_axes, continuous_time_s*1e3, ...
    continuous_bus_voltage_v, 'LineWidth', 1.2, ...
    'DisplayName', 'Vbus');
yline(continuous_bus_axes, minimum_bus_voltage_limit_v, ':k', ...
    '370 V minimum', 'DisplayName', 'Vbus minimum');
ylabel(continuous_bus_axes, 'Bus voltage / V');
ylim(continuous_bus_axes, [350, 630]);
grid(continuous_bus_axes, 'on');
xlabel(continuous_bus_axes, 'Continuous simulation time / ms');
legend(continuous_bus_axes, 'Location', 'best');
title(continuous_bus_axes, 'Bus-voltage reference and lagged actual voltage');

continuous_axes = [continuous_voltage_axes, continuous_current_axes, ...
    continuous_command_axes, continuous_compete_axes, ...
    continuous_modulation_axes, continuous_bus_axes];
major_events_s = [event.soft_start_begin_s, event.ramp_up_begin_s, ...
    event.ramp_down_begin_s, event.envelope_step_48_s, ...
    event.cc_transition_begin_s, event.cc_step_48_s, ...
    event.prepare_48_begin_s, event.step_limit_begin_s, ...
    event.ramp_limit_begin_s];
for axes_index = 1:numel(continuous_axes)
    for event_index = 1:numel(major_events_s)
        xline(continuous_axes(axes_index), major_events_s(event_index)*1e3, ...
            ':', 'HandleVisibility', 'off');
    end
end
linkaxes(continuous_axes, 'x');
xlim(continuous_voltage_axes, [0, continuous_end_s*1e3]);
title(continuous_layout, ...
    'CLLC single uninterrupted simulation: ramp, CR/envelope, CC and current limit');

%% 局部函数
function curve = build_hybrid_static_curve( ...
    command_grid, load_ohm, Vin_v, reference_voltage_v, fr_hz, fmax_hz, ...
    Lrp, Crp, Lmp, Lrs, Crs, n)
%BUILD_HYBRID_STATIC_CURVE 建立指定负载的 u -> Vout 静态曲线。
% 先计算满相移时 fr~fmax 的 PFM 电压曲线，再按边界连续条件得到 ut。
% 对 u<=ut 的 PSM 段，用基波幅值随相移变化的 sin(pi*duty) 近似；
% 对 u>ut 的 PFM 段，把 u 线性映射到 fmax~fr 并查询 FHA 电压。

    frequency_grid_hz = linspace(fr_hz, fmax_hz, 4001).';
    Rac_ohm = (8/pi^2)*load_ohm;
    full_voltage_v = calculate_forward_fha_voltage( ...
        frequency_grid_hz, Vin_v, Rac_ohm, ...
        Lrp, Crp, Lmp, Lrs, Crs, n);
    transition_u = full_voltage_v(end)/full_voltage_v(1);
    output_voltage_v = nan(size(command_grid));
    psm = command_grid <= transition_u;
    pfm = ~psm;
    duty = 0.5*command_grid(psm)/transition_u;
    output_voltage_v(psm) = full_voltage_v(end)*sin(pi*duty);
    mapped_frequency_hz = fmax_hz - ...
        (command_grid(pfm) - transition_u)/(1 - transition_u) * ...
        (fmax_hz - fr_hz);
    output_voltage_v(pfm) = interp1( ...
        frequency_grid_hz, full_voltage_v, mapped_frequency_hz, 'linear');
    operating_frequency_hz = interp1( ...
        full_voltage_v, frequency_grid_hz, reference_voltage_v, 'linear');
    curve.output_voltage_v = output_voltage_v;
    curve.transition_u = transition_u;
    curve.operating_frequency_hz = operating_frequency_hz;
    curve.input_voltage_v = Vin_v;
end

function voltage_target_v = interpolate_full_range_target( ...
    command, reference_voltage_v, bus_voltage_v, ...
    minimum_voltage_v, nominal_voltage_v, maximum_voltage_v, ...
    minimum_curve, nominal_curve, maximum_curve, command_grid)
%INTERPOLATE_FULL_RANGE_TARGET 插值 24~72 V 包络内的 FHA 静态目标电压。
% 三条端点/中点曲线对应不同负载和母线。先除以各自建模母线得到无量纲
% 增益，再按参考电压在相邻工作点间线性插值，最后乘当前动态母线电压。

    minimum_gain = interp1(command_grid, ...
        minimum_curve.output_voltage_v/minimum_curve.input_voltage_v, ...
        command, 'linear');
    nominal_gain = interp1(command_grid, ...
        nominal_curve.output_voltage_v/nominal_curve.input_voltage_v, ...
        command, 'linear');
    maximum_gain = interp1(command_grid, ...
        maximum_curve.output_voltage_v/maximum_curve.input_voltage_v, ...
        command, 'linear');

    if reference_voltage_v <= nominal_voltage_v
        blend = (reference_voltage_v - minimum_voltage_v)/ ...
            (nominal_voltage_v - minimum_voltage_v);
        gain = minimum_gain + blend*(nominal_gain - minimum_gain);
    else
        blend = (reference_voltage_v - nominal_voltage_v)/ ...
            (maximum_voltage_v - nominal_voltage_v);
        gain = nominal_gain + blend*(maximum_gain - nominal_gain);
    end
    voltage_target_v = bus_voltage_v*gain;
end

function output_voltage_v = calculate_forward_fha_voltage( ...
    frequency_hz, Vin_v, Rac_ohm, Lrp, Crp, Lmp, Lrs, Crs, n)
%CALCULATE_FORWARD_FHA_VOLTAGE 计算正向满相移 FHA 输出电压。
% 使用复阻抗描述谐振网络。./ 和 .* 对每一个频率点逐元素计算，因此
% 一次函数调用即可得到整条频率曲线。abs 取电压传递函数幅值。

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
    output_voltage_v = Vin_v*abs(Hprimary.*Hsecondary/n);
end

function [switching_frequency_hz, duty] = hybrid_modulator( ...
    command, transition_u, fr_hz, fmax_hz)
%HYBRID_MODULATOR 将单一 0~1 指令映射到 PSM/PFM。
% u<ut：频率固定 fmax，占空比 0~50%；u>=ut：占空比固定 50%，
% 频率由 fmax 线性降至 fr。两段在 u=ut 处拥有相同频率和占空比，
% 因此执行量映射连续，不需要额外迟滞来修复静态增益跳变。

    command = min(max(command, 0), 1);
    if command < transition_u
        switching_frequency_hz = fmax_hz;
        duty = 0.5*command/transition_u;
    else
        duty = 0.5;
        switching_frequency_hz = fmax_hz - ...
            (command - transition_u)/(1 - transition_u)*(fmax_hz - fr_hz);
    end
end

function metrics = calculate_reference_step_metrics( ...
    time_s, response, event_s, window_end_s, initial_value, final_value)
%CALCULATE_REFERENCE_STEP_METRICS 计算参考阶跃指标。
% progress 把正向或反向阶跃统一归一化为 0~1，再寻找 10% 和 90% 时刻；
% 稳定时间由 find_settling_time 从后向前寻找最后一次越出误差带的位置。

    window = time_s >= event_s & time_s < window_end_s;
    event_time = time_s(window);
    event_response = response(window);
    step_size = final_value - initial_value;
    progress = sign(step_size)*(event_response - initial_value)/abs(step_size);
    index_10 = find(progress >= 0.1, 1, 'first');
    index_90 = find(progress >= 0.9, 1, 'first');
    if isempty(index_10) || isempty(index_90)
        rise_time_s = NaN;
    else
        rise_time_s = event_time(index_90) - event_time(index_10);
    end
    settling_time_s = find_settling_time( ...
        event_time, event_response, final_value, ...
        0.02*abs(step_size), event_s);
    overshoot_v = max(sign(step_size)*(event_response - final_value));
    metrics.rise_time_s = rise_time_s;
    metrics.settling_time_s = settling_time_s;
    metrics.overshoot_v = max(0, overshoot_v);
end

function settling_time_s = find_settling_time( ...
    event_time_s, response, final_value, tolerance, event_s)
%FIND_SETTLING_TIME 查找进入误差带后不再离开的时间。
% 不能只找第一次进入误差带，因为欠阻尼响应可能随后再次越界。
% 本函数找“最后一个越界点”的下一点；若窗口末端仍越界，则返回 NaN，
% 提醒调用者延长观察窗口或重新调整控制器。

    outside = abs(response - final_value) > tolerance;
    last_outside = find(outside, 1, 'last');
    if isempty(last_outside)
        settling_time_s = 0;
    elseif last_outside == numel(response)
        settling_time_s = NaN;
    else
        settling_time_s = event_time_s(last_outside + 1) - event_s;
    end
end
