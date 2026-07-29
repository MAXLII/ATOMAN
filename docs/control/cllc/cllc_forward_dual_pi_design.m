%% CLLC 正向电压/电流双环竞争 PI 设计
% 学习目标：本文件展示如何把一个电力电子网络逐步整理成可用于控制器
% 设计的线性模型，并在 MATLAB 中完成 PI 参数计算和频域验证。建议按以下
% 顺序阅读，而不是只看最后打印出来的 Kp、Ki：
%
%   1. 用 FHA（基波近似）计算 CLLC 在不同频率下的稳态增益；
%   2. 找到额定工作点，并求该点附近的局部斜率 dVout/du；
%   3. 用“一阶输出包络 + 纯延时”构造小信号被控对象；
%   4. 根据指定交越频率和 PI 零点解析计算 Kp、Ki；
%   5. 按实际 C 程序的离散积分形式重新计算环路频率响应；
%   6. 检查交越频率、相位裕度、增益裕度和稳态竞争关系。
%
% 这是“原理性设计”而不是开关模型辨识。FHA 和一阶包络模型适合先确定
% 控制方向、带宽数量级和初始 PI 参数；最终参数还应在 PLECS 开关模型及
% 实机上用扫频或阶跃实验复核。
%
% MATLAB 语法提示：本文件大量使用带点运算符的 .*, ./ 和 .^。它们表示
% 对频率向量中的每个元素分别运算；若去掉点号，MATLAB 会尝试矩阵运算。
% 变量名后缀同时标明单位，例如 _hz、_v、_ohm、_s，可避免量纲混淆。
%
% 本脚本按 code/lib/pi_dual_compete.c 的结构设计控制器：
%
%   voltage error --> PI_v --+
%                              +--> MIN competition --> u(0~1)
%   current error --> PI_i --+
%                         ^
%                         +-- shared integral
%
% 两个环路都直接控制同一个归一化调制量 u，不是串级外环/内环。
% 正向 CLLC 中 u 增大表示开关频率降低、传输功率增加，因此使用 MIN：
%
%   - 正常运行：电流低于限值，电流环候选输出较大，电压环获胜；
%   - 过流运行：电流误差变为负值，电流环候选输出更小并限制 u；
%   - 两路共享积分，切换时没有两个积分状态互相冲突。
%
% -------------------------------------------------------------------------
% 连续 PI 参数公式
% -------------------------------------------------------------------------
% 每一路独立小信号设计：
%
%   Cx(s) = Kpx + Kix/s = Kpx*(1 + wzx/s)
%
% 在目标交越频率 fcx 处：
%
%   Kpx = 1 / (|Gx(j*2*pi*fcx)|*sqrt(1 + (fzx/fcx)^2))
%   Kix = 2*pi*fzx*Kpx
%
% 电压环对象：Gv(s)=DeltaVout(s)/Delta u(s)
% 电流环对象：Gi(s)=DeltaIload(s)/Delta u(s)=Gv(s)/Rload
%
% 两路零点均放在输出包络主极点：
%
%   tau_o = Rload*Cout/2
%   fz = 1/(2*pi*tau_o)
%
% -------------------------------------------------------------------------
% 与 pi_dual_compete.c 一致的离散积分参数
% -------------------------------------------------------------------------
% C 实现每次调用执行：
%
%   i_share[n] = i_share[n-1] + ki_step_active*error_active[n]
%
% 因此传给 C 模块的 ki 不是连续域 Ki，而是：
%
%   ki_step = Ki*Ts
%
% 候选输出和竞争规则：
%
%   out_v = Kpv*ev + i_share
%   out_i = Kpi*ei + i_share
%   u = min(out_v, out_i)
%
% 只有 MIN 竞争的获胜通道更新共享积分。

clear;
clc;
close all;

%% 用户可调参数
% 这一节是设计入口。学习和调参时优先修改这里，不要直接修改后面的中间量。
% 电压、电流 PI 的交越频率代表响应速度；交越频率越高，响应通常越快，
% 但延时、噪声和未建模高频极点带来的相位损失也越明显。
Vout_reference_v = 48;
output_voltage_range_v = [24, 72];
bus_voltage_gain = 8;
bus_voltage_offset_v = 25;
minimum_bus_voltage_limit_v = 370;
bus_voltage_range_v = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*output_voltage_range_v + bus_voltage_offset_v);
% PI 在 48 V 额定点设计，对应母线为 8*48+25=409 V。时域脚本会
% 进一步加入母线动态；频域设计先在该稳态工作点线性化。
Vin_v = max(minimum_bus_voltage_limit_v, ...
    bus_voltage_gain*Vout_reference_v + bus_voltage_offset_v);
