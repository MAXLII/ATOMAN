function metrics = npc_distortion_metrics(substeps, delay_periods, cases)
% Compare pre-damping and production code using 50 kHz electrical waveforms.
% Residual excludes DC and the fitted 50 Hz fundamental, not only integer harmonics.
if nargin < 1, substeps = 200; end
if nargin < 2, delay_periods = 1; end
folder = fileparts(mfilename('fullpath'));
repo = fileparts(fileparts(fileparts(folder)));
out = fullfile(repo,'platform','plecs','npc','build','midpoint_analysis',sprintf('steps_%d',substeps));
if delay_periods == 0, out = fullfile(out,'immediate'); end
if nargin < 3
    cases = [6 -63.38 3; 6 0 0; 6 0 4; 16 -63.38 3; 16 0 0; 16 0 4];
    if delay_periods == 0, cases = cases(4:6,:); end
end
metrics = zeros(size(cases,1),9);
for n = 1:size(cases,1)
    c = cases(n,:);
    file = fullfile(out,sprintf('mode_%d_delta_%g_scenario_%d_fast.csv',c));
    x = dlmread(file,',',1,0);
    assert(size(x,1)==10000,'Expected 0.2 s at 50 kHz from a completed native run.');
    basis = [ones(size(x,1),1),cos(2*pi*50*x(:,1)),sin(2*pi*50*x(:,1))];
    fundamental = basis\x(:,2:4);
    residual = x(:,2:4)-basis*fundamental;
    spectrum = fft(residual)/size(x,1);
    bin = (0:size(x,1)-1)';
    frequency = min(bin,size(x,1)-bin)*50000/size(x,1);
    low = frequency>0 & frequency<=2000;
    switching = frequency>=2500;
    total_rms = sqrt(mean(residual.^2));
    low_rms = sqrt(sum(abs(spectrum(low,:)).^2));
    switching_rms = sqrt(sum(abs(spectrum(switching,:)).^2));
    current = basis\x(:,5:7);
    trace = dlmread(fullfile(out,sprintf('mode_%d_delta_%g_scenario_%d.csv',c)),',',1,0);
    assert(trace(end,1)>11.999);
    tail = trace(:,1)>=11;
    metrics(n,:) = [c max(total_rms) max(low_rms) max(switching_rms) ...
        max(abs(current(1,:))) mean(trace(tail,2)) max(abs(trace(tail,2)))];
    fprintf('mode=%d scenario=%d residual=%.3f low=%.3f switching=%.3f V RMS, DC current=%.3f A\n', ...
        c(1),c(3),metrics(n,4:7));
end
fid = fopen(fullfile(out,'distortion_metrics.csv'),'w'); assert(fid>=0);
fprintf(fid,'mode,initial_delta,scenario,residual_rms_v,below_2khz_rms_v,above_2p5khz_rms_v,max_dc_current_a,mean_delta_v,tail_peak_delta_v\n');
fprintf(fid,'%g,%g,%g,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n',metrics');
fclose(fid);
end
