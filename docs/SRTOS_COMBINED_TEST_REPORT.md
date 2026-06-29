# SRTOS Combined Test Report

## 1. Purpose

This document merges the previously executed SRTOS functional test report with the later critical-section risk reproduction and TIMER2 interrupt jitter stress test.

The goal is to keep one English test record for already covered conditions so that future test work does not repeat the same cases without a reason.

## 2. Test Platform

| Item | Value |
| --- | --- |
| Target MCU | GD32G553RCT6 |
| Firmware project | `gd32g553c` |
| Compiler | GCC through `mingw32-make.exe` |
| Download tool | J-Link |
| Debug interface | SWD, 4000 kHz |
| Serial port used in serial-output tests | COM8 |
| Default debug UART baud rate | 921600 |
| Default SRTOS state | Enabled |
| Firmware version recorded in the original functional report | `1.2.9.6` |
| Functional test date | 2026-06-29 |
| Critical-section and jitter test date | 2026-06-30 |

## 3. Common Pass Criteria

Normal runtime cases pass when:

```text
task_fault_reason == 0
task_context_save_fail_count == 0
task_context_release_fail_count == 0
```

Fault-injection cases pass when:

```text
task_fault_reason == expected_fault_reason
```

The common context pool "keep running" policy passes when:

```text
task_fault_reason == 0
task_context_save_fail_count > 0
task_context_release_fail_count == 0
```

Fault reason mapping:

| Fault reason | Meaning |
| ---: | --- |
| `0` | No fault |
| `1` | Common context pool full |
| `2` | PSP out of bounds |
| `3` | Context restore frame too large |
| `4` | Context release order error |
| `5` | PSP runtime stack configuration too small |

## 4. Consolidated Result Matrix

| Case | Status | Key Result | Duplicate-Test Guidance |
| --- | --- | --- | --- |
| SRTOS-BASIC-001 default 20 s run | Passed | `fault=0`, `save_fail=0`, `release_fail=0` | Do not repeat unless default scheduler code changes. |
| SRTOS-BASIC-002 100 ms / 123 ms long-task UART output | Passed | 108 valid lines in 6 s; both tasks continued output; `fault=0` | Repeat only after UART, print path, or task scheduling changes. |
| SRTOS-SLICE-001 1-tick time slice | Passed | `fault=0`, `save_fail=0`, `release_fail=0` after 10 s | Covered high-frequency switching. |
| SRTOS-SLICE-003 50-tick time slice | Passed | `fault=0`, `save_fail=0`, `release_fail=0` after 10 s | Covered low-frequency switching. |
| SRTOS-CTX-006 variable-duration task | Passed after fix | 5 short runs plus 1 long run, 3 ms period; no guard corruption or pool leak | Covered short/long alternation for one task. |
| SRTOS-POOL-003 16-word pool, fault policy | Passed | `fault_reason=1`, `save_fail=1`, `release_fail=0` | Covered pool-full fault path. |
| SRTOS-POOL-004 16-word pool, keep-running policy | Passed | `fault=0`, `save_fail=51091`, `release_fail=0` after 5 s | Covered pool-full no-switch policy. |
| SRTOS-STACK-002 64-word runtime stack | Passed | `fault=0`, `save_fail=0`, `release_fail=0` | Covered small-stack runtime pressure. |
| SRTOS-STACK-003 32-word runtime stack too small | Passed | `fault_reason=5`, `context_required_words=33` | Covered startup protection for too-small PSP stack. |
| SRTOS-UART-006 9600 baud automatic regression | Blocked | Build failed because `BSP_USART_DBG_BAUDRATE` cannot be overridden through `DEFINES` without redefinition under `-Werror` | Do not rerun until baud-rate configuration is made macro-override safe. |
| SRTOS-CTX-003 floating-point scheduler probe task | Blocked | Build failed due to strict-aliasing and uninitialized warnings in test code | Do not rerun until test code is fixed. |
| Critical-section race probe, empty critical section | Failed as expected | `probe_reentry_count=0x1134` | Risk reproduced; no need to repeat unless queue code changes. |
| Critical-section race probe, PRIMASK only in existing hooks | Failed as expected | `probe_reentry_count=0x091B` | Shows existing hook coverage is insufficient. |
| Critical-section race probe, PRIMASK plus internal SRTOS queue protection | Passed | `probe_reentry_count=0`, `probe_max_depth=1` | This is the validated mitigation for the reproduced race. |
| TIMER2 jitter, 240 tasks, 2048-word common context pool | Failed as expected | Context pool full: `2031 / 2048 words`, `fault_reason=1`; max TIMER2 entry count before fault was 199 ticks | Confirms 2048 words is too small for this stress load. |
| TIMER2 jitter, 240 tasks, 8192-word common context pool | Passed | No SRTOS fault; pool used `2849 / 8192 words`; max TIMER2 entry count 201 ticks, 0.931 us | Covered high task-count PRIMASK jitter stress. |

