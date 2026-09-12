function report=npc_transient_optimize(mode)
% Compare gain candidates against production eight-PI law with one sample delay.
% Average Yd plant, floating neutrals. Does not edit the PLECS model or firmware.
if nargin==0, mode='tune'; end
assert(any(strcmp(mode,{'local','wide','tune','robust','deadtime','hardware','pi_design','loss_design'})),'Unsupported analysis mode');
root=fileparts(mfilename('fullpath')); out=fullfile(root,'output',['transient_optimize_' mode]);
if ~exist(out,'dir'), mkdir(out); end
m=npc_yd_model; base=npc_yd_design_config;
% Preserve the recorded-run baseline when deployed defaults change.
base.kpv=3.76502583583044; base.kiv=652.196482352376;
base.kpi=0.339347951147684; base.kii=0.250232668115426;
base.trip_voltage=1000;
powers={[1 1 1],[1e4 1e4 1e4],[1e5 1e5 1e5],[1e6 1e6 1e6],[1e6 1 1]};
plants=cellfun(@(w)m.plant(base,m.tr,34500^2./w),powers,'UniformOutput',false);
if strcmp(mode,'robust')
    for factor=[0.9 1.1]
        q=base; q.L=base.L*factor; plants{end+1}=m.plant(q,m.tr,34500^2./[1 1 1]);
        q=base; q.C=base.C*factor; plants{end+1}=m.plant(q,m.tr,34500^2./[1 1 1]);
    end
end
% Focus on current-integrator recovery first; retain outer PI unless evidence improves it.
rows=[];
gains=[base.kpv base.kiv base.kpi base.kii];
if strcmp(mode,'loss_design')
    % Preserve the oscillating recorded-run baseline after deployment.
    gains=[3.18141815658827 3238.28511410547 .0677038431492699 .154658697372947;1 1300 .15 .05];
    base.kpv=gains(1,1);base.kiv=gains(1,2);base.kpi=gains(1,3);base.kii=gains(1,4);
elseif strcmp(mode,'pi_design')
    saved=load(fullfile(root,'output','pi_design','report_joint.mat'));
    gains=table2array(saved.report.gains([1 end],:));
    base.kpv=gains(1,1); base.kiv=gains(1,2); base.kpi=gains(1,3); base.kii=gains(1,4);
elseif strcmp(mode,'hardware')
    % Compare current deployed PI against the hardware-derived ideal-loop seed.
    deployed=npc_yd_design_config;
    wi=2*pi*600; zeta=1/sqrt(2); wn=2*pi*120/sqrt(2+sqrt(5));
    gains=[deployed.kpv deployed.kiv deployed.kpi deployed.kii; ...
        2*zeta*base.C*wn base.C*wn^2 base.L*wi base.R*wi];
    base.kpv=gains(1,1); base.kiv=gains(1,2); base.kpi=gains(1,3); base.kii=gains(1,4);
elseif strcmp(mode,'deadtime')
    previous=load(fullfile(root,'output','transient_optimize_tune','report.mat'));
    gains=table2array(previous.report.scan(1:2,1:4));
    base.dead_time=10e-6; % Sensitivity surrogate, not an exact switch/diode model.
elseif strcmp(mode,'robust')
    previous=load(fullfile(root,'output','transient_optimize_tune','report.mat'));
    gains=table2array(previous.report.scan(:,1:4));
elseif strcmp(mode,'tune')
    seeds=[0.1 10 0.1 10;1 100 0.2 50;base.kpv base.kiv base.kpi base.kii];
    opts=optimset('Display','off','MaxIter',180,'MaxFunEvals',300,'TolX',1e-4);
    for seed=1:size(seeds,1)
        [z,cost]=fminsearch(@(z)objective(z,base,plants([1 4 5])),log10(seeds(seed,:)),opts);
        gains(end+1,:)=10.^z; %#ok<AGROW>
        fprintf('PI tuning seed %d cost=%.8f gains=%s\n',seed,cost,mat2str(10.^z,9));
    end
elseif strcmp(mode,'wide')
    for kii=[1 5 20 80]
        for kpi=[0.1 0.2 0.34 0.5]
            for kpv=[0.3 1 2 4]
                for kiv=[10 50 150 400]
                    gains(end+1,:)=[kpv kiv kpi kii]; %#ok<AGROW>
                end
            end
        end
    end
