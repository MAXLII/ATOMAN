function p = npc_sequence_config()
% Nominal low-voltage-side LC model; illustrative load, not the transformer model.
p.fs = 5000; p.ts = 1/p.fs; p.f0 = 50; p.w0 = 2*pi*p.f0;
p.L = 80e-6; p.R = 1e-3; p.C = 200e-6;
p.vdc = 1331; p.vll = 690; p.vpeak = p.vll*sqrt(2/3);
p.ipeak = 160; % Design assumption: peak phase-current limit, A.
p.modulation_headroom = 0.95;
p.load_ohm = [10 10 10]; % Explicit illustrative per-phase star load.
p.k_sogi = sqrt(2);
p.kpi = 0.6; p.kii = 5; % Candidate selected over nominal/balanced-step/unbalanced loads.
p.kpv = 0.03; p.kiv = 1;
p.duration = 2.4; p.ramp_time = 0.3;
p.seq_enable = true; p.aw_enable = true;
p.active_damping = true; % Discrete LC/delay state feedback, using available iL/vC and previous command.
p.clarke = (2/3)*[1 -0.5 -0.5; 0 sqrt(3)/2 -sqrt(3)/2];
p.invclarke = [1 0; -0.5 sqrt(3)/2; -0.5 -sqrt(3)/2];
% Exact discrete resonator at 50 Hz (not a damped/quasi-PR approximation).
ar = [0 -p.w0; p.w0 0];
er = expm([ar [1;0]; zeros(1,3)]*p.ts);
p.ar = er(1:2,1:2); p.br = er(1:2,3);
x = p.w0*p.ts; d = 4+2*p.k_sogi*x+x*x;
p.sogi = [2*p.k_sogi*x/d, (2*x*x-8)/d, ...
          (x*x-2*p.k_sogi*x+4)/d, p.k_sogi*x*x/d];
g = npc_sequence_plant(p,p.load_ohm);
ix=[1 3]; aa=[g.A(ix,ix) g.B(ix,1); zeros(1,3)]; bb=[0;0;1];
aa5=[aa zeros(3,2); -p.br*[1 0 0] p.ar]; bb5=[bb;0;0];
qq=diag([1/p.ipeak^2 1/p.vpeak^2 0.01/p.vdc^2 ...
         (p.w0/p.ipeak)^2 (p.w0/p.ipeak)^2]);
kk=dlqr(aa5,bb5,qq,0.05/p.vdc^2);
p.damping_K=kk(1:3); p.resonant_K=kk(4:5);
end