## 5. Functional Test Details

### 5.1 Default Long Run

Configuration:

```text
Default configuration
Run time: 20 s
```

Result:

```text
task_fault_reason = 0
task_context_save_fail_count = 0
task_context_release_fail_count = 0
```

Conclusion: the default RTOS configuration ran normally in this test.

### 5.2 Two Long Tasks With UART Output

Configuration:

```text
Default configuration
COM8 = 921600 baud
Capture duration: 6 s
```

Result:

```text
valid_output_lines = 108
TASK_DEAD_LOOP_100MS = 60
TASK_DEAD_LOOP_123MS = 48
bad_lines = 1
task_fault_reason = 0
task_context_save_fail_count = 0
task_context_release_fail_count = 0
```

Conclusion: both long-running output tasks continued to run and did not block SRTOS scheduling.

### 5.3 High-Frequency Time Slice

Configuration:

```text
SECTION_TASK_SLICE_TICKS = 1
Run time: 10 s
```

Result:

```text
task_fault_reason = 0
task_context_save_fail_count = 0
task_context_release_fail_count = 0
```

Conclusion: context save and restore worked under high-frequency time slicing.

### 5.4 Common Context Pool Full, Fault Policy

Configuration:

```text
SECTION_TASK_CONTEXT_POOL_WORDS = 16
SECTION_TASK_CONTEXT_POOL_FULL_POLICY = SECTION_TASK_CONTEXT_POOL_FAULT
```

Result:

```text
task_fault_reason = 1
task_context_save_fail_count = 1
task_context_release_fail_count = 0
```

Conclusion: the RTOS correctly entered the pool-full fault path when the common context pool was insufficient.

### 5.5 Common Context Pool Full, Keep-Running Policy

Configuration:

```text
SECTION_TASK_CONTEXT_POOL_WORDS = 16
SECTION_TASK_CONTEXT_POOL_FULL_POLICY = SECTION_TASK_CONTEXT_POOL_KEEP_RUNNING
Run time: 5 s
```

Result:

```text
task_fault_reason = 0
task_context_save_fail_count = 51091
task_context_release_fail_count = 0
```

Conclusion: the keep-running policy worked. When the context pool was insufficient, the RTOS refused the task switch, kept the current task running, did not enter fault, and accumulated save-failure counts.

### 5.6 PSP Runtime Stack Too Small

Configuration:

```text
SECTION_TASK_RUNTIME_STACK_WORDS = 32
```

Result:

```text
task_fault_reason = 5
task_context_required_words = 33
task_context_save_fail_count = 0
task_context_release_fail_count = 0
```

Conclusion: when the runtime stack was smaller than the minimum startup frame requirement, the protection path was correctly triggered.

### 5.7 Variable-Duration Task

Configuration:

```text
DEMO_TASK_DEAD_LOOP_ENABLE = 0
DEMO_TASK_VARIABLE_LENGTH_ENABLE = 1
Task period: 3 ms
Run pattern: 5 short runs, then 1 long run
Short run time: about tens of microseconds
Long run time: about 10 ms
```

Task behavior:

```text
phase 1..5: short path, completes quickly
phase 0: long path, waits for 100 x 100 us ticks, about 10 ms
```

20 s sample result:

```text
enter_count = 15944
short_count = 13287
long_count = 2657
long_enter_count = 2657
max_run_ticks = 106
local_error_count = 0
task_fault_reason = 0
task_context_save_fail_count = 0
task_context_release_fail_count = 0
task_context_pool_used = 69
task_stack_free_words = 495
```

