param([ValidateSet('fifo','new_first')][string]$Policy = 'fifo')
$ErrorActionPreference = 'Stop'
$platform = (Resolve-Path "$PSScriptRoot/../..").Path
$projectSources = 'src/main.c src/systick.c src/gd32e50x_it.c src/syscalls.c bsp/src/bsp_usart.c ../../code/section/srtos_m/section.c tests/srtos_fairness/fairness.c'
$driver = 'Firmware/GD32E50x_standard_peripheral/Source/'
$vendorSources = 'Firmware/CMSIS/GD/GD32E50x/Source/system_gd32e50x.c ' +
    (@('rcu','fmc','gpio','usart','dma','misc','timer') | ForEach-Object { $driver + 'gd32e50x_' + $_ + '.c' }) -join ' '
$defines = '-DGD32E50X -DGD32E50X_CL -DUSE_STDPERIPH_DRIVER -DPLATFORM_GD32E507 -DTOOLCHAIN_GCC -DPERF_TASK_ENABLE=0 -DPERF_INTERRUPT_ENABLE=0 -DPERF_CODE_ENABLE=0'
if ($Policy -eq 'new_first') {
    # Reconstruct the preceding policy in a build-only source copy. The global
    # window, deadlines, context implementation and test workload stay identical.
    $source = Get-Content -Raw -Encoding UTF8 "$platform/../../code/section/srtos_m/section.c"
    function Replace-Once([string]$pattern, [string]$replacement) {
        $regex = [regex]::new($pattern)
        if ($regex.Matches($script:source).Count -ne 1) { throw "Baseline source mismatch: $pattern" }
        $script:source = $regex.Replace($script:source, $replacement, 1)
    }
    Replace-Once '(?m)^static reg_task_t \*p_srtos_task_ready_tail[^\r\n]*' ('$0' + "`nstatic reg_task_t *p_srtos_task_unfinished_first = NULL;`nstatic reg_task_t *p_srtos_task_unfinished_tail = NULL;")
    Replace-Once '(?m)^    p_srtos_task_ready_tail\s*= NULL;' ('$0' + "`n    p_srtos_task_unfinished_first = NULL;`n    p_srtos_task_unfinished_tail = NULL;")
    $pattern = '(reg_task_t \*\*first\s*= &p_srtos_task_)ready(_first;\s*reg_task_t \*\*tail\s*= &p_srtos_task_)ready(_tail;)'
    if ([regex]::Matches($source, $pattern).Count -ne 2) { throw 'Baseline enqueue sites changed' }
    $source = [regex]::Replace($source, $pattern, '${1}unfinished${2}unfinished${3}')
    Replace-Once '(?m)^    reg_task_t \*candidate = srtos_task_ready_pop_unlocked\(&p_srtos_task_ready_first, &p_srtos_task_ready_tail\);' ('$0' + "`n    if (candidate == NULL)`n    {`n        candidate = srtos_task_ready_pop_unlocked(&p_srtos_task_unfinished_first, &p_srtos_task_unfinished_tail);`n    }")
    $source = $source.Replace('if (p_srtos_task_ready_first != NULL)', 'if ((p_srtos_task_ready_first != NULL) || (p_srtos_task_unfinished_first != NULL))')
    Replace-Once '    /\* The fast path must not bypass an older suspended context\. \*/[\s\S]*?    reg_task_t \*next = srtos_task_ready_pop_unlocked\(&p_srtos_task_ready_first, &p_srtos_task_ready_tail\);' "    reg_task_t *next = srtos_task_ready_pop_unlocked(&p_srtos_task_ready_first, &p_srtos_task_ready_tail);`n    if (next == NULL)`n    {`n        return 0u;`n    }"
    [void](New-Item -ItemType Directory -Force -Path "$platform/build_fairness_new_first")
    [IO.File]::WriteAllText("$platform/build_fairness_new_first/section.c", $source, [Text.UTF8Encoding]::new($false))
    $projectSources = $projectSources.Replace('../../code/section/srtos_m/section.c', 'build_fairness_new_first/section.c')
}
Push-Location $platform
try {
    foreach ($reverse in @(0,1)) {
        $build = if ($Policy -eq 'fifo') { "build_fairness_$reverse" } else { "build_fairness_new_first_$reverse" }
        [void](New-Item -ItemType Directory -Force -Path $build)
        & mingw32-make -j8 "BUILD_DIR=$build" 'TARGET=gd32e507_fairness' "PROJECT_SOURCES=$projectSources" "VENDOR_SOURCES=$vendorSources" "PROJECT_DEFINES=$defines -DFAIRNESS_REVERSE=$reverse" all *> "$build/build.log"
        if ($LASTEXITCODE) { Get-Content -Encoding UTF8 "$build/build.log" -Tail 15; throw 'Fairness firmware build failed' }
        Get-Content -Encoding UTF8 "$build/build.log" | Select-String 'FLASH:|RAM:'
    }
} finally { Pop-Location }
