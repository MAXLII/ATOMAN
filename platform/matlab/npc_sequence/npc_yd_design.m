function report=npc_yd_design(mode)
% PI-only redesign for the full Yd plant. No firmware or PLECS changes.
if nargin==0, mode='verify'; end
m=npc_yd_model; p=npc_yd_design_config; tr=m.tr;
out=fullfile(fileparts(mfilename('fullpath')),'output','yd_design');
if ~exist(out,'dir'), mkdir(out); end
loads={[119025 119025 119025],[119025 11902.5 1190.25],[1000 1000 1000]};
plants=cellfun(@(r)m.plant(p,tr,r),loads,'UniformOutput',false);
if startsWith(mode,'tune')
    seeds=[0.01 0.1 0.01 1;0.1 1 0.05 10;0.1 10 0.1 100;1 50 0.01 5];
    if strcmp(mode,'tune_robust')
        seeds=[p.kpv p.kiv p.kpi p.kii;0.01 100 0.4 0.4];
        actual=p; actual.C=1.2*p.C;
        plants{end+1}=m.plant(actual,tr,loads{1});
        actual.L=1.2*p.L;
        plants{end+1}=m.plant(actual,tr,loads{1});
    end
    options=optimset('Display','off','MaxIter',220,'MaxFunEvals',400,'TolX',1e-3);
    best=inf; gains=[];
    for k=1:size(seeds,1)
        [z,cost]=fminsearch(@(z)objective(z,p,plants),log10(seeds(k,:)),options);
        fprintf('seed %d rho=%.9g gains=[%.9g %.9g %.9g %.9g]\n',k,cost,10.^z);
        if cost<best, best=cost; gains=10.^z; end
    end
    report.gains=gains; report.worst_rho=best;
    save(fullfile(out,'tuning.mat'),'report');