Conclusion: the same task could alternate between completing in one short path and being saved/restored across multiple time slices in the long path. The local guard value was not corrupted, the common context pool did not leak, and no save or release failure was observed.

This case initially triggered `fault_reason=4`. The root cause was the common context pool wrap-around release logic. After the ring buffer wrapped, `head` pointed to a tail gap while the valid restored-task snapshot was at pool offset 0. The release logic checked `snapshot_offset == head` before skipping the gap and therefore misdiagnosed a release order error. The fix was to identify and skip a gap at `head` before and after releasing a snapshot.

## 6. Critical-Section Risk Reproduction

The reviewed risk was that `section_critical_enter()` and `section_critical_exit()` did not actually protect all shared scheduler queue state. The reproduction intentionally allowed `SysTick_Handler()` to touch the task scheduler queue path by enabling a test hook. This simulates future ISR-side task activation or refactoring where ISR code mutates scheduler queues.

Instrumentation defines:

```text
SECTION_CRITICAL_RACE_PROBE_ENABLE=1
SECTION_CRITICAL_RACE_PROBE_SPIN=512
SECTION_TASK_TICK_FROM_SYSTICK_TEST=1
DEMO_TASK_DEAD_LOOP_ENABLE=0
DEMO_TASK_VARIABLE_LENGTH_ENABLE=1
```

Probe counters:

| Counter | Meaning |
| --- | --- |
| `probe_enter_count` | Number of instrumented queue operation entries |
| `probe_reentry_count` | Number of times a queue operation was entered while another queue operation was still active |
| `probe_invariant_fail_count` | Basic queue invariant failure count |
| `probe_max_depth` | Maximum nested queue operation depth |
| `critical_enter_count` | Critical-section entry count |
| `critical_exit_count` | Critical-section exit count |

### 6.1 Empty Critical Section

Result after about 5 s:

| Field | Value |
| --- | ---: |
| `probe_enter_count` | `0x00013615` |
| `probe_reentry_count` | `0x00001134` |
| `probe_invariant_fail_count` | `0x00000000` |
| `probe_max_depth` | `0x00000002` |
| `critical_enter_count` | `0x0010B009` |
| `critical_exit_count` | `0x0010AFC3` |
| `task_fault_reason` | `0` |
| `task_context_save_fail_count` | `0` |
| `task_context_release_fail_count` | `0` |

Conclusion: the race risk is reproducible. The scheduler queue path was re-entered thousands of times while an existing queue operation was active.

### 6.2 PRIMASK Only In Existing Critical Hooks

Additional define:

```text
SECTION_CRITICAL_USE_PRIMASK=1
```

Result after about 5 s:

| Field | Value |
| --- | ---: |
| `probe_enter_count` | `0x0001165A` |
| `probe_reentry_count` | `0x0000091B` |
| `probe_invariant_fail_count` | `0x00000000` |
| `probe_max_depth` | `0x00000002` |
| `critical_enter_count` | `0x000EFDAB` |
| `critical_exit_count` | `0x000EFDAB` |
| `task_fault_reason` | `0` |
| `task_context_save_fail_count` | `0` |
| `task_context_release_fail_count` | `0` |

Conclusion: adding PRIMASK only inside the existing `section_critical_enter()` and `section_critical_exit()` call sites is not sufficient. Some SRTOS ready/unfinished queue operations are still executed outside those existing critical-section boundaries.

### 6.3 PRIMASK With Internal SRTOS Queue Protection

Additional defines:

```text
SECTION_CRITICAL_USE_PRIMASK=1
SECTION_SRTOS_QUEUE_INTERNAL_CRITICAL=1
```

Result after about 5 s:

| Field | Value |
| --- | ---: |
| `probe_enter_count` | `0x00013A38` |
| `probe_reentry_count` | `0x00000000` |
| `probe_invariant_fail_count` | `0x00000000` |
| `probe_max_depth` | `0x00000001` |
| `critical_enter_count` | `0x0011FD64` |
| `critical_exit_count` | `0x0011FD62` |
| `task_fault_reason` | `0` |
| `task_context_save_fail_count` | `0` |
| `task_context_release_fail_count` | `0` |

