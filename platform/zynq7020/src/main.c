// SPDX-License-Identifier: MIT
/**
 * @file    main.c
 * @brief   Zynq-7020 section and communication platform entry.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize PS UART1 and the global-timer section time base
 *          - Start linker-section discovery and registered initialization
 *          - Run the section implementation selected by the platform build
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The A9 SRTOS build keeps SVC and IRQ stacks separate from task context
 *          - Hardware access is abstracted through the Zynq BSP
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
#include "bsp_usart.h"
#include "section.h"

#include "xstatus.h"

int main(void)
{
    int32_t uart_status = XST_FAILURE; /* PS UART1 初始化结果。 */
    int32_t timer_status = XST_FAILURE; /* 10 kHz section 中断定时器初始化结果。 */

    bsp_timer_init();                               /* 建立 section 的 100 us 单调时间基准。 */
    section_port_init();                            /* 由选中的 section 实现完成运行端口初始化。 */
    uart_status = bsp_usart_init();                 /* 初始化连接板载 CH340 的 PS UART1。 */
    if (uart_status != XST_SUCCESS)
    {
        for (;;)
        {
            /* UART 不可用时停止启动，避免进入不可观测的通信调度状态。 */
        }
    }

    bsp_usart_dbg_printf("\r\nZynq-7020 section platform boot\r\n"); /* 输出最早期可观测启动信息。 */
    section_init();                                                   /* 发现并初始化全部 section 注册项。 */
    timer_status = bsp_timer_interrupt_start(10000U);                 /* 启动 10 kHz SECTION_INTERRUPT 调度。 */
    if (timer_status != XST_SUCCESS)
    {
        bsp_usart_dbg_printf("section interrupt timer init failed: %ld\r\n", (long)timer_status);
        for (;;)
        {
            /* 中断调度不可用时停止运行，避免给出不完整的 dbg 自测结果。 */
        }
    }

    while (1)
    {
        run_task(); /* 调度 UART、Shell、comm 和平台自检任务。 */
    }
}
