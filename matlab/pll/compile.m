function mexFile = compile()
%COMPILE Build the PLL C MEX S-Function with the selected MATLAB mex compiler.
%
% Run this script from MATLAB:
%   cd <repo_root>/matlab/pll
%   compile

% Path layout, with the base repository root written as ./:
projectDir = fileparts(mfilename('fullpath')); % projectDir = ./matlab/pll
matlabDir  = fileparts(projectDir);            % matlabDir  = ./matlab
repoDir    = fileparts(matlabDir);             % repoDir    = ./
commonDir  = fullfile(matlabDir, 'common');    % commonDir  = ./matlab/common
codeDir    = fullfile(repoDir, 'code');        % codeDir    = ./code

includeDirs = {
    projectDir
    fullfile(projectDir, 'app')
    commonDir
    fullfile(codeDir, 'section')
    fullfile(codeDir, 'lib')
};

sourceStems = {
    fullfile(commonDir, 'sim_sfunc')
    fullfile(projectDir, 'app', 'app')
    fullfile(codeDir, 'section', 'section')
    fullfile(codeDir, 'lib', 'pi_tustin')
    fullfile(codeDir, 'lib', 'pll')
};

sources = resolveSources(sourceStems);
includeArgs = cellfun(@(p) ['-I', p], includeDirs, 'UniformOutput', false);
includeArgs = reshape(includeArgs, 1, []);
sources = reshape(sources, 1, []);

clear sfunc;
clear mex;

compiler = mex.getCompilerConfigurations('C', 'Selected');
if isempty(compiler)
    error('No selected MATLAB C mex compiler. Run mex -setup C first.');
end

isMsvc = contains(compiler.Manufacturer, 'Microsoft', 'IgnoreCase', true);
if isMsvc
    toolchainArg = '-DTOOLCHAIN_MSVC';
    warningArgs = {'COMPFLAGS=$COMPFLAGS /W3'};
else
    toolchainArg = '-DTOOLCHAIN_GCC';
    warningArgs = {'COMPFLAGS=$COMPFLAGS -Wall'};
end

mexArgs = {};
mexArgs = [mexArgs, {'-R2018a', '-outdir', projectDir, '-output', 'sfunc'}];
mexArgs = [mexArgs, {'-DS_FUNCTION_NAME=sfunc', '-DIS_MATLAB', '-DIS_PLL', toolchainArg, '-D__RAM_FUNC='}];
mexArgs = [mexArgs, warningArgs];
mexArgs = [mexArgs, includeArgs, sources];

mex(mexArgs{:});

addpath(projectDir);
mexFile = fullfile(projectDir, ['sfunc.', mexext]);
fprintf('Built %s\n', mexFile);
end

function sources = resolveSources(sourceStems)
sources = cell(size(sourceStems));
for i = 1:numel(sourceStems)
    source = [sourceStems{i}, '.c'];
    if exist(source, 'file') ~= 2
        error('C source file not found: %s.c', sourceStems{i});
    end
    sources{i} = source;
end
end
