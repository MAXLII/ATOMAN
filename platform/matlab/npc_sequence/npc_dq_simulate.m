function r=npc_dq_simulate(p,name)
% The harness supplies theta and physical samples; controller does not own phase.
n=round(p.duration/p.ts); x=zeros(38,1);
r.t=(0:n-1)'*p.ts; r.v=zeros(n,3); r.i=zeros(n,3);
r.vdq=zeros(n,4); r.idq=zeros(n,4); r.iref=zeros(n,4);
r.u=zeros(n,2); r.limits=false(n,2); r.ref=zeros(n,1);
g0=npc_sequence_plant(p,p.load_ohm);
switch name
    case 'balanced_step', load2=[5 5 5];
    case 'unbalanced', load2=[5 10 20];
    case 'overload_recovery', load2=[1 1 1];
    otherwise, load2=p.load_ohm;
end
g1=npc_sequence_plant(p,load2);
for k=1:n
    t=r.t(k); g=g0;
    if t>=p.event_time && (~strcmp(name,'overload_recovery') || t<p.event_time+0.3)
        g=g1;
    end
    reference=p.vpeak*min(t/p.ramp_time,1);
    if strcmp(name,'reference_step') && t>=p.event_time, reference=0.8*reference; end
    theta=p.w0*t+0.37; % Nonzero external initial phase exercises interface alignment.
    [xn,y]=npc_dq_step(x,theta,reference,p,g,true);
    r.v(k,:)=(p.invclarke*x(3:4))'; r.i(k,:)=(p.invclarke*x(1:2))';
    r.vdq(k,:)=y.v_dq'; r.idq(k,:)=y.i_l_dq'; r.iref(k,:)=y.i_l_dq_ref';
    r.u(k,:)=[y.u_alpha y.u_beta]; r.ref(k)=reference;
    r.limits(k,:)=[y.current_limited y.voltage_limited];
    x=xn;
end
last=r.t>p.duration-0.2;
r.vll_rms=sqrt(mean((r.v(last,1)-r.v(last,2)).^2));
r.vuf=100*mean(vecnorm(r.vdq(last,3:4),2,2))/max(mean(vecnorm(r.vdq(last,1:2),2,2)),eps);
% Independent fundamental phasor measurement on physical voltages, not DSOGI feedback.
ab=(p.clarke*r.v(last,:)')'; z=ab(:,1)+1j*ab(:,2);
angle=p.w0*r.t(last)+0.37;
r.physical_vuf=100*abs(mean(z.*exp(1j*angle)))/max(abs(mean(z.*exp(-1j*angle))),eps);
r.dq_error=max(abs(mean(r.vdq(last,:),1)-[r.ref(end) 0 0 0]));
r.peak_current=max(abs(r.i),[],'all');
r.max_iref=max(vecnorm(r.iref(:,1:2),2,2)+vecnorm(r.iref(:,3:4),2,2));
phase=(p.invclarke*r.u')'; r.max_span=max(max(phase,[],2)-min(phase,[],2));
r.finite=all(isfinite([r.v r.i r.vdq r.idq r.u r.iref]),'all');
end
