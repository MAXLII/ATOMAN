%% CLLC 反向单母线电压环 PI 设计与频域验证
% 电池端放电，经 CLLC 向高压母线送能。控制结构只有一个电压环：
%
%   Vbus_ref - Vbus  -->  PI  -->  u(0~1)  -->  CLLC  -->  Vbus
%
% 本脚本依次完成：反向 FHA 建模、可达范围检查、工作点线性化、PI 参数
% 解析计算、数字控制器波特图验证。它是平均模型的原理性设计；最后仍需
% 用 PLECS 开关模型核对谐振电流、软开关范围、延时和器件应力。
%
% 反向混合调制定义（u 增大表示传输功率增大）：
%   0 <= u <= ut：固定 fs=fr，相移等效占空比从 0 增至 50%；
%   ut <  u <= 1：固定 50% 占空比，fs 从 fr 降至 fmin。
% 两段在 ut 处连续。设计点位于低于谐振频率的 PFM 单调支路。
%
% PI 公式：C(s)=Kp+Ki/s=Kp*(1+wz/s)
%   Kp = 1/(|G(jwc)|*sqrt(1+(wz/wc)^2))
%   Ki = wz*Kp
% 数字程序若每次执行 i[n]=i[n-1]+ki_step*e[n]，则
%   ki_step = Ki*Ts

clear;
clc;
close all;

%% 1. 用户可调规格
battery_voltage_v = 48;          % 电池端标称电压
bus_voltage_range_v = [400, 500];
bus_voltage_reference_v = 450;   % PI 线性化点
design_power_w = 3000;           % 该点可覆盖 400~500 V 的标称设计功率
maximum_rated_power_w = 6600;    % 用于可达能力扫描，不代表 500 V 可满功率
Cbus_f = 1360e-6;                % 高压母线电容

voltage_crossover_hz = 200;
control_update_frequency_hz = 100e3;
delay_cycles = 1;

% 图示 CLLC 网络参数，n=Np/Ns=8。反向计算时低压侧为输入，高压侧为输出。
Lr_primary_h = 40e-6;
Cr_primary_f = 80e-9;
Lm_primary_h = 200e-6;
Lr_secondary_h = 0.625e-6;
Cr_secondary_f = 5.12e-6;
turns_ratio = 24/3;

%% 2. 反向 FHA 与调制范围
fr_hz = 1/(2*pi*sqrt(Lr_primary_h*Cr_primary_f));
frequency_scan_hz = linspace(20e3, fr_hz, 12001).';
design_load_ohm = bus_voltage_reference_v^2/design_power_w;

full_bridge_voltage_v = calculate_reverse_fha_voltage( ...
    frequency_scan_hz, battery_voltage_v, design_load_ohm, ...
    Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);

% 先找本负载的谐振峰，再把最低频率放在峰值右侧约 3%，避免进入
% “继续降频反而降增益”的非单调区域。32 kHz 下限同时保留少量裕量。
[~, peak_index] = max(full_bridge_voltage_v);
peak_frequency_hz = frequency_scan_hz(peak_index);
fmin_hz = max(32e3, 1.03*peak_frequency_hz);
if fmin_hz >= fr_hz
    error('自动得到的最低频率不低于谐振频率。');
end

voltage_at_fr_v = interp1(frequency_scan_hz, full_bridge_voltage_v, fr_hz);
voltage_at_fmin_v = interp1(frequency_scan_hz, full_bridge_voltage_v, fmin_hz);
transition_u = voltage_at_fr_v/voltage_at_fmin_v;
if transition_u <= 0 || transition_u >= 1
    error('反向混合调制过渡点不在 0~1 内，请检查频率范围。');
end

operating_frequency_hz = find_high_frequency_crossing( ...
    frequency_scan_hz, full_bridge_voltage_v, bus_voltage_reference_v);
if ~isfinite(operating_frequency_hz) || operating_frequency_hz < fmin_hz
    error('设计负载下找不到位于 fmin~fr 单调支路的 450 V 工作点。');
end
operating_u = transition_u + (fr_hz-operating_frequency_hz) / ...
    (fr_hz-fmin_hz)*(1-transition_u);

%% 3. 工作点小信号对象 Gv(s)=DeltaVbus/Delta u
% dV/du=(dV/dfs)*(dfs/du)。在所选反向支路两项都为负，因此 dV/du>0。
voltage_slope_v_per_hz = gradient(full_bridge_voltage_v, frequency_scan_hz);
Kvf_v_per_hz = interp1(frequency_scan_hz, voltage_slope_v_per_hz, ...
    operating_frequency_hz, 'linear');
frequency_gain_hz_per_u = -(fr_hz-fmin_hz)/(1-transition_u);
Ku_v_per_u = Kvf_v_per_hz*frequency_gain_hz_per_u;
if Ku_v_per_u <= 0
    error('dVbus/du 不为正，当前频率支路会使负反馈方向错误。');
end

