# SRTOS Critical Section Risk Reproduction Report

## 1. Purpose

This report records a hardware reproduction test for the SRTOS critical-section risk on the GD32G553 platform.

The reviewed risk was that `section_critical_enter()` and `section_critical_exit()` did not actually protect shared scheduler state. The test intentionally made `SysTick_Handler()` touch the task scheduler queue path to simulate a future ISR-side scheduler API or tick-driven activation path.

## 2. Hardware And Tools

| Item | Value |
| --- | --- |
| Target MCU | GD32G553RCT6 |
| Debug probe | J-Link |
| Debug interface | SWD, 4000 kHz |
| Firmware project | `gd32g553c` |
| Compiler | `arm-none-eabi-gcc` through `mingw32-make.exe` |
| Test date | 2026-06-30 |

## 3. Test Instrumentation

The test used temporary compile-time switches:

```text
SECTION_CRITICAL_RACE_PROBE_ENABLE=1
SECTION_CRITICAL_RACE_PROBE_SPIN=512
SECTION_TASK_TICK_FROM_SYSTICK_TEST=1
DEMO_TASK_DEAD_LOOP_ENABLE=0
DEMO_TASK_VARIABLE_LENGTH_ENABLE=1
```

The probe added the following runtime counters:

| Counter | Meaning |
| --- | --- |
| `probe_enter_count` | Number of instrumented queue operation entries |
| `probe_reentry_count` | Number of times a queue operation was entered while another queue operation was still active |
| `probe_invariant_fail_count` | Basic first/tail queue invariant violation count |
| `probe_max_depth` | Maximum nested queue operation depth observed |
| `critical_enter_count` | Number of critical-section entries |
| `critical_exit_count` | Number of critical-section exits |

`probe_reentry_count > 0` means the scheduler queue path was re-entered before the previous queue operation completed. This is direct evidence that the queue update path is not protected against ISR preemption under the tested access pattern.

## 4. Test Builds

### 4.1 Empty Critical Section

Build defines:

```text
-DSECTION_CRITICAL_RACE_PROBE_ENABLE=1
-DSECTION_CRITICAL_RACE_PROBE_SPIN=512
-DSECTION_TASK_TICK_FROM_SYSTICK_TEST=1
-DDEMO_TASK_DEAD_LOOP_ENABLE=0
-DDEMO_TASK_VARIABLE_LENGTH_ENABLE=1
```

Build size:

```text
text=58592 data=13560 bss=18000 dec=90152
```

Result after approximately 5 seconds:

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

Conclusion: the risk is reproducible. The scheduler queue path was re-entered thousands of times while an existing queue operation was active.

### 4.2 PRIMASK Only In Existing Critical Hooks

Build defines:

```text
-DSECTION_CRITICAL_RACE_PROBE_ENABLE=1
-DSECTION_CRITICAL_RACE_PROBE_SPIN=512
-DSECTION_TASK_TICK_FROM_SYSTICK_TEST=1
-DSECTION_CRITICAL_USE_PRIMASK=1
-DDEMO_TASK_DEAD_LOOP_ENABLE=0
-DDEMO_TASK_VARIABLE_LENGTH_ENABLE=1
```

Build size:

```text
text=58648 data=13560 bss=18000 dec=90208
```

Result after approximately 5 seconds:

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

Conclusion: implementing PRIMASK only inside the existing `section_critical_enter()` / `section_critical_exit()` call sites is not sufficient. Some SRTOS ready/unfinished queue operations are executed outside those existing critical-section call sites.

### 4.3 PRIMASK With SRTOS Queue Internal Protection

Build defines:

```text
-DSECTION_CRITICAL_RACE_PROBE_ENABLE=1
-DSECTION_CRITICAL_RACE_PROBE_SPIN=512
-DSECTION_TASK_TICK_FROM_SYSTICK_TEST=1
-DSECTION_CRITICAL_USE_PRIMASK=1
-DSECTION_SRTOS_QUEUE_INTERNAL_CRITICAL=1
-DDEMO_TASK_DEAD_LOOP_ENABLE=0
-DDEMO_TASK_VARIABLE_LENGTH_ENABLE=1
```

Build size:

```text
text=58760 data=13560 bss=18000 dec=90320
```

Result after approximately 5 seconds:

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

## 5. Findings

1. The empty critical-section implementation is a real concurrency risk if scheduler queues are touched from both exception context and thread/PendSV paths.
2. Filling only the existing generic critical-section hooks with PRIMASK does not fully protect SRTOS, because not every SRTOS ready/unfinished queue operation is currently inside those hooks.
3. The tested fault counters stayed at zero in all three runs. This means the race was observed before it escalated into a detected SRTOS fault. The absence of `task_fault_reason` does not prove the queue path is safe.
4. The test intentionally enabled `SECTION_TASK_TICK_FROM_SYSTICK_TEST`; the current production SysTick path only requests PendSV and does not normally scan task queues. The reproduction therefore validates the risk for future ISR-side scheduler access or refactoring, not necessarily a failure in the current default SysTick path.

## 6. Recommendation

Use a two-level rule:

1. Keep the current SysTick path lightweight. SysTick should request PendSV and avoid scanning or mutating scheduler queues unless there is a strong design reason.
2. Protect all scheduler queue mutations and queue pops with a real critical section, not only the current non-SRTOS ready-queue call sites.

For the GD32G553 platform, the first production-grade implementation can use PRIMASK because the protected queue operations are short:

```c
static uint32_t section_critical_enter(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    __DSB();
    __ISB();

    return primask;
}

static void section_critical_exit(uint32_t primask)
{
    __set_PRIMASK(primask);
    __DSB();
    __ISB();
}
```

The longer-term implementation should consider BASEPRI if high-priority control interrupts must remain unmasked. In that case, any interrupt that calls SRTOS scheduler APIs must run at a priority masked by the SRTOS critical-section threshold.

## 7. Follow-Up Work

| Priority | Work item |
| --- | --- |
| High | Define the official SRTOS critical-section boundary and apply it to all ready/unfinished queue operations |
| High | Decide whether ISR-side task activation is allowed. If not allowed, document and enforce it |
| Medium | Add a permanent diagnostic counter for queue re-entry or queue invariant violation in debug builds |
| Medium | Add a BASEPRI version for platforms where high-priority control ISR latency must not be affected |
| Medium | Add a regression build target for the race-probe configuration |

