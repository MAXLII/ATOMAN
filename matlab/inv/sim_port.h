// SPDX-License-Identifier: MIT
/**
 * @file    sim_port.h
 * @brief   MATLAB inverter port definition module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define inverter MATLAB input signal indexes
 *          - Define inverter MATLAB output signal indexes
 *          - Provide port counts and sample timing used by the MATLAB S-Function adapter
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

#ifndef MATLAB_INV_PORT_H
#define MATLAB_INV_PORT_H

#include "my_math.h"

#define SIM_SAMPLE_TIME_S CTRL_TS
#define SIM_TICK_STEP_S (1.0e-4)
#define SIM_TICK_UNIT_US (100U)
#define SIM_TICKS_PER_SECOND (10000.0)

typedef enum
{
    SIM_INPUT_I_L = 0,
    SIM_INPUT_V_CAP,
    SIM_INPUT_V_BUS,
    SIM_INPUT_RUN,
    SIM_INPUT_MAX,
} SIM_INPUT_E;

typedef enum
{
    SIM_OUTPUT_PWM_FAST_DUTY = 0,
    SIM_OUTPUT_PWM_FAST_UP_EN,
    SIM_OUTPUT_PWM_FAST_DN_EN,
    SIM_OUTPUT_PWM_SLOW_DUTY,
    SIM_OUTPUT_PWM_SLOW_UP_EN,
    SIM_OUTPUT_PWM_SLOW_DN_EN,
    SIM_OUTPUT_INV_RLY,
    SIM_OUTPUT_RUN_STATE,
    SIM_OUTPUT_DBG,
    SIM_OUTPUT_MAX = 30,
} SIM_OUTPUT_E;

#define SIM_INPUT_NUM SIM_INPUT_MAX
#define SIM_OUTPUT_NUM SIM_OUTPUT_MAX

#endif
