%% PFC-I32 母线 PI + 整数电流 PI 原理性时域仿真
% 对应 code/ctrl/pfc_i32 的量化域：Vac 12 bit signed、Vbus 12 bit、
% 电流 14 bit signed、PWM reload=64000。图中同时给出连续域基准和量化域结果。
clear; clc; close all;

fs=30e3; Ts=1/fs; t=(0:Ts:.80).'; n=numel(t);
L=440e-6; Cbus=1300e-6; Vrms=230; Vpk=sqrt(2)*Vrms;
vac_max=400; vac_code_max=2047; vbus_max=500; vbus_code_max=4095;
curr_max=100; curr_code_max=8191; pwm_reload=64000;
wc_v=2*pi*20; pm_v=deg2rad(60);
Kpv=sin(pm_v)*wc_v*Cbus; Kiv=Kpv*wc_v/tan(pm_v);
wc_i=2*pi*2500; pm_i=deg2rad(50);
Kpi=sin(pm_i)*wc_i*L; Kii=Kpi*wc_i/tan(pm_i);

freq=50*ones(n,1); freq(t>=.40)=49; freq(t>=.50)=51;
phase=cumsum(2*pi*freq)*Ts; vg=Vpk.*sin(phase);
Pload=900*ones(n,1); Pload(t>=.20)=2300; Pload(t>=.34)=1400;
ramp_idx=t>=.44 & t<.54;
Pload(ramp_idx)=1400+700*(t(ramp_idx)-.44)/.10; Pload(t>=.54)=2100;
vref_cmd=400*ones(n,1); vref_cmd(t>=.30)=430; vref_cmd(t>=.50)=400;

[vf,iflt,df]=run_case(false,t,Ts,vg,Vrms,Pload,vref_cmd,L,Cbus,Kpv,Kiv,Kpi,Kii,0,0,0,0,0);
[vq,iq,dq]=run_case(true,t,Ts,vg,Vrms,Pload,vref_cmd,L,Cbus,Kpv,Kiv,Kpi,Kii,...
    vac_max,vac_code_max,vbus_max,vbus_code_max,curr_code_max/curr_max,pwm_reload);
assert(all(isfinite([vf;iflt;df;vq;iq;dq])),'PFC-I32 simulation diverged.');

figure('Name','PFC int32 principle time-domain verification','Color','w');
tiledlayout(4,1,'TileSpacing','compact','Padding','compact');
nexttile; plot(t,vf,t,vq,'--','LineWidth',1.05); grid on; ylabel('Vbus / V'); legend('continuous domain','I32 quantized'); title('PFC-I32 principle verification');
nexttile; plot(t,iflt,t,iq,'--','LineWidth',.8); grid on; ylabel('Grid current / A'); legend('continuous','I32');
nexttile; plot(t,dq,t,(dq-df)*1000,'LineWidth',.8); grid on; ylabel('duty / mduty'); legend('I32 duty','quantization error x1000');
nexttile; plot(t,vq-vf,t,iq-iflt,'LineWidth',.8); grid on; ylabel('quantization error'); xlabel('Time / s'); legend('Vbus error / V','current error / A');
idx=t>.72; fprintf('PFC-I32 physical PI: Kpv=%.6g, Kiv=%.6g, Kpi=%.6g, Kii=%.6g\n',Kpv,Kiv,Kpi,Kii);
fprintf('PFC-I32 quantization: Vbus RMS difference=%.5f V, current RMS difference=%.5f A, max cmp=%d\n',...
    rms(vq(idx)-vf(idx)),rms(iq(idx)-iflt(idx)),round(max(abs(dq))*pwm_reload));

function [vbus,il,duty]=run_case(qen,t,Ts,vg,Vrms,Pload,vref_cmd,L,C,Kpv,Kiv,Kpi,Kii,vacmax,vacmaxc,vbmax,vbmaxc,icpv,reload)
n=numel(t); vbus=zeros(n,1); vbus(1)=330; il=zeros(n,1); duty=zeros(n,1);
vref=330; iamp=0; iv=0; ii=0; task_count=round(.01/Ts);
for k=1:n-1
    vref=ramp_to(vref,vref_cmd(k),200*Ts);
    vgk=vg(k); vb=vbus(k); ik=il(k);
    if qen
        vgk=round(sat(vg(k)/vacmax*vacmaxc,-vacmaxc,vacmaxc))/vacmaxc*vacmax;
        vb=round(sat(vbus(k)/vbmax*vbmaxc,0,vbmaxc))/vbmaxc*vbmax;
        ik=round(sat(il(k)*icpv,-8191,8191))/icpv;
    end
    if mod(k-1,task_count)==0
        ev=vref-vb; iu=Kpv*ev+iv; iamp=sat(iu,-3,60);
        if abs(iu-iamp)<1e-12, iv=iv+Kiv*.01*ev; end
    end
    iref=iamp*230*vgk/(Vrms^2);
    ei=iref-ik; vl=sat(Kpi*ei+ii,-30,30);
    ii=ii+Kii*Ts*ei;
    d=sat((vgk-vl)/max(vb,1),-.98,.98);
    if qen, d=round(d*reload)/reload; end
    duty(k)=d;
    il(k+1)=il(k)+Ts*(vg(k)-d*vbus(k))/L;
    pin=max(vg(k)*il(k),0);
    vbus(k+1)=max(1,vbus(k)+Ts*(pin-Pload(k))/(C*max(vbus(k),50)));
end
duty(end)=duty(end-1);
end
function y=sat(x,lo,hi), y=min(max(x,lo),hi); end
function y=ramp_to(x,target,step), y=x+sat(target-x,-step,step); end