elseif strcmp(mode,'verify')
    q=[p.kpv p.kiv p.kpi p.kii];
    report.config=p; report.gains=q;
    report.stability=zeros(3,3);
    for k=1:3
        report.stability(k,:)=[radius(m.p,plants{k}),radius(p,plants{k}),inner_radius(p,plants{k})];
    end
    report.robustness=robustness(p,m,loads);
    fprintf('L/C/leakage robustness worst rho=%.9g\n',max(report.robustness(:,5)));
    names={'light_start','saved_load_start','unbalanced_step','load_removal'};
    starts=[1 3 1 3]; ends=[1 3 2 1]; rows=cell(4,7);
    for k=1:4
        r=simulate(p,plants{starts(k)},plants{ends(k)},tr);
        report.(names{k})=r;
        fprintf('%s Vll=[%.4f %.4f %.4f] VUF=%.5f%% Ipeak=%.3f Iref=%.3f I_lim=%d U_lim=%d\n', ...
            names{k},r.vll,r.vuf,r.peak,max(vecnorm(r.iref(:,1:2),2,2)+vecnorm(r.iref(:,3:4),2,2)),sum(r.limits(:,1)),sum(r.limits(:,2)));
        rows(k,:)={names{k},r.vll(1),r.vll(2),r.vll(3),r.vuf,r.peak,r.finite};
        writematrix([r.t r.ref r.v r.i r.vdq r.iref double(r.limits)],fullfile(out,[names{k} '.csv']));
        f=figure('Visible','off','Position',[50 50 1100 850]); tiledlayout(4,1);
        nexttile; plot(r.t,[r.vdq(:,1:2) r.ref]); grid on; ylabel('+dq (V)'); title(strrep(names{k},'_',' '));
        nexttile; plot(r.t,r.vdq(:,3:4)); grid on; ylabel('-dq (V)');
        nexttile; plot(r.t,r.i); hold on; yline(p.ipeak,'--'); yline(-p.ipeak,'--'); grid on; ylabel('Current abc (A)');
        nexttile; plot(r.t,r.v); xlim([p.duration-0.06 p.duration]); grid on; ylabel('LV abc (V)'); xlabel('Time (s)');
        exportgraphics(f,fullfile(out,[names{k} '.png']),'Resolution',120); close(f);
    end
    report.summary=cell2table(rows,'VariableNames',{'case_name','Vab_rms','Vbc_rms','Vca_rms','VUF_percent','Ipeak_A','finite'});
    limited=p; limited.ipeak=160; limited.trip_current=192; limited.guard_enabled=true;
    report.retained_160A=simulate(limited,plants{1},plants{3},tr);
    protected=p; protected.guard_enabled=true;
    report.removal_guard=simulate(protected,plants{3},plants{1},tr);
    fprintf('160 A setting: trip=%d at %.6f s, reason=%s, observed peak=%.3f A\n', ...
        report.retained_160A.tripped,report.retained_160A.trip_time,report.retained_160A.trip_reason,report.retained_160A.peak);
    fprintf('Load removal: phase-voltage peak %.3f V; guard trip at %.6f s, reason=%s\n', ...
        max(abs(report.load_removal.v),[],'all'),report.removal_guard.trip_time,report.removal_guard.trip_reason);
    writetable(report.summary,fullfile(out,'summary.csv'));
    writematrix(report.stability,fullfile(out,'stability.csv'));
    writematrix(report.robustness,fullfile(out,'robustness.csv'));
    save(fullfile(out,'analysis.mat'),'report');
    assert(all(report.stability(:,2:3)<1,'all'),'Candidate full or inner loop is unstable.');
    assert(all(report.robustness(:,5)<1),'Parameter robustness failed.');
    for k=1:3
        r=report.(names{k});
        assert(r.finite && all(abs(r.vll-p.vll)<0.02*p.vll) && r.vuf<0.5,'Voltage regulation failed.');
        assert(r.peak<p.ipeak && max(abs(r.v),[],'all')<p.trip_voltage,'Normal-case electrical budget exceeded.');
        assert(~any(r.limits(end-999:end,:),'all'),'Persistent steady saturation.');
    end
    assert(report.retained_160A.tripped && strcmp(report.retained_160A.trip_reason,'overcurrent'));
    assert(report.removal_guard.tripped && strcmp(report.removal_guard.trip_reason,'overvoltage'));
    fprintf('NORMAL LOAD CHECKS PASSED; abrupt full-load removal remains an energy-management failure.\n');
elseif strcmp(mode,'robust')
    rows=robustness(p,m,loads);
    fprintf('ROBUST worst rho=%.9g\n',max(rows(:,5)));
    report.rows=rows; save(fullfile(out,'robustness.mat'),'report');
elseif strcmp(mode,'inspect')
    for k=1:3
        rho=radius(p,plants{k});
        vc=[p.vpeak;-1j*p.vpeak]; ac=plants{k}.Ac;
        tx=(1j*p.w0*eye(6)-ac(5:10,5:10))\(ac(5:10,3:4)*vc);
        il=tx(1:2)+1j*p.w0*p.C*vc;
        required=abs((il(1)+1j*il(2))/2)+abs((conj(il(1))+1j*conj(il(2)))/2);
        fprintf('load %d fullrho=%.9g innerrho=%.9g required_sum_peak=%.6f A\n',k,rho,inner_radius(p,plants{k}),required);
    end
    report.gains=[p.kpv p.kiv p.kpi p.kii];
else
    error('Unknown mode.');
end
end

