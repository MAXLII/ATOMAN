function report=npc_yd_load_analysis()
% Saved npc.plecs load reproduction: LC -> Yd150 transformer -> delta resistors.
% Uses the unchanged npc_dq_control.m. This is an averaged bridge model, not PLECS co-simulation.
% Reference: https://docs.plexim.com/plecs/latest/components-by-category/trafo3ph2wdg/
root=fileparts(mfilename('fullpath')); out=fullfile(root,'output','yd_load');
if ~exist(out,'dir'), mkdir(out); end
p=npc_dq_config;
p.R=0.002; % NPC3 internal 1 mOhm plus external R9/R11/R10 = 1 mOhm.
tr.L1=17.93e-6; tr.L2=0.13448;
tr.R1=0.0009522; tr.R2=7.1415;
tr.Rwire=0.001; % R6/R7/R8, between the capacitor node and primary terminals.
tr.n=50*sqrt(3); % Secondary/primary WINDING turns ratio.
tr.a=tr.n/sqrt(3); % Secondary/primary LINE voltage ratio, 50.
tr.Lm=0.06062; tr.Rfe=95.22; % im=1, phim=0.06062: linear magnetizing inductance.
tr.lag=150*pi/180; % Secondary positive-sequence phase lag.
tr.Q=[cos(tr.lag) sin(tr.lag);-sin(tr.lag) cos(tr.lag)];
tr.D=[1 -1 0;0 1 -1;-1 0 1]; % R2=AB, R3=BC, R4=CA (polarity irrelevant to resistors).
tr.R2_ref=tr.R2/tr.n^2; tr.L2_ref=tr.L2/tr.n^2;
power=[1e4 1e5 1e6]; high_vll=p.vll*tr.a;
rd=high_vll^2./power;
names={'saved_1kohm','rated_power_unbalance'};
loads={[1000 1000 1000],rd};
report.p=p; report.transformer=tr; report.rated_delta_resistance=rd;
fprintf('Yd lag=150 deg, line ratio=%.6g, HV target=%.3f V RMS\n',tr.a,high_vll);
fprintf('Saved delta R=1000/1000/1000 ohm; nominal branch power=%.3f kW each\n',high_vll^2/1000/1000);
fprintf('10kW/100kW/1MW delta R=%.6f %.6f %.6f ohm\n',rd);
light=plant(p,tr,repmat(rd(1),1,3));
[~,report.light_load_rho]=floquet(p,light);
fprintf('Balanced 10 kW/branch transformer baseline rho=%.9g\n',report.light_load_rho);
rows=cell(2,10);
for scenario=1:2
    g1=plant(p,tr,loads{scenario});
    if scenario==1
        g0=g1; % Saved model: all three 1 kOhm resistors are present at startup.
    else
        g0=plant(p,tr,repmat(rd(1),1,3)); % Balanced 10 kW/branch, then specified imbalance at 2 s.
    end
    % Small-signal model: physical(10), actual controller(32), computation delay(2).
    [mu,rho]=floquet(p,g1);
    count=round(p.duration/p.ts); x=zeros(44,1); t=(0:count-1)'*p.ts;
    primary=zeros(count,3); current=primary; secondary=primary; power_trace=primary;
    dq=zeros(count,4); iref=dq; command=zeros(count,2); limits=false(count,2);
    for k=1:count
        g=g0; if t(k)>=2, g=g1; end
        ref=p.vpeak; if k==1, ref=0; end % Same direct command behavior as the PLECS platform.
        [xn,y]=step(x,p.w0*t(k),ref,p,g,true);
        primary(k,:)=(p.invclarke*x(3:4))'; current(k,:)=(p.invclarke*x(1:2))';
        % Secondary terminal virtual phase voltages; only their line differences are physical.
        vh=tr.a*tr.Q*g.Zload*x(7:8);
        secondary(k,:)=(tr.D*p.invclarke*vh)';
        power_trace(k,:)=secondary(k,:).^2./g.resistance;
        dq(k,:)=y.v_dq'; iref(k,:)=y.i_l_dq_ref';
        command(k,:)=[y.u_alpha y.u_beta]; limits(k,:)=[y.current_limited y.voltage_limited];
        x=xn;
    end
    final=t>=p.duration-0.2;
    low_line=(tr.D*primary')';
    lrms=sqrt(mean(low_line(final,:).^2,1)); hrms=sqrt(mean(secondary(final,:).^2,1));
    watts=mean(power_trace(final,:),1); peak=max(abs(current),[],'all');
    ab=(p.clarke*primary(final,:)')'; z=ab(:,1)+1j*ab(:,2); ang=p.w0*t(final);
    vuf=100*abs(mean(z.*exp(1j*ang)))/max(abs(mean(z.*exp(-1j*ang))),eps);
    fprintf('%s rho=%.8g LV_rms=[%.3f %.3f %.3f] HV_rms=[%.3f %.3f %.3f]\n',names{scenario},rho,lrms,hrms);
    fprintf('  actual_load_kW=[%.3f %.3f %.3f] Ipeak=%.3f A VUF=%.5f%% I_lim=%d U_lim=%d\n', ...
        watts/1000,peak,vuf,sum(limits(:,1)),sum(limits(:,2)));
    r=struct('t',t,'primary',primary,'secondary_line',secondary,'current',current, ...
        'power',power_trace,'dq',dq,'iref',iref,'command',command,'limits',limits, ...
        'rho',rho,'multipliers',mu,'LV_rms',lrms,'HV_rms',hrms,'mean_power_W',watts,'VUF_percent',vuf);
    report.(names{scenario})=r;
    rows(scenario,:)={names{scenario},rho,lrms(1),lrms(2),lrms(3),peak,vuf, ...
        sum(limits(:,1)),sum(limits(:,2)),all(isfinite(x))};
    writematrix([t primary current secondary power_trace dq iref command double(limits)], ...
        fullfile(out,[names{scenario} '.csv']));
    f=figure('Visible','off','Position',[50 50 1150 1000]); tiledlayout(5,1);
    nexttile; plot(t,dq(:,1:2)); hold on; yline(p.vpeak,'--'); grid on; ylabel('LV +dq (V)'); title(strrep(names{scenario},'_',' '));
    nexttile; plot(t,dq(:,3:4)); grid on; ylabel('LV -dq (V)');
    nexttile; plot(t,current); hold on; yline(p.ipeak,'--'); yline(-p.ipeak,'--'); grid on; ylabel('Inductor abc (A)');
    nexttile; plot(t,secondary/1000); xlim([3.94 4]); grid on; ylabel('HV line (kV)');
    nexttile; plot(t,movmean(power_trace,100)/1000); grid on; ylabel('Delta branch (kW)'); xlabel('Time (s)');
    exportgraphics(f,fullfile(out,[names{scenario} '.png']),'Resolution',120); close(f);
end
report.summary=cell2table(rows,'VariableNames',{'case_name','rho','LV_AB_rms','LV_BC_rms','LV_CA_rms', ...
    'Ipeak_A','VUF_percent','current_limit_samples','voltage_limit_samples','finite'});
writetable(report.summary,fullfile(out,'summary.csv'));
save(fullfile(out,'analysis.mat'),'report');
fprintf('Artifacts: %s\n',out);
end

function [mu,rho]=floquet(p,g)
phi=eye(44); samples=round(p.fs/p.f0);
for k=0:samples-1
    a=step(eye(44),2*pi*k/samples,0,p,g,false);
    phi=a*phi;
end
mu=eig(phi); rho=max(abs(mu));
end

function g=plant(p,tr,resistance)
% Primary neutral and capacitor neutral are floating; no zero-sequence line current.
% Symmetric linear 3-leg core enforces zero-sum flux. Delta circulating zero
% sequence remains zero from zero initial state and is eliminated analytically.
Gabc=tr.D'*diag(1./resistance)*tr.D;
Ghv=p.clarke*Gabc*p.invclarke;
Gref=tr.a^2*tr.Q'*Ghv*tr.Q;
g.Zload=Gref\eye(2); g.resistance=resistance;
eye2=eye(2); a=zeros(10); b=zeros(10,2);
% x = [iL;vC;i_primary;i_secondary_referred;i_magnetizing], 2 axes each.
% Core voltage e = Rfe*(i_primary-i_secondary_referred-i_magnetizing).
E=[zeros(2,4) tr.Rfe*eye2 -tr.Rfe*eye2 -tr.Rfe*eye2];
a(1:2,1:2)=-p.R/p.L*eye2; a(1:2,3:4)=-eye2/p.L; b(1:2,:)=eye2/p.L;
a(3:4,1:2)=eye2/p.C; a(3:4,5:6)=-eye2/p.C;
a(5:6,3:4)=eye2/tr.L1; a(5:6,5:6)=-(tr.R1+tr.Rwire)/tr.L1*eye2;
a(5:6,:)=a(5:6,:)-E/tr.L1;
a(7:8,:)=E/tr.L2_ref;
a(7:8,7:8)=a(7:8,7:8)-(tr.R2_ref*eye2+g.Zload)/tr.L2_ref;
a(9:10,:)=E/tr.Lm;
% Energy identity guards winding reflection, current signs and passive loading.
mass=diag(repelem([p.L p.C tr.L1 tr.L2_ref tr.Lm],2));
loss=zeros(10); loss(1:2,1:2)=p.R*eye2; loss(5:6,5:6)=(tr.R1+tr.Rwire)*eye2;
loss(7:8,7:8)=tr.R2_ref*eye2+g.Zload; loss=loss+E'*E/tr.Rfe;
assert(norm((mass*a+a'*mass)/2+loss,'fro')<1e-8,'Transformer energy balance failed.');
ed=expm([a b;zeros(2,12)]*p.ts); g.A=ed(1:10,1:10); g.B=ed(1:10,11:12);
assert(max(real(eig(a)))<0,'Passive transformer/load model is not asymptotically stable.');
end

function [xn,y]=step(x,theta,reference,p,g,limited)
input.v_abc=p.invclarke*x(3:4,:); input.i_l_abc=p.invclarke*x(1:2,:);
input.theta=theta; input.vd_pos_ref=reference;
[sn,y]=npc_dq_control(x(11:42,:),input,p,limited);
xn=[g.A*x(1:10,:)+g.B*x(43:44,:);sn;y.u_alpha;y.u_beta];
end
