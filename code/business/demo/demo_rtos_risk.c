// SPDX-License-Identifier: MIT
/**
 * @file    demo_rtos_risk.c
 * @brief   SRTOS risk-oriented test module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register optional stress tasks that exercise scheduler risk areas
 *          - Record observable pass/fail counters in RAM for debugger extraction
 *          - Test SRTOS behavior without modifying SRTOS core implementation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "demo_rtos_risk.h"

#include "section.h"

#include <stdint.h>
#include <string.h>

#if (DEMO_RTOS_RISK_TEST_ENABLE == 1u)
volatile demo_rtos_risk_debug_t g_demo_rtos_risk_debug;

static volatile uint32_t s_demo_rtos_risk_sink;
static uint32_t s_fast_last_tick;

static void demo_rtos_risk_mark(uint32_t tag)
{
    const uint32_t index = g_demo_rtos_risk_debug.order_index & (DEMO_RTOS_RISK_ORDER_LOG_SIZE - 1u);

    g_demo_rtos_risk_debug.order_log[index] = tag;
    g_demo_rtos_risk_debug.order_index++;
}

static void demo_rtos_risk_fast_1tick(void)
{
    const uint32_t now = SECTION_SYS_TICK;
    uint32_t delta = 0u;

    if (g_demo_rtos_risk_debug.start_tick == 0u)
    {
        g_demo_rtos_risk_debug.start_tick = now;
        s_fast_last_tick = now;
    }

    delta = now - s_fast_last_tick;
    if (delta > 0u)
    {
        if ((g_demo_rtos_risk_debug.min_fast_delta == 0u) || (delta < g_demo_rtos_risk_debug.min_fast_delta))
        {
            g_demo_rtos_risk_debug.min_fast_delta = delta;
        }
        if (delta > g_demo_rtos_risk_debug.max_fast_delta)
        {
            g_demo_rtos_risk_debug.max_fast_delta = delta;
        }
    }

    s_fast_last_tick = now;
    g_demo_rtos_risk_debug.fast_1tick_count++;
    demo_rtos_risk_mark(0xA1000000u | (g_demo_rtos_risk_debug.fast_1tick_count & 0x0000FFFFu));
}

static void demo_rtos_risk_fast_2tick(void)
{
    g_demo_rtos_risk_debug.fast_2tick_count++;
    demo_rtos_risk_mark(0xA2000000u | (g_demo_rtos_risk_debug.fast_2tick_count & 0x0000FFFFu));
}

static void demo_rtos_risk_fast_3tick(void)
{
    g_demo_rtos_risk_debug.fast_3tick_count++;
    demo_rtos_risk_mark(0xA3000000u | (g_demo_rtos_risk_debug.fast_3tick_count & 0x0000FFFFu));
}

static void demo_rtos_risk_same_period(uint32_t id)
{
    if (id == 0u)
    {
        g_demo_rtos_risk_debug.same_period_epoch++;
    }
    else if (g_demo_rtos_risk_debug.same_period_count[id - 1u] < g_demo_rtos_risk_debug.same_period_epoch)
    {
        g_demo_rtos_risk_debug.same_period_order_error++;
    }
    else
    {
    }

    g_demo_rtos_risk_debug.same_period_count[id]++;
    demo_rtos_risk_mark(0xB0000000u | ((id & 0x000000FFu) << 16) |
                        (g_demo_rtos_risk_debug.same_period_count[id] & 0x0000FFFFu));
}

static void demo_rtos_risk_same0(void)
{
    demo_rtos_risk_same_period(0u);
}

static void demo_rtos_risk_same1(void)
{
    demo_rtos_risk_same_period(1u);
}

static void demo_rtos_risk_same2(void)
{
    demo_rtos_risk_same_period(2u);
}

static void demo_rtos_risk_same3(void)
{
    demo_rtos_risk_same_period(3u);
}

static void demo_rtos_risk_same4(void)
{
    demo_rtos_risk_same_period(4u);
}

static void demo_rtos_risk_same5(void)
{
    demo_rtos_risk_same_period(5u);
}

static void demo_rtos_risk_same6(void)
{
    demo_rtos_risk_same_period(6u);
}

static void demo_rtos_risk_same7(void)
{
    demo_rtos_risk_same_period(7u);
}

static void demo_rtos_risk_variable(void)
{
    const uint32_t start = SECTION_SYS_TICK;
    const uint32_t enter = g_demo_rtos_risk_debug.variable_enter_count;
    uint32_t guard_a = 0x13579BDFu ^ enter;
    uint32_t guard_b = 0x2468ACE0u + enter;
    uint32_t run_ticks = 0u;

    g_demo_rtos_risk_debug.variable_enter_count++;
    if ((g_demo_rtos_risk_debug.variable_enter_count & 0x00000003u) == 0u)
    {
        const uint32_t long_start = SECTION_SYS_TICK;

        while ((uint32_t)(SECTION_SYS_TICK - long_start) < 40u)
        {
            guard_a ^= 0x55AA55AAu;
            guard_a ^= 0x55AA55AAu;
            guard_b += 7u;
            guard_b -= 7u;
        }
        g_demo_rtos_risk_debug.variable_long_count++;
    }
    else
    {
        for (uint32_t i = 0u; i < 96u; ++i)
        {
            guard_a += i;
            guard_a -= i;
            guard_b ^= i;
            guard_b ^= i;
        }
        g_demo_rtos_risk_debug.variable_short_count++;
    }

    if ((guard_a != (0x13579BDFu ^ enter)) || (guard_b != (0x2468ACE0u + enter)))
    {
        g_demo_rtos_risk_debug.variable_error_count++;
    }

    run_ticks = SECTION_SYS_TICK - start;
    if (run_ticks > g_demo_rtos_risk_debug.variable_max_run_ticks)
    {
        g_demo_rtos_risk_debug.variable_max_run_ticks = run_ticks;
    }
}

static void demo_rtos_risk_long(void)
{
    const uint32_t start = SECTION_SYS_TICK;
    const uint32_t enter = g_demo_rtos_risk_debug.long_enter_count;
    uint32_t guard = 0x89ABCDEFu ^ enter;
    uint32_t run_ticks = 0u;

    g_demo_rtos_risk_debug.long_enter_count++;
    while ((uint32_t)(SECTION_SYS_TICK - start) < 55u)
    {
        guard ^= 0xAA55AA55u;
        guard ^= 0xAA55AA55u;
        s_demo_rtos_risk_sink++;
        s_demo_rtos_risk_sink--;
    }

    if (guard != (0x89ABCDEFu ^ enter))
    {
        g_demo_rtos_risk_debug.long_error_count++;
    }

    run_ticks = SECTION_SYS_TICK - start;
    if (run_ticks > g_demo_rtos_risk_debug.long_max_run_ticks)
    {
        g_demo_rtos_risk_debug.long_max_run_ticks = run_ticks;
    }
    g_demo_rtos_risk_debug.long_complete_count++;
}

static uint32_t demo_rtos_risk_nested_level3(uint32_t value)
{
    uint32_t acc = value;

    for (uint32_t i = 0u; i < 128u; ++i)
    {
        acc ^= (i + 0x10203040u);
        acc ^= (i + 0x10203040u);
    }

    return acc;
}

static uint32_t demo_rtos_risk_nested_level2(uint32_t value)
{
    return demo_rtos_risk_nested_level3(value ^ 0x55AA55AAu) ^ 0x55AA55AAu;
}

static uint32_t demo_rtos_risk_nested_level1(uint32_t value)
{
    return demo_rtos_risk_nested_level2(value + 0x01020304u) - 0x01020304u;
}

static void demo_rtos_risk_nested(void)
{
    const uint32_t seed = g_demo_rtos_risk_debug.nested_count ^ 0xCAFEBABEu;
    const uint32_t result = demo_rtos_risk_nested_level1(seed);

    if (result != seed)
    {
        g_demo_rtos_risk_debug.nested_error_count++;
    }
    g_demo_rtos_risk_debug.nested_count++;
}

static void demo_rtos_risk_stack(void)
{
    uint32_t local_words[32] = {0u};
    uint32_t checksum = 0u;

    for (uint32_t i = 0u; i < 32u; ++i)
    {
        local_words[i] = 0xA5A50000u | i;
    }
    for (uint32_t i = 0u; i < 32u; ++i)
    {
        checksum ^= local_words[i];
    }

    if (checksum != 0u)
    {
        g_demo_rtos_risk_debug.stack_error_count++;
    }
    g_demo_rtos_risk_debug.stack_count++;
}

static void demo_rtos_risk_float(void)
{
    const float seed = (float)(g_demo_rtos_risk_debug.float_count & 0x000000FFu);
    float acc = seed;
    uint32_t bits = 0u;

    acc = (acc * 1.25f) + 0.5f;
    acc = (acc - 0.5f) / 1.25f;
    (void)memcpy(&bits, &acc, sizeof(bits));

    if (acc != seed)
    {
        g_demo_rtos_risk_debug.float_error_count++;
    }

    g_demo_rtos_risk_debug.float_bits = bits;
    g_demo_rtos_risk_debug.float_count++;
}

static void demo_rtos_risk_monitor(void)
{
    const uint32_t same0 = g_demo_rtos_risk_debug.same_period_count[0];
    const uint32_t same7 = g_demo_rtos_risk_debug.same_period_count[7];

    g_demo_rtos_risk_debug.monitor_count++;
    g_demo_rtos_risk_debug.last_tick = SECTION_SYS_TICK;

    if ((same0 > (same7 + 1u)) || (same7 > (same0 + 1u)))
    {
        g_demo_rtos_risk_debug.ready_fairness_error++;
    }
}

REG_TASK(1, demo_rtos_risk_fast_1tick)
REG_TASK(2, demo_rtos_risk_fast_2tick)
REG_TASK(3, demo_rtos_risk_fast_3tick)
REG_TASK(13, demo_rtos_risk_same0)
REG_TASK(13, demo_rtos_risk_same1)
REG_TASK(13, demo_rtos_risk_same2)
REG_TASK(13, demo_rtos_risk_same3)
REG_TASK(13, demo_rtos_risk_same4)
REG_TASK(13, demo_rtos_risk_same5)
REG_TASK(13, demo_rtos_risk_same6)
REG_TASK(13, demo_rtos_risk_same7)
REG_TASK(17, demo_rtos_risk_variable)
REG_TASK(19, demo_rtos_risk_long)
REG_TASK(23, demo_rtos_risk_nested)
REG_TASK(29, demo_rtos_risk_stack)
REG_TASK(31, demo_rtos_risk_float)
REG_TASK(100, demo_rtos_risk_monitor)
#endif /* DEMO_RTOS_RISK_TEST_ENABLE */
