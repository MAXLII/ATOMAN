// SPDX-License-Identifier: MIT
/**
 * @file srtos_probe.c
 * @brief Optional equal-work task probes for ESP32-C5 SRTOS hardware acceptance.
 * @details Entity: 2 periodic tasks with independent counters and live contexts.
 *          Prior: bounded calibration finishes before the sampling timer starts.
 *          Time: 650 us of calibrated work per invocation, never wall-clock work.
 *          Wi-Fi, sampling and other registered tasks remain enabled for stress.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "section.h"
#include "bsp_system.h"
#include "shell.h"
#include <stdint.h>

#define PROBE_TARGET_US        650u     /**< CPU execution time requested for each probe invocation. */
#define PROBE_TRIAL_ITERATIONS 10000u   /**< Initial bounded loop used to estimate instruction cost. */
#define PROBE_MAX_ITERATIONS   2000000u /**< Reject a broken clock or unreasonable calibration ratio. */
#define PROBE_SENTINEL_WORDS   8u       /**< Independent stack words verified after every suspension. */

static volatile uint32_t entries_a;    /**< Number of A invocations entered, with A as the only writer. */
static volatile uint32_t entries_b;    /**< Number of B invocations entered, with B as the only writer. */
static volatile uint32_t completed_a;  /**< Number of A invocations returned after all fixed work. */
static volatile uint32_t completed_b;  /**< Number of B invocations returned after all fixed work. */
static volatile uint32_t errors;       /**< Calibration, register, stack or overlapping-entry errors. */
static volatile uint32_t yield_enable; /**< Optional midpoint cooperative yield; defaults to disabled. */
static uint32_t iterations;            /**< Calibrated loop count; immutable once REG_INIT completes. */
static uint32_t calibrated_us;         /**< Last measured execution time with board interrupts masked. */

/**
 * @param count Fixed number of instruction iterations, never elapsed time.
 * @param cookie Per-invocation identity, distinct for the 2 task contexts.
 * @param allow_yield 1 requests one midpoint yield; calibration always passes 0.
 * @return 0 when all live register and stack sentinels survived; 1 otherwise.
 * @details The empty-effect assembly makes all 8 values genuinely register-live
 *          in every loop iteration. The compiler must recheck each result; it
 *          cannot prove the values unchanged and fold the acceptance checks.
 */
static DBG_NOINLINE __attribute__((noclone)) uint32_t fixed_work(uint32_t count, uint32_t cookie, uint32_t allow_yield)
{
    uint32_t r0                                      = cookie ^ UINT32_C(0x12345678); /**< Live register sentinel 0. */
    uint32_t r1                                      = cookie ^ UINT32_C(0x87654321); /**< Live register sentinel 1. */
    uint32_t r2                                      = cookie ^ UINT32_C(0xA5A55A5A); /**< Live register sentinel 2. */
    uint32_t r3                                      = cookie ^ UINT32_C(0x5A5AA5A5); /**< Live register sentinel 3. */
    uint32_t r4                                      = cookie ^ UINT32_C(0x89ABCDEF); /**< Live register sentinel 4. */
    uint32_t r5                                      = cookie ^ UINT32_C(0xFEDCBA98); /**< Live register sentinel 5. */
    uint32_t r6                                      = cookie ^ UINT32_C(0x55AA33CC); /**< Live register sentinel 6. */
    uint32_t r7                                      = cookie ^ UINT32_C(0xCC33AA55); /**< Live register sentinel 7. */
    volatile uint32_t sentinel[PROBE_SENTINEL_WORDS] = {0}; /**< Real per-invocation stack storage. */
    const uint32_t yield_at = allow_yield == 1u ? count / 2u : UINT32_MAX; /**< At most 1 explicit context switch. */

    for (uint32_t i = 0u; i < PROBE_SENTINEL_WORDS; ++i) /* Initialize every stack word before preemption. */
        sentinel[i] = cookie ^ (UINT32_C(0xC35A7E19) + i * UINT32_C(0x1020304));

    for (uint32_t i = 0u; i < count; ++i) /* Fixed instruction count excludes all time spent suspended. */
    {
        __asm__ volatile("nop"
                         : "+r"(r0), "+r"(r1), "+r"(r2), "+r"(r3), "+r"(r4), "+r"(r5), "+r"(r6), "+r"(r7)
                         :
                         : "memory");

        if (i == yield_at)
            section_task_yield(); /* Both register and stack values remain live across this voluntary suspension. */
    }

    if (    (r0 != (cookie ^ UINT32_C(0x12345678)))  /* Detect restoration of another invocation's register set. */
         || (r1 != (cookie ^ UINT32_C(0x87654321)))  /* Check each live register independently. */
         || (r2 != (cookie ^ UINT32_C(0xA5A55A5A)))  /* Avoid cancellation in a combined XOR checksum. */
         || (r3 != (cookie ^ UINT32_C(0x5A5AA5A5)))  /* Detect either lost or stale saved register values. */
         || (r4 != (cookie ^ UINT32_C(0x89ABCDEF)))  /* Keep both caller and callee register pressure present. */
         || (r5 != (cookie ^ UINT32_C(0xFEDCBA98)))  /* Different patterns expose wrong frame offsets. */
         || (r6 != (cookie ^ UINT32_C(0x55AA33CC)))  /* Preserve checks across optimized compiler builds. */
         || (r7 != (cookie ^ UINT32_C(0xCC33AA55)))) /* Complete the 8-register check before stack verification. */
        return 1u;

    for (uint32_t i = 0u; i < PROBE_SENTINEL_WORDS; ++i) /* Restore and validate every saved stack word. */
    {
        if (sentinel[i] != (cookie ^ (UINT32_C(0xC35A7E19) + i * UINT32_C(0x1020304))))
            return 1u;
    }
    return 0u;
}

