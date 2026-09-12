function report=npc_loss_pi_redesign(mode)
% Robust PI design against low-current modulation loss with uncertain lag.
% The dynamic loss is an uncertainty surrogate guided by the recorded trace,
% not a replacement for a PLECS semiconductor model.
if nargin==0,mode='inspect';end
m=npc_yd_model;p=npc_yd_design_config;root=fileparts(mfilename('fullpath'));
% Pin the oscillating recorded-run baseline independently of deployed defaults.
p=set_gains(p,[3.18141815658827 3238.28511410547 .0677038431492699 .154658697372947]);
out=fullfile(root,'output','loss_redesign');if ~exist(out,'dir'),mkdir(out);end
if strcmp(mode,'time')
    saved=load(fullfile(out,'report_validate.mat'));gains=table2array(saved.report.gains);rows=[];report.traces={};
    g=m.plant(p,m.tr,34500^2./[1 1 1]);
    for id=1:size(gains,1)
        pp=set_gains(p,gains(id,:));
        for volts=[15 35]
            for tau=[.0025 .005]
                r=nonlinear(pp,g,volts,tau);
                rows(end+1,:)=[id volts tau r.rms r.peak r.limited]; %#ok<AGROW>
                report.traces{end+1}=r;
                fprintf('nonlinear id%d Vloss%.0f lag%.4f residual%.3f peak%.3f limited%.4f\n',id,volts,tau,r.rms,r.peak,r.limited);
            end
        end
    end
    report.metrics=array2table(rows,'VariableNames',{'candidate','loss_V','lag_s','residual_V','peak_V','limited_fraction'});
    writetable(report.metrics,fullfile(out,'nonlinear.csv'));save(fullfile(out,'nonlinear.mat'),'report');return;
end
if strcmp(mode,'robust')
    pp=set_gains(p,[1 1300 .15 .05]);rows=[];
    powers={[1 1 1],[1e4 1e4 1e4],[1e5 1e5 1e5],[1e6 1e6 1e6],[1e6 1 1],[1e4 1e5 1e6]};
    for lf=[.9 1.1]
        for cf=[.9 1.1]
            pp.L=p.L*lf;pp.C=p.C*cf;
            for j=1:6
                gg=m.plant(pp,m.tr,34500^2./powers{j});
                for loss=[0 .4 1]
                    for tau=[0 .0025 .005]
                        for extra=[0 .5 1]
                            cc=loss_plant(gg,pp,loss,tau,extra);rho=max(abs(eig(matrix(pp,cc))))^100;
                            rows(end+1,:)=[lf cf j loss tau extra rho]; %#ok<AGROW>
                        end
                    end
                end
            end
        end
    end
    report=array2table(rows,'VariableNames',{'L_factor','C_factor','load','loss_ohm','lag_s','extra_delay','rho20ms'});
    disp(groupsummary(report,{'extra_delay'},'max','rho20ms'));
    writetable(report,fullfile(out,'robustness.csv'));return;
end
cases={};names=[];
for power=[1 1e6]
    g=m.plant(p,m.tr,34500^2./[power power power]);
    for loss=[0 .4 1]
        for tau=[0 .0025 .005]
            cases{end+1}=loss_plant(g,p,loss,tau,.5); %#ok<AGROW>
            names(end+1,:)=[power loss tau]; %#ok<AGROW>
        end
    end
end
gain=[p.kpv p.kiv p.kpi p.kii];
if strcmp(mode,'validate')
    saved=load(fullfile(out,'report_tune.mat'));gain=table2array(saved.report.gains);
    gain(end+1,:)=[1 1300 .15 .05];