% 用输出包络的一阶模型近似母线动态。tau=R*C/2 与 FHA 整流平均模型
% 配套；exp(-sTd) 表示采样、计算及调制更新的总延时。
output_time_constant_s = design_load_ohm*Cbus_f/2;
output_pole_hz = 1/(2*pi*output_time_constant_s);
delay_s = delay_cycles/operating_frequency_hz;
sample_time_s = 1/control_update_frequency_hz;

frequency_hz = logspace(-1, log10(0.45*control_update_frequency_hz), 6001).';
s = 1j*2*pi*frequency_hz;
voltage_plant = Ku_v_per_u./(1+s*output_time_constant_s).*exp(-s*delay_s);

%% 4. PI 参数解析设计及离散环路复核
voltage_zero_hz = output_pole_hz;
wc = 2*pi*voltage_crossover_hz;
wz = 2*pi*voltage_zero_hz;
plant_at_crossover = interp1(frequency_hz, voltage_plant, ...
    voltage_crossover_hz, 'linear');
voltage_Kp = 1/(abs(plant_at_crossover)*sqrt(1+(wz/wc)^2));
voltage_Ki = wz*voltage_Kp;
voltage_ki_step = voltage_Ki*sample_time_s;

% 精确采用嵌入式常见的后向矩形积分器：Ki*Ts/(1-z^-1)。
z = exp(s*sample_time_s);
digital_controller = voltage_Kp + voltage_ki_step./(1-1./z);
open_loop = digital_controller.*voltage_plant;
closed_loop = open_loop./(1+open_loop);
[actual_crossover_hz, phase_margin_deg, gain_margin_db] = ...
    analyze_loop_margins(frequency_hz, open_loop);

%% 5. 400~500 V 与 100~6600 W 可达范围
power_scan_w = (100:50:maximum_rated_power_w).';
reference_points_v = (400:10:500).';
reachable = false(numel(power_scan_w), numel(reference_points_v));
for voltage_index = 1:numel(reference_points_v)
    for power_index = 1:numel(power_scan_w)
        load_ohm = reference_points_v(voltage_index)^2/power_scan_w(power_index);
        curve_v = calculate_reverse_fha_voltage( ...
            frequency_scan_hz, battery_voltage_v, load_ohm, ...
            Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
            Lr_secondary_h, Cr_secondary_f, turns_ratio);
        controlled_curve_v = curve_v(frequency_scan_hz >= fmin_hz);
        maximum_controlled_voltage_v = max(controlled_curve_v);
        reachable(power_index, voltage_index) = ...
            maximum_controlled_voltage_v >= reference_points_v(voltage_index);
    end
end

maximum_reachable_power_w = nan(size(reference_points_v));
for voltage_index = 1:numel(reference_points_v)
    if any(reachable(:, voltage_index))
        maximum_reachable_power_w(voltage_index) = ...
            max(power_scan_w(reachable(:, voltage_index)));
    end
end

%% 6. 打印可直接移植的参数
fprintf('\nCLLC reverse single bus-voltage PI design\n');
fprintf('  Battery / bus design point : %.1f V -> %.1f V, %.0f W\n', ...
    battery_voltage_v, bus_voltage_reference_v, design_power_w);
fprintf('  fr / fmin / fop           : %.3f / %.3f / %.3f kHz\n', ...
    fr_hz/1e3, fmin_hz/1e3, operating_frequency_hz/1e3);
fprintf('  transition_u / operating_u: %.6f / %.6f\n', ...
    transition_u, operating_u);
fprintf('  dV/df / dV/du             : %.6g V/Hz / %.6g V/u\n', ...
    Kvf_v_per_hz, Ku_v_per_u);
fprintf('  output pole / PI zero     : %.3f / %.3f Hz\n', ...
    output_pole_hz, voltage_zero_hz);
fprintf('  Kp                        : %.12g u/V\n', voltage_Kp);
fprintf('  Ki                        : %.12g u/(V*s)\n', voltage_Ki);
fprintf('  ki_step at %.0f kHz       : %.12g u/V/sample\n', ...
    control_update_frequency_hz/1e3, voltage_ki_step);
fprintf('  crossover / PM / GM       : %.2f Hz / %.2f deg / %.2f dB\n', ...
    actual_crossover_hz, phase_margin_deg, gain_margin_db);
fprintf('\nSelected monotonic branch reachability (48 V battery):\n');
disp(table(reference_points_v, maximum_reachable_power_w, ...
    'VariableNames', {'Vbus_V', 'ApproxMaxPower_W'}));

%% 7. 绘图：静态特性、开闭环波特图、功率边界
figure('Name', 'CLLC reverse voltage PI design', 'Color', 'w');
tiledlayout(3, 2, 'TileSpacing', 'compact', 'Padding', 'compact');