Conclusion: protecting the SRTOS queue operations themselves removed the observed re-entry under the same forced ISR access pattern.

## 7. TIMER2 Interrupt Jitter Stress Test

This test measured TIMER2 interrupt entry delay when SRTOS critical sections are protected with PRIMASK.

The measurement records `TIMER_CNT(TIMER2)` immediately at the beginning of `TIMER2_IRQHandler()`, before checking or clearing the TIMER2 update flag and before calling `section_interrupt()`.

This placement matters because, if the TIMER2 update event occurs while interrupts are disabled, the ISR only starts after interrupts are re-enabled. The TIMER2 counter value at ISR entry therefore directly reflects the delayed entry time after the hardware event.

### 7.1 Stress Firmware

Build options for the no-fault stress run:

```text
DEMO_JITTER_CAPTURE_ENABLE=1
DEMO_TASK_DEAD_LOOP_ENABLE=0
DEMO_TASK_VARIABLE_LENGTH_ENABLE=0
SECTION_CRITICAL_USE_PRIMASK=1
SECTION_SRTOS_QUEUE_INTERNAL_CRITICAL=1
SECTION_TASK_RUNTIME_STACK_WORDS=1024
SECTION_TASK_CONTEXT_POOL_WORDS=8192
```

Stress load:

- 240 registered periodic tasks.
- Task periods are intentionally unordered and non-uniform.
- Period range: 1 to 1499 SRTOS ticks.
- Workload mix: short CPU loops, medium CPU loops, long 15-21 tick tasks, and variable long/short tasks with periodic 20-30 tick long runs.

TIMER2 setup observed from firmware:

- TIMER2 interrupt rate: 30,000 Hz.
- TIMER2 counter clock: 216,000,000 Hz.
- TIMER2 period: 7200 timer ticks.
- One TIMER2 counter tick: 4.6296 ns.

### 7.2 Measurement Variables

RAM ring buffer:

```text
g_demo_jitter_timer2_count[1024]
```

Online summary:

```text
g_demo_jitter_debug.min_entry_count
g_demo_jitter_debug.max_entry_count
g_demo_jitter_debug.last_entry_count
g_demo_jitter_debug.write_index
g_demo_jitter_debug.timer2_counter_hz
g_demo_jitter_debug.timer2_period_ticks
```

The online maximum is the important worst-case value because the RAM ring buffer only keeps the latest 1024 samples and can overwrite an earlier peak.

### 7.3 240 Tasks, 2048-Word Common Context Pool

Configuration:

```text
SECTION_TASK_RUNTIME_STACK_WORDS=1024
SECTION_TASK_CONTEXT_POOL_WORDS=2048
```

Result:

- The firmware reached a context pool full fault.
- `task_context_pool_used`: 2031 / 2048 words.
- `task_context_save_fail_count`: 1.
- `task_fault_reason`: 1, context pool full.
- `task_runtime_stack_used_words`: 31 words.
- `task_stack_free_words`: 993 / 1024 words.

Jitter data before the fault:

- Online minimum TIMER2 entry count: 15 ticks.
- Online maximum TIMER2 entry count: 199 ticks.
- Online maximum entry delay: 0.921 us.
- Last 1024 RAM samples: min 15, max 105, P50 21, P90 21, P99 62, P99.9 100 ticks.

Conclusion: 2048 words is not enough common context pool for this 240-task stress case.

### 7.4 240 Tasks, 8192-Word Common Context Pool

Configuration:

```text
SECTION_TASK_RUNTIME_STACK_WORDS=1024
SECTION_TASK_CONTEXT_POOL_WORDS=8192
```

Result:

- No SRTOS fault was observed during the sampled run.
- `task_fault_reason`: 0.
- `task_context_save_fail_count`: 0.
- `task_context_release_fail_count`: 0.
- `task_context_pool_used`: 2849 / 8192 words.
- `task_runtime_stack_used_words`: 17 words.
- `task_stack_free_words`: 1007 / 1024 words.

TIMER2 entry statistics:

