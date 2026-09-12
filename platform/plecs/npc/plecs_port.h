// SPDX-License-Identifier: MIT
/**
 * @file    plecs_port.h
 * @brief   NPC DLL signal ordering.
 * @details
 *          This file is part of the base digital power framework project.
 *          Define bus, output-voltage and inductor-current feedback, 6 duties and bridge enable.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef PLECS_NPC_PORT_H
#define PLECS_NPC_PORT_H

#define PLECS_NPC_CONTROL_PERIOD_S (0.0002) /* Fixed 5 kHz DLL control period, seconds. */
typedef enum
{
    PLECS_INPUT_V_DC_P = 0, /* Positive half-bus voltage, V. */
    PLECS_INPUT_V_DC_N,     /* Negative half-bus magnitude, V. */
    PLECS_INPUT_V_OUT_A,    /* Phase A output-to-neutral voltage, V. */
    PLECS_INPUT_V_OUT_B,    /* Phase B output-to-neutral voltage, V. */
    PLECS_INPUT_V_OUT_C,    /* Phase C output-to-neutral voltage, V. */
    PLECS_INPUT_I_L_A,      /* Phase A inductor current, positive from bridge toward output, A. */
    PLECS_INPUT_I_L_B,      /* Phase B inductor current, positive from bridge toward output, A. */
    PLECS_INPUT_I_L_C,      /* Phase C inductor current, positive from bridge toward output, A. */
    PLECS_INPUT_MAX         /* Number of DLL inputs. */
} PLECS_INPUT_E;
typedef enum
{
    PLECS_OUTPUT_A_POSITIVE_DUTY = 0, /* Phase A P fraction, [0,1]. */
    PLECS_OUTPUT_A_NEGATIVE_DUTY,     /* Phase A N fraction, [0,1]. */
    PLECS_OUTPUT_B_POSITIVE_DUTY,     /* Phase B P fraction, [0,1]. */
    PLECS_OUTPUT_B_NEGATIVE_DUTY,     /* Phase B N fraction, [0,1]. */
    PLECS_OUTPUT_C_POSITIVE_DUTY,     /* Phase C P fraction, [0,1]. */
    PLECS_OUTPUT_C_NEGATIVE_DUTY,     /* Phase C N fraction, [0,1]. */
    PLECS_OUTPUT_PWM_ENABLE, /* Global bridge enable: 1 only with a complete valid duty frame. */
    PLECS_OUTPUT_MAX       /* Number of DLL outputs: 6 duty channels and 1 enable. */
} PLECS_OUTPUT_E;
#endif /* PLECS_NPC_PORT_H */
