function report=npc_pi_design(mode)
% Hardware-derived transfer functions and exact discrete NPC loop design.
% Analysis only: production defaults, DLL and PLECS files are never changed.
if nargin==0, mode='inspect'; end
root=fileparts(mfilename('fullpath')); out=fullfile(root,'output','pi_design');
if ~exist(out,'dir'), mkdir(out); end
m=npc_yd_model; p=npc_yd_design_config;
% Historical deployed baseline used in the design report, independent of
% subsequent deployment of the selected joint_design gains.
p.kpv=0.0467584596936169; p.kiv=4078.87978020056;
p.kpi=0.0538951481241233; p.kii=0.0972071329183108;
power={[1 1 1],[1e4 1e4 1e4],[1e5 1e5 1e5],[1e6 1e6 1e6],[1e6 1 1],[1e4 1e5 1e6]};
g=cellfun(@(w)m.plant(p,m.tr,34500^2./w),power,'UniformOutput',false);
if strcmp(mode,'boundary')
    report=current_boundary(p,g,out);return;
end
if strcmp(mode,'loss')
    p=npc_yd_design_config; rows=[];
    for loss=[0 .01 .03 .1 .2 .3 .4 .5 1 2 5]
        pp=p;pp.R=m.p.R+loss;gg=m.plant(pp,m.tr,34500^2./[1 1 1]);
        poles=eig(matrix(pp,gg,false));[~,idx]=max(abs(poles));
        rows(end+1,:)=[loss abs(poles(idx))^100 log(abs(poles(idx)))/p.ts abs(angle(poles(idx)))/(2*pi*p.ts)]; %#ok<AGROW>
    end
    report=array2table(rows,'VariableNames',{'equivalent_loss_ohm','rho20ms','growth_per_s','mode_Hz'});
    disp(report);writetable(report,fullfile(out,'equivalent_loss_sensitivity.csv'));return;
end
wi=2*pi*600; wn=2*pi*120/sqrt(2+sqrt(5));
q=[p.kpv p.kiv p.kpi p.kii; sqrt(2)*p.C*wn p.C*wn^2 p.L*wi p.R*wi];
labels={'deployed','ideal_seed'};
% A stationary representation of the rotating PI integrator states removes
% the periodic coefficients exactly, even for stationary unbalanced loads.
for j=1:numel(g)
    A=matrix(p,g{j},false); phi=eye(44);
    for k=0:99, phi=raw_matrix(p,g{j},p.w0*k*p.ts)*phi; end
    relative=abs(max(abs(eig(A)))^100-max(abs(eig(phi))));
    assert(relative<1e-7,'Stationary/Floquet equivalence failed');
end
fprintf('Exact stationary/Floquet equivalence passed for all %d loads.\n',numel(g));
if strcmp(mode,'validate')
    saved=load(fullfile(out,'report_joint.mat')); q=table2array(saved.report.gains);
    labels=saved.report.gains.Properties.RowNames;
elseif strcmp(mode,'joint')
    opts=optimset('Display','off','MaxIter',400,'MaxFunEvals',650,'TolX',1e-5);
    best=Inf;
    seeds=[q;1 1500 0.14 0.19;2 650 0.33 0.25;0.05 6000 0.035 0.09];
    for n=1:size(seeds,1)
        [z,cost]=fminsearch(@(z)joint_cost(z,p,g),log10(seeds(n,:)),opts);
        fprintf('Joint cost %.6f gains %s\n',cost,mat2str(10.^z,10));
        if cost<best, best=cost; qbest=10.^z; end
    end
    q(end+1,:)=qbest; labels{end+1}='joint_design';
