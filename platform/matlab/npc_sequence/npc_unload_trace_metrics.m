function report=npc_unload_trace_metrics()
% Quantify the latest production CSV without changing the simulation or firmware.
root=fileparts(mfilename('fullpath'));
files=dir(fullfile(root,'..','..','plecs','npc','build','bin','npc_trace*.csv'));
[~,idx]=max([files.datenum]); file=fullfile(files(idx).folder,files(idx).name);
d=readtable(file); t=d.time_s; ts=median(diff(t));
v=[d.va d.vb d.vc]; i=[d.ia d.ib d.ic];
loaded=max(sqrt(movmean(i.^2,[99 0])),[],2)>500;
stop=find(diff(double(loaded))<0,1,'last')+1;
assert(~isempty(stop),'Capture does not include removal of a >500 A RMS load');
event=t(stop); tail=t>event+0.4;
beta=(v(:,2)-v(:,3))/sqrt(3); x=beta(tail); n=numel(x);
window=0.5-0.5*cos(2*pi*(0:n-1)'/(n-1));
sp=abs(fft((x-mean(x)).*window)); freq=(0:n-1)'/(n*ts);
band=find(freq>0.5 & freq<25); [~,pk]=max(sp(band)); f=freq(band(pk));
rows=[];
for start=event+0.3:0.5:t(end)-1
    ix=t>=start & t<start+1; tt=t(ix);
    basis=[ones(size(tt)),sin(2*pi*f*tt),cos(2*pi*f*tt),sin(2*pi*50*tt),cos(2*pi*50*tt)];
    c=basis\beta(ix); amp=hypot(c(2),c(3));
    rows(end+1,:)=[start-event amp]; %#ok<AGROW>
end
decay=polyfit(rows(:,1),log(rows(:,2)),1);
report.file=file; report.removal_s=event; report.low_frequency_Hz=f;
report.decay_tau_s=-1/decay(1); report.fitted_2percent_s=log(50)*report.decay_tau_s;
ix=t>event-0.1 & t<event+0.1;
report.event_voltage_peak=max(abs(v(ix,:)),[],'all');
report.voltage_limited_samples=sum(d.voltage_limited(ix));
report.current_limited_samples=sum(d.current_limited(ix));
report.envelope=array2table(rows,'VariableNames',{'time_after_event_s','beta_low_frequency_peak_V'});
out=fullfile(root,'output','trace_unload'); if ~exist(out,'dir'), mkdir(out); end
writetable(report.envelope,fullfile(out,'envelope.csv'));
save(fullfile(out,'report.mat'),'report'); disp(rmfield(report,'envelope'));
end
