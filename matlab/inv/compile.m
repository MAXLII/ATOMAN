function mexFile = compile()
%COMPILE Build the inverter C-MEX S-Function.
%
% Run this script from MATLAB:
%   cd <repo_root>/matlab/inv
%   compile

projectDir = fileparts(mfilename('fullpath'));
matlabDir = fileparts(projectDir);
repoDir = fileparts(matlabDir);
commonDir = fullfile(matlabDir, 'common');
codeDir = fullfile(repoDir, 'code');

cfg = mex.getCompilerConfigurations('C', 'Selected');
if isempty(cfg)
    error(['No C compiler is selected for mex. Run "mex -setup C" in MATLAB ', ...
           'and select the MinGW64 C compiler.']);
end

manufacturer = lower(cfg.Manufacturer);
if ~contains(manufacturer, 'gnu') && ~contains(manufacturer, 'mingw')
    error(['This imported inverter project uses GNU section symbols. ', ...
           'Select the MinGW64 C compiler with "mex -setup C" before building.']);
end

includeDirs = {
    projectDir
    fullfile(projectDir, 'app')
    fullfile(projectDir, 'bsp')
    commonDir
    fullfile(codeDir, 'section')
    fullfile(codeDir, 'lib')
    fullfile(codeDir, 'ctrl')
    fullfile(codeDir, 'ctrl', 'inv')
};

sources = {
    fullfile(commonDir, 'sim_sfunc.c')
    fullfile(projectDir, 'app', 'app.c')
    fullfile(projectDir, 'bsp', 'bsp_pwm.c')
    fullfile(codeDir, 'section', 'section.c')
    fullfile(codeDir, 'lib', 'fll.c')
    fullfile(codeDir, 'lib', 'notch.c')
    fullfile(codeDir, 'lib', 'pi_tustin.c')
    fullfile(codeDir, 'lib', 'pr.c')
    fullfile(codeDir, 'lib', 'sogi.c')
    fullfile(codeDir, 'ctrl', 'inv', 'inv_cfg.c')
    fullfile(codeDir, 'ctrl', 'inv', 'inv_ctrl.c')
    fullfile(codeDir, 'ctrl', 'inv', 'inv_fsm.c')
    fullfile(codeDir, 'ctrl', 'inv', 'inv_hal.c')
};

includeArgs = cellfun(@(p) ['-I', p], includeDirs, 'UniformOutput', false);
includeArgs = reshape(includeArgs, 1, []);
sources = reshape(sources, 1, []);

mexArgs = {};
mexArgs = [mexArgs, {'-R2018a', '-v', '-outdir', projectDir, '-output', 'sfunc'}];
mexArgs = [mexArgs, {'-DS_FUNCTION_NAME=sfunc', '-DIS_MATLAB', '-DIS_INV', '-D__RAM_FUNC='}];
mexArgs = [mexArgs, {'CFLAGS=$CFLAGS -std=c11 -Wall -Wextra'}];
mexArgs = [mexArgs, {'LDFLAGS=$LDFLAGS -Wl,--undefined=__start_section -Wl,--undefined=__stop_section'}];
mexArgs = [mexArgs, includeArgs, sources];

mex(mexArgs{:});

addpath(projectDir);
mexFile = fullfile(projectDir, ['sfunc.', mexext]);
fprintf('Built %s\n', mexFile);
end