output_power_w = 6600;
Cout_f = 10e-3;

% 额定电流为 150 A，过流保护取 110% 额定值，即 165 A。24 V 时正常
% 最大功率为 24*150=3600 W；44 V 以上才可达到 6600 W。必须给正常
% 150 A 工作点留出竞争余量，否则电流环会在额定点与电压环同时生效。
rated_current_a = 150;
load_current_limit_a = 1.10*rated_current_a;
voltage_crossover_hz = 350;
current_crossover_hz = 3.5e3;

% control_update_frequency_hz 决定数字控制器的调用周期 Ts。
% delay_cycles 用一个开关周期近似采样、计算和调制更新形成的总延时。
control_update_frequency_hz = 100e3;
delay_cycles = 1;

% CLLC 网络参数
% 原边串联谐振支路：Lr_primary_h、Cr_primary_f；
% 励磁支路：Lm_primary_h；副边串联谐振支路：Lr_secondary_h、Cr_secondary_f。
% turns_ratio=Np/Ns，后续用 n^2 把副边阻抗折算到原边。
Lr_primary_h = 40e-6;
Cr_primary_f = 80e-9;
Lm_primary_h = 200e-6;
Lr_secondary_h = 0.625e-6;
Cr_secondary_f = 5.12e-6;
turns_ratio = 24/3;

%% FHA 工作点与归一化混合调制
% 串联谐振频率 fr=1/(2*pi*sqrt(Lr*Cr))。本设计规定 PFM 的最高频率
% fmax=2fr，并用足够密集的频率向量离散扫描 fr~2fr。
% linspace(...).' 中的 .' 把行向量转成列向量，便于后续统一处理。
fr_hz = 1/(2*pi*sqrt(Lr_primary_h*Cr_primary_f));
fmax_hz = 2*fr_hz;
switching_frequency_hz = linspace(fr_hz, fmax_hz, 4001).';

load_resistance_ohm = Vout_reference_v^2/output_power_w;
nominal_load_current_a = Vout_reference_v/load_resistance_ohm;

% FHA 中整流器、输出电容和直流负载需要等效为交流基波负载。
% 全波整流常用 Rac=(8/pi^2)*Rdc。这个等效决定谐振腔看到的阻尼。
ac_load_resistance_ohm = (8/pi^2)*load_resistance_ohm;

% calculate_forward_fha_voltage 返回满相移（桥臂有效占空比为 50%）时，
% 每个开关频率对应的输出电压和直流电压增益。整个频率向量一次传入，
% MATLAB 会以向量方式计算，避免显式 for 循环。
[full_psm_output_voltage_v, dc_gain] = calculate_forward_fha_voltage( ...
    switching_frequency_hz, Vin_v, ac_load_resistance_ohm, ...
    Lr_primary_h, Cr_primary_f, Lm_primary_h, ...
    Lr_secondary_h, Cr_secondary_f, turns_ratio);

voltage_at_fr_v = full_psm_output_voltage_v(1);
voltage_at_fmax_v = full_psm_output_voltage_v(end);

maximum_range_voltage_at_fr_v = ...
    dc_gain(1)*bus_voltage_range_v(2);
if maximum_range_voltage_at_fr_v < output_voltage_range_v(2)
    error(['最高输出点的母线在 fr 处最多输出 %.3f V，', ...
           '无法覆盖 %.3f V。'], ...
        maximum_range_voltage_at_fr_v, output_voltage_range_v(2));
end

% 单一调制命令 u 的定义：
%   0~ut：固定 fmax，PSM 占空比从 0 增加到 50%；
%   ut~1：固定 50% 占空比，频率从 fmax 降到 fr。
% 为让两段在边界处电压连续，令 ut=V(fmax)/V(fr)。
transition_u = voltage_at_fmax_v/voltage_at_fr_v;

% 在 fr 以上寻找 FHA 增益等于 Vout/Vin 的频率。限制在谐振点以上，
% 是因为这里选用“频率降低、增益升高”的正向 PFM 单调工作区。
operating_frequency_hz = find_gain_crossing_above_resonance( ...
    switching_frequency_hz, dc_gain, ...
    Vout_reference_v/Vin_v, fr_hz);
