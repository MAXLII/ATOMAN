function p = npc_dq_config()
% Pure positive/negative dq cascade. Gains are analysis candidates, not firmware defaults.
p.fs=5000; p.ts=1/p.fs; p.f0=50; p.w0=2*pi*p.f0;
p.L=80e-6; p.R=1e-3; p.C=200e-6;
p.vdc=1331; p.vll=690; p.vpeak=p.vll*sqrt(2/3);
p.load_ohm=[10 10 10]; % Illustrative floating-star load.
p.ipeak=160; p.modulation_headroom=0.95;
p.kpv=0.2609166; p.kiv=17.33260; p.kpi=0.4003825; p.kii=398.8813;
p.kaw_v=10; p.kaw_i=10; % Back-calculation rates, 1/s.
p.duration=4; p.ramp_time=0.5; p.event_time=2;
p.clarke=(2/3)*[1 -0.5 -0.5;0 sqrt(3)/2 -sqrt(3)/2];
p.invclarke=[1 0;-0.5 sqrt(3)/2;-0.5 -sqrt(3)/2];
% Fixed nominal SOGI tuning; theta is supplied by the caller, never integrated here.
w=p.w0*p.ts; k=sqrt(2); den=4+2*k*w+w*w;
p.sogi=[2*k*w/den,(2*w*w-8)/den,(w*w-2*k*w+4)/den,k*w*w/den];
end
