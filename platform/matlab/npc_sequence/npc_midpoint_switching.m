function results = npc_midpoint_switching(substeps, selected, delay_periods, capacitor_series_resistance)
% Native production C controller + switched NPC capacitor-charge validation.
% Run in MATLAB or Octave. Requires C:/mingw64/bin/gcc.exe.
% 0 = balance disabled; 1 = previous raw-current path; 2 = production fix.
% 4/5 are offline experimental bias damping / common-mode slew, NOT firmware.
% 6 executes the production low-frequency bias correction, with no extra native compensation.
% 16 executes the C voltage-damping candidate with gain 0.15; its release default is disabled.
% Cases 13/14/17: unbalanced / light / balanced full load; optional delay_periods is 0 or 1.
% Cases 19:25 call production averaged balancing and the actual gate-duty adapter.
% 19/20/21: unbalanced/light/full; 22/23: unbalanced/full load steps; 24/25: +/-100 V.
% Optional capacitor_series_resistance changes only this offline plant, never npc.plecs.
% Cases include +/-100 V initial error and 4..8 s balanced/unbalanced load steps.
% Case 8 starts with constant 1 MW / 1 W / 1 W and 1 kohm / 1.1 kohm bleeders.
% The test approximates diode commutation with bounded implicit pole voltages.
% It cannot certify the PLECS event solver, device losses or real hardware.
if nargin < 1, substeps = 200; end
if nargin < 3, delay_periods = 1; end
if nargin < 4, capacitor_series_resistance = 0; end
cases = [0 0 0; 1 0 0; 2 0 0; 2 100 0; 2 -100 0; 2 0 1; 2 0 2; 2 -63.38 3; 4 -63.38 3; 5 -63.38 3; 6 -63.38 3; 6 0 0; 16 -63.38 3; 16 0 0; 16 0 1; 16 0 2; 16 0 4; 6 0 4;
    80 -63.38 3;80 0 0;80 0 4;80 0 1;80 0 2;80 100 0;80 -100 0;
    82 0 5;82 0 4;82 0 3];
if nargin < 2, selected = 1:8; end % Experimental cases require explicit selection.
folder = fileparts(mfilename('fullpath'));
repo = fileparts(fileparts(fileparts(folder)));
out = fullfile(repo,'platform','plecs','npc','build','midpoint_analysis',sprintf('steps_%d',substeps));
if delay_periods == 0, out = fullfile(out,'immediate'); end
if capacitor_series_resistance > 0
    out = fullfile(out,sprintf('capacitor_series_%g',capacitor_series_resistance));
end
if ~exist(out,'dir'), mkdir(out); end
m = npc_yd_model(); p = npc_yd_design_config(); dt = p.ts/substeps;
npc_predictor_design(out,p);
% Three HV delta-branch power configurations at nominal 34.5 kV line voltage.
powers = [1e3 1e3 1e3; 1e4 1e5 1e6; 1e6 1e6 1e6; 1e6 1 1; 1 1 1];
fid = fopen(fullfile(out,'plant.h'),'w'); assert(fid >= 0);
fprintf(fid,'#define SUBSTEPS %d\n#define DT %.17g\n',substeps,dt);
count = size(powers,1);
aa = zeros(10,10,count); bb = zeros(10,3,count); hh = zeros(3,3,count);
voltage_output = zeros(2,10); voltage_output(:,3:4) = eye(2);
voltage_change = zeros(2,10);
voltage_change(:,1:2) = capacitor_series_resistance*eye(2);
voltage_change(:,5:6) = -capacitor_series_resistance*eye(2);
voltage_output = voltage_output + voltage_change;
for n = 1:count
    g = m.plant(p,m.tr,34500^2./powers(n,:));
    % A series capacitor resistor changes terminal voltage by R*(iL-i_primary).
    % Keep the capacitor internal voltage as the state; feed back terminal voltage.
    g.Ac(1:2,:) = g.Ac(1:2,:) - voltage_change/p.L;
    g.Ac(5:6,:) = g.Ac(5:6,:) + voltage_change/m.tr.L1;
    ed = expm([g.Ac g.Bc;zeros(2,12)]*dt);
    aa(:,:,n) = ed(1:10,1:10);
    bb(:,:,n) = ed(1:10,11:12)*p.clarke;
    hh(:,:,n) = p.invclarke*bb(1:2,:,n);