/** @param count Nonzero number of observed failures to retain without a lost update. */
static void record_error(uint32_t count)
{
    bsp_system_runtime_enter(); /* Serialize the 2 task writers on the SDK host context. */
    errors += count;            /* Retain failure evidence; normal calls never enter this path. */
    bsp_system_runtime_exit();  /* Keep normal sampling interrupts enabled. */
}

/**
 * @param p_entries Single-writer entry counter belonging to the active task.
 * @param p_completed Single-writer completion counter belonging to the active task.
 * @param tag Distinguishes A's live values from B's live values.
 */
static DBG_NOINLINE void run(volatile uint32_t *p_entries, volatile uint32_t *p_completed, uint32_t tag)
{
    uint32_t entry    = 0u; /**< Invocation identity must remain live across fixed_work and suspension. */
    uint32_t failures = 0u; /**< At most 2 failures for one completed invocation. */

    if (iterations == 0u)
        return;
    entry = *p_entries + 1u;
    *p_entries = entry;
    failures   = fixed_work(iterations, entry ^ tag, yield_enable);

    if (*p_entries != entry)
        ++failures;

    if (failures != 0u)
        record_error(failures);
    ++*p_completed;
}

static void probe_a(void)
{
    run(&entries_a, &completed_a, UINT32_C(0x0A0A0A0A));
}

static void probe_b(void)
{
    run(&entries_b, &completed_b, UINT32_C(0xB0B0B0B0));
}

REG_TASK_MS(1u, probe_a)
REG_TASK_MS(1u, probe_b)

static void calibrate(void)
{
    uint32_t irq_state = 0u; /**< Board interrupt mask restored before any initialization returns. */
    uint32_t start     = 0u; /**< Monotonic hardware microseconds, valid while interrupts are masked. */
    uint32_t elapsed   = 0u; /**< Current calibration duration, never used by the periodic tasks. */
    uint32_t count     = PROBE_TRIAL_ITERATIONS; /**< Bounded candidate instruction count. */
    uint64_t scaled    = 0u; /**< Full-width ratio before validating and narrowing a candidate. */

    bsp_system_runtime_enter(); /* Do not suspend calibration through a partially initialized scheduler. */
    irq_state = bsp_system_critical_enter(); /* Exclude SDK scheduling time from the measured CPU work. */
    errors += fixed_work(1000u, UINT32_C(0xCAFEBABE), 0u); /* Warm the exact function's instruction-cache path. */

    for (uint32_t attempt = 0u; attempt < 4u; ++attempt) /* Bound total initialization time and correction passes. */
    {
        start = bsp_system_time_us_get();
        errors += fixed_work(count, UINT32_C(0xCAFEBABE), 0u);
        elapsed       = bsp_system_time_us_get() - start;
        calibrated_us = elapsed;

        if (    (elapsed >= 640u)  /* Preserve comparable A/B workload near the requested 650 us. */
             && (elapsed <= 660u)) /* A few microseconds of clock resolution do not affect fairness. */
        {
            iterations = count;
            break;
        }

        if (elapsed == 0u)
            break;
        scaled = ((uint64_t)count * PROBE_TARGET_US + elapsed / 2u) / elapsed;

        if (    (scaled == 0u) /* Reject a clock or workload that cannot be measured. */
             || (scaled > PROBE_MAX_ITERATIONS)) /* Keep invalid calibration from creating an unbounded task. */
            break;
        count = (uint32_t)scaled;
    }

    if (iterations == 0u)
        ++errors;
    bsp_system_critical_exit(irq_state); /* Restore exactly the original board interrupt-enable mask. */
    bsp_system_runtime_exit();           /* Finish before the priority-1 sampling timer registration starts. */
}

REG_INIT(0, calibrate)
REG_SHELL_VAR(SRTOS_FAIR_A, entries_a, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_FAIR_B, entries_b, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_DONE_A, completed_a, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_DONE_B, completed_b, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_ERRORS, errors, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_CAL_US, calibrated_us, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_ITERATIONS, iterations, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_PROBE_YIELD, yield_enable, SHELL_UINT32, 1u, 0u, NULL, SHELL_STA_NULL)
