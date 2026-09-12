function m=npc_yd_model()
% Shared design model copied from the reviewed Yd load reproduction.
p=npc_dq_config;
p.R=0.002; % NPC3 internal 1 mOhm plus external R9/R11/R10 = 1 mOhm.
tr.L1=17.93e-6; tr.L2=0.13448;
tr.R1=0.0009522; tr.R2=7.1415;
tr.Rwire=0.001; % R6/R7/R8, between the capacitor node and primary terminals.
tr.n=50*sqrt(3); % Secondary/primary WINDING turns ratio.
tr.a=tr.n/sqrt(3); % Secondary/primary LINE voltage ratio, 50.
tr.Lm=0.06062; tr.Rfe=95.22; % im=1, phim=0.06062: linear magnetizing inductance.
tr.lag=150*pi/180; % Secondary positive-sequence phase lag.
tr.Q=[cos(tr.lag) sin(tr.lag);-sin(tr.lag) cos(tr.lag)];
tr.D=[1 -1 0;0 1 -1;-1 0 1]; % R2=AB, R3=BC, R4=CA (polarity irrelevant to resistors).
tr.R2_ref=tr.R2/tr.n^2; tr.L2_ref=tr.L2/tr.n^2;
m.p=p; m.tr=tr; m.plant=@plant;
end

function g=plant(p,tr,resistance)
% Primary neutral and capacitor neutral are floating; no zero-sequence line current.
% Symmetric linear 3-leg core enforces zero-sum flux. Delta circulating zero
% sequence remains zero from zero initial state and is eliminated analytically.
Gabc=tr.D'*diag(1./resistance)*tr.D;
Ghv=p.clarke*Gabc*p.invclarke;
Gref=tr.a^2*tr.Q'*Ghv*tr.Q;
g.Zload=Gref\eye(2); g.resistance=resistance;
eye2=eye(2); a=zeros(10); b=zeros(10,2);
% x = [iL;vC;i_primary;i_secondary_referred;i_magnetizing], 2 axes each.
% Core voltage e = Rfe*(i_primary-i_secondary_referred-i_magnetizing).
E=[zeros(2,4) tr.Rfe*eye2 -tr.Rfe*eye2 -tr.Rfe*eye2];
a(1:2,1:2)=-p.R/p.L*eye2; a(1:2,3:4)=-eye2/p.L; b(1:2,:)=eye2/p.L;
a(3:4,1:2)=eye2/p.C; a(3:4,5:6)=-eye2/p.C;
a(5:6,3:4)=eye2/tr.L1; a(5:6,5:6)=-(tr.R1+tr.Rwire)/tr.L1*eye2;
a(5:6,:)=a(5:6,:)-E/tr.L1;
a(7:8,:)=E/tr.L2_ref;
a(7:8,7:8)=a(7:8,7:8)-(tr.R2_ref*eye2+g.Zload)/tr.L2_ref;
a(9:10,:)=E/tr.Lm;
% Energy identity guards winding reflection, current signs and passive loading.
mass=diag(repelem([p.L p.C tr.L1 tr.L2_ref tr.Lm],2));
loss=zeros(10); loss(1:2,1:2)=p.R*eye2; loss(5:6,5:6)=(tr.R1+tr.Rwire)*eye2;
loss(7:8,7:8)=tr.R2_ref*eye2+g.Zload; loss=loss+E'*E/tr.Rfe;
assert(norm((mass*a+a'*mass)/2+loss,'fro')<1e-8,'Transformer energy balance failed.');
g.Ac=a; g.Bc=b;
ed=expm([a b;zeros(2,12)]*p.ts); g.A=ed(1:10,1:10); g.B=ed(1:10,11:12);
assert(max(real(eig(a)))<0,'Passive transformer/load model is not asymptotically stable.');
end

