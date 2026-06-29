# SRTOS Risk-Oriented Test Report

## 1. Purpose

This report defines and records risk-oriented SRTOS tests for the GD32G553 platform.

The purpose is not to optimize SRTOS behavior. The purpose is to expose potential scheduler, context, stack, pool, timing, and interrupt-latency risks, then record the observed behavior even when the result is not visually perfect but still acceptable for the current design.

## 2. Public References Used For Test Design

This is not a standards compliance or certification report. The public standards and references below were used only to select test dimensions and documentation style:

- ISO/IEC/IEEE 29119 software testing: used as a general test-process and test-documentation reference. IEEE describes 29119 as a software testing standards series that defines general concepts, test processes, documentation, and techniques usable by organizations performing software testing. Source: <https://standards.ieee.org/ieee/29119-1/10779/> and <https://www.iso.org/standard/56737.html>.
- POSIX real-time interfaces: used as a reference for common RTOS capability categories such as scheduling, timers, synchronization, and priority-related behavior. Source: <https://pubs.opengroup.org/onlinepubs/007908799/xsh/realtime.html> and <https://pubs.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_08.html>.
- IEC 61508 functional-safety verification mindset: used as a reference for systematic verification, validation, boundary testing, and fault-injection thinking. Source: <https://www.parasoft.com/solutions/iec-61508/> and <https://risknowlogy.com/articles/detail/14608/>.

The current SRTOS is intentionally small and does not expose a POSIX-like API set. Therefore, tests are adapted to the actual mechanisms in this repository: auto-registered periodic tasks, time slicing, shared ready/unfinished task queues, common context pool, PSP runtime stack, PendSV/TIMER2 switching, and compile-time diagnostics.

## 3. Test Platform

| Item | Value |
| --- | --- |
| Target MCU | GD32G553RCT6 |
| Firmware project | `gd32g553c` |
| Compiler | GCC through `mingw32-make.exe` |
| Download tool | J-Link |
| Debug interface | SWD, 4000 kHz |
| Test method | Build firmware, download through J-Link, run on hardware, read RAM debug counters |
| Test date | 2026-06-30 |

## 4. New Risk Test Instrumentation

The new risk test module is disabled by default and is compiled in only when:

```text
DEMO_RTOS_RISK_TEST_ENABLE=1
```

Files added:

```text
code/app/demo/demo_rtos_risk.c
code/app/demo/demo_rtos_risk.h
```

The module registers optional test tasks and records results in:

```text
g_demo_rtos_risk_debug
g_section_fault_debug
```

The module does not modify SRTOS core code. It only adds tasks that stress behavior through the existing task registration mechanism.

## 5. Test Case Inventory

The following test cases are adapted from the public RTOS test dimensions above and from the actual SRTOS implementation.

