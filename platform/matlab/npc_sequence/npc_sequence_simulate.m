function r = npc_sequence_simulate(p, scenario)
% Exact-ZOH averaged physical LC plant. No switched devices or transformer.
n=round(p.duration/p.ts); x=zeros(26,1);
if strcmp(scenario,'unbalanced_no_sequence'), p.seq_enable=false; end
r.t=(0:n-1)'*p.ts;
r.v=zeros(n,3); r.i=zeros(n,3); r.vseq=zeros(n,4); r.command=zeros(n,2);
r.iref=zeros(n,2); r.limits=false(n,2);
g0=npc_sequence_plant(p,p.load_ohm);
if strcmp(scenario,'balanced_step'), load2=[5 5 5];
elseif startsWith(scenario,'unbalanced'), load2=[5 10 20];
elseif strcmp(scenario,'overload'), load2=[1 1 1];
else, load2=p.load_ohm;
end
g1=npc_sequence_plant(p,load2);
for k=1:n
 t=r.t(k); g=g0;
 if t>=0.6 && (~strcmp(scenario,'overload') || t<0.9), g=g1; end
 ref=p.vpeak*min(1,t/p.ramp_time);
 [xn,y]=npc_sequence_step(x,p.w0*t,ref,p,g,true);
 r.v(k,:)=(p.invclarke*x(3:4))'; r.i(k,:)=(p.invclarke*x(1:2))';
 r.vseq(k,:)=y.vseq'; r.command(k,:)=y.u'; r.iref(k,:)=y.ir';
 r.limits(k,:)=[y.current_limited y.voltage_limited];
 x=xn;
end
r.name=scenario;
last=r.t>p.duration-0.2;
r.vll_rms=sqrt(mean((r.v(last,1)-r.v(last,2)).^2));
r.vpos=mean(vecnorm(r.vseq(last,1:2),2,2));
r.vneg=mean(vecnorm(r.vseq(last,3:4),2,2));
r.vuf=100*r.vneg/max(r.vpos,eps);
r.peak_current=max(abs(r.i),[],'all');
r.finite=all(isfinite(r.v),'all') && all(isfinite(r.i),'all');
r.current_limited_samples=sum(r.limits(:,1));
r.voltage_limited_samples=sum(r.limits(:,2));
end
