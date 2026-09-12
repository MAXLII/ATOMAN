function [sn,y] = npc_dq_control(s,input,p,limited)
% Input: v_abc [V], i_l_abc [A], theta [rad], vd_pos_ref [phase peak V].
% All samples and theta refer to the same instant. Amplitude-invariant Clarke.
% State (32 x N): voltage SOGIs(12), current SOGIs(12), voltage PI(4), current PI(4).
% dq order throughout: [d_positive;q_positive;d_negative;q_negative].
% Zero state initializes/resets the controller. Multiple columns support linear analysis.
% limited=false is solely the unsaturated small-signal analysis path.
assert(size(s,1)==32 && size(input.v_abc,1)==3 && size(input.i_l_abc,1)==3);
r=[cos(input.theta) -sin(input.theta);sin(input.theta) cos(input.theta)];
v=p.clarke*input.v_abc; i=p.clarke*input.i_l_abc;
[vs,sv]=sequence(v,s(1:12,:),p.sogi,r);
[is,si]=sequence(i,s(13:24,:),p.sogi,r);
ev=[input.vd_pos_ref;0;0;0]-vs;
iraw=p.kpv*ev+s(25:28,:);
scale_i=ones(1,size(s,2));
if limited
    % Bound the sum of positive/negative vector magnitudes, not each dq axis alone.
    scale_i=min(1,p.ipeak./max(vecnorm(iraw(1:2,:))+vecnorm(iraw(3:4,:)),eps));
end
iref=iraw.*scale_i;
ei=iref-is;
useq=p.kpi*ei+s(29:32,:);
raw=r*useq(1:2,:)+r'*useq(3:4,:);
scale_u=ones(1,size(s,2));
if limited
    phase=p.invclarke*raw;
    span=max(phase,[],1)-min(phase,[],1);
    scale_u=min(1,p.modulation_headroom*p.vdc./max(span,eps));
end
% A common scale preserves the positive/negative mixture; apply the same
% allocation to all four integrators for consistent anti-windup feedback.
u=raw.*scale_u;
sn=[sv;si;s(25:28,:)+p.ts*(p.kiv*ev+p.kaw_v*(iref-iraw)); ...
    s(29:32,:)+p.ts*(p.kii*ei+p.kaw_i*(useq.*scale_u-useq))];
y.u_alpha=u(1,:); y.u_beta=u(2,:);
y.v_dq=vs; y.i_l_dq=is; y.i_l_dq_ref=iref; y.u_dq=useq.*scale_u;
y.current_limited=scale_i<1; y.voltage_limited=scale_u<1;
end

function [dq,sn]=sequence(ab,s,c,r)
[a,sa]=sogi(ab(1,:),s(1:6,:),c);
[b,sb]=sogi(ab(2,:),s(7:12,:),c);
pos=0.5*[a(1,:)-b(2,:);b(1,:)+a(2,:)];
neg=0.5*[a(1,:)+b(2,:);b(1,:)-a(2,:)];
dq=[r'*pos;r*neg]; sn=[sa;sb];
end

function [y,sn]=sogi(u,s,c)
% [previous input(2), previous in-phase output(2), quadrature output(2)].
d=c(1)*(u-s(2,:))-c(2)*s(3,:)-c(3)*s(4,:);
q=c(4)*(u+2*s(1,:)+s(2,:))-c(2)*s(5,:)-c(3)*s(6,:);
y=[d;q]; sn=[u;s(1,:);d;s(3,:);q;s(5,:)];
end