function r=simulate(p,g0,g1,tr)
n=round(p.duration/p.ts); x=zeros(44,1); r.t=(0:n-1)'*p.ts;
r.v=zeros(n,3); r.i=r.v; r.vdq=zeros(n,4); r.iref=r.vdq; r.ref=zeros(n,1); r.limits=false(n,2);
r.tripped=false; r.trip_time=NaN; r.trip_reason='none';
for k=1:n
    t=r.t(k); g=g0; if t>=p.event_time, g=g1; end
    r.v(k,:)=(p.invclarke*x(3:4))'; r.i(k,:)=(p.invclarke*x(1:2))';
    if p.guard_enabled
        if max(abs(r.i(k,:)))>p.trip_current, r.trip_reason='overcurrent'; end
        if max(abs(r.v(k,:)))>p.trip_voltage, r.trip_reason='overvoltage'; end
        if ~strcmp(r.trip_reason,'none')
            r.tripped=true; r.trip_time=t;
            break; % Stop at fault; all-gates-off diode behavior is outside this averaged model.
        end
    end
    ref=p.vpeak*min(1,t/p.ramp_time);
    [xn,y]=step(x,p.w0*t,ref,p,g,true);
    r.v(k,:)=(p.invclarke*x(3:4))'; r.i(k,:)=(p.invclarke*x(1:2))';
    r.vdq(k,:)=y.v_dq'; r.iref(k,:)=y.i_l_dq_ref'; r.ref(k)=ref;
    r.limits(k,:)=[y.current_limited y.voltage_limited]; x=xn;
end
if r.tripped
    fields={'t','v','i','vdq','iref','ref','limits'};
    for index=1:numel(fields), r.(fields{index})=r.(fields{index})(1:k,:); end
end
last=r.t>=p.duration-0.2; line=(tr.D*r.v')'; r.vll=sqrt(mean(line(last,:).^2,1));
ab=(p.clarke*r.v(last,:)')'; z=ab(:,1)+1j*ab(:,2); angle=p.w0*r.t(last);
r.vuf=100*abs(mean(z.*exp(1j*angle)))/max(abs(mean(z.*exp(-1j*angle))),eps);
r.peak=max(abs(r.i),[],'all'); r.finite=all(isfinite(x));
end

function rows=robustness(p,m,loads)
rows=zeros(81,5); index=0;
for ls=[0.8 1 1.2]
    for cs=[0.8 1 1.2]
        for leak=[0.8 1 1.2]
            actual=p; actual.L=p.L*ls; actual.C=p.C*cs;
            tr=m.tr; tr.L1=tr.L1*leak; tr.L2_ref=tr.L2_ref*leak;
            for k=1:3
                g=m.plant(actual,tr,loads{k}); index=index+1;
                rows(index,:)=[ls cs leak k radius(p,g)];
            end
        end
    end
end
end

function rho=inner_radius(p,g)
p.kpv=0; p.kiv=0; ix=[1:10 23:34 39:44]; phi=eye(numel(ix));
basis=zeros(44,numel(ix)); basis(ix,:)=eye(numel(ix));
for tick=0:99
    a=step(basis,2*pi*tick/100,0,p,g,false); phi=a(ix,:)*phi;
end
rho=max(abs(eig(phi)));
end

function cost=objective(z,p,plants)
if any(z < -5 | z > 5), cost=1e5+norm(z)^2; return; end
q=10.^z; p.kpv=q(1); p.kiv=q(2); p.kpi=q(3); p.kii=q(4);
cost=0;
for k=1:numel(plants)
    rho=radius(p,plants{k}); cost=max(cost,rho);
end
if ~isfinite(cost), cost=1e20; end
end

function [rho,mu]=radius(p,g)
phi=eye(44); n=round(p.fs/p.f0);
for k=0:n-1
    a=step(eye(44),2*pi*k/n,0,p,g,false); phi=a*phi;
end
mu=eig(phi); rho=max(abs(mu));
end

function [xn,y]=step(x,theta,reference,p,g,limited)
input.v_abc=p.invclarke*x(3:4,:); input.i_l_abc=p.invclarke*x(1:2,:);
input.theta=theta; input.vd_pos_ref=reference;
[sn,y]=npc_dq_control(x(11:42,:),input,p,limited);
xn=[g.A*x(1:10,:)+g.B*x(43:44,:);sn;y.u_alpha;y.u_beta];
end