| ID | Test Case | Risk Target | Status |
| --- | --- | --- | --- |
| RTOS-RISK-001 | Default 20 s run | Basic scheduler stability | Executed previously, passed |
| RTOS-RISK-002 | Two long UART print tasks, 100 ms and 123 ms | Long task coexistence and output path blocking | Executed previously, passed |
| RTOS-RISK-003 | 1-tick time slice | High-frequency context switching | Executed previously, passed |
| RTOS-RISK-004 | 50-tick time slice | Low-frequency context switching | Executed previously, passed |
| RTOS-RISK-005 | Variable-duration task, short/long alternating | Context save/restore when a task sometimes finishes and sometimes spans slices | Executed previously, passed after one release-order bug fix |
| RTOS-RISK-006 | 16-word context pool with fault policy | Context pool exhaustion fault path | Executed previously, passed |
| RTOS-RISK-007 | 16-word context pool with keep-running policy | Pool-full no-switch behavior | Executed previously, passed |
| RTOS-RISK-008 | 64-word runtime stack | Small PSP runtime stack pressure | Executed previously and re-executed, passed but tight |
| RTOS-RISK-009 | 32-word runtime stack | Too-small PSP stack protection | Executed previously and re-executed, passed as expected fault |
| RTOS-RISK-010 | Empty critical section with forced ISR queue access | Scheduler queue re-entry risk | Executed previously, risk reproduced |
| RTOS-RISK-011 | PRIMASK only in existing critical hooks | Incomplete critical-section coverage | Executed previously, still reproduced |
| RTOS-RISK-012 | PRIMASK plus internal SRTOS queue protection | Queue re-entry mitigation | Executed previously, passed |
| RTOS-RISK-013 | TIMER2 ISR entry timestamp under 240 tasks, 2048-word pool | Interrupt delay and pool capacity | Executed previously, pool faulted |
| RTOS-RISK-014 | TIMER2 ISR entry timestamp under 240 tasks, 8192-word pool | Interrupt delay under high task count | Executed previously, passed |
| RTOS-RISK-015 | 1/2/3-tick fast tasks in one firmware | Short-period scheduling continuity | Executed in this report, passed |
| RTOS-RISK-016 | Eight same-period tasks | Same-tick task ordering assumption | Executed in this report, exposed non-stable ordering |
| RTOS-RISK-017 | Long task lasting about 55 ticks | Context save/restore across repeated preemption | Executed in this report, passed |
| RTOS-RISK-018 | Variable task with 3 short runs and 1 long run | Alternating execution time and local guard preservation | Executed in this report, passed |
| RTOS-RISK-019 | Nested call-chain task | Return address and callee-saved state preservation | Executed in this report, passed |
| RTOS-RISK-020 | Local array stack task | PSP stack margin under local-array pressure | Executed in this report, passed with tight 64-word stack margin |
| RTOS-RISK-021 | Floating-point task | FPU context and strict-aliasing-safe test path | Executed in this report, passed |
| RTOS-RISK-022 | Monitor task checking same-period count drift | Fairness or ordering assumptions among same-period tasks | Executed in this report, exposed transient drift |
| RTOS-RISK-023 | 4096-word context pool with mixed risk tasks | Pool leak under mixed stress | Executed in this report, passed |
| RTOS-RISK-024 | 1024-word runtime stack with mixed risk tasks | Baseline stack margin for mixed stress | Executed in this report, passed |
| RTOS-RISK-025 | 64-word runtime stack with mixed risk tasks | Boundary stack margin | Executed in this report, acceptable but risky |
| RTOS-RISK-026 | 32-word runtime stack with mixed risk tasks | Startup minimum context requirement | Executed in this report, expected fault |
| RTOS-RISK-027 | Low-baud UART automatic regression | Blocking print path under slow output | Blocked by baud-rate macro redefinition |
| RTOS-RISK-028 | Floating-point scheduler probe legacy test | Legacy test-code correctness | Blocked by strict-aliasing warning in older test code |
| RTOS-RISK-029 | PSP out-of-bounds injected fault | Runtime stack overflow detection | Not executed; needs abnormal SP or large-frame injection |
| RTOS-RISK-030 | Restore-frame-too-large injected fault | Corrupt snapshot protection | Not executed; needs snapshot injection |
| RTOS-RISK-031 | Release-order injected fault | Pool release ordering protection | Not executed as injection; one real false positive was found and fixed earlier |
| RTOS-RISK-032 | Long-duration context-pool wrap counter | Ring pool wrap robustness | Partially covered; no permanent wrap counter yet |
| RTOS-RISK-033 | BASEPRI critical-section variant | High-priority ISR latency isolation | Not executed; implementation variant not present |
| RTOS-RISK-034 | ISR-side task activation policy test | Future API misuse from interrupt context | Partially covered by forced SysTick queue-access reproduction |
| RTOS-RISK-035 | Real application task-set regression | Production load behavior | Not executed; requires final application task set |

## 6. New Test Execution Results

### 6.1 Mixed Risk Tasks, 1024-Word Runtime Stack

Build options:

```text
DEMO_RTOS_RISK_TEST_ENABLE=1
DEMO_TASK_DEAD_LOOP_ENABLE=0
DEMO_TASK_VARIABLE_LENGTH_ENABLE=0
DEMO_JITTER_CAPTURE_ENABLE=0
SECTION_TASK_RUNTIME_STACK_WORDS=1024
SECTION_TASK_CONTEXT_POOL_WORDS=4096
```

