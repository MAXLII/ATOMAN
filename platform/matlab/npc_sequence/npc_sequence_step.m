function [xn, y] = npc_sequence_step(x, theta, reference, p, g, limited)
% x=[iL(2);vC(2);SOGI_alpha(6);SOGI_beta(6);Iv(4);PR_alpha(2);PR_beta(2);u_delay(2)].
% Supports multiple columns for exact linear periodic state-matrix construction.
% Reference angle is an internal oscillator: islanded control does not PLL-lock to its own output.
r = [cos(theta) -sin(theta); sin(theta) cos(theta)];
j = [0 -1;1 0];
va = x(3,:); vb = x(4,:);
[fa, sa] = sogi(va,x(5:10,:),p.sogi);
[fb, sb] = sogi(vb,x(11:16,:),p.sogi);
vpos = 0.5*[fa(1,:)-fb(2,:);fb(1,:)+fa(2,:)];
vneg = 0.5*[fa(1,:)+fb(2,:);fb(1,:)-fa(2,:)];
vseq = [r'*vpos;r*vneg];
err = [reference;0;0;0] - vseq;
if ~p.seq_enable
    err(3:4,:) = 0;
end
% C*dv/dt = iL-iLoad, with +/-omega frame cross-coupling feedforward.
cross = p.w0*p.C*[j*vseq(1:2,:);-j*vseq(3:4,:)];
if ~p.seq_enable
    cross(3:4,:) = 0;
end
iraw = p.kpv*err+x(17:20,:)+cross;
iscale = ones(1,size(x,2));
if limited
    iscale = min(1,p.ipeak./max(vecnorm(iraw(1:2,:))+vecnorm(iraw(3:4,:)),eps));
end
iseq = iraw.*iscale;
iref = r*iseq(1:2,:)+r'*iseq(3:4,:);
ei = iref-x(1:2,:);
% Alpha-beta PR current regulator with extracted positive-sequence voltage
% feedforward (2014 paper, section 3.6). Raw delayed vC feedforward is not equivalent.
uraw = vpos+p.kpi*ei+p.kii*[x(21,:);x(23,:)];
if p.active_damping
    % LC/delay state feedback suppresses the near-Nyquist resonance. The known
    % islanded voltage reference supplies feedforward, avoiding self-positive-feedback.
    kk=p.damping_K;
    uraw=(1+kk(2)+kk(3))*r*[reference;0]+p.kpi*iref-kk(1)*x(1:2,:) ...
         -kk(2)*x(3:4,:)-kk(3)*x(25:26,:) ...
         -[p.resonant_K*x(21:22,:);p.resonant_K*x(23:24,:)];
end
uphase = p.invclarke*uraw;
uscale = ones(1,size(x,2));
if limited
    span = max(uphase,[],1)-min(uphase,[],1);
    uscale = min(1,p.modulation_headroom*p.vdc./max(span,eps));
end
u = uraw.*uscale;
av = zeros(size(err)); ai = zeros(size(ei));
if limited && p.aw_enable
    av = (p.kiv/p.kpv)*(iseq-iraw);
    ai = (u-uraw)/p.kpi;
end
xn = zeros(size(x));
xn(1:4,:) = g.A*x(1:4,:)+g.B*x(25:26,:); % One full computation/update delay, plus plant ZOH.
xn(5:10,:) = sa; xn(11:16,:) = sb;
xn(17:20,:) = x(17:20,:)+p.ts*(p.kiv*err+av);
xn(21:22,:) = p.ar*x(21:22,:)+p.br*(ei(1,:)+ai(1,:));
xn(23:24,:) = p.ar*x(23:24,:)+p.br*(ei(2,:)+ai(2,:));
xn(25:26,:) = u;
y.vseq=vseq; y.iseq=iseq; y.u=u; y.ir=iref;
y.current_limited=iscale<1; y.voltage_limited=uscale<1;
end

function [out,sn] = sogi(u,s,c)
% State: u[k-1],u[k-2],d[k-1],d[k-2],q[k-1],q[k-2].
d = c(1)*(u-s(2,:))-c(2)*s(3,:)-c(3)*s(4,:);
q = c(4)*(u+2*s(1,:)+s(2,:))-c(2)*s(5,:)-c(3)*s(6,:);
out=[d;q]; sn=[u;s(1,:);d;s(3,:);q;s(5,:)];
end
