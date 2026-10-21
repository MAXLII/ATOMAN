param([string]$OutputPath)
$ErrorActionPreference = 'Stop'
$baseRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if (-not $OutputPath) { $OutputPath = Join-Path $baseRoot 'build/host-realtime' }
New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
$flags = @('-std=c11','-Wall','-Wextra','-Werror','-Wpedantic','-Wconversion','-Wsign-conversion',
    '-Wshadow','-Wcast-align','-Wstrict-prototypes','-Wmissing-prototypes','-Wundef',
    '-DPLATFORM_TESTBENCH','-DTOOLCHAIN_GCC','-DREALTIME_SCOPE_ENABLE=1')
foreach ($include in @('code/dbg','code/section/baremetal','code/section','code/data_source/comm','code/lib')) {
    $flags += '-I' + (Join-Path $baseRoot $include)
}
$core = Join-Path $baseRoot 'code/dbg/realtime_scope.c'
foreach ($test in @('test_realtime_scope','test_service')) {
    $sources = @($core,(Join-Path $PSScriptRoot ($test + '.c')))
    $testFlags = $flags
    if ($test -eq 'test_service') {
        $sources += Join-Path $baseRoot 'code/dbg/realtime_scope_service.c'
        $sources += Join-Path $baseRoot 'code/business/demo/demo_realtime_scope.c'
        $testFlags += '-DDEMO_REALTIME_SCOPE_ENABLE=1'
    }
    $executable = Join-Path $OutputPath ($test + '.exe')
    & gcc @testFlags @sources -o $executable
    if ($LASTEXITCODE -ne 0) { throw "$test compile failed" }
    & $executable (Join-Path $OutputPath 'ea.bin')
    if ($LASTEXITCODE -ne 0) { throw "$test failed" }
}
