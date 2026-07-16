// SPDX-License-Identifier: MIT
/**
 * @file    a9_section_control.c
 * @brief   Cortex-A9 control operations for the section SRTOS port.
 * @details
 *          This file is part of the Zynq-7020 platform project.
 *
 *          Module responsibilities:
 *          - Generate SVC exceptions for cooperative public-stack switching
 *          - Latch IRQ-exit context-switch requests for the assembly handler
 *          - Preserve debugger-readable counters and fatal fault information
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Request publication is ordered with a Cortex-A9 data barrier
 *          - Fatal fault handling intentionally does not return
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

#include "a9_section_port.h"

#include <stdint.h>

volatile uint32_t g_a9_section_switch_requested = 0U; /* IRQ 返回前的任务切换请求。 */
volatile a9_section_port_debug_t g_a9_section_port_debug = {
    .yield_request_count = 0U,
    .irq_switch_request_count = 0U,
    .fault_reason = A9_SECTION_PORT_FAULT_NONE,
}; /* Cortex-A9 section SRTOS 端口调试状态。 */

void a9_section_port_yield(void)
{
    g_a9_section_port_debug.yield_request_count++;
    __asm volatile("svc 0" ::: "memory", "cc"); /* 进入 A9 公共栈切换入口。 */
}

void a9_section_port_switch_request(void)
{
    g_a9_section_port_debug.irq_switch_request_count++;
    g_a9_section_switch_requested = 1U;
    __asm volatile("dsb" ::: "memory"); /* 使汇编 IRQ 返回路径立即看到请求。 */
}

void a9_section_port_fault(uint32_t reason)
{
    g_a9_section_port_debug.fault_reason = reason;
    __asm volatile("cpsid if" ::: "memory", "cc"); /* 保留故障现场并阻止嵌套异常。 */
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb" ::: "memory");

    for (;;)
    {
        __asm volatile("wfi" ::: "memory"); /* 等待 JTAG 读取故障状态。 */
    }
}
