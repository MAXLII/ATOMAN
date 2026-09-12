function report=npc_trace_analysis()
root=fileparts(mfilename('fullpath'));
files=dir(fullfile(root,'..','..','plecs','npc','build','bin','npc_trace_*.csv'));
assert(~isempty(files),'No NPC capture found'); [~,j]=max([files.datenum]);
file=fullfile(files(j).folder,files(j).name); d=readtable(file);
out=fullfile(root,'output','trace_analysis'); if ~exist(out,'dir'), mkdir(out); end
t=d.time_s-d.time_s(1); last=t>=10; s=d(last,:); n=height(s); fs=5000;
report.file=file; report.rows=height(d); report.dt_range=[min(diff(d.time_s)),max(diff(d.time_s))];
report.run_fraction=mean(d.pwm_enable==1); report.status=unique(d.status)';
names={'va','vb','vc','ia','ib','ic','vd_pos','vq_pos','vd_neg','vq_neg', ...
    'id_pos','iq_pos','id_neg','iq_neg','id_pos_ref','iq_pos_ref','id_neg_ref','iq_neg_ref', ...
    'alpha','beta','integral_idp','integral_iqp','integral_idn','integral_iqn'};
values=zeros(numel(names),4);
for k=1:numel(names)
    v=s.(names{k}); values(k,:)=[mean(v),std(v),min(v),max(v)];
end
report.stats=array2table(values,'VariableNames',{'mean','std','min','max'},'RowNames',names);
disp(report.stats);
% Replay the actual sampled inputs through the independent MATLAB controller.
p=npc_yd_design_config; state=zeros(32,1); replay_error=zeros(1,4);
for k=1:height(d)
    input=struct('v_abc',[d.va(k);d.vb(k);d.vc(k)],'i_l_abc',[d.ia(k);d.ib(k);d.ic(k)], ...
        'theta',d.theta(k),'vd_pos_ref',d.ref_actual(k));
    p.vdc=d.vdc_p(k)+d.vdc_n(k);
    [state,y]=npc_dq_control(state,input,p,true);
    err=[max(abs(y.v_dq-[d.vd_pos(k);d.vq_pos(k);d.vd_neg(k);d.vq_neg(k)])), ...
        max(abs(y.i_l_dq-[d.id_pos(k);d.iq_pos(k);d.id_neg(k);d.iq_neg(k)])), ...
        max(abs(y.i_l_dq_ref-[d.id_pos_ref(k);d.iq_pos_ref(k);d.id_neg_ref(k);d.iq_neg_ref(k)])), ...
        max(abs([y.u_alpha;y.u_beta]-[d.alpha(k);d.beta(k)]))];
    replay_error=max(replay_error,err);
end
report.replay_error=replay_error; fprintf('Replay errors Vdq/Idq/Iref/Uab: '); disp(replay_error);
vdq=[d.vd_pos d.vq_pos d.vd_neg d.vq_neg]; idq=[d.id_pos d.iq_pos d.id_neg d.iq_neg];
iref=[d.id_pos_ref d.iq_pos_ref d.id_neg_ref d.iq_neg_ref];
iv=[d.integral_vdp d.integral_vqp d.integral_vdn d.integral_vqn];
ii=[d.integral_idp d.integral_iqp d.integral_idn d.integral_iqn];
e=-vdq; e(:,1)=e(:,1)+d.ref_actual;
pred=p.kpv*e+[zeros(1,4);iv(1:end-1,:)];
pred=pred.*min(1,p.ipeak./max(vecnorm(pred(:,1:2),2,2)+vecnorm(pred(:,3:4),2,2),eps));
uraw=p.kpi*(iref-idq)+[zeros(1,4);ii(1:end-1,:)];
cs=cos(d.theta); ss=sin(d.theta);
ab=[cs.*(uraw(:,1)+uraw(:,3))+ss.*(uraw(:,4)-uraw(:,2)), ...
    ss.*(uraw(:,1)-uraw(:,3))+cs.*(uraw(:,2)+uraw(:,4))];
