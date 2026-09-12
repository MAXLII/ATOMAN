function [xn,y]=npc_dq_step(x,theta,reference,p,g,limited)
% Analysis harness state: physical iL/vC(4), controller(32), applied delay(2).
% The controller has no plant states and receives only its public input interface.
input.v_abc=p.invclarke*x(3:4,:);
input.i_l_abc=p.invclarke*x(1:2,:);
input.theta=theta; input.vd_pos_ref=reference;
[sn,y]=npc_dq_control(x(5:36,:),input,p,limited);
xn=[g.A*x(1:4,:)+g.B*x(37:38,:);sn;y.u_alpha;y.u_beta];
end
