%% CLLC 正向 PI 与 PI+PR 的 100 Hz 母线纹波时域对比
% 本脚本只验证稳定工作点的纹波抑制，不包含软启动、参考值阶跃、
% 负载阶跃、限流切换和正反向切换。
%
% 测试条件：
%   Vbus(t) = Vbus_dc + 10*sin(2*pi*100*t) V
%   Vout_ref = 48 V，Pout = 6600 W，Cout = 10 mF
%
% 对比对象：
%   1) 原正向双 PI 竞争控制器（稳态时电压环获胜）
%   2) 双 PI + 100 Hz PR：u = sat(u_pi + u_pr, 0, 1)
%
% 为了和嵌入式代码一致，脚本复现：
%   - code/lib/pi_dual_compete.c 的共积分、MIN 竞争和抗饱和；
%   - code/lib/pr.c 的 Tustin 差分方程和冻结式抗饱和结构。
%
% 关键修正：PR 是叠加在 PI 工作点上的交流补偿量，必须能以 0 为中心
% 输出正、负修正量。只有最终调制指令需要限制在 0~1：
%
%   -0.5 <= u_pr <= 0.5
%   u_final = sat(u_pi + u_pr, 0, 1)

clear;
clc;
close all;

%% 1. 稳态工作点和平均模型
parameter.control_frequency_hz = 30e3;
parameter.control_period_s = 1/parameter.control_frequency_hz;
parameter.simulation_time_s = 0.30;

parameter.output_reference_v = 48.0;
parameter.output_power_w = 6600.0;
parameter.output_capacitance_f = 10e-3;
parameter.load_resistance_ohm = ...
    parameter.output_reference_v^2/parameter.output_power_w;
parameter.output_time_constant_s = ...
    0.5*parameter.load_resistance_ohm*parameter.output_capacitance_f;

% 正向对象采用现有 PI/PR 设计使用的一阶平均模型：
%
%                      Ku
%   G_u(s) = ------------------------
%                1 + tau_o*s
%
% 母线扰动到输出电压的静态增益按稳态变比近似：
%
%   Kbus = Vout_nom/Vbus_nom
%   G_bus(s) = Kbus/(1 + tau_o*s)
parameter.control_gain_v_per_u = 98.26;
parameter.bus_dc_voltage_v = max(...
    370.0, 8.0*parameter.output_reference_v + 25.0);
parameter.bus_disturbance_gain_v_per_v = ...
    parameter.output_reference_v/parameter.bus_dc_voltage_v;

% u0 只是线性化平均模型的稳态偏置。控制器计算的是绝对 0~1 指令，
% 对象使用 u-u0 计算工作点附近的电压变化。
parameter.nominal_normalized_output = 0.50;
parameter.output_lower_limit = 0.0;
parameter.output_upper_limit = 1.0;

% 稳态 PI 工作点取 0.5，因此 PR 采用对称的 +/-0.5 最大补偿范围。
% 正常纹波抑制时实际 PR 输出远小于该限值，最终 0~1 限幅负责调制安全。
parameter.pr_output_lower_limit = -0.5;
parameter.pr_output_upper_limit = 0.5;

%% 2. 双 PI 共积分参数
% PI 参数与 cllc_forward_dual_pi_design.m / cllc_cfg.h 相同。
% 连续域 PI：C(s) = Kp + Ki/s；嵌入式积分步长为 Ki_step=Ki*Ts。
parameter.voltage_pi_kp = 0.0390560696419;
parameter.voltage_pi_ki_per_s = 22.3758732323;
parameter.current_pi_kp = 0.136341056236;
parameter.current_pi_ki_per_s = 78.1120634685;
parameter.voltage_pi_ki_step = ...
    parameter.voltage_pi_ki_per_s*parameter.control_period_s;
parameter.current_pi_ki_step = ...
    parameter.current_pi_ki_per_s*parameter.control_period_s;

% 150 A 为额定电流，控制代码使用 110% 限流值。稳定额定工况下，
% 负载电流为 137.5 A，因此电流环不应抢占电压环。
parameter.rated_current_a = 150.0;
parameter.current_limit_a = 1.10*parameter.rated_current_a;

%% 3. 100 Hz 非理想 PR 参数和离散系数
%                  2*Kr*wc*s
% Gpr(s) = Kp + ---------------------
%                s^2+2*wc*s+w0^2
parameter.pr_kp = 0.0;
parameter.pr_target_loop_gain = 20.0;
parameter.pr_frequency_hz = 100.0;
parameter.pr_bandwidth_hz = 5.0;
parameter.pr_w0_rad_per_s = 2*pi*parameter.pr_frequency_hz;
parameter.pr_wc_rad_per_s = 2*pi*parameter.pr_bandwidth_hz;

