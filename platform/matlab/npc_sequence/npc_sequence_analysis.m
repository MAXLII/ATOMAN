function report = npc_sequence_analysis()
% Reproducible standalone sequence-voltage control study (MATLAB + Control System Toolbox).
% Sources/assumptions and limits are documented in docs/NPC_SEQUENCE_CONTROL_ANALYSIS.md.
root=fileparts(mfilename('fullpath')); out=fullfile(root,'output');
if ~exist(out,'dir'), mkdir(out); end
p=npc_sequence_config(); report.config=p;
r=npc_sequence_stability(p); report.stability=r;
fprintf('fs=%.0f Hz, LC resonance=%.3f Hz\n',p.fs,1/(2*pi*sqrt(p.L*p.C)));
fprintf('Full periodic rho: baseline=%.6f active_damping=%.6f unbalanced=%.6f\n',r.baseline_rho,r.rho,r.unbalanced_rho);
disp(r.sweep);
fprintf('Inner resonant return ratio: PM=%.3f deg, crossover=%.3f Hz, stable=%d\n',r.pmi,r.wpmi/(2*pi),r.inner_stable);
fprintf('State feedback K=[%.9g %.9g %.9g], resonator K=[%.9g %.9g]\n',p.damping_K,p.resonant_K);
names={'startup','balanced_step','unbalanced','overload','unbalanced_no_sequence'};
summary=cell(numel(names),9);
for k=1:numel(names)
 tr=npc_sequence_simulate(p,names{k}); report.time_domain.(names{k})=tr;
 summary(k,:)={names{k},tr.vll_rms,tr.vuf,tr.peak_current,tr.current_limited_samples,tr.voltage_limited_samples,tr.finite,abs(tr.vll_rms-p.vll)<0.02*p.vll,tr.peak_current<=p.ipeak};
 fprintf('%s: Vll=%.3f V VUF=%.4f%% Ipeak=%.3f A I_lim=%d U_lim=%d\n', ...
 names{k},tr.vll_rms,tr.vuf,tr.peak_current,tr.current_limited_samples,tr.voltage_limited_samples);
 data=array2table([tr.t tr.v tr.i tr.vseq tr.command tr.iref double(tr.limits)], ...
 'VariableNames',{'time_s','va','vb','vc','ia','ib','ic','vd_pos','vq_pos','vd_neg','vq_neg','u_alpha','u_beta','iref_alpha','iref_beta','current_limit','voltage_limit'});
 writetable(data,fullfile(out,[names{k} '.csv']));
 fig=figure('Visible','off','Position',[50 50 1200 850]);
 tiledlayout(4,1);
 nexttile; plot(tr.t,tr.vseq(:,1:2)); hold on; yline(p.vpeak,'--'); grid on; ylabel('Positive dq (V)'); legend('d+','q+','reference');
 title(['Standalone LC: ' strrep(names{k},'_',' ')]);
 nexttile; plot(tr.t,tr.vseq(:,3:4)); grid on; ylabel('Negative dq (V)'); legend('d-','q-');
 nexttile; plot(tr.t,tr.i); hold on; yline(p.ipeak,'--'); yline(-p.ipeak,'--'); grid on; ylabel('Phase current (A)'); legend('a','b','c','ref limit');
 nexttile; plot(tr.t,tr.v); grid on; xlim([p.duration-0.06 p.duration]); ylabel('Phase voltage (V)'); xlabel('Time (s)');
 exportgraphics(fig,fullfile(out,[names{k} '.png']),'Resolution',150); close(fig);
end
report.summary=cell2table(summary,'VariableNames',{'case_name','Vll_rms_V','VUF_percent','peak_current_A','current_limited_samples','voltage_limited_samples','finite','voltage_within_2percent','actual_current_within_reference_limit'});
writetable(report.summary,fullfile(out,'summary.csv')); writetable(r.sweep,fullfile(out,'robustness.csv'));
fig=figure('Visible','off','Position',[50 50 1100 850]); tiledlayout(2,2);
nexttile; ang=linspace(0,2*pi,400); plot(cos(ang),sin(ang),'k--'); hold on; plot(real(r.multipliers),imag(r.multipliers),'x'); axis equal; grid on; title(sprintf('Full Floquet multipliers: rho=%.4f',r.rho)); xlabel('Real'); ylabel('Imaginary');
f=logspace(-1,log10(0.49*p.fs),1200); hi=squeeze(freqresp(r.Li,2*pi*f));
nexttile; semilogx(f,20*log10(abs(hi))); yline(0,'--'); grid on; title('Inner resonant return ratio around stabilized LC'); ylabel('dB');
nexttile; semilogx(f,unwrap(angle(hi))*180/pi); grid on; xlabel('Hz'); ylabel('Phase (deg)');
nexttile; bar(r.sweep.rho); yline(1,'r--'); grid on; ylabel('Spectral radius'); xlabel('L/C +/-20% case'); title('Fixed controller, varied physical LC');
exportgraphics(fig,fullfile(out,'stability.png'),'Resolution',150); close(fig);
save(fullfile(out,'analysis.mat'),'report');
assert(r.rho<1,'Nominal periodic closed loop is unstable.');
assert(r.unbalanced_rho<1 && all(r.sweep.rho<1),'Robustness or unbalanced-load check failed.');
assert(r.inner_stable,'Inner tracking loop is unstable.');
assert(all(report.summary.finite),'A time-domain case diverged.');
assert(all(report.summary.voltage_within_2percent(1:4)),'Voltage regulation/recovery failed.');
assert(report.time_domain.unbalanced.vuf<0.5,'Negative sequence suppression failed.');
assert(report.time_domain.unbalanced.vuf<report.time_domain.unbalanced_no_sequence.vuf,'Sequence controller did not improve VUF.');
assert(all(report.summary.actual_current_within_reference_limit(1:3)),'Normal-load current constraint failed.');
fprintf('ANALYSIS CHECKS PASSED. Overload recovery is not a hardware-current-protection test.\n');
end