if ~isfinite(operating_frequency_hz)
    error('fr~2fr 范围内没有找到额定输出电压工作点。');
end

operating_u = transition_u + ...
    (fmax_hz - operating_frequency_hz)/(fmax_hz - fr_hz) * ...
    (1 - transition_u);

% 小信号控制需要工作点附近的斜率，而不是整条非线性曲线。
% gradient(y,x) 数值计算 dVout/dfs，interp1 再取额定频率处的值。
% 因为 u 增大时频率降低，所以 dfs/du 为负；在当前单调区 dVout/dfs
% 也为负，两者相乘得到正的 dVout/du，表示 u 增大将提高输出电压。
voltage_slope_v_per_hz = gradient( ...
    full_psm_output_voltage_v, switching_frequency_hz);
Kvf_v_per_hz = interp1( ...
    switching_frequency_hz, voltage_slope_v_per_hz, ...
    operating_frequency_hz, 'linear');
frequency_gain_hz_per_u = -(fmax_hz - fr_hz)/(1 - transition_u);
Ku_v_per_u = Kvf_v_per_hz*frequency_gain_hz_per_u;

% 这个符号检查很重要：若控制增益符号错误，PI 会形成正反馈。
if Ku_v_per_u <= 0
    error('额定点 dVout/du 不是正值，请检查调制方向。');
end

%% 两个直接调制对象
% 输出电容为 10 mF，使用 tau_o=Rload*Cout/2 近似整流后输出包络。
% 电压对象由额定点静态斜率 Ku 和一阶惯性组成，并乘 exp(-sTd)
% 表示纯延时：
%
%   Gv(s) = Ku/(1+s*tau_o)*exp(-s*Td)
%   Gi(s) = Gv(s)/Rload
%
% 第二式来自小信号关系 DeltaIload=DeltaVout/Rload。两个对象的输入
% 都是同一个 u，因此这是“两个限制器竞争同一执行量”，不是串级双环。
output_time_constant_s = load_resistance_ohm*Cout_f/2;
output_pole_hz = 1/(2*pi*output_time_constant_s);
delay_s = delay_cycles/operating_frequency_hz;
sample_time_s = 1/control_update_frequency_hz;

frequency_hz = logspace( ...
    -2, log10(0.45*control_update_frequency_hz), 5001).';

% 频率响应直接在 s=jw 上求值。上限取控制频率的 0.45 倍，保持在
% Nyquist 频率以下；logspace 让低频到高频在对数坐标上均匀分布。
s = 1j*2*pi*frequency_hz;
voltage_plant = Ku_v_per_u ./ ...
    (1 + s*output_time_constant_s).*exp(-s*delay_s);
current_plant = voltage_plant/load_resistance_ohm;

%% 电压环 PI：正常稳压通道
% PI 零点放到输出主极点处，用零点提供的相位超前抵消一阶对象在带宽
% 附近的相位滞后。design_pi_at_crossover 再利用 |C(jwc)G(jwc)|=1
% 解析求出比例增益，省去手工反复试凑。
voltage_zero_hz = output_pole_hz;
[voltage_Kp, voltage_Ki, voltage_controller] = ...
    design_pi_at_crossover( ...
        frequency_hz, voltage_plant, ...
        voltage_crossover_hz, voltage_zero_hz);
voltage_ki_step = voltage_Ki*sample_time_s;

% 上面的 voltage_controller 是连续 PI，只用于解释参数来源；真正的
% 环路分析使用 voltage_digital_controller，因为它严格对应 C 代码：
% i[n]=i[n-1]+ki_step*e[n]。这样会自动包含离散积分器的幅相特性。
voltage_digital_controller = evaluate_c_compete_pi( ...
    frequency_hz, sample_time_s, voltage_Kp, voltage_ki_step);
voltage_loop = voltage_digital_controller.*voltage_plant;

% 负反馈闭环互补灵敏度 T=L/(1+L)。它描述参考电压到输出电压的跟随
% 特性；开环 L 用于裕度设计，闭环 T 用于观察带宽和高频衰减。
voltage_closed_loop = voltage_loop./(1 + voltage_loop);
[voltage_actual_crossover_hz, voltage_pm_deg, voltage_gm_db] = ...
    analyze_loop_margins(frequency_hz, voltage_loop);

