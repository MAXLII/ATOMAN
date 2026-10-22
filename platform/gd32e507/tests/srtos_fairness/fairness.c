// SPDX-License-Identifier: MIT
/**
 * @file fairness.c
 * @brief Two equal CPU-bound periodic tasks for physical SRTOS fairness checks.
 * @details Uses the production scheduler and exception port. Only these two
 *          tasks remain in the runtime list; observation uses nonhalting SWD.
 *          Work is a calibrated instruction count, so suspension is not work.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "section.h"
#include "systick.h"
#include "gd32e50x.h"
#include <stdint.h>

#ifndef FAIRNESS_REVERSE
#define FAIRNESS_REVERSE 0
#endif
typedef struct fairness_task_result
{
    uint32_t entries, completed, context_errors;
    uint32_t min_wall_cycles, max_wall_cycles, last_tick;
} fairness_task_result_t;
typedef struct fairness_result
{
    uint32_t magic, target_us, cpu_hz, iterations, calibrated_cycles;
    uint32_t discovered_tasks, active_tasks, boot_tick, first_task, error;
    fairness_task_result_t a, b;
} fairness_result_t;
volatile fairness_result_t g_srtos_fairness;
extern section_item_t *p_task_first;

static DBG_NOINLINE void cpu_work(uint32_t iterations)
{
    for (uint32_t i = 0u; i < iterations; ++i)
        __NOP();
}
static DBG_NOINLINE void run(volatile fairness_task_result_t *p_result)
{
    uint32_t entry  = ++p_result->entries;
    uint32_t cookie = entry ^ UINT32_C(0x5A3CC3A5);
    uint32_t start  = DWT->CYCCNT;
    cpu_work(g_srtos_fairness.iterations);

    if ((cookie ^ UINT32_C(0x5A3CC3A5)) != p_result->entries)
        ++p_result->context_errors;
    uint32_t elapsed = DWT->CYCCNT - start;

    if (elapsed < p_result->min_wall_cycles)
        p_result->min_wall_cycles = elapsed;

    if (elapsed > p_result->max_wall_cycles)
        p_result->max_wall_cycles = elapsed;
    p_result->last_tick = systick_gettime_100us();
    ++p_result->completed;
}
static void fairness_task_a(void)
{
    run(&g_srtos_fairness.a);
}
static void fairness_task_b(void)
{
    run(&g_srtos_fairness.b);
}
REG_TASK_MS(1u, fairness_task_a)
REG_TASK_MS(1u, fairness_task_b)

static void fairness_init(void)
{
    section_item_t *p_a = NULL, *p_b = NULL;
    g_srtos_fairness.magic     = UINT32_C(0x46525032);
    g_srtos_fairness.target_us = 650u;
    g_srtos_fairness.cpu_hz    = SystemCoreClock;

    for (section_item_t *p = p_task_first; p != NULL; p = p->p_next)
    {
        ++g_srtos_fairness.discovered_tasks;

        if (p->p_obj == &reg_task_fairness_task_a)
            p_a = p;

        if (p->p_obj == &reg_task_fairness_task_b)
            p_b = p;
    }

    if (    p_a == NULL
         || p_b == NULL)
    {
        g_srtos_fairness.error = 1u;
        return;
    }
    /* Exclude the core's unused Link polling task without changing its code. */
#if FAIRNESS_REVERSE
    p_task_first                = p_b;
    p_b->p_next                 = p_a;
    p_a->p_next                 = NULL;
    g_srtos_fairness.first_task = 2u;
#else
    p_task_first                = p_a;
    p_a->p_next                 = p_b;
    p_b->p_next                 = NULL;
    g_srtos_fairness.first_task = 1u;
#endif
    g_srtos_fairness.active_tasks = 2u;
    /* These IRQs belong to the reused board port, with no test traffic. */
    NVIC_DisableIRQ(USART0_IRQn);
    NVIC_DisableIRQ(DMA0_Channel3_IRQn);
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    uint32_t start = DWT->CYCCNT;
    cpu_work(10000u);
    uint32_t cycles = DWT->CYCCNT - start;

    if (cycles == 0u)
    {
        g_srtos_fairness.error = 2u;
        __set_PRIMASK(state);
        return;
    }
    uint64_t divisor = (uint64_t)cycles * 1000000u;
    uint64_t numerator = (uint64_t)SystemCoreClock * 650u * 10000u;
    g_srtos_fairness.iterations = (uint32_t)((numerator + divisor / 2u) / divisor);
    start = DWT->CYCCNT;
    cpu_work(g_srtos_fairness.iterations);
    g_srtos_fairness.calibrated_cycles = DWT->CYCCNT - start;
    g_srtos_fairness.a.min_wall_cycles = UINT32_MAX;
    g_srtos_fairness.b.min_wall_cycles = UINT32_MAX;
    __set_PRIMASK(state);
    g_srtos_fairness.boot_tick = systick_gettime_100us();
}
REG_INIT(255, fairness_init)
