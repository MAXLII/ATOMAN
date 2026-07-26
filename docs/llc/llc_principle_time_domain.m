%% LLC 单电压环与母线 100 Hz 前馈原理性时域仿真
% 对应 code/ctrl/llc：Vout 50 Hz LPF、PI、Vbus 100 Hz notch 前馈。
% LLC 谐振腔用辨识友好的低阶平均模型表示；最终参数仍需由开关模型校准。
clear; clc; close all;

fs=30e3; Ts=1/fs; t=(0:Ts:.35).'; n=numel(t);
Kp=.005; wc=2*pi*500; Ki=Kp*wc/tan(deg2rad(60));
fc_lpf=50; tau=1/(2*pi*fc_lpf); plant_tau=2.5e-3;
vbus=400+10*sin(2*pi*100*t);           % 母线叠加 10 V/100 Hz 纹波
vref=3.6*ones(n,1); vref(t>=.12)=4.2; vref(t>=.22)=3.6;
i_load=.8*ones(n,1); i_load(t>=.18)=2.5; i_load(t>=.28)=1.2;

[v_ff,u_ff,ff]=run_case(true,t,Ts,vbus,vref,i_load,Kp,Ki,tau,plant_tau);
[v_no,u_no,~]=run_case(false,t,Ts,vbus,vref,i_load,Kp,Ki,tau,plant_tau);
assert(all(isfinite([v_ff;u_ff;v_no;u_no])),'LLC simulation diverged.');

figure('Name','LLC principle time-domain verification','Color','w');
tiledlayout(4,1,'TileSpacing','compact','Padding','compact');
nexttile; plot(t,vbus,'LineWidth',1); grid on; ylabel('Vbus / V'); title('LLC single voltage loop');
nexttile; plot(t,vref,'--',t,v_no,t,v_ff,'LineWidth',1.05); grid on;
ylabel('Vout / V'); legend('ref','without FF','with 100 Hz FF','Location','best');
nexttile; plot(t,u_no,t,u_ff,'LineWidth',1.05); grid on; ylabel('normalized u'); legend('without FF','with FF'); ylim([0 1]);
nexttile; plot(t,i_load,t,ff*100,'LineWidth',1.05); grid on; ylabel('load / FFx100'); xlabel('Time / s'); legend('load current','bus FF x100');
idx=t>.30; fprintf('LLC PI: Kp=%.6g, Ki=%.6g\n',Kp,Ki);
fprintf('LLC steady ripple: no FF %.5f Vpp, with FF %.5f Vpp\n',range(v_no(idx)),range(v_ff(idx)));

function [v,u,ff]=run_case(use_ff,t,Ts,vbus,vref,iload,Kp,Ki,tau_lpf,tau_p)
n=numel(t); v=zeros(n,1); vf=zeros(n,1); u=zeros(n,1); ff=zeros(n,1);
v(1)=3.6; vf(1)=v(1); integ=.6; bus_dc=vbus(1);
for k=1:n-1
    bus_dc=bus_dc+Ts*(vbus(k)-bus_dc)/(1/(2*pi*30));
    ripple=vbus(k)-bus_dc;
    if use_ff, ff(k)=-ripple/800; end
    e=vref(k)-vf(k); uu=Kp*e+integ+ff(k); u(k)=sat(uu,0,1);
    if abs(uu-u(k))<1e-12, integ=integ+Ki*Ts*e; end
    % u 到输出电压的低阶增益，外加负载等效压降。
    vtarget=6.0*u(k)*(vbus(k)/400)-.10*iload(k);
    v(k+1)=max(0,v(k)+Ts*(vtarget-v(k))/tau_p);
    vf(k+1)=vf(k)+Ts*(v(k)-vf(k))/tau_lpf;
end
u(end)=u(end-1); ff(end)=ff(end-1);
end
function y=sat(x,lo,hi), y=min(max(x,lo),hi); end