% 根据目标 100 Hz 开环增益计算 Kr，与 cllc_forward_pr_design.m 和
% CLLC_CTRL_FORWARD_PR_TARGET_LOOP_GAIN 使用同一个设计值。
parameter.plant_magnitude_at_pr_frequency_v_per_u = ...
    parameter.control_gain_v_per_u/sqrt(1 + ...
    (parameter.pr_w0_rad_per_s*parameter.output_time_constant_s)^2);
parameter.pr_kr = parameter.pr_target_loop_gain/...
    parameter.plant_magnitude_at_pr_frequency_v_per_u;

% 直接使用 code/lib/pr.c 中 pr_update_freq() 的 Tustin 公式。
ts = parameter.control_period_s;
kp = parameter.pr_kp;
kr = parameter.pr_kr;
w0 = parameter.pr_w0_rad_per_s;
wc = parameter.pr_wc_rad_per_s;

n0 = ts^2*kp*w0^2 + 4*ts*kp*wc + 4*ts*kr*wc + 4*kp;
n1 = 2*ts^2*kp*w0^2 - 8*kp;
n2 = ts^2*kp*w0^2 - 4*ts*kp*wc - 4*ts*kr*wc + 4*kp;
d0 = ts^2*w0^2 + 4*ts*wc + 4;
d1 = 2*ts^2*w0^2 - 8;
d2 = ts^2*w0^2 - 4*ts*wc + 4;

parameter.pr_b0 = n0/d0;
parameter.pr_b1 = n1/d0;
parameter.pr_b2 = n2/d0;
parameter.pr_a1 = d1/d0;
parameter.pr_a2 = d2/d0;

%% 4. 构造连续的稳态母线纹波并运行两组仿真
time_s = (0:parameter.control_period_s:parameter.simulation_time_s).';
bus_voltage_v = parameter.bus_dc_voltage_v + ...
    10.0*sin(2*pi*parameter.pr_frequency_hz*time_s);

pi_only = simulate_forward_ripple(false, parameter, time_s, bus_voltage_v);
pi_plus_pr = simulate_forward_ripple(true, parameter, time_s, bus_voltage_v);

%% 5. 只统计末尾 10 个完整纹波周期
measurement_cycle_count = 10;
measurement_duration_s = ...
    measurement_cycle_count/parameter.pr_frequency_hz;
measurement_index = time_s >= ...
    (parameter.simulation_time_s - measurement_duration_s);

pi_only_ac_v = pi_only.output_voltage_v(measurement_index) - ...
    mean(pi_only.output_voltage_v(measurement_index));
pi_plus_pr_ac_v = pi_plus_pr.output_voltage_v(measurement_index) - ...
    mean(pi_plus_pr.output_voltage_v(measurement_index));

pi_only_ripple_pp_v = max(pi_only_ac_v) - min(pi_only_ac_v);
pi_plus_pr_ripple_pp_v = max(pi_plus_pr_ac_v) - min(pi_plus_pr_ac_v);
pi_only_ripple_rms_v = sqrt(mean(pi_only_ac_v.^2));
pi_plus_pr_ripple_rms_v = sqrt(mean(pi_plus_pr_ac_v.^2));

ripple_ratio = pi_plus_pr_ripple_pp_v/pi_only_ripple_pp_v;
ripple_change_percent = 100*(1-ripple_ratio);
ripple_change_db = 20*log10(ripple_ratio);

%% 6. 在同一个 figure 中显示完整稳态测试
figure('Name', 'CLLC forward PI+PR 100 Hz ripple suppression', ...
       'Color', 'w');
