function npc_predictor_design(out,p)
% Exact sampled LC model plus a sinusoidal load-disturbance observer.
% The observer uses terminal voltage and applied command only; no plant states.
if exist('OCTAVE_VERSION','builtin'), pkg load control; end
wr = 1/sqrt(p.L*p.C); w = 2*pi*50; ts = p.ts;
ac = [0 wr 0 0; -wr -p.R/p.L wr 0; 0 0 0 w; 0 0 -w 0];
bc = [0;wr;0;0];
ed = expm([ac bc;zeros(1,5)]*ts);
a = ed(1:4,1:4); b = ed(1:4,5); c = [1 0 0 0];
k = place(a(1:2,1:2),b(1:2),[0.70 0.75]);
observer_poles = [0.2 0.35 0.9*exp(1i*w*ts) 0.9*exp(-1i*w*ts)];
% Correction before prediction: error transition A*(I-L*C).
l = a\place(a',c',observer_poles)';
acl = a(1:2,1:2)-b(1:2)*k;
z = exp(1i*w*ts);
g = [1 0]/(z*eye(2)-acl);
reference = 1/(g*b(1:2));
disturbance = (g*a(1:2,3:4)*[1;1i])/(g*b(1:2));
assert(max(abs(eig(a*(eye(4)-l*c))))<1);
assert(max(abs(eig(acl)))<1);
fid=fopen(fullfile(out,'predictor.h'),'w'); assert(fid>=0);
fprintf(fid,'static const double predictor_a[4][4] = {\n');
for row=1:4, fprintf(fid,'{');fprintf(fid,'%.17g,',a(row,:));fprintf(fid,'},\n');end
fprintf(fid,'};\n');
write_vector(fid,'predictor_b',b);
write_vector(fid,'predictor_l',l);
write_vector(fid,'predictor_k',k);
write_vector(fid,'predictor_reference',[real(reference) imag(reference)]);
write_vector(fid,'predictor_disturbance',[real(disturbance) imag(disturbance)]);
fclose(fid);
end
function write_vector(fid,name,x)
fprintf(fid,'static const double %s[%d] = {',name,numel(x));
fprintf(fid,'%.17g,',x); fprintf(fid,'};\n');
end
