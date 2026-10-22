$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot/../../..").Path
$output = Join-Path $repo 'build/host/section_schedule'
[void](New-Item -ItemType Directory -Force -Path $output)
function Read-Function([string]$source, [string]$name) {
    $pattern = '(?m)^(?:static )?(?:void|uint32_t|reg_task_t \*)\s*' + $name + '\([^;]*?\)\r?\n\{[\s\S]*?^\}'
    $match = [regex]::Match($source, $pattern)
    if (!$match.Success) { throw "Production function missing: $name" }
    return $match.Value
}
$encoding = [Text.UTF8Encoding]::new($false)
$bare = Get-Content -Raw -Encoding UTF8 "$repo/code/section/baremetal/section.c"
foreach ($runtime in @('srtos_m', 'srtos_a9')) {
    $source = Get-Content -Raw -Encoding UTF8 "$repo/code/section/$runtime/section.c"
    $state = [regex]::Match($source, '(?m)^typedef enum\r?\n\{[^}]*\} task_stack_state_t;').Value
    if (!$state) { throw 'Task state declaration missing' }
    [IO.File]::WriteAllText("$output/task_state.inc", $state, $encoding)
    $functions = foreach ($name in @('srtos_task_schedule_next','srtos_task_ready_enqueue_unlocked','srtos_task_ready_pop_unlocked','srtos_task_activate_if_due','task_stack_pick_next','task_slice_advance','section_task_slice_elapsed','section_task_complete_current','section_task_continue_current','task_activate_if_due','section_task_tick')) { Read-Function $source $name }
    if ($runtime -eq 'srtos_m') { $functions += Read-Function $source 'section_task_start_ready_new_inline' }
    $functions += (Read-Function $bare 'task_schedule_next').Replace('task_schedule_next(', 'baremetal_task_schedule_next(')
    [IO.File]::WriteAllText("$output/schedule_production.inc", ($functions -join "`n"), $encoding)
    $a9 = if ($runtime -eq 'srtos_a9') { 1 } else { 0 }
    & gcc -std=c11 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Wundef -DPERF_CODE_ENABLE=0 -DPERF_TASK_ENABLE=0 -DPERF_INTERRUPT_ENABLE=0 -DPLATFORM_TESTBENCH -DSECTION_TESTBENCH_DYNAMIC_TICK "-DTEST_A9=$a9" -I "$repo/code/section/$runtime" -I "$repo/code/section" -I "$repo/code/dbg" -I $output "$PSScriptRoot/test_schedule.c" -o "$output/$runtime.exe"
    if ($LASTEXITCODE) { throw "$runtime policy compilation failed" }
    & "$output/$runtime.exe"
    if ($LASTEXITCODE) { throw "$runtime policy test failed" }
}