tiledlayout(4, 1, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile;
plot(time_s*1e3, bus_voltage_v, 'k', 'LineWidth', 1.1);
grid on;
ylabel('V_{bus} / V');
title('Stable operating point: 409 Vdc + 10 Vpk, 100 Hz bus ripple');

nexttile;
plot(time_s*1e3, pi_only.output_voltage_v, ...
    'LineWidth', 1.1);
hold on;
plot(time_s*1e3, pi_plus_pr.output_voltage_v, ...
    'LineWidth', 1.1);
yline(parameter.output_reference_v, ':k', '48 V reference');
grid on;
ylabel('V_{out} / V');
legend('PI only', 'PI + PR', 'Location', 'best');
title('Output voltage');

nexttile;
plot(time_s(measurement_index)*1e3, pi_only_ac_v, ...
    'LineWidth', 1.2);
hold on;
plot(time_s(measurement_index)*1e3, pi_plus_pr_ac_v, ...
    'LineWidth', 1.2);
grid on;
ylabel('Ripple / V');
legend('PI only', 'PI + PR', 'Location', 'best');
title(sprintf(['Last %d cycles: PI %.4f Vpp, PI+PR %.4f Vpp, ', ...
    'change %+.2f dB'], measurement_cycle_count, ...
    pi_only_ripple_pp_v, pi_plus_pr_ripple_pp_v, ripple_change_db));

nexttile;
plot(time_s*1e3, pi_plus_pr.pi_output, ...
    'LineWidth', 1.0);
hold on;
plot(time_s*1e3, pi_plus_pr.pr_output, ...
    'LineWidth', 1.0);
plot(time_s*1e3, pi_plus_pr.final_output, ...
    'LineWidth', 1.2);
grid on;
xlabel('Time / ms');
ylabel('Normalized u');
legend('u_{PI}', 'u_{PR} (-0.5...0.5)', 'sat(u_{PI}+u_{PR})', ...
    'Location', 'best');
title('Controller outputs in the PI+PR case');

%% 7. 打印数值结果并保存在工作区
fprintf('\nCLLC forward stable 100 Hz ripple time-domain test\n');
fprintf('--------------------------------------------------\n');
fprintf('Bus voltage                  = %.3f Vdc + %.3f Vpk @ %.1f Hz\n', ...
    parameter.bus_dc_voltage_v, 10.0, parameter.pr_frequency_hz);
fprintf('PI-only output ripple        = %.9f Vpp, %.9f Vrms\n', ...
    pi_only_ripple_pp_v, pi_only_ripple_rms_v);
fprintf('PI+PR output ripple          = %.9f Vpp, %.9f Vrms\n', ...
    pi_plus_pr_ripple_pp_v, pi_plus_pr_ripple_rms_v);
fprintf('PI+PR / PI ripple ratio      = %.9f\n', ripple_ratio);
fprintf('Ripple reduction             = %.6f %%\n', ripple_change_percent);
fprintf('Ripple change                = %+.6f dB\n', ripple_change_db);
fprintf('PR output range              = %.9f ... %.9f\n', ...
    min(pi_plus_pr.pr_output(measurement_index)), ...
    max(pi_plus_pr.pr_output(measurement_index)));
fprintf('PR saturation sample ratio   = %.3f %%\n', ...
    100*mean((pi_plus_pr.pr_output(measurement_index) <= ...
              parameter.pr_output_lower_limit + 1e-12) | ...
             (pi_plus_pr.pr_output(measurement_index) >= ...
              parameter.pr_output_upper_limit - 1e-12)));
fprintf('Voltage-loop active ratio    = %.3f %%\n', ...
    100*mean(pi_plus_pr.active_channel(measurement_index) == 1));

if ripple_change_percent > 0
    fprintf('Conclusion: under this nonlinear implementation, PR reduces ripple.\n');
else
    fprintf(['Conclusion: PR does not reduce ripple with the present ', ...
        'parameters; inspect polarity, phase and plant parameters.\n']);
end

ripple_test_result = struct();
ripple_test_result.parameter = parameter;
ripple_test_result.time_s = time_s;
ripple_test_result.bus_voltage_v = bus_voltage_v;
ripple_test_result.pi_only = pi_only;
ripple_test_result.pi_plus_pr = pi_plus_pr;
ripple_test_result.measurement_index = measurement_index;
ripple_test_result.pi_only_ripple_pp_v = pi_only_ripple_pp_v;
ripple_test_result.pi_plus_pr_ripple_pp_v = pi_plus_pr_ripple_pp_v;
ripple_test_result.ripple_reduction_percent = ripple_change_percent;
ripple_test_result.ripple_change_db = ripple_change_db;

%% 局部函数：复现双 PI 共积分和 PR 差分控制器
function simulation = simulate_forward_ripple(...
    enable_pr, parameter, time_s, bus_voltage_v)

sample_count = numel(time_s);
output_voltage_v = zeros(sample_count, 1);
load_current_a = zeros(sample_count, 1);
pi_output = zeros(sample_count, 1);
pr_output = zeros(sample_count, 1);
final_output = zeros(sample_count, 1);
active_channel = zeros(sample_count, 1);

% 从稳定工作点直接开始，不模拟启动过程。
output_voltage_v(1) = parameter.output_reference_v;
shared_integral = parameter.nominal_normalized_output;

% code/lib/pr.c 保存 e[k-1], e[k-2] 和 u[k-1], u[k-2]。
pr_error_1 = 0.0;
pr_error_2 = 0.0;
pr_output_1 = 0.0;
pr_output_2 = 0.0;

for sample_index = 1:(sample_count-1)
    load_current_a(sample_index) = ...
        output_voltage_v(sample_index)/parameter.load_resistance_ohm;

    % 两路 PI 共用同一个积分项，MIN 模式取较小输出。
    voltage_error_v = parameter.output_reference_v - ...
        output_voltage_v(sample_index);
    current_error_a = parameter.current_limit_a - ...
        load_current_a(sample_index);
    voltage_p = parameter.voltage_pi_kp*voltage_error_v;
    current_p = parameter.current_pi_kp*current_error_a;
    voltage_candidate = voltage_p + shared_integral;
    current_candidate = current_p + shared_integral;

    if voltage_candidate <= current_candidate
        active_channel(sample_index) = 1; % 1 = 电压环
        active_error = voltage_error_v;
        active_ki_step = parameter.voltage_pi_ki_step;
        active_p = voltage_p;
    else
        active_channel(sample_index) = 2; % 2 = 电流环
        active_error = current_error_a;
        active_ki_step = parameter.current_pi_ki_step;
        active_p = current_p;
    end

    shared_integral = shared_integral + active_ki_step*active_error;
    pi_raw = active_p + shared_integral;
    pi_output(sample_index) = min(max(pi_raw, ...
        parameter.output_lower_limit), parameter.output_upper_limit);

    % 与 pi_dual_compete.c 一样，饱和时反算共积分项。
    if pi_output(sample_index) ~= pi_raw
        shared_integral = pi_output(sample_index) - active_p;
    end
    shared_integral = min(max(shared_integral, ...
        parameter.output_lower_limit-active_p), ...
        parameter.output_upper_limit-active_p);

    if enable_pr
        % PR 给定为 0，反馈为 Vout-Vref，因此 e_pr=Vref-Vout。
        pr_error_0 = parameter.output_reference_v - ...
            output_voltage_v(sample_index);
        pr_raw = parameter.pr_b0*pr_error_0 + ...
            parameter.pr_b1*pr_error_1 + ...
            parameter.pr_b2*pr_error_2 - ...
            parameter.pr_a1*pr_output_1 - ...
            parameter.pr_a2*pr_output_2;
        pr_sat = min(max(pr_raw, parameter.pr_output_lower_limit), ...
            parameter.pr_output_upper_limit);

        % 与 pr.c 一致：只有误差会把饱和推得更深时才冻结状态。
        hold_pr_state = ...
            ((pr_raw > parameter.pr_output_upper_limit) && (pr_error_0 > 0)) || ...
            ((pr_raw < parameter.pr_output_lower_limit) && (pr_error_0 < 0));
        if ~hold_pr_state
            pr_error_2 = pr_error_1;
            pr_error_1 = pr_error_0;
            pr_output_2 = pr_output_1;
            pr_output_1 = pr_sat;
        end
        pr_output(sample_index) = pr_sat;
    end

    final_output(sample_index) = min(max(...
        pi_output(sample_index) + pr_output(sample_index), ...
        parameter.output_lower_limit), parameter.output_upper_limit);

    % 一阶平均对象同时接受调制扰动和母线电压扰动。
    target_voltage_v = parameter.output_reference_v + ...
        parameter.control_gain_v_per_u*...
            (final_output(sample_index) - ...
             parameter.nominal_normalized_output) + ...
        parameter.bus_disturbance_gain_v_per_v*...
            (bus_voltage_v(sample_index) - parameter.bus_dc_voltage_v);
    output_voltage_v(sample_index+1) = output_voltage_v(sample_index) + ...
        parameter.control_period_s/parameter.output_time_constant_s*...
        (target_voltage_v - output_voltage_v(sample_index));
end

% 末点仅用于绘图和统计，沿用最后一次控制输出。
load_current_a(end) = output_voltage_v(end)/parameter.load_resistance_ohm;
pi_output(end) = pi_output(end-1);
pr_output(end) = pr_output(end-1);
final_output(end) = final_output(end-1);
active_channel(end) = active_channel(end-1);

simulation = struct();
simulation.output_voltage_v = output_voltage_v;
simulation.load_current_a = load_current_a;
simulation.pi_output = pi_output;
simulation.pr_output = pr_output;
simulation.final_output = final_output;
simulation.active_channel = active_channel;
end
