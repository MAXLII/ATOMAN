function route_bridge_build_node02()
% Build node 0x02 directly from shared library sources with MinGW64.
assert(ispc && strcmp(computer('arch'), 'win64'), 'Windows x64 is required.');
root = fileparts(mfilename('fullpath'));
module = 'route_bridge_node02';
output = fullfile(root, 'build');
build = fullfile(output, 'cmake');
assert(~mislocked(module), 'Stop the simulation before rebuilding this S-Function.');
clear(module);
run_command(sprintf(['cmake -S "%s" -B "%s" -G "MinGW Makefiles" ' ...
    '-DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe ' ...
    '-DCMAKE_MAKE_PROGRAM=C:/mingw64/bin/mingw32-make.exe -DMATLAB_ROOT="%s"'], ...
    root, build, matlabroot));
run_command(sprintf('cmake --build "%s" --target %s --parallel 4', build, module));
addpath(output);
rehash;
fprintf('Built %s\n', fullfile(output, [module '.' mexext]));
end

function run_command(command)
[status, output] = system(command);
fprintf('%s', output);
assert(status == 0, 'route_bridge:build', 'Build failed (exit %d).', status);
end
