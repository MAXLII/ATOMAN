// SPDX-License-Identifier: MIT
/**
 * @file    bsp_adc.h
 * @brief   PLECS LLC ADC adapter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define physical analog input macros for the PLECS LLC simulation project
 *          - Keep output voltage, output current, and bus-voltage reads stateless
 *          - Leave cached measurement mirrors to the application layer
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-10
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "plecs.h"

#define BSP_ADC_V_OUT (plecs_get_input(PLECS_INPUT_V_OUT))
#define BSP_ADC_I_OUT (plecs_get_input(PLECS_INPUT_I_OUT))
#define BSP_ADC_V_BUS (plecs_get_input(PLECS_INPUT_V_BUS))

#endif
