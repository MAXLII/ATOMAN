// SPDX-License-Identifier: MIT
/**
 * @file    plecs_port.h
 * @brief   PLECS LLC port definition.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the PLECS LLC input signal order
 *          - Define the PLECS LLC output signal order
 *          - Keep project-specific PLECS port layout outside the common bridge
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
#ifndef __PLECS_LLC_PORT_H
#define __PLECS_LLC_PORT_H

typedef enum
{
    PLECS_INPUT_V_OUT = 0,
    PLECS_INPUT_I_OUT,
    PLECS_INPUT_V_BUS,
    PLECS_INPUT_RUN,
    PLECS_INPUT_V_OUT_REF,
    PLECS_INPUT_MAX,
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_PWM_EN = 0,
    PLECS_OUTPUT_PWM_DUTY,
    PLECS_OUTPUT_PWM_FREQ,
    PLECS_OUTPUT_V_BUS_NOTCH,
    PLECS_OUTPUT_PI_REF,
    PLECS_OUTPUT_PI_FBK,
    PLECS_OUTPUT_PI_OUT,
    PLECS_OUTPUT_MAX,
} PLECS_OUTPUT_E;

#endif
