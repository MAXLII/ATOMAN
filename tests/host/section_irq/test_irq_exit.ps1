$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../../..").Path
$output = Join-Path $repo 'build/host/section_irq'
[void](New-Item -ItemType Directory -Force -Path $output)
$source = Get-Content -Raw -Encoding UTF8 "$repo/code/section/srtos_m/section.c"
$functions = foreach ($name in @('section_task_scheduler_started', 'section_task_slice_elapsed', 'section_task_irq_exit_request')) {
    $pattern = '(?m)^(?:void|uint32_t) ' + $name + '\(void\)\r?\n\{[\s\S]*?^\}'
    $match = [regex]::Match($source, $pattern)
    if (!$match.Success) { throw "Production function not found: $name" }
    $match.Value
}
[IO.File]::WriteAllText((Join-Path $output 'irq_exit_production.inc'), ($functions -join "`n"), [Text.UTF8Encoding]::new($false))
& gcc -std=c11 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef -DPLATFORM_TESTBENCH -DSECTION_TESTBENCH_DYNAMIC_TICK -I "$repo/code/section" -I $output "$PSScriptRoot/test_irq_exit.c" -o "$output/test_irq_exit.exe"
if ($LASTEXITCODE) { throw 'SRTOS IRQ test build failed' }
& "$output/test_irq_exit.exe"
if ($LASTEXITCODE) { throw 'SRTOS IRQ test failed' }
