function r=npc_dq_stability(p,resistance)
% Floquet multipliers of the complete unsaturated periodic sampled-data system.
% Includes both DSOGIs, eight PI integrators, exact plant ZOH and one-sample delay.
if nargin<2, resistance=p.load_ohm; end
g=npc_sequence_plant(p,resistance);
n=round(p.fs/p.f0);
assert(abs(n-p.fs/p.f0)<1e-10,'Integer samples per electrical period required.');
phi=eye(38);
for k=0:n-1
    a=npc_dq_step(eye(38),2*pi*k/n,0,p,g,false);
    phi=a*phi;
end
r.multipliers=eig(phi); r.rho=max(abs(r.multipliers));
r.stable=r.rho<1; r.period_s=n*p.ts;
end
