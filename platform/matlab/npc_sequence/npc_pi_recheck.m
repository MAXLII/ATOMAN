function report=npc_pi_recheck(mode)
% Read-only firmware audit: balanced Yd loads, timing and PWM sensitivity.
% Does not alter firmware, DLL or the PLECS model. No controller retuning.
root=fileparts(mfilename('fullpath')); out=fullfile(root,'output','pi_recheck_delay1');
if nargin==0, mode='delay1'; end
assert(any(strcmp(mode,{'delay1','saved_5k'})),'Unknown analysis mode');
if strcmp(mode,'saved_5k'), out=fullfile(root,'output','pi_recheck_saved_5k'); end
if ~exist(out,'dir'), mkdir(out); end
m=npc_yd_model; candidate=npc_yd_design_config;
candidate.grounded=true; candidate.check_duration=6;
if strcmp(mode,'saved_5k'), candidate.grounded=false; candidate.check_duration=12; end
old=candidate; old.kpv=m.p.kpv; old.kiv=m.p.kiv; old.kpi=m.p.kpi; old.kii=m.p.kii;
% Both gain sets use the same 5200 A limit and 2 s ramp to isolate PI changes.
configs={old,candidate}; names={'old','current'}; powers=[1e4 1e5 1e6];
if strcmp(mode,'saved_5k'), powers=5e3; end
rows=cell(0,6); report=struct;
for c=1:2
    p=configs{c};
    for power=powers
        g=m.plant(p,m.tr,repmat(34500^2/power,1,3));
        for delay=1 % Required one-sample computational delay in both plant models.
            phi=eye(44);
            for k=0:99
                a=average_step(eye(44),p.w0*k*p.ts,0,p,g,delay,false);
                phi=a*phi;
            end
            rho=max(abs(eig(phi)));
            rows(end+1,:)={names{c},power,delay,rho,rho^(1/100),rho<1}; %#ok<AGROW>
        end
    end
end
report.stability=cell2table(rows,'VariableNames', ...
    {'PI','branch_power_W','delay_samples','rho_20ms','rho_per_sample','stable'});
disp(report.stability); writetable(report.stability,fullfile(out,'stability.csv'));
% Ideal averages and actual center-aligned P/O/N pulses at the same sample grid.
% PWM has no dead time; zero-sequence LC/primary leakage branch is included.
cases={ 'old_avg_10k',old,1e4,1,0; 'current_avg_10k',candidate,1e4,1,0; ...
    'current_avg_100k',candidate,1e5,1,0; 'current_pwm_10k',candidate,1e4,1,1; ...
    'current_pwm_100k',candidate,1e5,1,1; ...
    'current_avg_1M',candidate,1e6,1,0; 'current_pwm_1M',candidate,1e6,1,1};
if strcmp(mode,'saved_5k')
    cases={'current_avg_5k',candidate,5e3,1,0; 'current_pwm_5k',candidate,5e3,1,1};
end
summary=cell(size(cases,1),10);
for j=1:size(cases,1)
    label=cases{j,1}; p=cases{j,2}; power=cases{j,3}; delay=cases{j,4}; switched=cases{j,5};
    g=m.plant(p,m.tr,repmat(34500^2/power,1,3));
    r=trace(p,g,m.tr,delay,switched);
    report.(label)=r;
    summary(j,:)={label,r.vll(1),r.vll(2),r.vll(3),r.vpeak,r.ipeak,r.trip676,r.trip1000,r.completed,r.v0peak};
    fprintf('%s Vll=%g/%g/%g Vpk=%g Ipk=%g trip676=%g trip1000=%g completed=%d\n', ...
        label,r.vll,r.vpeak,r.ipeak,r.trip676,r.trip1000,r.completed);
    writematrix([r.t r.ref r.v r.i r.dq r.u],fullfile(out,[label '.csv']));
end
report.summary=cell2table(summary,'VariableNames',{'case_name','Vab','Vbc','Vca', ...
    'Vpeak','Ipeak','first_676_s','first_1000_s','completed','Vzero_peak'});
writetable(report.summary,fullfile(out,'summary.csv')); save(fullfile(out,'analysis.mat'),'report');
f=figure('Visible','off','Position',[80 80 1150 850]); tiledlayout(ceil(size(cases,1)/2),2);
for j=1:size(cases,1)
    r=report.(cases{j,1}); nexttile;
    plot(r.t,r.dq(:,1),r.t,r.ref,'--'); hold on; plot(r.t,r.dq(:,2)); grid on;
    title(strrep(cases{j,1},'_',' ')); xlabel('Time (s)'); ylabel('Voltage (V)');
    legend('d+','reference','q+','Location','best');
