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

cfgC = mex.getCompilerConfigurations('C', 'Selected');
if isempty(cfgC)
    error(['No C compiler is selected for mex. Run "mex -setup C" in MATLAB ', ...
           'and select the MinGW64 C compiler.']);
end

manufacturer = lower(cfgC.Manufacturer);
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

sourceStems = {
    fullfile(commonDir, 'sim_sfunc')
    fullfile(projectDir, 'app', 'app')
    fullfile(projectDir, 'bsp', 'bsp_pwm')
    fullfile(codeDir, 'section', 'section')
    fullfile(codeDir, 'lib', 'fll')
    fullfile(codeDir, 'lib', 'notch')
    fullfile(codeDir, 'lib', 'pi_tustin')
    fullfile(codeDir, 'lib', 'pr')
    fullfile(codeDir, 'lib', 'sogi')
    fullfile(codeDir, 'ctrl', 'inv', 'inv_cfg')
    fullfile(codeDir, 'ctrl', 'inv', 'inv_ctrl')
    fullfile(codeDir, 'ctrl', 'inv', 'inv_fsm')
    fullfile(codeDir, 'ctrl', 'inv', 'inv_hal')
};

sources = resolveSources(sourceStems);
hasCpp = any(endsWith(sources, {'.cc', '.cpp', '.cxx'}, 'IgnoreCase', true));
if hasCpp
    cfgCpp = mex.getCompilerConfigurations('C++', 'Selected');
    if isempty(cfgCpp)
        error(['C++ source files are present. Run "mex -setup C++" in MATLAB ', ...
               'and select the MinGW64 C++ compiler.']);
    end

    manufacturer = lower(cfgCpp.Manufacturer);
    if ~contains(manufacturer, 'gnu') && ~contains(manufacturer, 'mingw')
        error(['This imported inverter project uses GNU section symbols. ', ...
               'Select the MinGW64 C++ compiler with "mex -setup C++" before building.']);
    end
end

includeArgs = cellfun(@(p) ['-I', p], includeDirs, 'UniformOutput', false);
includeArgs = reshape(includeArgs, 1, []);
sources = reshape(sources, 1, []);

mexArgs = {};
mexArgs = [mexArgs, {'-R2018a', '-v', '-outdir', projectDir, '-output', 'sfunc'}];
mexArgs = [mexArgs, {'-DS_FUNCTION_NAME=sfunc', '-DIS_MATLAB', '-DIS_INV', '-D__RAM_FUNC='}];
mexArgs = [mexArgs, {'CFLAGS=$CFLAGS -std=c11 -Wall -Wextra'}];
if hasCpp
    mexArgs = [mexArgs, {'CXXFLAGS=$CXXFLAGS -std=c++17 -Wall -Wextra'}];
end
mexArgs = [mexArgs, {'LDFLAGS=$LDFLAGS -Wl,--undefined=__start_section -Wl,--undefined=__stop_section'}];
mexArgs = [mexArgs, includeArgs, sources];

mex(mexArgs{:});

addpath(projectDir);
mexFile = fullfile(projectDir, ['sfunc.', mexext]);
fprintf('Built %s\n', mexFile);
end

function sources = resolveSources(sourceStems)
sources = cell(size(sourceStems));
for i = 1:numel(sourceStems)
    candidates = {
        [sourceStems{i}, '.c']
        [sourceStems{i}, '.cpp']
        [sourceStems{i}, '.cc']
        [sourceStems{i}, '.cxx']
    };
    exists = cellfun(@(p) exist(p, 'file') == 2, candidates);
    if nnz(exists) == 0
        error('Source file not found: %s.{c,cpp,cc,cxx}', sourceStems{i});
    elseif nnz(exists) > 1
        error('Multiple source files found for module: %s', sourceStems{i});
    end
    sources{i} = candidates{find(exists, 1, 'first')};
end
end
