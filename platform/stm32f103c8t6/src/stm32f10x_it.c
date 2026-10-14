// SPDX-License-Identifier: MIT
/**
 * @file stm32f10x_it.c
 * @brief Cortex-M3 SECTION exception and IRQ port.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Save/restore R4-R11 and EXC_RETURN for the shared SRTOS runtime
 *          - Forward peripheral interrupts to BSP and registered SECTION callbacks
 *          Design notes:
 *          - C11 compatible; no dynamic allocation in platform drivers
 *          - Hardware access belongs to the BSP; framework bindings belong to src
 * @author Max.Li
 * @date 2026-10-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */
#include "stm32f10x_it.h"
#include "stm32f10x.h"
#include "section.h"
#include "systick.h"
#include "bsp_usart.h"
#include "bsp_timer.h"
static void fault_stop(void)
{
    __disable_irq();
#if (SECTION_RUNTIME_PREEMPTIVE == 1u)
    g_section_fault_debug.cfsr  = SCB->CFSR;
    g_section_fault_debug.hfsr  = SCB->HFSR;
    g_section_fault_debug.bfar  = SCB->BFAR;
    g_section_fault_debug.mmfar = SCB->MMFAR;
    g_section_fault_debug.msp   = __get_MSP();
    g_section_fault_debug.psp   = __get_PSP();
#endif

    for (;;)
    {
    }
}
void HardFault_Handler(void)
{
    fault_stop();
}
void MemManage_Handler(void)
{
    fault_stop();
}
void BusFault_Handler(void)
{
    fault_stop();
}
void UsageFault_Handler(void)
{
    fault_stop();
}

/* Naked exception entries use MSP; the shared task stack lives on PSP.
 * The 8-byte temporary frame preserves MSP alignment for every C call.
 * Cortex-M3 has no FPU, so EXC_RETURN and R4-R11 are the entire software frame. */
void __attribute__((naked)) SVC_Handler(void)
{
    __ASM volatile("push {r0, lr}                     \n"
                   "bl section_task_start_request    \n"
                   "pop {r0, r1}                      \n"
                   "bx r1                             \n");
}
void __attribute__((naked)) PendSV_Handler(void)
{
    __ASM volatile("push {r0, lr}                     \n"
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
}
void SysTick_Handler(void)
{
    bsp_timer_perf_resume(); /* Recover tracing after J-Link clears the DWT gates on detach. */
    delay_decrement();       /* Publish a new monotonic tick. */
    section_task_irq_exit_request(); /* Request runtime scheduling after ISR exit. */
}
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        section_interrupt(); /* Execute registered 1 ms interrupt callbacks. */
        section_task_irq_exit_request();
    }
}
void USART1_IRQHandler(void)
{
    bsp_usart_dbg_irq_handler();
}
void DMA1_Channel4_IRQHandler(void)
{
    bsp_usart_dbg_tx_irq_handler();
}
