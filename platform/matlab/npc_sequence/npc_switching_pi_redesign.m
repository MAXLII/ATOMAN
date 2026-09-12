function report=npc_switching_pi_redesign(mode,substeps)
% Fixed-step switched NPC sensitivity model. No changes to C or PLECS.
% Includes sign-sector SVPWM, Q1/Q3 and Q2/Q4 turn-on deadtime, ideal diode
% conduction selected by current direction, and one full duty-update delay.
if nargin<1,mode='baseline';end
if nargin<2,substeps=100;end
m=npc_yd_model;p=npc_yd_design_config;root=fileparts(mfilename('fullpath'));
out=fullfile(root,'output','switching_redesign');if ~exist(out,'dir'),mkdir(out);end
g=m.plant(p,m.tr,34500^2./[1 1 1]);dt=p.ts/substeps;
ed=expm([g.Ac g.Bc;zeros(2,12)]*dt);Ad=ed(1:10,1:10);Bd=ed(1:10,11:12);
gain=[p.kpv p.kiv p.kpi p.kii];
if strcmp(mode,'loss')
    data=load(fullfile(root,'output','loss_redesign','report_validate.mat'));
    gain=table2array(data.report.gains);
elseif strcmp(mode,'scan')
    gain=[gain;3.765 652.196 .339348 .25023; ...
        .5 500 .1 .05;1 1000 .1 .1;1 250 .2 .05; ...
        1 1000 .05 .05;.2 1000 .05 .05;2 200 .1 .05];
end
report.gains=gain;report.traces=cell(size(gain,1),1);rows=[];
for id=1:size(gain,1)
    p.kpv=gain(id,1);p.kiv=gain(id,2);p.kpi=gain(id,3);p.kii=gain(id,4);
    x=zeros(10,1);s=zeros(32,1);dut=zeros(3,2);age=zeros(3,4);n=round(8/p.ts);
    v=zeros(n,3);i=v;t=(0:n-1)'*p.ts;
    for k=1:n
        v(k,:)=(p.invclarke*x(3:4))';i(k,:)=(p.invclarke*x(1:2))';
        inp=struct('v_abc',v(k,:)','i_l_abc',i(k,:)','theta',p.w0*t(k),'vd_pos_ref',563*min(t(k)/2,1));
        [s,y]=npc_dq_control(s,inp,p,true);
        next=svpwm([y.u_alpha;y.u_beta],p);
        for j=1:substeps
            % Carrier begins at maximum; Q1 is centered, Q4 is edge-aligned.
            carrier=abs(2*(j-.5)/substeps-1);
            q1=dut(:,1)>carrier;q2=(1-dut(:,2))>carrier;
            desired=[q1 q2 ~q1 ~q2];age=(age+dt).*desired;
            gate=desired & age>=(10e-6+dt/2);
            lower=-ones(3,1);lower(gate(:,2))=0;lower(gate(:,1)&gate(:,2))=1;
            upper=ones(3,1);upper(gate(:,3))=0;upper(gate(:,3)&gate(:,4))=-1;
            cur=p.invclarke*x(1:2);pole=lower;pole(cur<0)=upper(cur<0);
            near=abs(cur)<.01;vc=p.invclarke*x(3:4)/(p.vdc/2);
            pole(near)=min(upper(near),max(lower(near),vc(near)));
            x=Ad*x+Bd*(p.clarke*pole*(p.vdc/2));
        end
        dut=next;
    end
    ix=t>=5;B=[ones(sum(ix),1) cos(p.w0*t(ix)) sin(p.w0*t(ix))];c=B\v(ix,:);res=v(ix,:)-B*c;
    value=[max(sqrt(mean(res.^2))),max(max(abs(v(ix,:)))),max(sqrt(mean(i(ix,:).^2)))];
    rows(end+1,:)=[id value]; %#ok<AGROW>
    report.traces{id}=struct('t',t,'v',v,'i',i);
    fprintf('candidate %d residual %.4f V peak %.4f V current RMS %.4f A\n',id,value);
end
report.metrics=array2table(rows,'VariableNames',{'candidate','nonfund_rms_V','phase_peak_V','current_rms_A'});
save(fullfile(out,sprintf('%s_%d.mat',mode,substeps)),'report');
writetable(report.metrics,fullfile(out,sprintf('%s_%d.csv',mode,substeps)));
end

function duty=svpwm(u,p)
v=p.invclarke*u;positive=v>0;lo=-p.vdc/2-v;hi=-v;
lo(positive)=-v(positive);hi(positive)=p.vdc/2-v(positive);
offset=(max(lo)+min(hi))/2;pole=v+offset;
duty=[max(pole,0) max(-pole,0)]/(p.vdc/2);
if max(abs(u))==0,duty(:)=0;end
duty=min(1,max(0,duty));
end
