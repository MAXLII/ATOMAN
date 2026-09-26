// SPDX-License-Identifier: MIT
/**
 * @file plecs_port.h
 * @brief CHB PLECS DLL input and output signal ordering.
 * @details Eight analog samples feed the three-cell rectifier. Each cell
 *          publishes one unipolar duty; PLECS forms the other leg as 1 - duty.
 *          The soft-start and main relay commands use separate output ports.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef PLECS_CHB_PORT_H
#define PLECS_CHB_PORT_H

#define PLECS_CHB_CONTROL_PERIOD_S 0.0001 /* 10 kHz control update, s. */

typedef enum
{
    PLECS_INPUT_GRID_V = 0, /* Instantaneous grid voltage, V. */
    PLECS_INPUT_I_L,        /* Input inductor current toward the cascaded bridges, A. */
    PLECS_INPUT_BUS_1_V,    /* First H-bridge DC bus voltage, V. */
    PLECS_INPUT_BUS_2_V,    /* Second H-bridge DC bus voltage, V. */
    PLECS_INPUT_BUS_3_V,    /* Third H-bridge DC bus voltage, V. */
    PLECS_INPUT_BUS_1_I,    /* First measured DC-branch current, A. */
    PLECS_INPUT_BUS_2_I,    /* Second measured DC-branch current, A. */
    PLECS_INPUT_BUS_3_I,    /* Third measured DC-branch current, A. */
    PLECS_INPUT_MAX         /* Eight analog input channels; first five retain their order. */
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_CHB_1_DUTY = 0, /* First H-bridge leg A duty, 0..1. */
    PLECS_OUTPUT_CHB_2_DUTY,     /* Second H-bridge leg A duty, 0..1. */
    PLECS_OUTPUT_CHB_3_DUTY,     /* Third H-bridge leg A duty, 0..1. */
    PLECS_OUTPUT_PWM_ENABLE,     /* Common bridge enable: 1 enables all three cells. */
    PLECS_OUTPUT_MAIN_RELAY,     /* Main relay command: 1 closed, 0 open. */
    PLECS_OUTPUT_SOFT_START_RELAY, /* Soft-start relay command: 1 closed, 0 open. */
    PLECS_OUTPUT_LOAD_R1,       /* First simulated load resistance, ohm. */
    PLECS_OUTPUT_LOAD_R2,       /* Second simulated load resistance, ohm. */
    PLECS_OUTPUT_LOAD_R3,       /* Third simulated load resistance, ohm. */
    PLECS_OUTPUT_MAX             /* Nine output channels; first six retain their order. */
} PLECS_OUTPUT_E;

#endif /* PLECS_CHB_PORT_H */
