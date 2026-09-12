function g = npc_sequence_plant(p, resistance)
% Floating-star resistive load; i_load = G*v in alpha-beta coordinates.
conductance = 1./resistance(:);
gabc = diag(conductance) - conductance*conductance'/sum(conductance);
g.G = p.clarke*gabc*p.invclarke;
a = [-p.R/p.L*eye(2), -eye(2)/p.L; eye(2)/p.C, -g.G/p.C];
b = [eye(2)/p.L; zeros(2)];
ed = expm([a b; zeros(2,6)]*p.ts);
g.A = ed(1:4,1:4); g.B = ed(1:4,5:6);
g.continuous_A = a;
end

