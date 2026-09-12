function npc_dq_test(p)
% Independent signal fixtures check sequence signs and external phase alignment.
for sigma=[1 -1]
    s=zeros(32,1); input.vd_pos_ref=0;
    for k=0:2999
        theta=p.w0*k*p.ts+0.61;
        r=[cos(sigma*theta) -sin(sigma*theta);sin(sigma*theta) cos(sigma*theta)];
        input.v_abc=p.invclarke*r*[100;30];
        input.i_l_abc=p.invclarke*r*[4;-2]; input.theta=theta;
        [s,y]=npc_dq_control(s,input,p,false);
    end
    expected_v=zeros(4,1); expected_i=zeros(4,1);
    if sigma==1, ix=1:2; else, ix=3:4; end
    expected_v(ix)=[100;30]; expected_i(ix)=[4;-2];
    assert(max(abs(y.v_dq-expected_v))<0.1,'Voltage sequence/phase convention failed.');
    assert(max(abs(y.i_l_dq-expected_i))<0.005,'Current sequence/phase convention failed.');
end
% Exact batched Jacobian must reproduce a full independent state perturbation.
rng(11); x=randn(38,1); g=npc_sequence_plant(p,p.load_ohm);
a=npc_dq_step(eye(38),0.73,0,p,g,false);
actual=npc_dq_step(x,0.73,0,p,g,false);
assert(norm(a*x-actual,inf)<1e-9,'Linear periodic matrix differs from actual controller.');
% Combined sequence limits and back-calculation must act in the same direction.
s=zeros(32,1); s(25:28)=[1e4;2e4;-1e4;3e4]; s(29:32)=[1e6;-1e6;2e6;1e6];
input.v_abc=zeros(3,1); input.i_l_abc=zeros(3,1); input.theta=0.73; input.vd_pos_ref=0;
[sn,y]=npc_dq_control(s,input,p,true);
assert(y.current_limited && y.voltage_limited,'Saturation fixture did not engage limits.');
assert(norm(y.i_l_dq_ref(1:2))+norm(y.i_l_dq_ref(3:4))<=p.ipeak*(1+1e-12));
u=p.invclarke*[y.u_alpha;y.u_beta];
assert(max(u)-min(u)<=p.vdc*p.modulation_headroom*(1+1e-12));
assert(norm(sn(25:28))<norm(s(25:28)) && norm(sn(29:32))<norm(s(29:32)), ...
    'Anti-windup failed to reduce the saturated integral states.');
fprintf('DQ INTERFACE / SEQUENCE / LINEARIZATION / LIMIT TESTS PASSED\n');
end