nexttile([1, 2]);
plot(frequency_scan_hz/1e3, full_bridge_voltage_v, 'LineWidth', 1.5);
hold on;
xline(fmin_hz/1e3, '--', 'f_{min}');
xline(operating_frequency_hz/1e3, '--r', 'f_{op}');
yline(bus_voltage_reference_v, ':k', '450 V');
grid on;
xlabel('Switching frequency / kHz');
ylabel('Full-bridge bus voltage / V');
title('Reverse FHA static characteristic at the design load');

nexttile;
semilogx(frequency_hz, 20*log10(abs(voltage_plant)), 'LineWidth', 1.2);
hold on;
semilogx(frequency_hz, 20*log10(abs(digital_controller)), 'LineWidth', 1.2);
semilogx(frequency_hz, 20*log10(abs(open_loop)), 'LineWidth', 1.5);
yline(0, ':k'); grid on;
xlabel('Frequency / Hz'); ylabel('Magnitude / dB');
legend('Plant', 'PI', 'Open loop', 'Location', 'southwest');
title('Controller + plant magnitude');

nexttile;
semilogx(frequency_hz, unwrap(angle(open_loop))*180/pi, 'LineWidth', 1.5);
hold on; yline(-180, ':k'); xline(actual_crossover_hz, '--r'); grid on;
xlabel('Frequency / Hz'); ylabel('Phase / deg');
title(sprintf('Open-loop phase, PM = %.1f deg', phase_margin_deg));

nexttile;
semilogx(frequency_hz, 20*log10(abs(closed_loop)), 'LineWidth', 1.5);
grid on; xlabel('Frequency / Hz'); ylabel('Magnitude / dB');
title('Closed-loop reference response');

nexttile;
plot(reference_points_v, maximum_reachable_power_w/1e3, 'o-', 'LineWidth', 1.5);
hold on; yline(maximum_rated_power_w/1e3, '--', '6.6 kW rating'); grid on;
xlabel('Bus reference / V'); ylabel('Approx. maximum power / kW');
title('Reachability on the selected monotonic branch');

sgtitle('CLLC reverse single bus-voltage PI frequency-domain design');

%% Local functions
function output_voltage_v = calculate_reverse_fha_voltage( ...
    frequency_hz, battery_voltage_v, load_resistance_ohm, ...
    Lrp_h, Crp_f, Lm_h, Lrs_h, Crs_f, turns_ratio)
% 把高压直流负载换成基波交流等效负载，然后按反向信号流求电压增益。
    angular_frequency = 2*pi*frequency_hz;
    Zr_primary = 1j*angular_frequency*Lrp_h + ...
        1./(1j*angular_frequency*Crp_f);
    Zr_secondary = 1j*angular_frequency*Lrs_h + ...
        1./(1j*angular_frequency*Crs_f);
    Zm_primary = 1j*angular_frequency*Lm_h;
    Rac_primary = (8/pi^2)*load_resistance_ohm;
    Zprimary_total = Zr_primary + Rac_primary;
    Zparallel_primary = Zm_primary.*Zprimary_total./ ...
        (Zm_primary+Zprimary_total);
    Zparallel_secondary = Zparallel_primary/turns_ratio^2;
    Hsecondary = Zparallel_secondary./(Zr_secondary+Zparallel_secondary);
    Hprimary = Rac_primary./Zprimary_total;
    dc_gain = abs(Hsecondary.*Hprimary*turns_ratio);
    output_voltage_v = battery_voltage_v*dc_gain;
end

function crossing_hz = find_high_frequency_crossing(frequency_hz, curve, target)
% 取最靠近谐振频率的交点，即峰值右侧、随频率升高而增益降低的支路。
    signs = (curve(1:end-1)-target).*(curve(2:end)-target);
    index = find(signs <= 0, 1, 'last');
    if isempty(index)
        crossing_hz = NaN;
        return;
    end
    crossing_hz = interp1(curve(index:index+1), ...
        frequency_hz(index:index+1), target, 'linear');
end

function [fc_hz, pm_deg, gm_db] = analyze_loop_margins(frequency_hz, loop)
    magnitude_db = 20*log10(abs(loop));
    phase_deg = unwrap(angle(loop))*180/pi;
    gain_crossing = find(magnitude_db(1:end-1).*magnitude_db(2:end) <= 0, 1);
    if isempty(gain_crossing)
        fc_hz = NaN; pm_deg = NaN;
    else
        fc_hz = interp1(magnitude_db(gain_crossing:gain_crossing+1), ...
            frequency_hz(gain_crossing:gain_crossing+1), 0);
        phase_at_fc = interp1(frequency_hz, phase_deg, fc_hz);
        pm_deg = 180+phase_at_fc;
    end
    phase_crossing = find((phase_deg(1:end-1)+180).* ...
        (phase_deg(2:end)+180) <= 0, 1);
    if isempty(phase_crossing)
        gm_db = Inf;
    else
        phase_frequency_hz = interp1( ...
            phase_deg(phase_crossing:phase_crossing+1), ...
            frequency_hz(phase_crossing:phase_crossing+1), -180);
        gm_db = -interp1(frequency_hz, magnitude_db, phase_frequency_hz);
    end
end
