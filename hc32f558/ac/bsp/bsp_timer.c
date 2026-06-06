// SPDX-License-Identifier: MIT
/**
 * @file    bsp_timer.c
 * @brief   HC32F558 timer BSP module.
 * @details
 *          This file is part of the HC32F558 project.
 *
 *          Module responsibilities:
 *          - Configure Timer6_6 as a free-running 32-bit counter
 *          - Run the perf counter from PCLK0 divided by 16
 *          - Register the Timer6 counter register and counter period as the perf base counter
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Perf reads are ISR-safe 32-bit register reads
 *          - Hardware access is abstracted through HC32 LL timer APIs
 *
 * @author  Max.Li
 * @date    2026-06-06
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_timer.h"

#include "bsp_clk.h"
#include "hc32_ll.h"
#include "perf.h"
#include "section.h"

#define BSP_TIMER_PERF_UNIT        (CM_TMR6_6)
#define BSP_TIMER_PERF_FCG         (FCG2_PERIPH_TMR6_6)
#define BSP_TIMER_PERF_CLK_DIV     (TMR6_CLK_DIV16)
#define BSP_TIMER_PERF_CLK_DIV_VAL (16UL)
#define BSP_TIMER_PERF_PCLK0_HZ    (BSP_CLK_PCLK0_HZ)
#define BSP_TIMER_PERF_FREQ_HZ     (BSP_TIMER_PERF_PCLK0_HZ / BSP_TIMER_PERF_CLK_DIV_VAL)
#define BSP_TIMER_PERF_PERIOD_S    (1.0f / (float)BSP_TIMER_PERF_FREQ_HZ)
#define BSP_TIMER_PERF_PERIOD_VAL  (0xFFFFFFFFUL)

REG_PERF_BASE_CNT((uint32_t *)&BSP_TIMER_PERF_UNIT->CNTER, BSP_TIMER_PERF_PERIOD_S)

int32_t bsp_timer_init(void)
{
    stc_tmr6_init_t tmr6_init;

    LL_PERIPH_WE(LL_PERIPH_FCG);
    FCG_Fcg2PeriphClockCmd(BSP_TIMER_PERF_FCG, ENABLE);

    (void)TMR6_StructInit(&tmr6_init);
    tmr6_init.u8CountSrc = TMR6_CNT_SRC_SW;
    tmr6_init.sw_count.u32ClockDiv = BSP_TIMER_PERF_CLK_DIV;
    tmr6_init.sw_count.u32CountMode = TMR6_MD_SAWTOOTH;
    tmr6_init.sw_count.u32CountDir = TMR6_CNT_UP;
    tmr6_init.u32CountReload = TMR6_CNT_RELOAD_ON;
    tmr6_init.u32PeriodValue = BSP_TIMER_PERF_PERIOD_VAL;

    TMR6_Stop(BSP_TIMER_PERF_UNIT);
    TMR6_SetCountValue(BSP_TIMER_PERF_UNIT, 0UL);
    if (LL_OK != TMR6_Init(BSP_TIMER_PERF_UNIT, &tmr6_init))
    {
        LL_PERIPH_WP(LL_PERIPH_FCG);
        return LL_ERR;
    }

    TMR6_Start(BSP_TIMER_PERF_UNIT);

    LL_PERIPH_WP(LL_PERIPH_FCG);
    return LL_OK;
}

uint32_t bsp_timer_get_perf_cnt(void)
{
    return TMR6_GetCountValue(BSP_TIMER_PERF_UNIT);
}

static void bsp_timer_init_entry(void)
{
    (void)bsp_timer_init();
}

REG_INIT(-1, bsp_timer_init_entry)
