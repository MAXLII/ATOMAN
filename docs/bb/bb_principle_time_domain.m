%% 四开关 Buck-Boost 多模式原理性时域仿真
% 对应 code/ctrl/bb：电压外环、电感电流内环、Buck/BB/Boost 模式切换。
% 注意：hw_params.h 中 BB 的 L/C 目前为 0；本文件只能采用显式的原理仿真参数。
clear; clc; close all;
warning('BB hardware L/C are zero in hw_params.h; using simulation-only L=200 uH, Cout=2 mF.');

%% 1. 原理仿真参数和按代码公式设计的 PI
fs=100e3; Ts=1/fs; L=200e-6; Cout=2e-3;
wc_v=2*pi*500; pm_v=deg2rad(60);
Kpv=sin(pm_v)*wc_v*Cout; Kiv=Kpv*wc_v/tan(pm_v);
wc_i=2*pi*2500; pm_i=deg2rad(45);
Kpi=sin(pm_i)*wc_i*L; Kii=Kpi*wc_i/tan(pm_i);
Imax=50; duty_max=0.97;

%% 2. 一个连续时间线覆盖 Buck、Buck-Boost、Boost 和轻载 DCM
t_end=.24; t=(0:Ts:t_end).'; n=numel(t);
vin=48*ones(n,1);
vin(t>=.070)=36;                       % 进入近等压 Buck-Boost 区
vin(t>=.140)=24;                       % 进入 Boost 区
vin(t>=.200)=48;                       % 返回 Buck 区
vref_cmd=36*ones(n,1);
vref_cmd(t>=.105)=42;
vref_cmd(t>=.170)=36;
Rload=2.5*ones(n,1);                   % 正常负载
Rload(t>=.190)=60;                     % <30 W，进入 DCM 原理区
vref=zeros(n,1); for k=2:n, vref(k)=ramp_to(vref(k-1),vref_cmd(k),300*Ts); end

vout=zeros(n,1); il=zeros(n,1); iref=zeros(n,1);
db=zeros(n,1); ds=zeros(n,1); mode=zeros(n,1); dcm=false(n,1);
iv=0; ii=0; vout(1)=.1;
for k=1:n-1
    p_est=max(vout(k)^2/Rload(k),0);
    if k==1, dcm(k)=false; else, dcm(k)=dcm(k-1); end
    % 启动阶段不能仅凭输入功率小就判成 DCM；先建立输出电压。
    dcm_allowed=(t(k)>.02) && (vout(k)>.8*vref(k));
    if ~dcm(k) && dcm_allowed && p_est<30
        dcm(k)=true;
    elseif dcm(k) && (p_est>50 || ~dcm_allowed)
        dcm(k)=false;
    end
    ev=vref(k)-vout(k); iu=Kpv*ev+iv; iref(k)=sat(iu,0,Imax);
    if abs(iu-iref(k))<1e-12, iv=iv+Kiv*Ts*ev; end
    ei=iref(k)-il(k); vl=sat(Kpi*ei+ii,-50,50);
    if abs(vl-(Kpi*ei+ii))<1e-12, ii=ii+Kii*Ts*ei; end
    gain=vout(k)/max(vin(k),.1);
    if gain<.85, mode(k)=1; elseif gain>1.17, mode(k)=3; else, mode(k)=2; end
    if mode(k)==1                    % Buck: vL=db*Vin-Vout
        db(k)=sat((vl+vout(k))/max(vin(k),1),0,duty_max); ds(k)=1;
    elseif mode(k)==3                % Boost: vL=Vin-ds*Vout
        db(k)=1; ds(k)=sat((vin(k)-vl)/max(vout(k),1),0,duty_max);
    else                             % 双桥共同调制
        db(k)=duty_max;
        ds(k)=sat((vin(k)*db(k)-vl)/max(vout(k),1),0,duty_max);
    end
    vl_act=db(k)*vin(k)-ds(k)*vout(k);
    % DCM 原理模型：轻载时负电感电流被钳在 0，模式标志仍由 30/50 W 迟滞产生。
    il(k+1)=max(0,il(k)+Ts*vl_act/L);
    vout(k+1)=max(0,vout(k)+Ts*(ds(k)*il(k)-vout(k)/Rload(k))/Cout);
    if k<n, dcm(k+1)=dcm(k); end
end
iref(end)=iref(end-1); db(end)=db(end-1); ds(end)=ds(end-1); mode(end)=mode(end-1);
assert(all(isfinite([vout;il;db;ds])),'BB simulation diverged.');

%% 3. 结果
figure('Name','BB principle time-domain verification','Color','w');
tiledlayout(4,1,'TileSpacing','compact','Padding','compact');
nexttile; plot(t,vin,t,vref,'--',t,vout,'LineWidth',1.05); grid on;
ylabel('Voltage / V'); legend('Vin','Vref','Vout','Location','best'); title('Four-switch Buck-Boost');
nexttile; plot(t,iref,'--',t,il,'LineWidth',1.05); grid on; ylabel('Current / A'); legend('Iref','IL');
nexttile; plot(t,db,t,ds,'LineWidth',1.05); grid on; ylabel('Duty'); legend('buck bridge','boost bridge'); ylim([0 1.05]);
nexttile; stairs(t,mode,'LineWidth',1.05); hold on; stairs(t,1+3*double(dcm),'--'); grid on;
yticks([1 2 3 4]); yticklabels({'Buck','BB','Boost','DCM flag'}); xlabel('Time / s');
fprintf('BB simulation-only PI: Kpv=%.6g, Kiv=%.6g, Kpi=%.6g, Kii=%.6g\n',Kpv,Kiv,Kpi,Kii);
fprintf('BB: max Vout=%.3f V, max current=%.3f A, duty=(%.3f, %.3f)\n',max(vout),max(il),max(db),max(ds));

function y=sat(x,lo,hi), y=min(max(x,lo),hi); end
function y=ramp_to(x,target,step), y=x+sat(target-x,-step,step); end