- Captured online sample count: 360,571 TIMER2 interrupts.
- Online minimum TIMER2 entry count: 15 ticks.
- Online maximum TIMER2 entry count: 201 ticks.
- Online maximum entry delay: 0.931 us.
- Last observed entry count: 21 ticks.
- Last 1024 RAM samples: min 21, max 95, P50 21, P90 21, P99 63, P99.9 88 ticks.

Interpretation:

```text
201 / 216,000,000 = 0.931 us
21 / 216,000,000 = 0.097 us
(201 - 21) / 216,000,000 = 0.833 us
```

The observed worst-case additional TIMER2 entry delay relative to the common 21-tick entry point was about 0.833 us.

## 8. Blocked Or Not-Yet-Covered Items

| Item | Status | Reason |
| --- | --- | --- |
| 9600 baud automatic regression | Blocked | `BSP_USART_DBG_BAUDRATE` is defined directly inside `bsp_usart.c`, so overriding it through `DEFINES` causes redefinition under `-Werror`. A previous temporary source edit tested 921600 down to 9600 without RTOS fault, but the automatic regression case is not counted as passed. |
| Floating-point scheduler probe | Blocked | `DEMO_TASK_SCHED_PROBE_ENABLE=1` failed to build because the test code used type-punned floating-point bit access and triggered strict-aliasing / uninitialized warnings. |
| SRTOS-POOL-005 long-term ring head/tail wrap statistics | Partially covered | Context-pool save/restore and wrap-related release behavior were covered, but there is no dedicated long-duration counter for wrap events. |
| SRTOS-FAULT-002 PSP out-of-bounds fault injection | Not executed | Requires a large local array or abnormal SP injection test. |
| SRTOS-FAULT-003 context restore frame too large | Not executed | Requires a dedicated abnormal snapshot-size injection test. |
| SRTOS-FAULT-004 context release order error injection | Not executed as a planned injection | A real release-order false positive was found and fixed during the variable-duration task test, but no dedicated injected release-order test was executed. |

## 9. Overall Findings

The executed tests cover the following behavior:

- Default RTOS long run.
- Concurrent long-running tasks with UART output.
- High-frequency and low-frequency time slicing.
- Context save and restore for one task alternating between short and long execution paths.
- Common context pool full behavior under both fault and keep-running policies.
- Runtime stack too-small protection.
- Small 64-word runtime stack pressure.
- Scheduler queue critical-section race reproduction and mitigation.
- TIMER2 interrupt entry jitter under a 240-task mixed workload with PRIMASK-based critical sections.

The current strongest conclusions are:

- Core scheduling, context save/restore, common context pool policy, and PSP runtime stack protection behaved correctly in the executed functional tests.
- Empty critical sections are not safe if scheduler queues can be touched from both exception context and thread/PendSV paths.
- PRIMASK only in the previous generic critical hooks is insufficient; the SRTOS queue operations themselves need critical-section protection.
- With internal SRTOS queue protection enabled and a 240-task mixed stress load, a 2048-word common context pool is insufficient, while an 8192-word pool passed the sampled run and reached 2849 words used.
- Under that same 240-task PRIMASK stress run, the measured worst TIMER2 ISR entry delay was 0.931 us on the GD32G553 board.

## 10. Recommendations

- Keep the current SysTick path lightweight. SysTick should request PendSV and avoid mutating scheduler queues unless there is a documented design reason.
- Protect all SRTOS ready/unfinished queue mutations and queue pops with a real critical section.
- Use PRIMASK only when the project interrupt-latency budget accepts the measured worst-case delay and future tests keep the protected regions short.
- Consider a BASEPRI implementation if high-priority control interrupts must remain unmasked. Any interrupt that calls SRTOS scheduler APIs must run at a priority masked by the SRTOS critical-section threshold.
- Do not use a 2048-word common context pool for the 240-task stress level.
- Use at least an 8192-word common context pool for the current 240-task stress scenario, then re-test with the real application task set.
- Make `BSP_USART_DBG_BAUDRATE` macro-override safe so low-baud serial regression can be automated.
- Fix the floating-point scheduler probe test code so it is compatible with strict-aliasing and `-Werror`.
- Add permanent debug-only counters for queue re-entry, queue invariant violations, and context-pool head/tail wrap events.