%% 电流环 PI：过流限制通道
% 电流限制环使用相同设计步骤，但对象是 Gi，且交越频率更高，使发生
% 过流时能比慢速稳压通道更快地降低 u。竞争逻辑决定哪个通道实际生效，
% 因此这里的两个闭环波特图表示“各通道单独获胜时”的局部线性特性。
current_zero_hz = output_pole_hz;
[current_Kp, current_Ki, current_controller] = ...
    design_pi_at_crossover( ...
        frequency_hz, current_plant, ...
        current_crossover_hz, current_zero_hz);
current_ki_step = current_Ki*sample_time_s;
current_digital_controller = evaluate_c_compete_pi( ...
    frequency_hz, sample_time_s, current_Kp, current_ki_step);
current_loop = current_digital_controller.*current_plant;
current_closed_loop = current_loop./(1 + current_loop);
[current_actual_crossover_hz, current_pm_deg, current_gm_db] = ...
    analyze_loop_margins(frequency_hz, current_loop);

%% 稳态竞争关系
% 额定稳态时 ev=0，共享积分应等于维持 48 V 所需的 u0。
% 电流仍低于 Ilimit，因此 ei=Ilimit-Iload>0，电流候选值应高于电压
% 候选值。MIN 竞争会选择较小的电压候选，系统正常稳压。
% 当 Iload>Ilimit 时 ei<0，电流候选会快速下降并成为获胜通道。
voltage_error_steady_v = 0;
current_error_steady_a = ...
    load_current_limit_a - nominal_load_current_a;
voltage_p_steady = voltage_Kp*voltage_error_steady_v;
current_p_steady = current_Kp*current_error_steady_a;
voltage_candidate_steady = voltage_p_steady + operating_u;
current_candidate_steady = current_p_steady + operating_u;

if voltage_candidate_steady > current_candidate_steady
    warning('额定点不是电压环获胜，请提高电流限值或检查 PI 参数。');
end
if voltage_pm_deg < 45 || voltage_gm_db < 6
    warning('电压环裕度没有达到 PM>=45 deg、GM>=6 dB。');
end
if current_pm_deg < 45 || current_gm_db < 6
    warning('电流环裕度没有达到 PM>=45 deg、GM>=6 dB。');
end

%% 输出结果
% fprintf 用于把最重要的设计结果打印到 Command Window；结构体 design
% 则把结果保留在工作区，后续可以用 design.voltage.Kp 等字段访问，
% 也便于其他脚本自动读取或加入 assert 做回归验证。
fprintf('\n================ Dual-compete operating point ================\n');
fprintf('Mode = MIN, shared integral initial value = %.9f\n', operating_u);
fprintf('Rload = %.9g Ohm, Iload = %.9g A, Ilimit = %.9g A\n', ...
    load_resistance_ohm, nominal_load_current_a, load_current_limit_a);
fprintf('Cout = %.9g F, output pole = %.9g Hz\n', ...
    Cout_f, output_pole_hz);
fprintf(['Bus law: Vbus_ref = max(%.6g V, %.6g*Vout + %.6g V), ', ...
         'range = %.6f~%.6f V\n'], ...
    minimum_bus_voltage_limit_v, bus_voltage_gain, bus_voltage_offset_v, ...
    bus_voltage_range_v(1), bus_voltage_range_v(2));
fprintf('fr = %.6f kHz, fmax = %.6f kHz\n', fr_hz/1e3, fmax_hz/1e3);
fprintf('ut = %.9f, u0 = %.9f, fs0 = %.6f kHz\n', ...
    transition_u, operating_u, operating_frequency_hz/1e3);
fprintf('Steady candidates: voltage = %.9f, current = %.9f\n', ...
    voltage_candidate_steady, current_candidate_steady);

fprintf('\n================ Voltage compete PI ==========================\n');
fprintf('Kp_v = %.9g u/V\n', voltage_Kp);
fprintf('Ki_v = %.9g u/(V*s)\n', voltage_Ki);
fprintf('ki_step_v = Ki_v*Ts = %.9g u/V/call\n', voltage_ki_step);
fprintf('fz_v = %.9g Hz\n', voltage_zero_hz);
fprintf('fc_v = %.6f Hz, PM_v = %.6f deg, GM_v = %.6f dB\n', ...
    voltage_actual_crossover_hz, voltage_pm_deg, voltage_gm_db);

fprintf('\n================ Current compete PI ==========================\n');
fprintf('Kp_i = %.9g u/A\n', current_Kp);
fprintf('Ki_i = %.9g u/(A*s)\n', current_Ki);
fprintf('ki_step_i = Ki_i*Ts = %.9g u/A/call\n', current_ki_step);
fprintf('fz_i = %.9g Hz\n', current_zero_hz);
fprintf('fc_i = %.6f kHz, PM_i = %.6f deg, GM_i = %.6f dB\n', ...
    current_actual_crossover_hz/1e3, current_pm_deg, current_gm_db);

