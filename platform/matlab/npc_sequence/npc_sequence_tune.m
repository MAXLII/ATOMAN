function result = npc_sequence_tune()
% Optional small-signal screening; never overwrites the validated configuration.
p=npc_sequence_config(); plants={npc_sequence_plant(p,p.load_ohm), ...
 npc_sequence_plant(p,[5 5 5]),npc_sequence_plant(p,[5 10 20])};
rows=zeros(27,4); row=0; n=round(p.fs/p.f0);
for kp=[0.3 0.6 0.9]
  for kv=[0.01 0.03 0.05]
   for iv=[0.5 1 2]
    p.kpi=kp; p.kpv=kv; p.kiv=iv;
    rho=0;
    for gp=1:numel(plants)
     phi=eye(26);
     for k=0:n-1
      a=npc_sequence_step(eye(26),2*pi*k/n,0,p,plants{gp},false);
      phi=a*phi;
     end
     rho=max(rho,max(abs(eig(phi))));
    end
    row=row+1; rows(row,:)=[kp kv iv rho];
   end
  end
end
result=sortrows(array2table(rows,'VariableNames',{'current_reference_gain','voltage_kp','voltage_ki','worst_rho'}),'worst_rho');
disp(result(1:5,:));
out=fullfile(fileparts(mfilename('fullpath')),'output');
if ~exist(out,'dir'), mkdir(out); end
writetable(result,fullfile(out,'tuning_candidates.csv'));
end