else
    for kii=[base.kii 2 5 10 20 40 80 160]
        for kpi=[base.kpi 0.2 0.45]
            for vf=[0.7 1 1.3], gains(end+1,:)=[base.kpv*vf base.kiv*vf kpi kii]; end %#ok<AGROW>
        end
    end
end

for candidate=1:size(gains,1)
            p=base; p.kpv=gains(candidate,1); p.kiv=gains(candidate,2); p.kpi=gains(candidate,3); p.kii=gains(candidate,4);
            rho=zeros(1,numel(plants)); tau=rho;
            for j=1:numel(plants)
                phi=eye(44);
                for k=0:99, phi=step(eye(44),p.w0*k*p.ts,0,p,plants{j},false)*phi; end
                rr=max(abs(eig(phi))); rho(j)=rr; tau(j)=-0.02/log(rr);
            end
            rows(end+1,:)=[p.kpv p.kiv p.kpi p.kii rho tau]; %#ok<AGROW>
end
names={'kpv','kiv','kpi','kii','rho_open','rho_10k','rho_100k','rho_1M','rho_unbalanced', ...
    'tau_open','tau_10k','tau_100k','tau_1M','tau_unbalanced'};
if strcmp(mode,'robust')
    names=[names(1:9),{'rho_L90','rho_C90','rho_L110','rho_C110'},names(10:14),{'tau_L90','tau_C90','tau_L110','tau_C110'}];
    report.scan=array2table(rows,'VariableNames',names); disp(report.scan);
    writetable(report.scan,fullfile(out,'scan.csv')); return;
end
report.scan=array2table(rows,'VariableNames',names);
writetable(report.scan,fullfile(out,'scan.csv'));
ok=find(max(rows(:,5:9),[],2)<1);
[~,order]=sort(max(rows(ok,10:14),[],2));
ids=ok(order(1:min(5,numel(order))));
baseline=find(abs(rows(:,4)-base.kii)<1e-9 & abs(rows(:,3)-base.kpi)<1e-9 & abs(rows(:,1)-base.kpv)<1e-9);
baseline=baseline(1);
ids=unique([baseline;ids],'stable');
if strcmp(mode,'hardware'), ids=(1:size(gains,1))'; end % Include unstable seeds explicitly.
disp(report.scan(ids,:));
metrics=[]; case_count=2;
if any(strcmp(mode,{'pi_design','loss_design'})), case_count=5; end
report.traces=cell(numel(ids),case_count);
for n=1:numel(ids)
    row=rows(ids(n),:); p=base; p.kpv=row(1); p.kiv=row(2); p.kpi=row(3); p.kii=row(4);
    for c=1:case_count
        heavy=plants{4}; if c==2, heavy=plants{5}; end
        if c==3, heavy=plants{2}; elseif c==4, heavy=plants{3};
        elseif c==5, heavy=m.plant(base,m.tr,34500^2./[1e4 1e5 1e6]); end
        r=simulate(p,plants{1},heavy);
        report.traces{n,c}=r;
        metrics(end+1,:)=[ids(n),c,r.load_min,r.load_settle,r.unload_peak,r.unload_settle,r.ipeak,r.voltage_limited,r.current_limited,r.complete]; %#ok<AGROW>
        fprintf('id=%d case=%d load_min=%.2f settle=%.4f unload_peak=%.2f settle=%.4f Ipk=%.2f complete=%d\n', ...
            ids(n),c,r.load_min,r.load_settle,r.unload_peak,r.unload_settle,r.ipeak,r.complete);
        if ~r.complete, fprintf('  Numerical stress run stopped at %.6f s; later event metrics are unavailable.\n',r.stop_time); end
    end
end
report.metrics=array2table(metrics,'VariableNames',{'id','case_id','load_min_V','load_settle_s', ...
    'unload_peak_V','unload_settle_s','Ipeak_A','voltage_limit_fraction','current_limit_fraction','complete'});
writetable(report.metrics,fullfile(out,'transients.csv'));
save(fullfile(out,'report.mat'),'report','base','ids');
f=figure('Visible','off','Position',[0 0 1200 850]); tiledlayout(2,2);
for c=1:2
    for event=[4 7]
        nexttile; hold on;
        for n=1:numel(ids)
            r=report.traces{n,c}; ix=r.t>=event & r.t<=event+3;
            plot(r.t(ix)-event,r.envelope(ix),'DisplayName',sprintf('id %d',ids(n)));
        end
        yline(base.vpeak,'--'); grid on; legend; xlabel('Time from event (s)'); ylabel('20ms voltage RMS envelope * sqrt(2), V');
        title(sprintf('case %d event %.0f s',c,event));
    end
