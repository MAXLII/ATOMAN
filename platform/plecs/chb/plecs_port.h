// SPDX-License-Identifier: MIT
/**
 * @file plecs_port.h
 * @brief Target-selected CHB and grid-source PLECS DLL signal ordering.
 * @details PLECS_GRID_SOURCE selects the zero-input, single-output source.
 *          Otherwise analog samples feed the cascaded rectifier. Each cell
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

#if defined(PLECS_GRID_SOURCE)

#include "grid_source.h"
#define PLECS_PARAMETER_NUM 1                          /* Grid fundamental RMS voltage from the model. */
#define PLECS_FAST_OUTPUT_HOOK grid_source_output_step /* Source updates at the DLL output cadence. */
#if defined(_WIN32)
#define PLECS_LOG_FILE_NAME L"plecs_grid_source_log.txt" /* Independent source log. */
#else
#define PLECS_LOG_FILE_NAME "plecs_grid_source_log.txt" /* Independent source log. */
#endif

typedef enum
{
    PLECS_INPUT_NULL,
    PLECS_INPUT_MAX, /* Standalone source has no analog inputs. */
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_GRID_SOURCE_V = 0, /* Instantaneous grid-source voltage, V. */
    PLECS_OUTPUT_MAX                /* One voltage output. */
} PLECS_OUTPUT_E;

#else

#define PLECS_PARAMETER_NUM 0
#define PLECS_CHB_CONTROL_PERIOD_S 0.0001L /* 10 kHz control update, s. */

typedef enum
{
    PLECS_INPUT_GRID_V = 0, /* Instantaneous grid voltage, V. */
    PLECS_INPUT_I_L,        /* Input inductor current toward the cascaded bridges, A. */
    /* DC-bus voltages in ascending cell index order, V. */
    PLECS_INPUT_BUS_1_V,
    PLECS_INPUT_BUS_2_V,
    PLECS_INPUT_BUS_3_V,
    PLECS_INPUT_BUS_4_V,
    PLECS_INPUT_BUS_5_V,
    /* Measured DC-branch currents in ascending cell index order, A. */
    PLECS_INPUT_BUS_1_I,
    PLECS_INPUT_BUS_2_I,
    PLECS_INPUT_BUS_3_I,
    PLECS_INPUT_BUS_4_I,
    PLECS_INPUT_BUS_5_I,
    PLECS_INPUT_MAX /* Analog input count. */
} PLECS_INPUT_E;

typedef enum
{
    /* H-bridge leg-A duties in ascending cell index order, 0..1. */
    PLECS_OUTPUT_CHB_1_DUTY = 0,
    PLECS_OUTPUT_CHB_2_DUTY,
    PLECS_OUTPUT_CHB_3_DUTY,
    PLECS_OUTPUT_CHB_4_DUTY,
    PLECS_OUTPUT_CHB_5_DUTY,
    PLECS_OUTPUT_PWM_ENABLE,       /* Common bridge enable: 1 enables all cells. */
    PLECS_OUTPUT_MAIN_RELAY,       /* Main relay command: 1 = ON, 0 = OFF. */
    PLECS_OUTPUT_SOFT_START_RELAY, /* Soft-start relay command: 1 = ON, 0 = OFF. */
    /* Simulated load resistances in ascending cell index order, ohm. */
    PLECS_OUTPUT_LOAD_R1,
    PLECS_OUTPUT_LOAD_R2,
    PLECS_OUTPUT_LOAD_R3,
    PLECS_OUTPUT_LOAD_R4,
    PLECS_OUTPUT_LOAD_R5,
    PLECS_OUTPUT_MAX /* Output count in model selector order. */
} PLECS_OUTPUT_E;

#endif /* PLECS_GRID_SOURCE */

#endif /* PLECS_CHB_PORT_H */