elseif strcmp(mode,'tune')
    opts=optimset('Display','off','MaxIter',300,'MaxFunEvals',500,'TolX',1e-5);
    best=Inf;
    for seed=[gain;1 500 .15 .05;1 1000 .1 .1;.5 200 .2 .03;3 100 .05 .02]'
        [z,cost]=fminsearch(@(z)objective(z,p,cases),log10(seed'),opts);
        fprintf('Loss-robust search cost %.7f gains %s\n',cost,mat2str(10.^z,10));
        if cost<best,best=cost;q=10.^z;end
    end
    gain(end+1,:)=q;
end
rows=[];
for n=1:size(gain,1)
    pp=set_gains(p,gain(n,:));
    for j=1:numel(cases)
        A=matrix(pp,cases{j});z=eig(A);[~,ix]=max(abs(z));
        rows(end+1,:)=[n names(j,:) abs(z(ix))^100 log(abs(z(ix)))/p.ts abs(angle(z(ix)))/(2*pi*p.ts)]; %#ok<AGROW>
    end
end
report.gains=array2table(gain,'VariableNames',{'kpv','kiv','kpi','kii'});
report.stability=array2table(rows,'VariableNames',{'candidate','branch_W','loss_ohm','lag_s','rho20ms','growth_per_s','mode_Hz'});
disp(report.gains);disp(report.stability);
writetable(report.gains,fullfile(out,['gains_' mode '.csv']));
writetable(report.stability,fullfile(out,['stability_' mode '.csv']));
save(fullfile(out,['report_' mode '.mat']),'report');
end

function g=loss_plant(g,p,loss,tau,extra)
if tau==0
    A=g.Ac;A(1:2,1:2)=A(1:2,1:2)-loss/p.L*eye(2);
    A=[A zeros(10,2);zeros(2,10) -eye(2)/p.ts];B=[g.Bc;zeros(2)];
else
    A=[g.Ac -loss*g.Bc;[eye(2) zeros(2,8)]/tau -eye(2)/tau];B=[g.Bc;zeros(2)];
end
E=expm([A B;zeros(2,14)]*p.ts*extra);F=expm([A B;zeros(2,14)]*p.ts*(1-extra));
g.E=E;g.F=F;
end

function A=matrix(p,g)
x=eye(48);input=struct('v_abc',p.invclarke*x(3:4,:),'i_l_abc',p.invclarke*x(1:2,:),'theta',0,'vd_pos_ref',0);
[sn,y]=npc_dq_control(x(11:42,:),input,p,false);
xx=[x(1:10,:);x(47:48,:)];mid=g.E(1:12,1:12)*xx+g.E(1:12,13:14)*x(45:46,:);
physical=g.F(1:12,1:12)*mid+g.F(1:12,13:14)*x(43:44,:);
A=[physical(1:10,:);sn;y.u_alpha;y.u_beta;x(43:44,:);physical(11:12,:)];
d=p.w0*p.ts;r=[cos(d) -sin(d);sin(d) cos(d)];A(35:42,:)=blkdiag(r,r',r,r')*A(35:42,:);
end

function p=set_gains(p,q)
p.kpv=q(1);p.kiv=q(2);p.kpi=q(3);p.kii=q(4);
end

function cost=objective(z,p,cases)
if any(z<[-3 -1 -3 -3] | z>[2 4 0 0]),cost=100+sum(z.^2);return;end
p=set_gains(p,10.^z);cost=0;
for j=1:numel(cases)
    cost=max(cost,max(abs(eig(matrix(p,cases{j}))))^100);
    if cost>5,return;end
end
end

function r=nonlinear(p,g,volts,tau)
A=[g.Ac -volts*g.Bc;zeros(2,10) -eye(2)/tau];B=[g.Bc zeros(10,2);zeros(2) eye(2)/tau];
E=expm([A B;zeros(4,16)]*p.ts/2);F=E(1:12,1:12);U=E(1:12,13:16);
x=zeros(12,1);s=zeros(32,1);old=zeros(2,1);older=old;n=round(8/p.ts);r.t=(0:n-1)'*p.ts;r.v=zeros(n,3);flags=zeros(n,1);
for k=1:n
    v=p.invclarke*x(3:4);i=p.invclarke*x(1:2);r.v(k,:)=v';
    input=struct('v_abc',v,'i_l_abc',i,'theta',p.w0*r.t(k),'vd_pos_ref',563*min(r.t(k)/2,1));
    [s,y]=npc_dq_control(s,input,p,true);flags(k)=y.current_limited||y.voltage_limited;
    x=F*x+U*[older;p.clarke*sign(i)];
    x=F*x+U*[old;p.clarke*sign(p.invclarke*x(1:2))];
    older=old;old=[y.u_alpha;y.u_beta];
end
ix=r.t>=5;b=[ones(sum(ix),1) cos(p.w0*r.t(ix)) sin(p.w0*r.t(ix))];res=r.v(ix,:)-b*(b\r.v(ix,:));
r.rms=max(sqrt(mean(res.^2)));r.peak=max(abs(r.v(ix,:)),[],'all');r.limited=mean(flags);
end
