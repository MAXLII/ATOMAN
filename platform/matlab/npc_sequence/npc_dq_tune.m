function result=npc_dq_tune()
% Optional reproducible PI-only search. Never rewrites the source configuration.
% Minimize worst one-cycle spectral radius over nominal, step and unbalanced loads.
p=npc_dq_config;
options=optimset('Display','off','MaxIter',160,'MaxFunEvals',300,'TolX',1e-3);
seeds=[p.kpv p.kiv p.kpi p.kii;0.1 10 0.1 50;0.2 1 0.01 10];
cost=inf; z=[];
for k=1:size(seeds,1)
    [candidate,value]=fminsearch(@(x)objective(x,p),log10(seeds(k,:)),options);
    if value<cost, z=candidate; cost=value; end
end
result.gains=10.^z; result.worst_rho=cost;
disp(result);
end

function cost=objective(z,p)
% Do not let the voltage proportional term alone request more than the
% current budget for a full nominal voltage error.
if any(z < -5 | z > 5) || z(1)>log10(p.ipeak/p.vpeak)
    cost=1e6+sum(z.^2); return;
end
q=10.^z; p.kpv=q(1); p.kiv=q(2); p.kpi=q(3); p.kii=q(4);
a=npc_dq_stability(p,[10 10 10]); b=npc_dq_stability(p,[5 5 5]);
c=npc_dq_stability(p,[5 10 20]);
cost=max([a.rho b.rho c.rho]);
if ~isfinite(cost), cost=1e20; end
end
