// SPDX-License-Identifier: MIT
/**
 * @file    gd32e50x_it.c
 * @brief   GD32E507 exception and timer interrupt implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Dispatch the 10 kHz demo interrupt callback chain
 *          - Maintain the SysTick time base and request deferred scheduling
 *          - Save and restore Cortex-M33 SRTOS task contexts through SVC/PendSV
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - SVC and PendSV are naked handlers with explicit register frames
 *          - TIMER2 flags are cleared before the registered ISR chain runs
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

#include "gd32e50x_it.h"

#include "bsp_usart.h"
#include "gd32e50x.h"
#include "section.h"
#include "systick.h"

#define GD32_EXCEPTION_NAKED __attribute__((naked))

static void fault_loop(void)
{
    __disable_irq();
    for (;;)
    {
    }
}

void NMI_Handler(void)
{
    fault_loop();
}

void HardFault_Handler(void)
{
    fault_loop();
}

void MemManage_Handler(void)
{
    fault_loop();
}

void BusFault_Handler(void)
{
    fault_loop();
}

void UsageFault_Handler(void)
{
    fault_loop();
}

void GD32_EXCEPTION_NAKED SVC_Handler(void)
{
    __ASM volatile(
        "push {r0, lr}                     \n"
        "bl section_task_start_request     \n"
        "pop {r0, r1}                      \n"
        "bx r1                             \n");
}

void DebugMon_Handler(void)
{
    fault_loop();
}

void GD32_EXCEPTION_NAKED PendSV_Handler(void)
{
#if defined(__FPU_USED) && (__FPU_USED == 1U)
    __ASM volatile(
        "push {r0, lr}                     \n"
        "bl section_task_scheduler_started \n"
        "cmp r0, #0                        \n"
        "beq 1f                            \n"
        "ldr lr, [sp, #4]                  \n"
        "tst lr, #0x04                     \n"
        "beq 2f                            \n"
        "mrs r0, psp                       \n"
        "cbz r0, 2f                        \n"
        "tst lr, #0x10                     \n"
        "it eq                             \n"
        "vstmdbeq r0!, {s16-s31}           \n"
        "stmdb r0!, {r4-r11, lr}           \n"
        "b 3f                              \n"
        "2:                                \n"
        "movs r0, #0                       \n"
        "3:                                \n"
        "bl section_task_switch_sp         \n"
        "cbz r0, 1f                        \n"
        "ldmia r0!, {r4-r11, lr}           \n"
        "tst lr, #0x10                     \n"
        "it eq                             \n"
        "vldmiaeq r0!, {s16-s31}           \n"
        "msr psp, r0                       \n"
        "add sp, sp, #8                    \n"
        "bx lr                             \n"
        "1:                                \n"
        "pop {r0, r1}                      \n"
        "bx r1                             \n");
#else
    __ASM volatile(
        "push {r0, lr}                     \n"
        "bl section_task_scheduler_started \n"
        "cmp r0, #0                        \n"
        "beq 1f                            \n"
        "ldr lr, [sp, #4]                  \n"
        "tst lr, #0x04                     \n"
        "beq 2f                            \n"
        "mrs r0, psp                       \n"
        "cbz r0, 2f                        \n"
        "stmdb r0!, {r4-r11, lr}           \n"
        "b 3f                              \n"
        "2:                                \n"
        "movs r0, #0                       \n"
        "3:                                \n"
        "bl section_task_switch_sp         \n"
        "cbz r0, 1f                        \n"
        "ldmia r0!, {r4-r11, lr}           \n"
        "msr psp, r0                       \n"
        "add sp, sp, #8                    \n"
        "bx lr                             \n"
        "1:                                \n"
        "pop {r0, r1}                      \n"
        "bx r1                             \n");
#endif
}

void SysTick_Handler(void)
{
    delay_decrement();
    section_task_irq_exit_request();
}

void TIMER2_IRQHandler(void)
{
    if (timer_interrupt_flag_get(TIMER2, TIMER_INT_FLAG_UP) != RESET)
    {
        timer_interrupt_flag_clear(TIMER2, TIMER_INT_FLAG_UP);
        section_interrupt();
    }
}

void USART0_IRQHandler(void)
{
    bsp_usart_dbg_irq_handler();
}