design.mode = 'MIN';
design.sample_time_s = sample_time_s;
design.output_limits = [0, 1];
design.operating.transition_u = transition_u;
design.operating.normalized_command = operating_u;
design.operating.switching_frequency_hz = operating_frequency_hz;
design.operating.load_resistance_ohm = load_resistance_ohm;
design.operating.load_current_a = nominal_load_current_a;
design.operating.current_limit_a = load_current_limit_a;
design.operating.rated_current_a = rated_current_a;
design.operating.output_capacitance_f = Cout_f;
design.operating.output_voltage_range_v = output_voltage_range_v;
design.operating.bus_voltage_range_v = bus_voltage_range_v;
design.operating.bus_voltage_gain = bus_voltage_gain;
design.operating.bus_voltage_offset_v = bus_voltage_offset_v;
design.operating.minimum_bus_voltage_limit_v = ...
    minimum_bus_voltage_limit_v;
design.voltage.Kp = voltage_Kp;
design.voltage.Ki_continuous = voltage_Ki;
design.voltage.ki_step = voltage_ki_step;
design.voltage.zero_hz = voltage_zero_hz;
design.voltage.crossover_hz = voltage_actual_crossover_hz;
design.voltage.phase_margin_deg = voltage_pm_deg;
design.voltage.gain_margin_db = voltage_gm_db;
design.current.Kp = current_Kp;
design.current.Ki_continuous = current_Ki;
design.current.ki_step = current_ki_step;
design.current.zero_hz = current_zero_hz;
design.current.crossover_hz = current_actual_crossover_hz;
design.current.phase_margin_deg = current_pm_deg;
design.current.gain_margin_db = current_gm_db;

%% 两个竞争环路的独立波特图
% 每个环路分别画四组曲线：对象 G、控制器 C、开环 L=C*G、闭环
% T=L/(1+L)。左列为幅值，右列为相位。阅读时重点看：
%   - L 的 0 dB 交点是否接近目标交越频率；
%   - 交点处相位距离 -180 deg 的余量（PM）；
%   - 相位到 -180 deg 时幅值距离 0 dB 的余量（GM）；
%   - T 在低频是否约为 0 dB、在高频是否衰减。
figure('Color', 'w', 'Name', 'CLLC dual-compete PI Bode');
layout = tiledlayout(4, 2, 'TileSpacing', 'compact', ...
    'Padding', 'compact');
plot_bode_pair(layout, frequency_hz, voltage_plant, ...
    'Voltage plant G_v');
plot_bode_pair(layout, frequency_hz, voltage_digital_controller, ...
    'Voltage PI C_v');
plot_bode_pair(layout, frequency_hz, voltage_loop, ...
    'Voltage open loop L_v');
plot_bode_pair(layout, frequency_hz, voltage_closed_loop, ...
    'Voltage closed loop T_v');
title(layout, sprintf( ...
    'Voltage compete loop: fc=%.1f Hz, PM=%.1f deg, GM=%.1f dB', ...
    voltage_actual_crossover_hz, voltage_pm_deg, voltage_gm_db));

figure('Color', 'w', 'Name', 'CLLC current-limit compete PI Bode');
layout = tiledlayout(4, 2, 'TileSpacing', 'compact', ...
    'Padding', 'compact');
plot_bode_pair(layout, frequency_hz, current_plant, ...
    'Load-current plant G_i');
plot_bode_pair(layout, frequency_hz, current_digital_controller, ...
    'Current-limit PI C_i');
plot_bode_pair(layout, frequency_hz, current_loop, ...
    'Current-limit open loop L_i');
plot_bode_pair(layout, frequency_hz, current_closed_loop, ...
    'Current-limit closed loop T_i');
title(layout, sprintf( ...
    'Current compete loop: fc=%.1f Hz, PM=%.1f deg, GM=%.1f dB', ...
    current_actual_crossover_hz, current_pm_deg, current_gm_db));

%% 局部函数
function [output_voltage_v, dc_gain] = calculate_forward_fha_voltage( ...
    frequency_hz, Vin_v, Rac_ohm, Lrp, Crp, Lmp, Lrs, Crs, n)
