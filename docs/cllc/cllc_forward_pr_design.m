%% CLLC 正向 100 Hz 输出电压纹波 PR 控制器设计
% 本脚本只进行频域设计和离散系数验证，不进行时域仿真。
%
% 控制结构：
%
%   u_final = sat_0_1(u_dual_pi + u_pr)
%
%   e_pr = 0 - (vout - vdc_command)
%
% 其中 vout 是输出电压反馈，vdc_command 是缓升后的直流电压目标。
% 两者相减后，PR 主要处理输出电压中的偏差和 100 Hz 纹波。
%
% 不能直接把包含 24~72 V 直流量的 vout 送入 PR：
% 直流误差会立即触发下限饱和，并可能使抗饱和逻辑长期冻结。
%
% 非理想 PR 传递函数：
%
%                   2*Kr*wc*s
%   Gpr(s) = Kp + ---------------------
%                 s^2+2*wc*s+w0^2
%
% 本设计取 Kp=0，因此 Gpr(0)=0，不改变原双 PI 的直流工作点。

clear;
clc;
close all;

%% 1. 正向被控对象参数
% 与 cllc_cfg.h 的正向一阶平均模型保持一致：
%
%                  Kv
%   Gp(s) = ----------------
%             tau*s + 1
%
%   Rload = Vout^2/Pout
%   tau   = 0.5*Rload*Cout

output_voltage_v = 48.0;
output_power_w = 6600.0;
output_capacitance_f = 10e-3;
plant_dc_gain_v_per_u = 98.26;

load_resistance_ohm = output_voltage_v^2/output_power_w;
plant_time_constant_s = ...
    0.5*load_resistance_ohm*output_capacitance_f;

s = tf('s');
plant = plant_dc_gain_v_per_u/(plant_time_constant_s*s + 1);

%% 2. PR 设计指标
% 100 Hz 是需要抑制的输出电压纹波频率。
% 非理想 PR 的带宽设为 5 Hz，用于降低频率偏差和参数误差敏感度。
% 目标是在 100 Hz 处获得约 26 dB，即 20 倍的 PR 开环增益。

control_frequency_hz = 30e3;
control_period_s = 1/control_frequency_hz;
resonant_frequency_hz = 100.0;
resonant_bandwidth_hz = 5.0;
target_loop_gain = 20.0;
target_loop_gain_db = 20*log10(target_loop_gain);

w0_rad_per_s = 2*pi*resonant_frequency_hz;
wc_rad_per_s = 2*pi*resonant_bandwidth_hz;

%% 3. 根据 100 Hz 对象增益计算 Kr
% 一阶对象在 w0 处的幅值为：
%
%                         Kv
%   |Gp(j*w0)| = ----------------------
%                 sqrt(1+(w0*tau)^2)
%
% 非理想 PR 的谐振支路在 w0 处幅值恰好为 Kr，因此：
%
%   |Lpr(j*w0)| = Kr*|Gp(j*w0)|
%
%   Kr = target_loop_gain/|Gp(j*w0)|

plant_magnitude_at_100hz = plant_dc_gain_v_per_u/...
    sqrt(1+(w0_rad_per_s*plant_time_constant_s)^2);

pr_kp = 0.0;
pr_kr = target_loop_gain/plant_magnitude_at_100hz;

pr_continuous = pr_kp + ...
    (2*pr_kr*wc_rad_per_s*s)/...
    (s^2 + 2*wc_rad_per_s*s + w0_rad_per_s^2);

pr_open_loop = minreal(pr_continuous*plant);
pr_sensitivity = feedback(1, pr_open_loop);

%% 4. 使用与 code/lib/pr.c 相同的 Tustin 公式离散化
% pr.c 使用下列差分方程：
%
%   u[k] = b0*e[k] + b1*e[k-1] + b2*e[k-2]
%          - a1*u[k-1] - a2*u[k-2]
%
% 下面直接复现 pr_update_freq() 的系数公式，便于 MATLAB 结果与 C 代码
% 一一对应。

ts = control_period_s;
kp = pr_kp;
kr = pr_kr;
w0 = w0_rad_per_s;
wc = wc_rad_per_s;

n0 = ts^2*kp*w0^2 + 4*ts*kp*wc + 4*ts*kr*wc + 4*kp;
n1 = 2*ts^2*kp*w0^2 - 8*kp;
n2 = ts^2*kp*w0^2 - 4*ts*kp*wc - 4*ts*kr*wc + 4*kp;

d0 = ts^2*w0^2 + 4*ts*wc + 4;
d1 = 2*ts^2*w0^2 - 8;
d2 = ts^2*w0^2 - 4*ts*wc + 4;

pr_b0 = n0/d0;
pr_b1 = n1/d0;
pr_b2 = n2/d0;
pr_a1 = d1/d0;
pr_a2 = d2/d0;

pr_discrete_manual = tf(...
    [pr_b0 pr_b1 pr_b2], ...
    [1 pr_a1 pr_a2], ...
    control_period_s, ...
    'Variable', 'z^-1');

