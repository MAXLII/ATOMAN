// SPDX-License-Identifier: MIT
/**
 * @file stm32f10x_it.h
 * @brief Cortex-M3 interrupt entry points.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Declare framework context-switch and peripheral handlers
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
#ifndef STM32F10X_IT_H
#define STM32F10X_IT_H
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
void TIM2_IRQHandler(void);
void USART1_IRQHandler(void);
void DMA1_Channel4_IRQHandler(void);
#endif /* STM32F10X_IT_H */
