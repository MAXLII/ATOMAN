function r = npc_sequence_stability(p)
% Exact small-signal periodic model, including DSOGI, PR, PI, ZOH and one delay.
n=round(p.fs/p.f0);
assert(abs(n-p.fs/p.f0)<1e-10,'Need an integer number of samples per electrical cycle.');
g=npc_sequence_plant(p,p.load_ohm);
[r.multipliers,r.rho,r.phi]=floquet(p,g,n);
base=p; base.active_damping=false;
[~,r.baseline_rho]=floquet(base,g,n);
rows=zeros(9,4); k=0;
for ls=[0.8 1 1.2]
 for cs=[0.8 1 1.2]
  actual=p; actual.L=p.L*ls; actual.C=p.C*cs;
  ga=npc_sequence_plant(actual,p.load_ohm);
  [~,rho]=floquet(p,ga,n); % Keep controller/predictor nominal; perturb only physical plant.
  k=k+1; rows(k,:)=[ls cs 10 rho];
 end
end
r.sweep=array2table(rows,'VariableNames',{'L_ratio','C_ratio','Rload_ohm','rho'});
[~,r.unbalanced_rho]=floquet(p,npc_sequence_plant(p,[5 10 20]),n);
% SISO nominal balanced inner-loop diagnostic. The outer sequence loops are
% assessed by the full periodic model, not by a first-order SOGI approximation.
ix=[1 3]; aa=[g.A(ix,ix) g.B(ix,1); zeros(1,3)]; bb=[0;0;1];
if p.active_damping
    gi=tf(ss(aa-bb*p.damping_K,bb,[1 0 0],0,p.ts));
    hr=tf(ss(p.ar,p.br,-p.resonant_K,0,p.ts)); % LQR-shaped resonant internal model.
    r.Li=minreal(hr*gi,1e-8);
    aa5=[aa zeros(3,2);-p.br*[1 0 0] p.ar]; bb5=[bb;0;0];
    ti=ss(aa5-bb5*[p.damping_K p.resonant_K],[p.kpi*bb;p.br],[1 0 0 0 0],0,p.ts);
else
    hr=tf(ss(p.ar,p.br,[p.kii 0],0,p.ts));
    r.Li=(p.kpi+hr)*tf(ss(aa,bb,[1 0 0],0,p.ts));
    ti=feedback(r.Li,1);
end
r.inner_tracking=ti;
r.inner_stable=isstable(ti);
[r.gmi,r.pmi,r.wgmi,r.wpmi]=margin(r.Li);
r.inner_allmargin=allmargin(r.Li);
end

function [mu,rho,phi]=floquet(p,g,n)
phi=eye(26);
for k=0:n-1
 % Zero reference, no limits: the step map is exactly linear in all 26 states.
 a=npc_sequence_step(eye(26),2*pi*k/n,0,p,g,false);
 phi=a*phi;
end
mu=eig(phi); rho=max(abs(mu));
end