Run time:

```text
15 s
```

Observed SRTOS fault counters:

```text
task_fault_reason = 0
task_context_save_fail_count = 0
task_context_release_fail_count = 0
task_context_pool_words = 4096
task_context_pool_used = 0 at halt
task_stack_words = 1024
task_stack_free_words = 1007
```

Observed risk-task counters:

```text
fast_1tick_count = 2624
fast_2tick_count = 2625
fast_3tick_count = 2630
variable_enter_count = 2432
variable_short_count = 1824
variable_long_count = 608
variable_error_count = 0
variable_max_run_ticks = 64
long_enter_count = 1843
long_complete_count = 1842
long_error_count = 0
long_max_run_ticks = 81
nested_count = 2614
nested_error_count = 0
stack_count = 2615
stack_error_count = 0
float_count = 2618
float_error_count = 0
monitor_count = 2187
min_fast_delta = 8
max_fast_delta = 153
```

Same-period ordering observations:

```text
same_period_count[0..7] = 2627, 2620, 2623, 2631, 2628, 2625, 2630, 2627
same_period_order_error = 2017
ready_fairness_error = 1326
```

Result:

- No SRTOS fault was observed.
- No variable-task guard error was observed.
- No long-task guard error was observed.
- No nested call-chain error was observed.
- No local-array stack checksum error was observed.
- No floating-point round-trip error was observed.
- Same-period task order was not stable according to the test assumption.
- Same-period task counts showed transient drift at monitor sampling points.

Interpretation:

The mixed risk workload is acceptable for the current SRTOS because it did not fault and did not corrupt task-local state. However, it exposed an important scheduling rule: application code must not depend on a deterministic execution order among tasks that become ready in the same scheduler tick. The current registration and scan behavior is implementation-dependent enough that same-period ordering should be treated as unspecified.

### 6.2 Mixed Risk Tasks, 64-Word Runtime Stack

Build options:

```text
DEMO_RTOS_RISK_TEST_ENABLE=1
DEMO_TASK_DEAD_LOOP_ENABLE=0
DEMO_TASK_VARIABLE_LENGTH_ENABLE=0
DEMO_JITTER_CAPTURE_ENABLE=0
SECTION_TASK_RUNTIME_STACK_WORDS=64
SECTION_TASK_CONTEXT_POOL_WORDS=4096
```

Run time:

```text
10 s
```

Observed SRTOS fault counters:

```text
task_fault_reason = 0
task_context_save_fail_count = 0
task_context_release_fail_count = 0
task_context_pool_words = 4096
task_context_pool_used = 33 at halt
task_stack_words = 64
task_stack_free_words = 47
```

Observed risk-task counters:

```text
fast_1tick_count = 4040
fast_2tick_count = 4032
fast_3tick_count = 4029
variable_enter_count = 3520
variable_short_count = 2640
variable_long_count = 880
variable_error_count = 0
variable_max_run_ticks = 50
long_enter_count = 2416
long_complete_count = 2415
long_error_count = 0
long_max_run_ticks = 61
nested_count = 4003
nested_error_count = 0
stack_count = 3976
stack_error_count = 0
float_count = 3962
float_error_count = 0
monitor_count = 1619
min_fast_delta = 1
max_fast_delta = 91
```

Same-period ordering observations:

```text
same_period_count[0..7] = 4016, 4017, 4018, 4019, 4019, 4020, 4020, 4020
same_period_order_error = 0
ready_fairness_error = 1212
```

Result:

- No SRTOS fault was observed.
- The local-array stack test did not report checksum corruption.
- The 64-word runtime stack still had 47 words free at the sampled fault-debug watermark.
- Same-period monitor still observed transient count drift.

Interpretation:

This is an acceptable but tight result. The 64-word stack did not fail in this test, but it has little design margin for future deeper call chains, interrupt nesting changes, compiler changes, or larger local variables. It should remain a boundary test, not a recommended production size.

