function report=npc_dq_analysis()
% Entry point for the SVG-defined eight-PI controller. No PR or state feedback.
root=fileparts(mfilename('fullpath')); out=fullfile(root,'output','dq');
if ~exist(out,'dir'), mkdir(out); end
p=npc_dq_config; npc_dq_test(p); report.config=p;
report.nominal=npc_dq_stability(p);
report.unbalanced=npc_dq_stability(p,[5 10 20]);
rows=zeros(9,3); k=0;
for ls=[0.8 1 1.2]
    for cs=[0.8 1 1.2]
        actual=p; actual.L=p.L*ls; actual.C=p.C*cs;
        r=npc_dq_stability(actual); k=k+1; rows(k,:)=[ls cs r.rho];
    end
end
report.robustness=array2table(rows,'VariableNames',{'L_ratio','C_ratio','rho'});
fprintf('Nominal rho=%.8f; unbalanced rho=%.8f; LC sweep worst=%.8f\n', ...
    report.nominal.rho,report.unbalanced.rho,max(rows(:,3)));
names={'startup','balanced_step','unbalanced','reference_step','overload_recovery'};
summary=cell(numel(names),9);
for k=1:numel(names)
    name=names{k}; r=npc_dq_simulate(p,name); report.time_domain.(name)=r;
    summary(k,:)={name,r.vll_rms,r.vuf,r.physical_vuf,r.dq_error,r.peak_current,sum(r.limits(:,1)),sum(r.limits(:,2)),r.finite};
    fprintf('%s: Vll=%.3f V VUF=%.4f%% dq_error=%.4f V Ipeak=%.3f A\n', ...
        name,r.vll_rms,r.vuf,r.dq_error,r.peak_current);
    data=array2table([r.t r.ref r.v r.i r.vdq r.idq r.iref r.u double(r.limits)], ...
        'VariableNames',{'time_s','vd_ref','va','vb','vc','ia','ib','ic', ...
        'vd_pos','vq_pos','vd_neg','vq_neg','id_pos','iq_pos','id_neg','iq_neg', ...
        'id_ref_pos','iq_ref_pos','id_ref_neg','iq_ref_neg','u_alpha','u_beta','current_limited','voltage_limited'});
    writetable(data,fullfile(out,[name '.csv']));
    f=figure('Visible','off','Position',[50 50 1100 850]); tiledlayout(4,1);
    nexttile; plot(r.t,[r.vdq(:,1:2) r.ref]); grid on; ylabel('Voltage +dq (V)'); legend('d+','q+','d reference'); title(strrep(name,'_',' '));
    nexttile; plot(r.t,r.vdq(:,3:4)); grid on; ylabel('Voltage -dq (V)'); legend('d-','q-');
    nexttile; plot(r.t,r.i); hold on; yline(p.ipeak,'--'); yline(-p.ipeak,'--'); grid on; ylabel('Current abc (A)');
    nexttile; plot(r.t,r.v); xlim([p.duration-0.06 p.duration]); grid on; ylabel('Voltage abc (V)'); xlabel('Time (s)');
    exportgraphics(f,fullfile(out,[name '.png']),'Resolution',120); close(f);
end
report.summary=cell2table(summary,'VariableNames',{'case_name','Vll_rms','DSOGI_VUF_percent','physical_VUF_percent','dq_error_V','peak_current_A','I_limit_samples','U_limit_samples','finite'});
writetable(report.summary,fullfile(out,'summary.csv'));
writetable(report.robustness,fullfile(out,'robustness.csv'));
f=figure('Visible','off','Position',[50 50 1100 440]); tiledlayout(1,2);
nexttile; ang=linspace(0,2*pi,400); plot(cos(ang),sin(ang),'k--'); hold on;
plot(real(report.nominal.multipliers),imag(report.nominal.multipliers),'x'); axis equal; grid on;
title(sprintf('Full sampled loop: rho=%.4f',report.nominal.rho)); xlabel('Real'); ylabel('Imaginary');
nexttile; bar(rows(:,3)); yline(1,'r--'); grid on; title('Fixed PI: L/C +/-20%'); ylabel('One-cycle spectral radius'); xlabel('Parameter case');
exportgraphics(f,fullfile(out,'stability.png'),'Resolution',120); close(f);
report.robust_pass=all(rows(:,3)<1);
save(fullfile(out,'analysis.mat'),'report');
assert(report.nominal.stable && report.unbalanced.stable,'Nominal/unbalanced small-signal instability.');
assert(all(report.summary.finite),'Non-finite time-domain result.');
for k=1:numel(names)
    r=report.time_domain.(names{k});
    assert(r.max_iref<=p.ipeak*(1+1e-10) && r.max_span<=p.vdc*p.modulation_headroom*(1+1e-10),'Command limit violation.');
    assert(r.dq_error<0.02*p.vpeak && r.vuf<0.5,'Steady regulation/recovery failed.');
    target=p.vll;
    if strcmp(names{k},'reference_step'), target=0.8*target; end
    assert(abs(r.vll_rms-target)<0.02*target && r.physical_vuf<0.5,'Independent physical voltage check failed.');
    if k<=4, assert(r.peak_current<=p.ipeak,'Normal-load actual current exceeded budget.'); end
end
unbalanced=report.time_domain.unbalanced;
last=unbalanced.t>p.duration-0.2;
assert(mean(vecnorm(unbalanced.iref(last,3:4),2,2))>1,'Negative-sequence current branch was not exercised.');
assert(report.robust_pass,'L/C sweep contains unstable cases; see robustness.csv.');
fprintf('DQ nominal functional checks passed. LC robustness pass=%d. Current-reference limiting is not hardware protection.\n',report.robust_pass);
end