end
exportgraphics(f,fullfile(out,'pi_comparison.png'),'Resolution',130); close(f);
end

function [xn,y]=average_step(x,theta,ref,p,g,delay,limited)
input=struct('v_abc',p.invclarke*x(3:4,:),'i_l_abc',p.invclarke*x(1:2,:), ...
    'theta',theta,'vd_pos_ref',ref);
[sn,y]=npc_dq_control(x(11:42,:),input,p,limited);
u=[y.u_alpha;y.u_beta]; applied=u; if delay, applied=x(43:44,:); end
xn=[g.A*x(1:10,:)+g.B*applied;sn;u];
end

function r=trace(p,g,tr,delay,switched)
n=round(p.check_duration/p.ts); x=zeros(44,1); z=zeros(3,1);
r.t=(0:n-1)'*p.ts; r.ref=p.vpeak*min(1,r.t/p.ramp_time);
r.v=zeros(n,3); r.i=r.v; r.dq=zeros(n,4); r.u=zeros(n,2);
r.trip676=NaN; r.trip1000=NaN; r.completed=true; r.v0peak=0;
% Grounded 3-leg core has zero common core voltage; primary leakage carries i0.
az=[-p.R/p.L -1/p.L 0;1/p.C 0 -1/p.C;0 1/tr.L1 -(tr.Rwire+tr.R1)/tr.L1];
bz=[1/p.L;0;0];
for k=1:n
    r.v(k,:)=(p.invclarke*x(3:4)+z(2))'; r.i(k,:)=(p.invclarke*x(1:2)+z(1))';
    vp=max(abs(r.v(k,:))); r.v0peak=max(r.v0peak,abs(z(2)));
    if vp>676.059 && isnan(r.trip676), r.trip676=r.t(k); end
    if vp>1000 && isnan(r.trip1000), r.trip1000=r.t(k); end
    if ~all(isfinite(x)) || vp>1e4 || max(abs(r.i(k,:)))>2e4
        r.completed=false; break;
    end
    [xn,y]=average_step(x,p.w0*r.t(k),r.ref(k),p,g,delay,true);
    r.dq(k,:)=y.v_dq'; r.u(k,:)=[y.u_alpha y.u_beta];
    if switched
        u=r.u(k,:)'; if delay, u=x(43:44); end
        [xphysical,z]=pwm_period(x(1:10),z,u,p,g,az,bz);
        xn(1:10)=xphysical;
    end
    x=xn;
end
fields={'t','ref','v','i','dq','u'};
for j=1:numel(fields), r.(fields{j})=r.(fields{j})(1:k,:); end
last=r.t>=r.t(end)-0.2;
if r.completed, r.vll=sqrt(mean(((tr.D*r.v(last,:)')').^2,1)); else, r.vll=[NaN NaN NaN]; end
r.vpeak=max(abs(r.v),[],'all'); r.ipeak=max(abs(r.i),[],'all');
end

function [x,z]=pwm_period(x,z,u,p,g,az,bz)
% Exact edge integration: no fixed microstep quantization or dead-time approximation.
v=p.invclarke*u; h=p.vdc/2; pos=v>0;
if all(v==0), pole=v; else
    lower=-h-v; upper=-v; lower(pos)=-v(pos); upper(pos)=h-v(pos);
    assert(max(lower)<=min(upper)+1e-6,'Infeasible modulator voltage');
    pole=v+(max(lower)+min(upper))/2;
end
dp=max(pole,0)/h; dn=max(-pole,0)/h;
edges=unique([0;1;(1-dp)/2;(1+dp)/2;dn/2;1-dn/2]);
for j=1:numel(edges)-1
    dt=(edges(j+1)-edges(j))*p.ts; if dt<=0, continue; end
    t=(edges(j+1)+edges(j))/2;
    level=h*(double(abs(t-0.5)<dp/2)-double(t<dn/2 | t>1-dn/2));
    % Eigendecomposition cached per plant; exact propagation at arbitrary PWM edges.
    x=propagate(g.Ac,g.Bc,x,p.clarke*level,dt,1);
    if p.grounded, z=propagate(az,bz,z,mean(level),dt,2); end
end
end

function x=propagate(a,b,x,u,dt,slot)
persistent cache
if isempty(cache), cache=cell(1,2); end
if isempty(cache{slot}) || ~isequal(cache{slot}.a,a)
    [v,d]=eig(a); cache{slot}=struct('a',a,'v',v,'iv',inv(v),'lambda',diag(d),'ab',a\b);
end
c=cache{slot}; equilibrium=c.ab*u;
x=real(c.v*(exp(c.lambda*dt).*(c.iv*(x+equilibrium)))-equilibrium);
end
