// SPDX-License-Identifier: MIT
/**
 * @file    bsp_timer.c
 * @brief   GD32E507 demo timer BSP implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure TIMER1 as a 2 MHz free-running Perf counter
 *          - Configure TIMER2 as the 10 kHz registered interrupt dispatcher
 *          - Register TIMER1 with the portable Perf service
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - TIMER2_IRQHandler clears the update flag before dispatch
 *          - Hardware access uses the GD32E50x standard peripheral library
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_timer.h"

#include "gd32e50x.h"
#include "perf.h"
#include "section.h"

#include <stdint.h>

#define BSP_TIMER_PERF_FREQ_HZ 2000000u /* TIMER1 Perf counter frequency. */
#define BSP_TIMER_ISR_FREQ_HZ 10000u    /* TIMER2 interrupt-dispatch frequency. */
#define BSP_TIMER_CNT_PERIOD_S 0.5e-6f  /* TIMER1 counter period in seconds. */

REG_PERF_BASE_CNT((uint32_t *)(uintptr_t)(TIMER1 + 0x24u), BSP_TIMER_CNT_PERIOD_S)

void bsp_timer_init(void)
{
    timer_parameter_struct timer_config = {0}; /* Shared base-timer configuration. */
    uint32_t timer_clock_hz = SystemCoreClock; /* TIMER1/TIMER2 clock after APB multiplier. */
    uint32_t perf_divider = 1u;                /* Divider used to produce the 2 MHz counter. */

    if (timer_clock_hz >= BSP_TIMER_PERF_FREQ_HZ)
    {
        perf_divider = timer_clock_hz / BSP_TIMER_PERF_FREQ_HZ;
    }

    rcu_periph_clock_enable(RCU_TIMER1);
    rcu_periph_clock_enable(RCU_TIMER2);

    timer_deinit(TIMER1);
    timer_struct_para_init(&timer_config);
    timer_config.prescaler = (uint16_t)(perf_divider - 1u);
    timer_config.alignedmode = TIMER_COUNTER_EDGE;
    timer_config.counterdirection = TIMER_COUNTER_UP;
    timer_config.period = 0xFFFFFFFFu;
    timer_config.clockdivision = TIMER_CKDIV_DIV1;
    timer_config.repetitioncounter = 0u;
    timer_init(TIMER1, &timer_config);
    timer_counter_value_config(TIMER1, 0u);
    timer_enable(TIMER1);

    timer_deinit(TIMER2);
    timer_struct_para_init(&timer_config);
    timer_config.prescaler = 0u;
    timer_config.alignedmode = TIMER_COUNTER_EDGE;
    timer_config.counterdirection = TIMER_COUNTER_UP;
    timer_config.period = (timer_clock_hz / BSP_TIMER_ISR_FREQ_HZ) - 1u;
    timer_config.clockdivision = TIMER_CKDIV_DIV1;
    timer_config.repetitioncounter = 0u;
    timer_init(TIMER2, &timer_config);
    timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(TIMER2, TIMER_INT_UP);
    nvic_irq_enable(TIMER2_IRQn, 1u, 0u);
    timer_enable(TIMER2);
}

REG_INIT(0, bsp_timer_init)