%CALCULATE_FORWARD_FHA_VOLTAGE 计算正向满相移 FHA 输出电压。
% 等效电路按复阻抗直接计算：原边串联谐振支路 Zrp，励磁支路 Zm，
% 副边串联谐振支路与 Rac 串联后按 n^2 折算到原边。Zm 与折算负载
% 并联形成 Zparallel，然后分别计算原边分压 Hprimary 和副边负载分压
% Hsecondary。abs 取复数传递函数的幅值，忽略稳态相位。

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
%FIND_GAIN_CROSSING_ABOVE_RESONANCE 查找谐振点以上的目标增益交点。
% 先找相邻两点误差异号的位置，再在这两点间做线性插值。相比直接找
% “最接近点”，插值得到的工作频率不会受扫描步长明显限制。

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

function [Kp, Ki, controller] = design_pi_at_crossover( ...
    frequency_hz, plant, crossover_hz, zero_hz)
%DESIGN_PI_AT_CROSSOVER 按 PI 零点和 0 dB 条件计算参数。
% PI 可写为 C(s)=Kp*(1+wz/s)。在 wc 处其幅值为
% Kp*sqrt(1+(wz/wc)^2)。因此先插值得到 |G(jwc)|，再由
% |C(jwc)G(jwc)|=1 求 Kp，最后用 Ki=wz*Kp 求积分增益。

    plant_gain = interp1(log10(frequency_hz), abs(plant), ...
        log10(crossover_hz), 'linear');
    shape_gain = sqrt(1 + (zero_hz/crossover_hz)^2);
    Kp = 1/(plant_gain*shape_gain);
    Ki = 2*pi*zero_hz*Kp;
    s = 1j*2*pi*frequency_hz;
    controller = Kp + Ki./s;
end

function controller = evaluate_c_compete_pi( ...
    frequency_hz, ts, Kp, ki_step)
%EVALUATE_C_COMPETE_PI 复现 pi_dual_compete.c 的离散 PI 频率响应。
% 累加器 i[n]=i[n-1]+ki_step*e[n] 的 Z 域传递函数为
% ki_step/(1-z^-1)。令 z^-1=exp(-j*w*Ts)，即可沿单位圆计算数字
% 控制器的频率响应。这里不能再次给 ki_step 乘 Ts，否则会重复离散化。

    z_inverse = exp(-1j*2*pi*frequency_hz*ts);
    controller = Kp + ki_step./(1 - z_inverse);
end

function [crossover_hz, phase_margin_deg, gain_margin_db] = ...
    analyze_loop_margins(frequency_hz, loop)
%ANALYZE_LOOP_MARGINS 计算全部交点中的最差 PM 和 GM。
% unwrap 消除 angle 在 +/-180 deg 处的跳变。实际高阶环路可能有多个
% 0 dB 或 -180 deg 交点，因此不能只取第一个；本函数枚举全部交点，
% 返回最小裕度，对稳定性判断更保守。

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
    orders = first_order:last_order;
    crossing_groups = cell(numel(orders), 1);
    for order_index = 1:numel(orders)
        target_phase = -180 - 360*orders(order_index);
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
% 波特图横轴是 log10(f)，所以交点也在对数频率坐标内线性插值。
% relative_data 相邻点乘积 <=0 表示曲线穿过或刚好落在目标值上。

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

function plot_bode_pair(layout, frequency_hz, response, plot_title)
%PLOT_BODE_PAIR 绘制幅频和相频曲线。
% 复数频率响应 H(jw) 转成波特图时：幅值=20log10(|H|)，
% 相位=unwrap(angle(H))*180/pi。0 dB 和 -180 deg 虚线是裕度读图基准。

    magnitude_axes = nexttile(layout);
    phase_axes = nexttile(layout);
    semilogx(magnitude_axes, frequency_hz, ...
        20*log10(abs(response)), 'LineWidth', 1.5);
    semilogx(phase_axes, frequency_hz, ...
        rad2deg(unwrap(angle(response))), 'LineWidth', 1.5);
    yline(magnitude_axes, 0, ':k');
    yline(phase_axes, -180, ':k');
    grid(magnitude_axes, 'on');
    grid(phase_axes, 'on');
    xlabel(magnitude_axes, 'Frequency / Hz');
    xlabel(phase_axes, 'Frequency / Hz');
    ylabel(magnitude_axes, 'Magnitude / dB');
    ylabel(phase_axes, 'Phase / degree');
    title(magnitude_axes, plot_title);
    title(phase_axes, plot_title);
end
