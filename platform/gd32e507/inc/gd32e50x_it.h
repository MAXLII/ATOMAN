// SPDX-License-Identifier: MIT
/**
 * @file    gd32e50x_it.h
 * @brief   GD32E507 exception and timer interrupt interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Declare Cortex-M exception handlers used by the GNU vector table
 *          - Declare the 10 kHz demo interrupt handler
 *          - Expose the SRTOS SVC and PendSV integration points
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - SVC and PendSV handlers are naked context-switch paths
 *          - Timer access is contained in the platform interrupt module
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

#ifndef GD32E50X_IT_H
#define GD32E50X_IT_H

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void TIMER2_IRQHandler(void);
void USART0_IRQHandler(void);

#endif /* GD32E50X_IT_H */