end
write_matrices(fid,'plant_a',aa);
write_matrices(fid,'plant_b',bb);
write_matrices(fid,'current_gain',hh);
write_matrices(fid,'voltage_output',voltage_output);
fprintf(fid,'static const double inverse_clarke[3][2] = {');
for row=1:3, fprintf(fid,'{%.17g,%.17g},',p.invclarke(row,:)); end
fprintf(fid,'};\n'); fclose(fid);

includes = {'code/ctrl/npc','code/interface/npc/common','code/lib','code/section','code/section/baremetal','code/dbg'};
cmd = 'C:/mingw64/bin/gcc.exe -std=c11 -O2 -DPLATFORM_TESTBENCH';
for n=1:numel(includes), cmd=[cmd ' -I"' fullfile(repo,includes{n}) '"']; end
cmd=[cmd ' -I"' out '" "' fullfile(folder,'npc_midpoint_native.c') '"'];
sources={'code/ctrl/npc/npc_cfg.c','code/lib/dsogi.c','code/lib/sogi.c','code/lib/svpwm_3level.c','code/lib/pr.c'};
for n=1:numel(sources), cmd=[cmd ' "' fullfile(repo,sources{n}) '"']; end
% Use a fresh output path so a failed Windows compiler invocation cannot run an old binary.
exe=[tempname(out) '.exe']; cmd=[cmd ' -o "' exe '"'];
[status, message]=system(cmd); assert(status==0,message);
assert(exist(exe,'file')==2,'Native compilation did not produce a fresh executable.');
previous=pwd; cleanup=onCleanup(@()cd(previous)); cd(out);
results=zeros(numel(selected),6);
for n=1:numel(selected)
    c=cases(selected(n),:);
    run_cmd = sprintf('"%s" %d %g %d',exe,c);
    if delay_periods == 0, run_cmd = [run_cmd ' 0.1 0.00001 0']; end
    [status,message]=system(run_cmd);
    fprintf('%s',message); assert(status==0,'Native switching run failed.');
    file=sprintf('mode_%d_delta_%g_scenario_%d.csv',c);
    trace=dlmread(file,',',1,0); tail=trace(:,1)>=11;
    assert(trace(end,1)>11.999,'Native run ended before the final analysis interval.');
    results(n,:)=[c max(abs(trace(:,2))) max(abs(trace(tail,2))) max(trace(:,6))];
    if c(1)==2 && c(3)==0
        assert(results(n,5)<5,'Corrected light-load midpoint did not settle within 5 V.');
    end
    if results(n,6)>1000
        warning('Load scenario %d has output overvoltage; midpoint settling is not full-loop acceptance.',c(3));
    end
end
fid=fopen('metrics.csv','w');
fprintf(fid,'mode,initial_delta,load_scenario,peak_delta_v,tail_peak_delta_v,peak_voltage_vector_v\n');
fprintf(fid,'%g,%g,%g,%.9g,%.9g,%.9g\n',results'); fclose(fid);
disp(results);
end

function write_matrices(fid,name,values)
[rows,cols,count]=size(values);
fprintf(fid,'static const double %s[%d][%d][%d] = {\n',name,count,rows,cols);
for matrix=1:count
    fprintf(fid,'{');
    for row=1:rows
        fprintf(fid,'{'); fprintf(fid,'%.17g,',values(row,:,matrix)); fprintf(fid,'},\n');
    end
    fprintf(fid,'},\n');
end
fprintf(fid,'};\n');
end
