// SPDX-License-Identifier: MIT
/**
 * @file    plecs_port.h
 * @brief   PLECS inverter port definition module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define inverter PLECS input signal indexes
 *          - Define inverter PLECS output signal indexes
 *          - Provide port counts used by the shared PLECS DLL adapter
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-19
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef PLECS_INV_PORT_H
#define PLECS_INV_PORT_H

typedef enum
{
    PLECS_INPUT_I_L = 0,
    PLECS_INPUT_V_CAP,
    PLECS_INPUT_V_BUS,
    PLECS_INPUT_RUN,
    PLECS_INPUT_MAX,
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_PWM_FAST_DUTY = 0,
    PLECS_OUTPUT_PWM_FAST_UP_EN,
    PLECS_OUTPUT_PWM_FAST_DN_EN,
    PLECS_OUTPUT_PWM_SLOW_DUTY,
    PLECS_OUTPUT_PWM_SLOW_UP_EN,
    PLECS_OUTPUT_PWM_SLOW_DN_EN,
    PLECS_OUTPUT_INV_RLY,
    PLECS_OUTPUT_RUN_STATE,
    PLECS_OUTPUT_DBG,
    PLECS_OUTPUT_MAX = 30,
} PLECS_OUTPUT_E;

#endif
