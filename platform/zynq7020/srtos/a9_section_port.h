// SPDX-License-Identifier: MIT
/**
 * @file    a9_section_port.h
 * @brief   Cortex-A9 exception port interface for the section SRTOS.
 * @details
 *          This file is part of the Zynq-7020 platform project.
 *
 *          Module responsibilities:
 *          - Expose Cortex-A9 vector installation and SVC yield operations
 *          - Publish the IRQ-exit context-switch request shared with assembly
 *          - Record port-level switch requests and fatal exception reasons
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Context save and restore are implemented in a9_section_port.S
 *          - Scheduler C code executes on the banked SVC stack
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

#ifndef A9_SECTION_PORT_H
#define A9_SECTION_PORT_H

#include <stdint.h>

typedef enum
{
    A9_SECTION_PORT_FAULT_NONE = 0U,             /* 当前未记录端口故障。 */
    A9_SECTION_PORT_FAULT_CONTEXT = 0xA901U,     /* 调度器返回了无效任务上下文。 */
    A9_SECTION_PORT_FAULT_UNDEFINED = 0xA902U,   /* Cortex-A9 未定义指令异常。 */
    A9_SECTION_PORT_FAULT_PREFETCH = 0xA903U,    /* Cortex-A9 预取中止异常。 */
    A9_SECTION_PORT_FAULT_DATA_ABORT = 0xA904U,  /* Cortex-A9 数据中止异常。 */
} a9_section_port_fault_t;

typedef struct
{
    uint32_t yield_request_count;      /* 任务主动触发 SVC 的累计次数。 */
    uint32_t irq_switch_request_count; /* 定时 IRQ 请求上下文切换的累计次数。 */
    uint32_t fault_reason;             /* 最后一次不可恢复端口故障原因。 */
} a9_section_port_debug_t;

extern volatile uint32_t g_a9_section_switch_requested;
extern volatile a9_section_port_debug_t g_a9_section_port_debug;

/**
 * @brief 将 Cortex-A9 VBAR 指向 A9 section SRTOS 异常向量表。
 */
void a9_section_port_install_vector_table(void);

/**
 * @brief 通过 SVC 立即让出当前公共任务栈。
 */
void a9_section_port_yield(void);

/**
 * @brief 请求在当前 IRQ 返回前执行一次公共栈上下文切换。
 */
void a9_section_port_switch_request(void);

/**
 * @brief 记录不可恢复故障并关闭 Cortex-A9 IRQ/FIQ。
 * @param[in] reason section 或异常端口故障原因。
 */
void a9_section_port_fault(uint32_t reason) __attribute__((noreturn));

#endif /* A9_SECTION_PORT_H */
