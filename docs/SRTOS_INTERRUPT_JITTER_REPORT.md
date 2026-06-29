# SRTOS Interrupt Jitter Stress Report

## Scope

This report records a GD32G553 hardware test of the current SRTOS critical-section scheme under a high task load.

The test focuses on TIMER2 interrupt entry latency when SRTOS critical sections are protected with PRIMASK. The measurement records `TIMER_CNT(TIMER2)` immediately at the beginning of `TIMER2_IRQHandler()`, before checking or clearing the TIMER2 update flag and before calling `section_interrupt()`.

This placement is intentional. If the TIMER2 update event occurs while interrupts are disabled, the ISR only starts after interrupts are re-enabled. The TIMER2 counter value at ISR entry therefore directly reflects the delayed entry time after the hardware event.

## Test Firmware

Build target:

```text
gd32g553c
```

Build options used for the no-fault stress run:

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
- Workload mix:
  - Short CPU loops.
  - Medium CPU loops.
  - Long tasks that keep running for 15 to 21 SRTOS ticks.
  - Variable tasks that normally run short work and periodically run for 20 to 30 SRTOS ticks.

TIMER2 setup observed from firmware:

- TIMER2 interrupt rate: 30,000 Hz.
- TIMER2 counter clock: 216,000,000 Hz.
- TIMER2 period: 7200 timer ticks.
- One TIMER2 counter tick: 4.6296 ns.

## Measurement Method

The ISR entry hook stores the low 16 bits of `TIMER_CNT(TIMER2)` into a RAM ring buffer:

```text
g_demo_jitter_timer2_count[1024]
```

The hook also keeps online summary data:

```text
g_demo_jitter_debug.min_entry_count
g_demo_jitter_debug.max_entry_count
g_demo_jitter_debug.last_entry_count
g_demo_jitter_debug.write_index
g_demo_jitter_debug.timer2_counter_hz
g_demo_jitter_debug.timer2_period_ticks
```

The online maximum is the important worst-case value because the RAM ring buffer only keeps the latest 1024 samples and can overwrite an earlier peak.

Common pool and runtime stack usage were read from:

```text
g_section_fault_debug.task_context_pool_words
g_section_fault_debug.task_context_pool_used
g_section_fault_debug.task_stack_words
g_section_fault_debug.task_stack_free_words
g_section_fault_debug.task_runtime_stack_used_words
g_section_fault_debug.task_fault_reason
```

## Results

### 240 Tasks, 2048-Word Common Context Pool

This run used:

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
- Last 1024 RAM samples:
  - Minimum: 15 ticks.
  - Maximum: 105 ticks.
  - P50: 21 ticks.
  - P90: 21 ticks.
  - P99: 62 ticks.
  - P99.9: 100 ticks.

Conclusion for this configuration: 2048 words is not enough common context pool for this 240-task stress case.

### 240 Tasks, 8192-Word Common Context Pool

This run used:

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
- Last 1024 RAM samples:
  - Minimum: 21 ticks.
  - Maximum: 95 ticks.
  - P50: 21 ticks.
  - P90: 21 ticks.
  - P99: 63 ticks.
  - P99.9: 88 ticks.

## Interpretation

With PRIMASK critical sections enabled, TIMER2 interrupt entry is delayed by any section of code that runs with interrupts globally disabled. Under the 240-task mixed workload, the largest observed TIMER2 entry count was 201 timer ticks.

Converted to time:

```text
201 / 216,000,000 = 0.931 us
```

The normal entry point was around 21 timer ticks:

```text
21 / 216,000,000 = 0.097 us
```

The observed worst-case extra entry delay relative to the common 21-tick entry point was therefore:

```text
(201 - 21) / 216,000,000 = 0.833 us
```

This is the measured impact of the tested PRIMASK-based SRTOS protection on the TIMER2 ISR entry path for this stress firmware and this GD32G553 board.

## Risk Assessment

- The PRIMASK scheme prevents the previously reproduced task/interrupt queue race, but it makes all maskable interrupts wait while SRTOS critical sections are active.
- In this test, the measured TIMER2 worst-case entry delay stayed below 1 us with 240 mixed periodic tasks and an 8192-word common context pool.
- The common context pool is the limiting resource for this stress model. A 2048-word pool faulted; an 8192-word pool did not fault in the sampled run and reached 2849 words used.
- The common runtime stack was not the limiting resource in this test. The observed use was 17 to 31 words out of a 1024-word configuration.
- The result is workload-specific. Longer critical sections, different interrupt priorities, additional ISRs, deeper call stacks, or a different task mix can increase the worst-case delay.

## Recommendation

For the current SRTOS implementation, PRIMASK-based critical sections are acceptable only if the project has an explicit interrupt latency budget and verifies that the longest critical section remains below that budget.

For the tested GD32G553 configuration:

- Keep the SRTOS internal queue protection enabled when using preemptive task switching from TIMER2.
- Do not use a 2048-word common context pool for this 240-task stress level.
- Use at least an 8192-word common context pool for this stress scenario, then re-test with the real application task set.
- Keep TIMER2 ISR entry timestamp capture available as a compile-time diagnostic, because it gives a direct hardware-timer view of interrupt delay caused by global interrupt masking.

