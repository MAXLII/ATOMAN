%% Buck 两相电压外环/电流内环原理性时域仿真
% 对应 code/ctrl/buck 的控制结构。功率级采用连续导通模式平均模型，
% 用于观察软启动、参考阶跃/斜坡、负载阶跃和限流，不替代开关级 PLECS 验证。
clear; clc; close all;

%% 1. 工程参数与 PI 参数
fs = 100e3; Ts = 1/fs;                 % 原理仿真的控制频率
Lph = 1.5e-6; Leq = Lph/2; Cout = 4e-3; % 两相并联等效电感、输出电容
wc_v = 2*pi*1200; pm_v = deg2rad(60);
Kpv = sin(pm_v)*wc_v*Cout;
Kiv = Kpv*wc_v/tan(pm_v);
wc_i = 2*pi*7000; pm_i = deg2rad(45);
Kpi = sin(pm_i)*wc_i*Leq;
Kii = Kpi*wc_i/tan(pm_i);
Imax = 100; Pmax = 1500; duty_max = 0.98;

%% 2. 连续工况：软启动 -> 阶跃 -> 斜坡 -> 负载阶跃
t_end = 0.16; t = (0:Ts:t_end).'; n = numel(t);
vin = 60*ones(n,1);                    % Buck 输入母线
vref_cmd = 12*ones(n,1);
vref_cmd(t >= 0.035) = 36;             % 电压阶跃
ramp = t >= 0.080 & t < 0.120;
vref_cmd(ramp) = 36 + (48-36)*(t(ramp)-0.080)/0.040;
vref_cmd(t >= 0.120) = 48;             % 电压斜坡结束
Rload = 3.0*ones(n,1);
Rload(t >= 0.125) = 1.6;               % 负载加重
vref = zeros(n,1); vref(1)=0;
for k=2:n                               % 500 V/s 软启动/参考斜率限制
    vref(k)=ramp_to(vref(k-1),vref_cmd(k),500*Ts);
end

vout=zeros(n,1); il=zeros(n,1); iref=zeros(n,1); duty=zeros(n,1);
iv=0; ii=0; vout(1)=0.1;
for k=1:n-1
    ev=vref(k)-vout(k);
    i_unsat=Kpv*ev+iv;
    i_lim=min([Imax, Pmax/max(vin(k),1), Imax]);
    iref(k)=sat(i_unsat,0,i_lim);
    if abs(i_unsat-iref(k))<1e-12 || sign(ev)~=sign(i_unsat-iref(k))
        iv=iv+Kiv*Ts*ev;
    end
    ei=iref(k)-il(k);
    vl_unsat=Kpi*ei+ii;
    d_unsat=(vout(k)+vl_unsat)/max(vin(k),1);
    duty(k)=sat(d_unsat,0,duty_max);
    if abs(d_unsat-duty(k))<1e-12
        ii=ii+Kii*Ts*ei;
    end
    i_load=vout(k)/Rload(k);
    il(k+1)=max(0,il(k)+Ts*(duty(k)*vin(k)-vout(k))/Leq);
    vout(k+1)=max(0,vout(k)+Ts*(il(k)-i_load)/Cout);
end
iref(end)=iref(end-1); duty(end)=duty(end-1);
assert(all(isfinite([vout;il;duty])),'Buck simulation diverged.');

%% 3. 结果
figure('Name','Buck principle time-domain verification','Color','w');
tl=tiledlayout(4,1,'TileSpacing','compact','Padding','compact');
nexttile; plot(t,vref_cmd,'--',t,vref,t,vout,'LineWidth',1.1); grid on;
ylabel('Voltage / V'); legend('command','ramped ref','Vout','Location','best');
title('Two-phase Buck: voltage outer loop + current inner loop');
nexttile; plot(t,iref,'--',t,il,'LineWidth',1.1); grid on;
ylabel('Current / A'); legend('Iref','inductor total','Location','best');
nexttile; plot(t,duty,'LineWidth',1.1); grid on; ylabel('Duty'); ylim([0 1]);
nexttile; plot(t,vout.^2./Rload,'LineWidth',1.1); grid on;
ylabel('Load power / W'); xlabel('Time / s');
fprintf('Buck PI: Kpv=%.6g, Kiv=%.6g, Kpi=%.6g, Kii=%.6g\n',Kpv,Kiv,Kpi,Kii);
fprintf('Buck: max Vout=%.3f V, max current=%.3f A, max duty=%.4f\n',max(vout),max(il),max(duty));

function y=sat(x,lo,hi), y=min(max(x,lo),hi); end
function y=ramp_to(x,target,step), y=x+sat(target-x,-step,step); end