phase=ab*p.invclarke'; span=max(phase,[],2)-min(phase,[],2);
ab=ab.*min(1,p.modulation_headroom*(d.vdc_p+d.vdc_n)./max(span,eps));
report.local_pi_error=[max(abs(pred-iref),[],'all'),max(abs(ab-[d.alpha d.beta]),[],'all')];
fprintf('Per-step PI using logged states: Iref/Uab errors '); disp(report.local_pi_error);
report.mean_power=mean(s.va.*s.ia+s.vb.*s.ib+s.vc.*s.ic);
fprintf('Mean sampled three-phase power=%g W\n',report.mean_power);
model=npc_yd_model; g=model.plant(p,model.tr,repmat(34500^2/5000,1,3));
for f=[3.3 50]
    basis=[cos(2*pi*f*s.time_s),sin(2*pi*f*s.time_s),ones(height(s),1)];
    vf=basis\s.va; ifit=basis\s.ia;
    ratio=(ifit(1)-1j*ifit(2))/(vf(1)-1j*vf(2));
    vc=[1;-1j]; tx=(1j*2*pi*f*eye(6)-g.Ac(5:10,5:10))\(g.Ac(5:10,3:4)*vc);
    expected=tx(1)+1j*2*pi*f*p.C;
    fprintf('f=%g measured I/V=%g%+gj expected=%g%+gj\n',f,real(ratio),imag(ratio),real(expected),imag(expected));
end
report.bus=[mean(s.vdc_p),min(s.vdc_p),max(s.vdc_p);mean(s.vdc_n),min(s.vdc_n),max(s.vdc_n)];
report.limits=[mean(s.current_limited),mean(s.voltage_limited)];
report.command_match=max(abs([s.alpha-s.alpha_pwm;s.beta-s.beta_pwm]));
report.zero=[max(abs((s.va+s.vb+s.vc)/3)),max(abs((s.ia+s.ib+s.ic)/3))];
report.vll=sqrt(mean([s.va-s.vb,s.vb-s.vc,s.vc-s.va].^2));
signals={s.va,s.ia,s.vd_pos,s.vq_pos,s.alpha,s.vdc_p-s.vdc_n};
labels={'va','ia','vd_pos','vq_pos','alpha','bus_difference'};
freq=(0:floor(n/2))'*fs/n;
for k=1:numel(signals)
    signal=signals{k}-mean(signals{k}); spectrum=2*abs(fft(signal))/n;
    spectrum=spectrum(1:numel(freq)); [~,ix]=sort(spectrum,'descend'); ix=ix(1:10);
    report.spectra.(labels{k})=[freq(ix),spectrum(ix)];
    fprintf('\n%s FFT [Hz peak amplitude]\n',labels{k}); disp(report.spectra.(labels{k}));
end
disp(report.bus); fprintf('limits=%g %g, command mismatch=%g, zero=%g/%g\n',report.limits,report.command_match,report.zero);
fprintf('Vll RMS=%g/%g/%g; rows=%d dt=%g..%g\n',report.vll,report.rows,report.dt_range);
f=figure('Visible','off','Position',[50 50 1200 950]); tiledlayout(4,1);
nexttile; plot(t,[d.vd_pos d.vq_pos d.vd_neg d.vq_neg]); grid on; ylabel('Voltage dq (V)'); legend('d+','q+','d-','q-');
nexttile; plot(t,[d.id_pos_ref d.id_pos d.iq_pos_ref d.iq_pos]); grid on; ylabel('Current dq (A)'); legend('d+ ref','d+','q+ ref','q+');
nexttile; plot(t,[d.va d.vb d.vc]); xlim([18 18.1]); grid on; ylabel('Phase V (V)');
nexttile; plot(t,[d.vdc_p d.vdc_n]); grid on; ylabel('DC halves (V)'); xlabel('Time since enable (s)');
exportgraphics(f,fullfile(out,'capture.png'),'Resolution',130); close(f);
writetable(report.stats,fullfile(out,'statistics.csv'),'WriteRowNames',true);
save(fullfile(out,'analysis.mat'),'report');
end
