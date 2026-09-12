function report=npc_raw_feedback_design(mode)
% Proposed raw-feedback dual rotating-integrator controller, MATLAB only.
% One common proportional path avoids double-counting the positive/negative
% proportional feedback. Raw voltage feedforward is delayed with the command.
if nargin==0,mode='inspect';end
m=npc_yd_model;p=npc_yd_design_config;root=fileparts(mfilename('fullpath'));
out=fullfile(root,'output','raw_feedback_design');if ~exist(out,'dir'),mkdir(out);end
power={[1 1 1],[1e4 1e4 1e4],[1e5 1e5 1e5],[1e6 1e6 1e6],[1e6 1 1],[1e4 1e5 1e6]};
g=cellfun(@(w)m.plant(p,m.tr,34500^2./w),power,'UniformOutput',false);
wi=2*pi*600;wn=2*pi*120/sqrt(2+sqrt(5));q=[sqrt(2)*p.C*wn p.C*wn^2 p.L*wi p.R*wi];
p.kpv=q(1);p.kiv=q(2);p.kpi=q(3);p.kii=q(4);
rows=[];
for j=1:6
    A=step(eye(20),zeros(2,20),p,g{j});
    pp=p;pp.kpv=0;pp.kiv=0; ai=step(eye(20),zeros(2,20),pp,g{j});keep=[1:10 15:20];ai=ai(keep,keep);
    rows(end+1,:)=[j max(abs(eig(ai)))^100 max(abs(eig(A)))^100]; %#ok<AGROW>
    fprintf('load%d raw inner/full rho20ms %.8g %.8g\n',rows(end,:));
end
report.gains=q;report.stability=array2table(rows,'VariableNames',{'load','rho_inner_20ms','rho_full_20ms'});
writetable(report.stability,fullfile(out,['stability_' mode '.csv']));
save(fullfile(out,['report_' mode '.mat']),'report');
end

function xn=step(x,ref,p,g)
e=ref-x(3:4,:);ir=p.kpv*e+x(11:12,:)+x(13:14,:);ei=ir-x(1:2,:);
u=x(3:4,:)+p.kpi*ei+x(15:16,:)+x(17:18,:);
d=p.w0*p.ts;r=[cos(d) -sin(d);sin(d) cos(d)];
xn=[g.A*x(1:10,:)+g.B*x(19:20,:);r*(x(11:12,:)+p.kiv*p.ts*e); ...
    r'*(x(13:14,:)+p.kiv*p.ts*e);r*(x(15:16,:)+p.kii*p.ts*ei); ...
    r'*(x(17:18,:)+p.kii*p.ts*ei);u];
end