% 用 Control System Toolbox 的 c2d 结果进行交叉检查。
pr_discrete_c2d = c2d(pr_continuous, control_period_s, 'tustin');

%% 5. 频域验证
frequency_hz = logspace(0, 4, 2400);
omega_rad_per_s = 2*pi*frequency_hz;

plant_response = squeeze(freqresp(plant, omega_rad_per_s));
pr_response = squeeze(freqresp(pr_continuous, omega_rad_per_s));
loop_response = squeeze(freqresp(pr_open_loop, omega_rad_per_s));
sensitivity_response = squeeze(freqresp(pr_sensitivity, omega_rad_per_s));

plant_at_100hz = squeeze(freqresp(plant, w0_rad_per_s));
pr_at_100hz = squeeze(freqresp(pr_continuous, w0_rad_per_s));
loop_at_100hz = plant_at_100hz*pr_at_100hz;
sensitivity_at_100hz = 1/(1+loop_at_100hz);

figure('Name', 'CLLC forward 100 Hz PR frequency design', ...
       'Color', 'w');

subplot(2, 1, 1);
semilogx(frequency_hz, 20*log10(abs(plant_response)), ...
    'LineWidth', 1.3);
hold on;
semilogx(frequency_hz, 20*log10(abs(pr_response)), ...
    'LineWidth', 1.3);
semilogx(frequency_hz, 20*log10(abs(loop_response)), ...
    'LineWidth', 1.5);
xline(resonant_frequency_hz, '--k', '100 Hz');
yline(0, ':k');
grid on;
xlabel('Frequency / Hz');
ylabel('Magnitude / dB');
title('Plant, PR controller and PR open-loop magnitude');
legend('G_p', 'G_{PR}', 'G_{PR}G_p', 'Location', 'best');

subplot(2, 1, 2);
semilogx(frequency_hz, 20*log10(abs(sensitivity_response)), ...
    'LineWidth', 1.5);
xline(resonant_frequency_hz, '--k', '100 Hz');
yline(0, ':k');
grid on;
xlabel('Frequency / Hz');
ylabel('|S| / dB');
title('Linearized output-voltage disturbance sensitivity');

%% 6. 输出设计结果
fprintf('\nCLLC forward 100 Hz PR design\n');
fprintf('---------------------------------------------\n');
fprintf('Control frequency             = %.3f Hz\n', ...
    control_frequency_hz);
fprintf('Control period                = %.12g s\n', ...
    control_period_s);
fprintf('Resonant frequency            = %.3f Hz\n', ...
    resonant_frequency_hz);
fprintf('Resonant bandwidth            = %.3f Hz\n', ...
    resonant_bandwidth_hz);
fprintf('Plant magnitude at 100 Hz     = %.9f V/u\n', ...
    abs(plant_at_100hz));
fprintf('PR Kp                         = %.12g\n', pr_kp);
fprintf('PR Kr                         = %.12g u/V\n', pr_kr);
fprintf('PR loop magnitude at 100 Hz   = %.6f dB\n', ...
    20*log10(abs(loop_at_100hz)));
fprintf('Sensitivity at 100 Hz         = %.6f dB\n', ...
    20*log10(abs(sensitivity_at_100hz)));
fprintf('\nCoefficients used by code/lib/pr.c\n');
fprintf('a1 = %.12g\n', pr_a1);
fprintf('a2 = %.12g\n', pr_a2);
fprintf('b0 = %.12g\n', pr_b0);
fprintf('b1 = %.12g\n', pr_b1);
fprintf('b2 = %.12g\n', pr_b2);
fprintf('\nNonlinear limits in C code\n');
fprintf('u_pr    = sat(pr_output, -0.5, 0.5)\n');
fprintf('u_final = sat(u_dual_pi + u_pr, 0, 1)\n');
fprintf(['Note: PR is a bipolar ripple correction around the PI ', ...
         'operating point; only the final modulation command is ', ...
         'limited to 0...1.\n']);

%% 7. 保存便于人工核对的结果结构体
pr_design_result = struct();
pr_design_result.control_frequency_hz = control_frequency_hz;
pr_design_result.control_period_s = control_period_s;
pr_design_result.resonant_frequency_hz = resonant_frequency_hz;
pr_design_result.resonant_bandwidth_hz = resonant_bandwidth_hz;
pr_design_result.kp = pr_kp;
pr_design_result.kr = pr_kr;
pr_design_result.w0_rad_per_s = w0_rad_per_s;
pr_design_result.wc_rad_per_s = wc_rad_per_s;
pr_design_result.a1 = pr_a1;
pr_design_result.a2 = pr_a2;
pr_design_result.b0 = pr_b0;
pr_design_result.b1 = pr_b1;
pr_design_result.b2 = pr_b2;
pr_design_result.plant = plant;
pr_design_result.pr_continuous = pr_continuous;
pr_design_result.pr_discrete_manual = pr_discrete_manual;
pr_design_result.pr_discrete_c2d = pr_discrete_c2d;
pr_design_result.open_loop = pr_open_loop;
pr_design_result.sensitivity = pr_sensitivity;
