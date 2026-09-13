function results = npc_ripple_comparison(substeps, make_plot)
% Compare actual C control/modulation before and after averaged midpoint control.
% Input records are 50 kHz switched-model samples, not 5 kHz control snapshots.
if nargin < 1, substeps = 400; end
if nargin < 2, make_plot = true; end
folder = fileparts(mfilename('fullpath'));
repo = fileparts(fileparts(fileparts(folder)));
out = fullfile(repo,'platform','plecs','npc','build','midpoint_analysis',sprintf('steps_%d',substeps));
cases = [0 0;0 4;-63.38 3];
names = {'Balanced 3 kW','Balanced 3 MW','1 MW / 1 W / 1 W'};
results = zeros(3,8);
if make_plot, fig = figure('visible','off','position',[100 100 1500 1100]); end
for item = 1:3
    old = dlmread(fullfile(out,sprintf('mode_6_delta_%g_scenario_%d_fast.csv',cases(item,:))),',',1,0);
    fresh = dlmread(fullfile(out,sprintf('mode_80_delta_%g_scenario_%d_fast.csv',cases(item,:))),',',1,0);
    assert(size(old,1)==10000 && size(fresh,1)==10000);
    basis = [ones(size(fresh,1),1),cos(2*pi*50*fresh(:,1)),sin(2*pi*50*fresh(:,1))];
    old_error = old(:,2:4)-basis*(basis\old(:,2:4));
    coefficients = basis\fresh(:,2:4);
    error = fresh(:,2:4)-basis*coefficients;
    current_error = fresh(:,5:7)-basis*(basis\fresh(:,5:7));
    current_fft = fft(current_error)/size(fresh,1);
    bin = (0:size(fresh,1)-1)';
    frequency = min(bin,size(fresh,1)-bin)*5;
    a = exp(1j*2*pi/3);
    phasors = coefficients(2,:)-1j*coefficients(3,:);
    positive = (phasors(1)+a*phasors(2)+a*a*phasors(3))/3;
    negative = (phasors(1)+a*a*phasors(2)+a*phasors(3))/3;
    trace = dlmread(fullfile(out,sprintf('mode_80_delta_%g_scenario_%d.csv',cases(item,:))),',',1,0);
    tail = trace(:,1)>=11;
    results(item,:) = [max(sqrt(mean(old_error.^2))),max(sqrt(mean(error.^2))),...
        abs(positive),100*abs(negative)/abs(positive),...
        max(sqrt(sum(abs(current_fft(frequency>=2500,:)).^2))),...
        max(sqrt(sum(abs(current_fft(frequency>0 & frequency<=2000,:)).^2))),...
        mean(trace(tail,2)),max(abs(trace(tail,2)))];
    fprintf('%s: old/new voltage residual %.3f/%.3f V; positive peak %.3f V; VUF %.3f%%; current >=2.5kHz %.3f A RMS; current <=2kHz %.3f A RMS; mean/peak delta %.3f/%.3f V\n',names{item},results(item,:));
    if make_plot
    shown = fresh(:,1)>=11.96;
    time = (fresh(shown,1)-11.96)*1000;
    subplot(4,3,item); plot(time,fresh(shown,2:4)); grid on; title(names{item}); ylabel('Output voltage (V)');
    [~,phase] = max(sqrt(mean(old_error.^2)));
    subplot(4,3,item+3); plot(time,old_error(shown,phase),time,error(shown,phase)); grid on; ylabel('Nonfundamental voltage (V)'); legend('Before','After');
    subplot(4,3,item+6); plot(time,fresh(shown,5:7)); grid on; ylabel('Inductor current (A)');
    shown_trace = trace(:,1)>=11.96;
    subplot(4,3,item+9); plot((trace(shown_trace,1)-11.96)*1000,trace(shown_trace,2)); grid on; ylabel('Vp - Vn (V)'); xlabel('Time (ms)');
    end
end
if make_plot
assets = fullfile(repo,'docs','design','assets');
if ~exist(assets,'dir'), mkdir(assets); end
print(fig,fullfile(assets,'npc_averaged_midpoint_comparison.png'),'-dpng','-r120'); close(fig);
end
fid=fopen(fullfile(out,'production_comparison.csv'),'w'); assert(fid>=0);
fprintf(fid,'old_voltage_residual_rms,new_voltage_residual_rms,positive_voltage_peak,vuf_percent,current_above_2p5khz_rms,current_below_2khz_rms,mean_delta,peak_delta\n');
fprintf(fid,'%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n',results'); fclose(fid);
end
