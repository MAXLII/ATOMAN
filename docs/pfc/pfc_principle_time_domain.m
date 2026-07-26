%% PFC 浮点母线 PI + PR 电流环原理性时域仿真
% 对应 code/ctrl/pfc：SOGI/FLL 提供电网正交量，母线 notch 后进入 PI，
% PI 输出生成正弦电流参考，PR 电流环输出电感电压命令。
clear; clc; close all;

fs=60e3; Ts=1/fs; t=(0:Ts:.80).'; n=numel(t);
L=440e-6; Cbus=1300e-6; Cac=12e-6; Vrms=230;
wc_v=2*pi*20; pm_v=deg2rad(60);
Kpv=sin(pm_v)*wc_v*Cbus; Kiv=Kpv*wc_v/tan(pm_v);
wx=2*pi*2500; pm_i=deg2rad(50); w0=2*pi*50; wr=2*pi*2;
Kpp=L*wx*sin(pm_i)-2*L*wr*cos(pm_i);
Krp=L*wx^2*cos(pm_i)/(2*wr);

freq=50*ones(n,1); freq(t>=.38)=49; freq(t>=.48)=51;
phase=cumsum(2*pi*freq)*Ts; vg=sqrt(2)*Vrms.*sin(phase);
Pload=1000*ones(n,1); Pload(t>=.20)=2500; Pload(t>=.34)=1600;
ramp_idx=t>=.44 & t<.54;
Pload(ramp_idx)=1600+600*(t(ramp_idx)-.44)/.10; Pload(t>=.54)=2200;
vref_cmd=400*ones(n,1); vref_cmd(t>=.30)=430; vref_cmd(t>=.50)=400;

vbus=zeros(n,1); vbus(1)=330; vref=zeros(n,1); vref(1)=330;
il=zeros(n,1); iref=zeros(n,1); duty=zeros(n,1); iamp=zeros(n,1);
iv=0; z1=0; z2=0; vbus_nf=vbus(1);
for k=1:n-1
    vref(k+1)=ramp_to(vref(k),vref_cmd(k),200*Ts);
    % 低通近似 notch 的 100 Hz 抑制后母线反馈。
    vbus_nf=vbus_nf+Ts*(vbus(k)-vbus_nf)/(1/(2*pi*35));
    ev=vref(k)-vbus_nf; iau=Kpv*ev+iv; iamp(k)=sat(iau,-3,60);
    if abs(iau-iamp(k))<1e-12, iv=iv+Kiv*Ts*ev; end
    vq=sqrt(2)*Vrms*cos(phase(k));
    iref(k)=iamp(k)*230*vg(k)/(Vrms^2)+vq*(2*pi*freq(k))*Cac;
    ei=iref(k)-il(k);
    z1n=z1+Ts*z2;
    z2=z2+Ts*(-2*wr*z2-w0^2*z1+ei); z1=z1n;
    vlcmd=sat(Kpp*ei+2*Krp*wr*z2,-30,30);
    draw=sat((vg(k)-vlcmd)/max(vbus(k),1),-.98,.98);
    duty(k)=draw;
    vl=vg(k)-duty(k)*vbus(k);
    il(k+1)=il(k)+Ts*vl/L;
    pin=max(vg(k)*il(k),0);             % 二极管/全桥只向母线送能
    vbus(k+1)=max(1,vbus(k)+Ts*(pin-Pload(k))/(Cbus*max(vbus(k),50)));
end
iref(end)=iref(end-1); duty(end)=duty(end-1); iamp(end)=iamp(end-1);
assert(all(isfinite([vbus;il;duty])),'PFC simulation diverged.');

figure('Name','PFC float principle time-domain verification','Color','w');
tiledlayout(4,1,'TileSpacing','compact','Padding','compact');
nexttile; plot(t,vref,'--',t,vbus,'LineWidth',1.05); grid on; ylabel('Vbus / V'); legend('ramped ref','Vbus'); title('Float PFC: bus PI + current PR');
nexttile; plot(t,vg/20,t,iref,t,il,'LineWidth',.85); grid on; ylabel('A, V/20'); legend('Vgrid/20','Iref','IL');
nexttile; plot(t,iamp,t,Pload/100,'LineWidth',1); grid on; ylabel('A, W/100'); legend('current amplitude','load power/100');
nexttile; plot(t,duty,t,freq/60,'LineWidth',.9); grid on; ylabel('duty, Hz/60'); xlabel('Time / s'); legend('PWM duty','grid freq/60');
idx=t>.72; pf=sum(vg(idx).*il(idx))/sqrt(sum(vg(idx).^2)*sum(il(idx).^2));
fprintf('PFC PI: Kp=%.6g, Ki=%.6g; PR: Kp=%.6g, Kr=%.6g, wc=%.3f rad/s\n',Kpv,Kiv,Kpp,Krp,wr);
fprintf('PFC final-window PF=%.5f, Vbus mean=%.3f V, Vbus ripple=%.3f Vpp\n',pf,mean(vbus(idx)),range(vbus(idx)));

function y=sat(x,lo,hi), y=min(max(x,lo),hi); end
function y=ramp_to(x,target,step), y=x+sat(target-x,-step,step); end