### 6.3 Mixed Risk Tasks, 32-Word Runtime Stack

Build options:

```text
DEMO_RTOS_RISK_TEST_ENABLE=1
DEMO_TASK_DEAD_LOOP_ENABLE=0
DEMO_TASK_VARIABLE_LENGTH_ENABLE=0
DEMO_JITTER_CAPTURE_ENABLE=0
SECTION_TASK_RUNTIME_STACK_WORDS=32
SECTION_TASK_CONTEXT_POOL_WORDS=4096
```

Run time before sampling:

```text
2 s
```

Observed SRTOS fault counters:

```text
task_fault_reason = 5
task_context_required_words = 33
task_context_pool_words = 4096
task_context_pool_used = 0
task_stack_words = 32
```

Result:

- The RTOS entered the expected runtime-stack-too-small protection path.
- The risk-task counters remained zero because the fault occurred before normal test-task execution.

Interpretation:

This is an expected negative test. It confirms that the configured runtime stack must be at least large enough for the minimum startup/context frame and that SRTOS reports the condition as `fault_reason=5`.

## 7. Risk Findings

### 7.1 Same-Period Task Order Should Be Treated As Unspecified

The eight same-period tasks did not consistently satisfy a strict registration-order assumption. This is acceptable if the design does not promise same-tick ordering, but it is a real application-level risk.

Recommended rule:

```text
Do not encode functional dependencies between tasks by relying on same-period registration order.
```

If deterministic ordering is required, use one task that calls sub-functions in a defined order or add an explicit ordering mechanism.

### 7.2 64-Word Runtime Stack Is A Boundary Configuration

The 64-word runtime stack test passed, but it is close to the minimum viable range and should remain a stress test configuration. It is not a robust default for production workloads that may later add deeper calls, larger local arrays, formatted output, or floating-point-heavy code.

### 7.3 Context Pool Sizing Depends Strongly On Task Count And Long-Task Mix

Previous TIMER2 jitter stress showed:

```text
240 tasks, 2048-word context pool: fault_reason=1, used 2031 / 2048 words
240 tasks, 8192-word context pool: no fault, used 2849 / 8192 words
```

The current mixed risk test used only 17 extra test tasks and a 4096-word pool, so it cannot replace the 240-task pool sizing result.

### 7.4 PRIMASK Critical Sections Are Effective But Have Interrupt-Latency Cost

Previous TIMER2 entry measurement under 240 mixed periodic tasks showed:

```text
TIMER2 counter clock = 216 MHz
maximum TIMER2 entry count = 201 ticks
maximum entry delay = 0.931 us
```

This was acceptable for that test, but it must be compared with the real product interrupt latency budget.

## 8. Overall Conclusion

The current SRTOS passed the newly executed mixed risk tests for:

- Short-period task continuity.
- Long task preemption and restoration.
- Variable short/long task behavior.
- Nested function return preservation.
- Local-array stack pressure.
- Floating-point context behavior in the added test path.
- 4096-word context pool mixed-load operation.
- 64-word boundary runtime stack operation.
- 32-word runtime stack fault detection.

The tests also exposed important limitations:

- Same-period task order must not be relied on.
- A 64-word runtime stack is a boundary test configuration, not a comfortable production setting.
- Common context pool sizing must be validated with the real task count and long-task mix.
- PRIMASK queue protection reduces scheduler race risk but must be checked against interrupt-latency requirements.

## 9. Recommended Next Tests

The next useful tests should focus on risk areas that are not yet fully covered:

1. Add a permanent debug-only counter for context-pool head/tail wrap events.
2. Add a dedicated injected PSP out-of-bounds test.
3. Add a dedicated injected restore-frame-too-large test.
4. Add a dedicated injected context-release-order test.
5. Make the debug UART baud rate macro override-safe, then run 921600, 115200, 57600, 38400, 19200, and 9600 baud regression automatically.
6. Run the same risk suite with the final real application task set.
7. If high-priority control interrupts are required, implement and test a BASEPRI critical-section variant.

