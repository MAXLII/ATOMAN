// SPDX-License-Identifier: MIT
/**
 * @file    bsp_timer.c
 * @brief   Zynq-7020 section time-base implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Read the Cortex-A9 global timer through the Xilinx standalone BSP
 *          - Convert global-timer counts into a monotonic 100 us section tick
 *          - Preserve unsigned wraparound behavior expected by section scheduling
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The read path is safe in task and IRQ context
 *          - Hardware access is isolated in the Zynq BSP
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_timer.h"

#include "perf.h"
#include "section.h"
#include "xil_exception.h"
#include "xparameters.h"
#include "xscugic.h"
#include "xscutimer.h"
#include "xstatus.h"
#include "xtime_l.h"

#include <stdint.h>

#define BSP_TIMER_TICKS_PER_SECOND 10000ULL /* Section 每秒包含的 100 us tick 数量。 */

volatile uint32_t sys_tick_100us = 0U; /* Trace 与调试模块共享的 100 us 系统时间。 */

static XTime s_start_count = 0ULL;          /* 平台启动时的全局计时器基准计数。 */
static XScuGic s_interrupt_controller;      /* Cortex-A9 通用中断控制器实例。 */
static XScuTimer s_section_interrupt_timer; /* 驱动 section_interrupt 的私有定时器实例。 */

REG_PERF_BASE_CNT((uintptr_t)(GLOBAL_TMR_BASEADDR + GTIMER_COUNTER_LOWER_OFFSET),
                  (1.0f / (float)COUNTS_PER_SECOND))

static void section_timer_interrupt_handler(void *callback_ref)
{
    XScuTimer *timer = (XScuTimer *)callback_ref; /* 触发本次 IRQ 的私有定时器实例。 */

    XScuTimer_ClearInterruptStatus(timer); /* 先清除私有定时器中断状态，允许下一周期触发。 */
    section_interrupt();                   /* 调度全部 SECTION_INTERRUPT 注册回调。 */
#if (SRTOS == 1)
    if ((section_task_scheduler_started() != 0U) && /* SRTOS 已经接管任务运行栈。 */
        (section_task_slice_elapsed() != 0U))       /* 当前任务已运行一个 1 ms 时间片。 */
    {
        a9_section_port_switch_request(); /* 请求自定义 IRQ 返回路径保存现场并切换任务。 */
    }
#endif
}

void bsp_timer_init(void)
{
    XTime_GetTime(&s_start_count); /* 记录软件平台启动时刻，避免继承 BootROM 运行时间。 */
}

uint32_t bsp_timer_gettime_100us(void)
{
    XTime current_count = 0ULL; /* 当前 Cortex-A9 全局计时器计数。 */
    XTime elapsed_count = 0ULL; /* 自平台启动以来累计的计时器计数。 */
    XTime ticks_100us = 0ULL;   /* 换算后的 100 us 系统 tick。 */

    XTime_GetTime(&current_count); /* 原子读取 64 位全局计时器。 */
    elapsed_count = current_count - s_start_count;
    ticks_100us = (elapsed_count * BSP_TIMER_TICKS_PER_SECOND) / (XTime)COUNTS_PER_SECOND;
    sys_tick_100us = (uint32_t)ticks_100us;

    return (uint32_t)ticks_100us;
}

int32_t bsp_timer_interrupt_start(uint32_t frequency_hz)
{
    XScuGic_Config *gic_config = NULL;     /* GIC 硬件配置描述符。 */
    XScuTimer_Config *timer_config = NULL; /* Cortex-A9 私有定时器配置描述符。 */
    uint64_t timer_clock_hz = 0ULL;        /* 私有定时器输入时钟频率，单位 Hz。 */
    uint64_t timer_load = 0ULL;            /* 目标中断频率对应的重装计数。 */
    int32_t status = XST_FAILURE;          /* Xilinx 驱动初始化或连接结果。 */

    if (frequency_hz == 0U)
    {
        return XST_INVALID_PARAM;
    }

    timer_config = XScuTimer_LookupConfig(XPAR_XSCUTIMER_0_DEVICE_ID);
    if (timer_config == NULL)
    {
        return XST_FAILURE;
    }

    status = XScuTimer_CfgInitialize(&s_section_interrupt_timer,
                                     timer_config,
                                     timer_config->BaseAddr);
    if (status != XST_SUCCESS)
    {
        return status;
    }

    gic_config = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
    if (gic_config == NULL)
    {
        return XST_FAILURE;
    }

    status = XScuGic_CfgInitialize(&s_interrupt_controller,
                                   gic_config,
                                   gic_config->CpuBaseAddress);
    if (status != XST_SUCCESS)
    {
        return status;
    }

    Xil_ExceptionInit(); /* 建立 Cortex-A9 异常向量分派环境。 */
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 &s_interrupt_controller); /* 将 IRQ 异常入口交给 GIC 分派。 */

    status = XScuGic_Connect(&s_interrupt_controller,
                             XPAR_SCUTIMER_INTR,
                             (Xil_ExceptionHandler)section_timer_interrupt_handler,
                             &s_section_interrupt_timer);
    if (status != XST_SUCCESS)
    {
        return status;
    }

    timer_clock_hz = (uint64_t)XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2ULL;
    timer_load = timer_clock_hz / (uint64_t)frequency_hz;
    if ((timer_load == 0ULL) || /* 目标频率高于私有定时器输入时钟。 */
        (timer_load > (uint64_t)UINT32_MAX)) /* 目标周期超出 32 位重装寄存器范围。 */
    {
        return XST_INVALID_PARAM;
    }

    XScuTimer_LoadTimer(&s_section_interrupt_timer, (uint32_t)(timer_load - 1ULL)); /* 配置目标 IRQ 周期。 */
    XScuTimer_EnableAutoReload(&s_section_interrupt_timer);                         /* 每次到期后自动重新装载。 */
    XScuGic_Enable(&s_interrupt_controller, XPAR_SCUTIMER_INTR);                    /* 在 GIC 中开放私有定时器 IRQ。 */
    XScuTimer_EnableInterrupt(&s_section_interrupt_timer);                          /* 开放私有定时器本地中断。 */
    Xil_ExceptionEnableMask((uint32_t)XIL_EXCEPTION_IRQ);                           /* 开放 Cortex-A9 IRQ 异常。 */
    XScuTimer_Start(&s_section_interrupt_timer);                                    /* 启动 10 kHz section 中断源。 */

    return XST_SUCCESS;
}
