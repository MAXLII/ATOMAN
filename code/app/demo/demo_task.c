// SPDX-License-Identifier: MIT
/**
 * @file    demo_task.c
 * @brief   task section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register periodic callbacks with SECTION_TASK
 *          - Demonstrate millisecond and scheduler-tick task periods
 *          - Keep task counters local to this demo file
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-18
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "section.h"
#include "bsp_usart.h"

#include <stdint.h>

static uint32_t s_demo_task_10ms_count = 0u;
static uint32_t s_demo_task_tick_count = 0u;

#ifndef DEMO_TASK_DEAD_LOOP_ENABLE
#define DEMO_TASK_DEAD_LOOP_ENABLE 1
#endif

#ifndef DEMO_TASK_SCHED_PROBE_ENABLE
#define DEMO_TASK_SCHED_PROBE_ENABLE 0
#endif

static void demo_task_10ms(void)
{
    s_demo_task_10ms_count++;
}

static void demo_task_100us(void)
{
    s_demo_task_tick_count++;
}

#if (DEMO_TASK_DEAD_LOOP_ENABLE == 1)
typedef struct
{
    uint32_t loop_100ms_print_count;
    uint32_t loop_123ms_print_count;
    uint32_t loop_100ms_delay_count;
    uint32_t loop_123ms_delay_count;
} demo_task_dead_loop_debug_t;

volatile demo_task_dead_loop_debug_t g_demo_task_dead_loop_debug;

static void demo_task_busy_delay(uint32_t rounds)
{
    volatile uint32_t sink = 0u;

    for (uint32_t i = 0u; i < rounds; ++i)
    {
        sink += (i ^ 0x5A5A5A5Au);
    }

    (void)sink;
}

static void demo_task_dead_loop_100ms(void)
{
    while (1)
    {
        demo_task_busy_delay(900000u);
        g_demo_task_dead_loop_debug.loop_100ms_delay_count++;
        g_demo_task_dead_loop_debug.loop_100ms_print_count++;
        bsp_usart_iso_printf("TASK_DEAD_LOOP_100MS count=%lu 10ms=%lu tick=%lu\r\n",
                             (unsigned long)g_demo_task_dead_loop_debug.loop_100ms_print_count,
                             (unsigned long)s_demo_task_10ms_count,
                             (unsigned long)s_demo_task_tick_count);
    }
}

static void demo_task_dead_loop_123ms(void)
{
    while (1)
    {
        demo_task_busy_delay(1107000u);
        g_demo_task_dead_loop_debug.loop_123ms_delay_count++;
        g_demo_task_dead_loop_debug.loop_123ms_print_count++;
        bsp_usart_iso_printf("TASK_DEAD_LOOP_123MS count=%lu 10ms=%lu tick=%lu\r\n",
                             (unsigned long)g_demo_task_dead_loop_debug.loop_123ms_print_count,
                             (unsigned long)s_demo_task_10ms_count,
                             (unsigned long)s_demo_task_tick_count);
    }
}
#endif

#if (DEMO_TASK_SCHED_PROBE_ENABLE == 1)
typedef struct
{
    uint32_t fast_count;
    uint32_t mid_count;
    uint32_t float_count;
    uint32_t long_count;
    uint32_t long_enter_count;
    uint32_t long_progress;
    uint32_t long_checksum;
    uint32_t local_error_count;
    uint32_t order_index;
    uint32_t order_log[16];
    uint32_t float_bits;
    uint32_t long_active;
    uint32_t ultra_count;
    uint32_t ultra_enter_count;
    uint32_t ultra_progress;
    uint32_t ultra_checksum;
    uint32_t ultra_error_count;
    uint32_t ultra_active;
} demo_task_sched_debug_t;

volatile demo_task_sched_debug_t g_demo_task_sched_debug;

static void demo_task_sched_mark(uint32_t tag)
{
    const uint32_t index = g_demo_task_sched_debug.order_index & 0x0fu;

    g_demo_task_sched_debug.order_log[index] = tag;
    g_demo_task_sched_debug.order_index++;
}

static void demo_sched_probe_fast(void)
{
    g_demo_task_sched_debug.fast_count++;
    demo_task_sched_mark(0xA1000000u | (g_demo_task_sched_debug.fast_count & 0x0000FFFFu));
}

static void demo_sched_probe_mid(void)
{
    g_demo_task_sched_debug.mid_count++;
    demo_task_sched_mark(0xB2000000u | (g_demo_task_sched_debug.mid_count & 0x0000FFFFu));
}

static void demo_sched_probe_float(void)
{
    float acc = (float)g_demo_task_sched_debug.float_count;
    uint32_t bits = 0u;

    acc = (acc * 1.125f) + 0.25f;
    bits = *((uint32_t *)&acc);
    g_demo_task_sched_debug.float_bits = bits;
    g_demo_task_sched_debug.float_count++;
    demo_task_sched_mark(0xC3000000u | (g_demo_task_sched_debug.float_count & 0x0000FFFFu));
}

static void demo_sched_probe_long(void)
{
    uint32_t local_seed = g_demo_task_sched_debug.long_count ^ 0x13579BDFu;
    uint32_t checksum = 0u;

    g_demo_task_sched_debug.long_active = 1u;
    g_demo_task_sched_debug.long_enter_count++;
    demo_task_sched_mark(0xD5000000u | (g_demo_task_sched_debug.long_enter_count & 0x0000FFFFu));

    for (uint32_t i = 0u; i < 180000u; ++i)
    {
        checksum += (local_seed ^ i) + (checksum << 1);
        if ((i & 0x3FFFu) == 0u)
        {
            g_demo_task_sched_debug.long_progress = i;
        }
    }

    if (local_seed != (g_demo_task_sched_debug.long_count ^ 0x13579BDFu))
    {
        g_demo_task_sched_debug.local_error_count++;
    }

    g_demo_task_sched_debug.long_checksum = checksum;
    g_demo_task_sched_debug.long_count++;
    g_demo_task_sched_debug.long_active = 0u;
    demo_task_sched_mark(0xD5FF0000u | (g_demo_task_sched_debug.long_count & 0x0000FFFFu));
}

static void demo_sched_probe_ultra_long(void)
{
    uint32_t guard_a = 0x11223344u;
    uint32_t guard_b = 0x55667788u;
    uint32_t guard_c = 0x99AABBCCu;
    uint32_t seed = g_demo_task_sched_debug.ultra_count ^ 0x2468ACE0u;
    uint32_t checksum = seed;

    g_demo_task_sched_debug.ultra_active = 1u;
    g_demo_task_sched_debug.ultra_enter_count++;
    demo_task_sched_mark(0xE7000000u | (g_demo_task_sched_debug.ultra_enter_count & 0x0000FFFFu));

    for (uint32_t i = 0u; i < 3000000u; ++i)
    {
        checksum = (checksum << 5) ^ (checksum >> 2) ^ (seed + i);
        guard_a ^= (i & 0x55AA55AAu);
        guard_a ^= (i & 0x55AA55AAu);
        guard_b += (i & 0x00000003u);
        guard_b -= (i & 0x00000003u);
        guard_c = (guard_c << 1) | (guard_c >> 31);
        guard_c = (guard_c >> 1) | (guard_c << 31);

        if ((i & 0x0003FFFFu) == 0u)
        {
            g_demo_task_sched_debug.ultra_progress = i;
        }
    }

    if ((guard_a != 0x11223344u) || (guard_b != 0x55667788u) || (guard_c != 0x99AABBCCu))
    {
        g_demo_task_sched_debug.ultra_error_count++;
    }

    g_demo_task_sched_debug.ultra_checksum = checksum;
    g_demo_task_sched_debug.ultra_count++;
    g_demo_task_sched_debug.ultra_active = 0u;
    demo_task_sched_mark(0xE7FF0000u | (g_demo_task_sched_debug.ultra_count & 0x0000FFFFu));
}
#endif

REG_TASK_MS(10, demo_task_10ms)
REG_TASK(1, demo_task_100us)
#if (DEMO_TASK_DEAD_LOOP_ENABLE == 1)
REG_TASK_STACK_MS(1, demo_task_dead_loop_100ms, 512u)
REG_TASK_STACK_MS(1, demo_task_dead_loop_123ms, 512u)
#endif
#if (DEMO_TASK_SCHED_PROBE_ENABLE == 1)
REG_TASK_STACK(1, demo_sched_probe_fast, 128u)
REG_TASK_STACK(2, demo_sched_probe_mid, 128u)
REG_TASK_STACK(3, demo_sched_probe_float, 128u)
REG_TASK_STACK(5, demo_sched_probe_long, 128u)
REG_TASK_STACK(7, demo_sched_probe_ultra_long, 192u)
#endif
