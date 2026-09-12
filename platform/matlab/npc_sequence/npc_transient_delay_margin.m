function report=npc_transient_delay_margin()
% Additional carrier-latch latency sensitivity, beyond the required one sample.
root=fileparts(mfilename('fullpath'));
saved=load(fullfile(root,'output','transient_optimize_tune','report.mat'));
m=npc_yd_model; p=npc_yd_design_config; rows=[];
for id=1:height(saved.report.scan)
    q=table2array(saved.report.scan(id,1:4)); p.kpv=q(1);p.kiv=q(2);p.kpi=q(3);p.kii=q(4);
    for power=[1 1e6]
        g=m.plant(p,m.tr,34500^2./[power power power]);
        for extra=[0 0.25 0.5 1]
            a=expm([g.Ac g.Bc;zeros(2,12)]*(extra*p.ts));
            b=expm([g.Ac g.Bc;zeros(2,12)]*((1-extra)*p.ts));
            phi=eye(46);
            for k=0:99
                x=eye(46); inp=struct('v_abc',p.invclarke*x(3:4,:), ...
                    'i_l_abc',p.invclarke*x(1:2,:),'theta',p.w0*k*p.ts,'vd_pos_ref',0);
                [sn,y]=npc_dq_control(x(11:42,:),inp,p,false);
                mid=a(1:10,1:10)*x(1:10,:)+a(1:10,11:12)*x(45:46,:);
                physical=b(1:10,1:10)*mid+b(1:10,11:12)*x(43:44,:);
                phi=[physical;sn;y.u_alpha;y.u_beta;x(43:44,:)]*phi;
            end
            rows(end+1,:)=[id power extra max(abs(eig(phi)))]; %#ok<AGROW>
        end
    end
end
report=array2table(rows,'VariableNames',{'candidate_id','branch_W','extra_delay_samples','rho_20ms'});
disp(report); writetable(report,fullfile(root,'output','transient_optimize_tune','delay_margin.csv'));
end