elseif strcmp(mode,'tune')
    opts=optimset('Display','off','MaxIter',250,'MaxFunEvals',450,'TolX',1e-5);
    best=Inf;
    for seed=[q(:,3:4);0.1 0.01;0.02 0.1;0.3 1]'
        [z,cost]=fminsearch(@(z)inner_cost(z,p,g),log10(seed'),opts);
        fprintf('Inner cost %.6f Kp %.9g Ki %.9g\n',cost,10.^z);
        if cost<best, best=cost; qi=10.^z; end
    end
    p.kpi=qi(1); p.kii=qi(2); best=Inf;
    for seed=[q(:,1:2);0.1 500;0.5 1000;0.03 2000]'
        [z,cost]=fminsearch(@(z)outer_cost(z,p,g),log10(seed'),opts);
        fprintf('Outer cost %.6f Kp %.9g Ki %.9g\n',cost,10.^z);
        if cost<best, best=cost; qv=10.^z; end
    end
    q(end+1,:)=[qv qi]; labels{end+1}='sequential_design';
end
rows=[]; f=logspace(-7,3.35,1800); bw=[]; modes=[];
for n=1:size(q,1)
    p=set_gains(p,q(n,:));
    for j=1:numel(g)
        ac=matrix(p,g{j},false); ai=matrix(p,g{j},true);
        rows(end+1,:)=[n j max(abs(eig(ai)))^100 max(abs(eig(ac)))^100]; %#ok<AGROW>
        poles=eig(ac); [~,order]=sort(abs(poles),'descend'); poles=poles(order(1:6));
        modes=[modes;repmat([n j],6,1),abs(poles),log(abs(poles))/p.ts,angle(poles)/(2*pi*p.ts)]; %#ok<AGROW>
    end
    for j=[1 4]
        response=frequency(p,g{j},f);
        lower=frequency(p,g{j},-f);
        for probe=[1 120 600]
            check_response(p,g{j},probe);
        end
        writetable(array2table([f' abs(response.Ti)' abs(response.Tv)' ...
            abs(lower.Ti)' abs(lower.Tv)' ...
            abs(response.Li)' angle(response.Li)'*180/pi abs(response.Lv)' angle(response.Lv)'*180/pi], ...
            'VariableNames',{'Hz','Ti_raw','Tv_raw','Ti_lower','Tv_lower','Li_abs','Li_phase_deg','Lv_abs','Lv_phase_deg'}), ...
            fullfile(out,sprintf('freq_%s_load%d.csv',labels{n},j)));
        b=[bandwidth(f,response.Ti),bandwidth(f,lower.Ti),bandwidth(f,response.Tv),bandwidth(f,lower.Tv)];
        bw(end+1,:)=[n j b]; %#ok<AGROW>
        fprintf('%s load%d: upper/lower raw BW %s Hz\n',labels{n},j,mat2str(b,6));
    end
end

function check_response(p,g,f)
% Independently compare the scalar transfer-function algebra to the full
% real 44-state implementation for a circular positive-sequence reference.
z=exp(1i*2*pi*(f+p.f0)*p.ts); d=p.w0*p.ts;
rr=[cos(d) -sin(d);sin(d) cos(d)]; J=eye(44);J(35:42,35:42)=blkdiag(rr,rr',rr,rr');
for inner=[false true]
    B=zeros(44,2);
    if inner
        B(39:40,:)=p.kii*p.ts*eye(2); B(43:44,:)=p.kpi*eye(2); index=1:2;
    else
        B(35:36,:)=p.kiv*p.ts*eye(2); B(39:40,:)=p.kii*p.ts*p.kpv*eye(2);
        B(43:44,:)=p.kpi*p.kpv*eye(2); index=3:4;
    end
    B=J*B; A=matrix(p,g,inner);
    if inner, B=B([1:34 39:44],:); end
    X=(z*eye(size(A))-A)\(B*[1;-1i]/2);
    value=[1 1i]*X(index); r=frequency(p,g,f);
    expected=r.Tv; if inner, expected=r.Ti; end
    assert(abs(value-expected)<1e-6*max(1,abs(value)),'Transfer/state mismatch');
end
end

function c=joint_cost(z,p,g)
if any(z < -5 | z > 5), c=1e3+sum(z.^2); return; end
g=g(1:5); % Mixed 10/100/1000 kW load is reserved for validation, not tuning.
p=set_gains(p,10.^z);rho=0;
for j=1:numel(g), rho=max(rho,max(abs(eig(matrix(p,g{j},false))))^100); end
if rho>=1, c=100+rho; return; end
c=rho;
for j=[1 4]
    r=frequency(p,g{j},[10 30 60 120 300 600 1000]);
    target=1./sqrt(1+([10 30 60 120 300 600 1000]/120).^2);
    c=c+0.02*mean(log(max(abs(r.Tv),1e-9)./target).^2);
end
end
report.gains=array2table(q,'VariableNames',{'kpv','kiv','kpi','kii'},'RowNames',labels);
report.stability=array2table(rows,'VariableNames',{'candidate','load','rho_inner_20ms','rho_full_20ms'});
report.bandwidth=array2table(bw,'VariableNames',{'candidate','load','inner_upper_Hz','inner_lower_Hz','outer_upper_Hz','outer_lower_Hz'});
report.modes=array2table(modes,'VariableNames',{'candidate','load','pole_abs','growth_per_s','stationary_Hz'});
disp(report.gains); disp(report.stability);
writetable(report.gains,fullfile(out,['gains_' mode '.csv']),'WriteRowNames',true);
writetable(report.stability,fullfile(out,['stability_' mode '.csv']));
writetable(report.bandwidth,fullfile(out,['bandwidth_' mode '.csv']));
writetable(report.modes,fullfile(out,['modes_' mode '.csv']));
if strcmp(mode,'validate')
    robust=[];
    for n=[1 size(q,1)]
        for lf=[0.9 1 1.1]
            for cf=[0.9 1 1.1]
                for rf=[0.8 1.2]
                    pp=set_gains(p,q(n,:));pp.L=m.p.L*lf;pp.C=m.p.C*cf;pp.R=m.p.R*rf;
                    for j=1:numel(g)
                        gg=m.plant(pp,m.tr,34500^2./power{j});
                        for extra=[0 0.5 1]
                            a=delayed_matrix(pp,gg,extra);
                            robust(end+1,:)=[n lf cf rf j extra max(abs(eig(a)))^100]; %#ok<AGROW>
                        end
                    end
                end
            end
        end
    end
    report.robust=array2table(robust,'VariableNames',{'candidate','L_factor','C_factor','R_factor','load','extra_delay','rho_20ms'});
    writetable(report.robust,fullfile(out,'robustness.csv'));
    disp(groupsummary(report.robust,{'candidate','extra_delay'},'max','rho_20ms'));
    rounded=double(single(q(end,:))); pp=set_gains(p,rounded); report.float32_gains=rounded;
    report.float32_rho=zeros(1,numel(g));
    for j=1:numel(g), report.float32_rho(j)=max(abs(eig(matrix(pp,g{j},false))))^100; end
    assert(all(report.float32_rho<1),'Float32 gain rounding destabilized the candidate');
    fig=figure('Visible','off','Position',[0 0 1250 850]);tiledlayout(2,2);
    for j=[1 4]
        for loop={'Ti','Tv'}
            nexttile;hold on;
            for n=[1 size(q,1)]
                pp=set_gains(p,q(n,:)); upper=frequency(pp,g{j},f);lower=frequency(pp,g{j},-f);
                uv=upper.(loop{1});lv=lower.(loop{1});
                semilogx(f,20*log10(abs(uv)),'DisplayName',[labels{n} ' upper']);
                semilogx(f,20*log10(abs(lv)),'--','DisplayName',[labels{n} ' lower']);
            end
            set(gca,'XScale','log');xlim([1e-4 2000]);ylim([-60 30]);yline(-3,':');
            grid on;legend('Location','southwest');xlabel('dq perturbation frequency (Hz)');ylabel('Raw physical tracking gain (dB)');
            title(sprintf('Load %d, %s',j,loop{1}));
        end
    end
    exportgraphics(fig,fullfile(out,'closed_loop_frequency.png'));close(fig);
end
save(fullfile(out,['report_' mode '.mat']),'report','p','power');
end

function A=delayed_matrix(p,g,extra)
% A full prior command is always present. Add 0..1 sample latch latency,
% then use exact ZOH for each fractional interval (no duplicate ZOH delay).
x=eye(46);a=expm([g.Ac g.Bc;zeros(2,12)]*(extra*p.ts));
b=expm([g.Ac g.Bc;zeros(2,12)]*((1-extra)*p.ts));
inp=struct('v_abc',p.invclarke*x(3:4,:),'i_l_abc',p.invclarke*x(1:2,:),'theta',0,'vd_pos_ref',0);
[sn,y]=npc_dq_control(x(11:42,:),inp,p,false);
mid=a(1:10,1:10)*x(1:10,:)+a(1:10,11:12)*x(45:46,:);
A=[b(1:10,1:10)*mid+b(1:10,11:12)*x(43:44,:);sn;y.u_alpha;y.u_beta;x(43:44,:)];
d=p.w0*p.ts; r=[cos(d) -sin(d);sin(d) cos(d)];
A(35:42,:)=blkdiag(r,r',r,r')*A(35:42,:);
end

function p=set_gains(p,q)
p.kpv=q(1);p.kiv=q(2);p.kpi=q(3);p.kii=q(4);
end

function A=raw_matrix(p,g,theta)
x=eye(44); inp=struct('v_abc',p.invclarke*x(3:4,:), ...
    'i_l_abc',p.invclarke*x(1:2,:),'theta',theta,'vd_pos_ref',0);
[sn,y]=npc_dq_control(x(11:42,:),inp,p,false);
A=[g.A*x(1:10,:)+g.B*x(43:44,:);sn;y.u_alpha;y.u_beta];
end

function A=matrix(p,g,inner)
if inner, p.kpv=0; p.kiv=0; end
A=raw_matrix(p,g,0); d=p.w0*p.ts; r=[cos(d) -sin(d);sin(d) cos(d)];
A(35:42,:)=blkdiag(r,r',r,r')*A(35:42,:);
if inner, keep=[1:34 39:44]; A=A(keep,keep); end
end

function c=inner_cost(z,p,g)
if any(z < -5 | z > 3), c=1e3+sum(z.^2); return; end
p.kpi=10^z(1);p.kii=10^z(2); rho=0;
for j=1:numel(g), rho=max(rho,max(abs(eig(matrix(p,g{j},true))))^100); end
% First require damping across all loads; frequency target is a soft objective.
if rho>=1, c=100+rho; return; end
r=frequency(p,g{4},600);
c=rho+0.02*log(max(abs(r.Ti),1e-9)*sqrt(2))^2;
end

function c=outer_cost(z,p,g)
if any(z < -5 | z > 5), c=1e3+sum(z.^2); return; end
p.kpv=10^z(1);p.kiv=10^z(2);rho=0;
for j=1:numel(g), rho=max(rho,max(abs(eig(matrix(p,g{j},false))))^100); end
if rho>=1, c=100+rho; return; end
r=frequency(p,g{4},120);
c=rho+0.02*log(max(abs(r.Tv),1e-9)*sqrt(2))^2;
end

function r=frequency(p,g,f)
% Balanced load: scalar complex alpha+j beta plant and both sequence paths.
r.Ti=zeros(size(f));r.Tv=r.Ti;r.Li=r.Ti;r.Lv=r.Ti;
for k=1:numel(f)
    z=exp(1i*2*pi*(f(k)+p.f0)*p.ts); d=p.w0*p.ts;
    X=(z*eye(10)-g.A)\g.B; Gi=X(1,1); Gv=X(3,1);
    c=p.sogi; den=1+c(2)/z+c(3)/z^2;
    D=c(1)*(1-1/z^2)/den; Q=c(4)*(1+2/z+1/z^2)/den;
    Fp=(D+1i*Q)/2; Fn=(D-1i*Q)/2;
    Cip=p.kpi+p.kii*p.ts/(z*exp(-1i*d)-1); Cin=p.kpi+p.kii*p.ts/(z*exp(1i*d)-1);
    Cvp=p.kpv+p.kiv*p.ts/(z*exp(-1i*d)-1); Cvn=p.kpv+p.kiv*p.ts/(z*exp(1i*d)-1);
    Li=Gi/z*(Cip*Fp+Cin*Fn);
    Lv=Gv/z*(Cip*Cvp*Fp+Cin*Cvn*Fn)/(1+Li);
    r.Li(k)=Li;r.Lv(k)=Lv;
    r.Ti(k)=Gi/z*Cip/(1+Li);
    r.Tv(k)=Gv/z*Cip*Cvp/((1+Li)*(1+Lv));
end
end

function b=bandwidth(f,t)
ix=find(abs(t)<abs(t(1))/sqrt(2),1); b=NaN;
if ~isempty(ix), b=f(ix); end
end

function report=current_boundary(p,g,out)
% Necessary DC bound is independent of Kp. Trace the positive-gain current
% loop stability region for the nominal light-load plant before any outer PI.
X=(eye(10)-g{1}.A)\g{1}.B; gi_dc=X(1,1);
delta=p.w0*p.ts; k=sqrt(2);
ki_dc=1/(gi_dc*k*p.ts/2*cot(delta/2));
fprintf('DC current plant gain %.12g A/V; necessary Ki < %.12g\n',gi_dc,ki_dc);
ki=linspace(1e-5,ki_dc*(1-1e-5),121); kp=logspace(-6,0.3,121);
rows=[]; response_max=0;
for n=1:numel(ki)
    pp=p;pp.kii=ki(n); stable=false(size(kp));
    for j=1:numel(kp)
        pp.kpi=kp(j); stable(j)=max(abs(eig(matrix(pp,g{1},true))))<1;
    end
    inds=find(stable);
    if isempty(inds), continue; end
    j=inds(end);assert(j<numel(kp),'Kp scan must include an unstable upper endpoint');
    lo=kp(j);hi=kp(j+1);
    for iteration=1:28
        pp.kpi=(lo+hi)/2;
        if max(abs(eig(matrix(pp,g{1},true))))<1,lo=pp.kpi;else,hi=pp.kpi;end
    end
    pp.kpi=lo*(1-1e-5);
    r1=frequency(pp,g{1},600);r4=frequency(pp,g{4},600);
    A=matrix(pp,g{1},true); poles=eig(A);[~,order]=sort(abs(poles),'descend');
    peak=max(abs([r1.Ti r4.Ti]));response_max=max(response_max,peak);
    rows(end+1,:)=[ki(n),kp(inds(1)),lo,abs(r1.Ti),abs(r4.Ti),angle(poles(order(1)))/(2*pi*p.ts)]; %#ok<AGROW>
end
report.ki_dc=ki_dc;report.gi_dc=gi_dc;
report.boundary=array2table(rows,'VariableNames',{'kii','kpi_low_grid','kpi_high','Ti600_light','Ti600_heavy','boundary_mode_Hz'});
writetable(report.boundary,fullfile(out,'current_stability_boundary.csv'));
fprintf('Numerical positive-gain boundary: max Kp %.12g; max |Ti(600)| %.12g\n',max(rows(:,3)),response_max);
% Verify the DC boundary on either side using the exact state matrix.
for factor=[0.99 1.01]
    pp=p;pp.kpi=0.1;pp.kii=ki_dc*factor;
    fprintf('DC bound factor %.3f: max real pole %.12g\n',factor,max(real(eig(matrix(pp,g{1},true)))));
end
save(fullfile(out,'current_stability_boundary.mat'),'report');
fig=figure('Visible','off','Position',[0 0 1100 450]);tiledlayout(1,2);
nexttile;plot(rows(:,1),rows(:,3));grid on;xlabel('Current Ki');ylabel('Upper stable current Kp');
title('Nominal light-load current loop, one-sample delay');xline(ki_dc,'--','DC bound');
nexttile;plot(rows(:,1),20*log10(rows(:,4:5)));grid on;xlabel('Current Ki');ylabel('|Ti(600 Hz)| (dB)');
yline(-3,'--','Target');legend('light load','3 MW','Location','best');
exportgraphics(fig,fullfile(out,'current_stability_boundary.png'));close(fig);
end
