// SPDX-License-Identifier: MIT
/**
 * @file stm32f10x_conf.h
 * @brief STM32F1 standard peripheral driver selection.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Select the vendor drivers required by this board port
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
#ifndef STM32F10X_CONF_H
#define STM32F10X_CONF_H
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#define assert_param(expr) ((void)0)
#endif /* STM32F10X_CONF_H */
