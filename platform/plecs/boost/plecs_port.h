// SPDX-License-Identifier: MIT
/**
 * @file    plecs_port.h
 * @brief   PLECS boost port definition.
 * @details
 *          This file is part of the BUCK2 project.
 *
 *          Module responsibilities:
 *          - Define the PLECS boost input signal order
 *          - Define the PLECS boost output signal order
 *          - Keep project-specific PLECS port layout outside the common bridge
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-28
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __PLECS_PORT_H
#define __PLECS_PORT_H

typedef enum
{
    PLECS_INPUT_HV = 0,
    PLECS_INPUT_LV,
    PLECS_INPUT_ILA,
    PLECS_INPUT_ILB,
    PLECS_INPUT_RUN,
    PLECS_INPUT_PWR_LMT,
    PLECS_INPUT_IN_CURR_LMT,
    PLECS_INPUT_OUT_CURR_LMT,
    PLECS_INPUT_OUT_VOLT_REF,
    PLECS_INPUT_IN_VOLT_LMT,
    PLECS_INPUT_MAX,
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_PWM_A_DUTY = 0,
    PLECS_OUTPUT_PWM_A_CMP,
    PLECS_OUTPUT_PWM_A_UP_EN,
    PLECS_OUTPUT_PWM_A_DN_EN,
    PLECS_OUTPUT_PWM_B_DUTY,
    PLECS_OUTPUT_PWM_B_CMP,
    PLECS_OUTPUT_PWM_B_UP_EN,
    PLECS_OUTPUT_PWM_B_DN_EN,
    PLECS_OUTPUT_RUN_STATE,
    PLECS_OUTPUT_HV_CODE,
    PLECS_OUTPUT_LV_CODE,
    PLECS_OUTPUT_ILA_CODE,
    PLECS_OUTPUT_ILB_CODE,
    PLECS_OUTPUT_MAX,
} PLECS_OUTPUT_E;

#endif
