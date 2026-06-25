function mexFile = compile()
%COMPILE Build the inverter C-MEX S-Function with the selected MATLAB mex compiler.
%
% Run this script from MATLAB:
%   cd <repo_root>/matlab/inv
%   compile

% Path layout, with the base repository root written as ./:
projectDir = fileparts(mfilename('fullpath')); % projectDir = ./matlab/inv
matlabDir  = fileparts(projectDir);            % matlabDir  = ./matlab
repoDir    = fileparts(matlabDir);             % repoDir    = ./
commonDir  = fullfile(matlabDir, 'common');    % commonDir  = ./matlab/common
codeDir    = fullfile(repoDir, 'code');        % codeDir    = ./code

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
includeArgs = cellfun(@(p) ['-I', p], includeDirs, 'UniformOutput', false);
includeArgs = reshape(includeArgs, 1, []);
sources = reshape(sources, 1, []);

mexArgs = {};
mexArgs = [mexArgs, {'-R2018a', '-outdir', projectDir, '-output', 'sfunc'}];
mexArgs = [mexArgs, {'-DS_FUNCTION_NAME=sfunc', '-DIS_MATLAB', '-DIS_INV', '-DTOOLCHAIN_MSVC', '-D__RAM_FUNC='}];
mexArgs = [mexArgs, {'COMPFLAGS=$COMPFLAGS /W3'}];
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
