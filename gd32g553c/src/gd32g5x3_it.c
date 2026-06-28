/*!
    \file    gd32g5x3_it.c
    \brief   interrupt service routines

    \version 2025-02-18, V1.1.0, demo for GD32G5x3
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32g5x3_it.h"
#include "section.h"
#include "systick.h"

#define SRAM_ECC_ERROR_HANDLE(s)    do{}while(1)
#define FLASH_ECC_ERROR_HANDLE(s)   do{}while(1)

#if defined(__GNUC__) || defined(__ARMCC_VERSION)
#define GD32_EXCEPTION_NAKED __attribute__((naked))
#else
#define GD32_EXCEPTION_NAKED
#endif

/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
    if(SET == syscfg_interrupt_flag_get(SYSCFG_INT_FLAG_TCMSRAMECCME)) {
        SRAM_ECC_ERROR_HANDLE("TCMSRAM multi-bits non-correction ECC error\r\n");
    }else if(SET == syscfg_interrupt_flag_get(SYSCFG_INT_FLAG_SRAM1ECCME)) {
        SRAM_ECC_ERROR_HANDLE("SRAM1 multi-bits non-correction ECC error\r\n");
    }else if(SET == syscfg_interrupt_flag_get(SYSCFG_INT_FLAG_SRAM0ECCME)) {
        SRAM_ECC_ERROR_HANDLE("SRAM0 multi-bits non-correction ECC error\r\n");
    }else if(SET == syscfg_interrupt_flag_get(SYSCFG_INT_FLAG_FLASHECC)) {
        FLASH_ECC_ERROR_HANDLE("FLASH ECC error\r\n");
    }else {
        /* if NMI exception occurs, go to infinite loop */
        /* HXTAL clock monitor NMI error or NMI pin error */
        while(1) {
        }
    }
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void GD32_EXCEPTION_NAKED SVC_Handler(void)
{
#if (SRTOS == 1)
    __ASM volatile(
        "push {r0, lr}                     \n"
        "bl section_task_start_request     \n"
        "pop {r0, r1}                      \n"
        "bx r1                             \n");
#else
    __ASM volatile("bx lr                  \n");
#endif
}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
    /* if DebugMon exception occurs, go to infinite loop */
    while(1) {
    }
}

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void GD32_EXCEPTION_NAKED PendSV_Handler(void)
{
#if (SRTOS == 1)
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
#else
    __ASM volatile("bx lr                  \n");
#endif
}

/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void)
{
    delay_decrement();
#if (SRTOS == 1)
    if((section_task_scheduler_started() != 0U) &&
       (section_task_slice_elapsed() != 0U)) {
        SRTOS_PENDSV_SET();
    }
#endif
}