end
exportgraphics(f,fullfile(out,'comparison.png')); close(f);
if strcmp(mode,'hardware')
    f=figure('Visible','off','Position',[0 0 1200 700]); tiledlayout(2,2);
    for n=1:numel(ids)
        r=report.traces{n,1}; ix=r.t<=4;
        nexttile; plot(r.t(ix),r.v(ix,:)); grid on;
        title(sprintf('PI %d: startup phase voltages',ids(n))); xlabel('Time (s)'); ylabel('V');
        nexttile; plot(r.t(ix),r.i(ix,:)); grid on;
        title(sprintf('PI %d: startup inductor currents',ids(n))); xlabel('Time (s)'); ylabel('A');
    end
    exportgraphics(f,fullfile(out,'startup.png')); close(f);
end
end

function [xn,y]=step(x,theta,ref,p,g,limited)
input=struct('v_abc',p.invclarke*x(3:4,:),'i_l_abc',p.invclarke*x(1:2,:), ...
    'theta',theta,'vd_pos_ref',ref);
[sn,y]=npc_dq_control(x(11:42,:),input,p,limited);
u=[y.u_alpha;y.u_beta]; applied=x(43:44,:);
if limited && isfield(p,'dead_time')
    % First-order average error of a half-bus NPC commutation, current polarity held per sample.
    applied=applied-(p.vdc/2)*(p.dead_time/p.ts)*p.clarke*sign(input.i_l_abc);
end
xn=[g.A*x(1:10,:)+g.B*applied;sn;u]; % Exactly one control-period delay.
end

function r=simulate(p,light,heavy)
n=round(13/p.ts); x=zeros(44,1); v=zeros(n,3); i=v;
r.t=(0:n-1)'*p.ts; flags=zeros(n,2); r.complete=true;
for k=1:n
    t=r.t(k); g=light; if t>=4 && t<7, g=heavy; end
    v(k,:)=(p.invclarke*x(3:4))'; i(k,:)=(p.invclarke*x(1:2))';
    [x,y]=step(x,p.w0*t,p.vpeak*min(t/2,1),p,g,true);
    flags(k,:)=[y.voltage_limited y.current_limited];
    if any(~isfinite(x)) || max(abs(v(k,:)))>10000
        r.complete=false;
        v(k+1:end,:)=NaN; i(k+1:end,:)=NaN; flags(k+1:end,:)=NaN;
        break;
    end
end
r.stop_time=r.t(k);
r.v=v; r.i=i;
% Trailing fundamental-period RMS: no DSOGI delay hidden in this measurement.
env=sqrt(2*movmean(v.^2,[99 0],1)); r.envelope=mean(env,2);
loadidx=r.t>=4 & r.t<7; unloadidx=r.t>=7;
r.load_min=min(env(loadidx,:),[],'all'); r.unload_peak=max(abs(v(unloadidx,:)),[],'all');
r.load_settle=settling(r.t,env,p.vpeak,4,7);
r.unload_settle=settling(r.t,env,p.vpeak,7,13);
r.ipeak=max(abs(i),[],'all','omitnan'); r.voltage_limited=mean(flags(:,1),'omitnan'); r.current_limited=mean(flags(:,2),'omitnan');
end

function t=settling(time,env,ref,start,stop)
ix=time>=start & time<stop;
if any(~isfinite(env(ix,:)),'all'), t=Inf; return; end
bad=find(ix & any(abs(env-ref)>0.02*ref,2),1,'last');
t=0; if ~isempty(bad), t=time(bad)-start; end
if t>=stop-start-0.02, t=Inf; end
end

function cost=objective(z,p,plants)
if any(z < -4 | z > 4) || z(1)>log10(8), cost=1e3+sum(z.^2); return; end
q=10.^z; p.kpv=q(1); p.kiv=q(2); p.kpi=q(3); p.kii=q(4);
cost=0;
for j=1:numel(plants)
    phi=eye(44);
    for k=0:99, phi=step(eye(44),p.w0*k*p.ts,0,p,plants{j},false)*phi; end
    cost=max(cost,max(abs(eig(phi))));
end
if ~isfinite(cost), cost=1e6; end
end
