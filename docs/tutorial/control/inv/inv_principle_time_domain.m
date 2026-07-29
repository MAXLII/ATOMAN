%% 单相逆变器 dq 电压环 + 电流比例环原理性时域仿真
% 对应 code/ctrl/inv。用 alpha/beta 两套正交平均 LC 状态实现 dq 变换，
% 连续覆盖 RMS 软启动、RMS 阶跃、频率斜坡和负载阶跃。
clear; clc; close all;

fs=30e3; Ts=1/fs; t=(0:Ts:1.80).'; n=numel(t);
L=440e-6; C=12e-6; Vbus=400; Kr=2.0;
wc=2*pi*300; pm=deg2rad(60);
Kp=sin(pm)*wc*C; Ki=Kp*wc/tan(pm);

rms_cmd=230*ones(n,1); rms_cmd(t>=1.20)=200; rms_cmd(t>=1.45)=240;
freq_cmd=50*ones(n,1); freq_cmd(t>=1.30)=55; freq_cmd(t>=1.60)=48;
R=80*ones(n,1); R(t>=.50)=25; R(t>=1.55)=50;
rms_ref=zeros(n,1); freq=zeros(n,1); freq(1)=50;
for k=2:n
    rms_ref(k)=ramp_to(rms_ref(k-1),rms_cmd(k),212*Ts);
    freq(k)=ramp_to(freq(k-1),freq_cmd(k),10*Ts);
end

va=zeros(n,1); vb=zeros(n,1); ia=zeros(n,1); ib=zeros(n,1);
vpwm=zeros(n,1); idref=zeros(n,1); iqref=zeros(n,1); phase=zeros(n,1);
intd=0; intq=0;
for k=1:n-1
    th=2*pi*phase(k); c=cos(th); s=sin(th); omega=2*pi*freq(k);
    vd=va(k)*c+vb(k)*s; vq=-va(k)*s+vb(k)*c;
    id=ia(k)*c+ib(k)*s; iq=-ia(k)*s+ib(k)*c;
    vdref=sqrt(2)*rms_ref(k); vqref=0;
    ed=vdref-vd; eq=vqref-vq;
    od0=Kp*ed+intd; oq0=Kp*eq+intq;
    % 工程中 dq 电压 PI 的输出上下限均为 +/-50 A。
    od=sat(od0,-50,50); oq=sat(oq0,-50,50);
    if abs(od0-od)<1e-12, intd=intd+Ki*Ts*ed; end
    if abs(oq0-oq)<1e-12, intq=intq+Ki*Ts*eq; end
    idref(k)=od+omega*C*vq; iqref(k)=oq-omega*C*vd;
    vpd=vdref+Kr*(idref(k)-id)+omega*L*iq;
    vpq=Kr*(iqref(k)-iq)-omega*L*id;
    vpa=sat(vpd*c-vpq*s,-.98*Vbus,.98*Vbus);
    vpb=sat(vpd*s+vpq*c,-.98*Vbus,.98*Vbus);
    vpwm(k)=vpa;
    % 半隐式欧拉对无源 LC 网络保持数值稳定；0.08 ohm 表示电感铜阻。
    ia(k+1)=ia(k)+Ts*(vpa-va(k)-.08*ia(k))/L;
    ib(k+1)=ib(k)+Ts*(vpb-vb(k)-.08*ib(k))/L;
    va(k+1)=va(k)+Ts*(ia(k+1)-va(k)/R(k))/C;
    vb(k+1)=vb(k)+Ts*(ib(k+1)-vb(k)/R(k))/C;
    phase(k+1)=mod(phase(k)+freq(k)*Ts,1);
end
vpwm(end)=vpwm(end-1); idref(end)=idref(end-1); iqref(end)=iqref(end-1);
assert(all(isfinite([va;ia;vpwm])),'INV simulation diverged.');

% 一周期滑动 RMS，便于观察幅值动态。
win=round(fs/50); vrms=sqrt(movmean(va.^2,[win-1 0]));
figure('Name','INV principle time-domain verification','Color','w');
tiledlayout(4,1,'TileSpacing','compact','Padding','compact');
nexttile; plot(t,va,'LineWidth',.8); grid on; ylabel('Vac / V'); title('Single-phase inverter dq control');
nexttile; plot(t,rms_ref,'--',t,vrms,'LineWidth',1.05); grid on; ylabel('RMS / V'); legend('reference','measured');
nexttile; plot(t,ia,t,va./R,'LineWidth',.9); grid on; ylabel('Current / A'); legend('inductor','load');
nexttile; plot(t,freq,t,vpwm/Vbus,'LineWidth',1.0); grid on; ylabel('Hz / duty'); xlabel('Time / s'); legend('frequency','normalized PWM');
fprintf('INV voltage PI: Kp=%.6g, Ki=%.6g; current Kr=%.6g\n',Kp,Ki,Kr);
fprintf('INV max |PWM duty|=%.4f, max |inductor current|=%.3f A\n',max(abs(vpwm/Vbus)),max(abs(ia)));

function y=sat(x,lo,hi), y=min(max(x,lo),hi); end
function y=ramp_to(x,target,step), y=x+sat(target-x,-step,step); end
