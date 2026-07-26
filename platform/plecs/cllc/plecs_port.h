// SPDX-License-Identifier: MIT
/**
 * @file    plecs_port.h
 * @brief   PLECS bidirectional CLLC port definition.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the DLL input order for CLLC feedback, commands, and references
 *          - Define the DLL output order for modulation, FSM, PI, and protection data
 *          - Keep the CLLC-specific port contract outside the common PLECS bridge
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Input and output order is part of the PLECS model integration contract
 *          - Append new ports at the tail to preserve existing signal indexes
 *
 * @author  Max.Li
 * @date    2026-07-26
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __PLECS_CLLC_PORT_H
#define __PLECS_CLLC_PORT_H

/** DLL input-vector indexes. */
typedef enum
{
    PLECS_INPUT_V_BATTERY = 0,   /* Battery-port voltage feedback in volts. */
    PLECS_INPUT_I_BATTERY,       /* Battery/load-current magnitude in amperes. */
    PLECS_INPUT_V_BUS,           /* High-voltage-bus feedback in volts. */
    PLECS_INPUT_RUN,             /* Run command: greater than 0.5 requests start. */
    PLECS_INPUT_DIRECTION,       /* 0: forward, greater than 0.5: reverse. */
    PLECS_INPUT_V_BATTERY_REF,   /* Forward battery-voltage reference in volts. */
    PLECS_INPUT_I_BATTERY_LIMIT, /* Forward battery-current ceiling in amperes. */
    PLECS_INPUT_V_BUS_REF,       /* Reverse bus-voltage reference in volts. */
    PLECS_INPUT_HARD_FAULT,      /* Rising edge immediately trips CLLC protection. */
    PLECS_INPUT_FAULT_RESET,     /* Rising edge resets fault when hard fault is inactive. */
    PLECS_INPUT_MAX
} PLECS_INPUT_E;

/** DLL output-vector indexes. */
typedef enum
{
    PLECS_OUTPUT_PRI_PWM_ENABLE = 0,   /* Dll:01 1 enables the primary-side bridge. */
    PLECS_OUTPUT_PRI_PWM_DUTY,         /* Dll:02 Primary phase-shift duty in the 0...0.5 range. */
    PLECS_OUTPUT_PRI_PWM_FREQUENCY_HZ, /* Dll:03 Primary switching frequency in hertz. */
    PLECS_OUTPUT_SEC_PWM_ENABLE,       /* Dll:04 1 enables the secondary-side bridge. */
    PLECS_OUTPUT_SEC_PWM_DUTY,         /* Dll:05 Secondary phase-shift duty in the 0...0.5 range. */
    PLECS_OUTPUT_SEC_PWM_FREQUENCY_HZ, /* Dll:06 Secondary switching frequency in hertz. */
    PLECS_OUTPUT_FSM_STATE,            /* Dll:07 Public CLLC_RUN_STATE_E value. */
    PLECS_OUTPUT_ACTIVE_DIRECTION,     /* Dll:08 Direction latched by the common FSM. */
    PLECS_OUTPUT_PI_REFERENCE,         /* Dll:09 Active direction voltage-loop reference. */
    PLECS_OUTPUT_PI_FEEDBACK,          /* Dll:10 Active direction voltage-loop feedback. */
    PLECS_OUTPUT_PI_OUTPUT,            /* Dll:11 Final normalized dual-PI plus PR output. */
    PLECS_OUTPUT_CURRENT_REFERENCE,    /* Dll:12 Forward current-limit reference. */
    PLECS_OUTPUT_CURRENT_FEEDBACK,     /* Dll:13 Battery/load-current feedback. */
    PLECS_OUTPUT_VOLTAGE_CANDIDATE,    /* Dll:14 Forward voltage-loop competition candidate. */
    PLECS_OUTPUT_CURRENT_CANDIDATE,    /* Dll:15 Forward current-loop competition candidate. */
    PLECS_OUTPUT_CURRENT_LIMIT_ACTIVE, /* Dll:16 1 when the forward current loop wins. */
    PLECS_OUTPUT_DIRECTION_MISMATCH,   /* Dll:17 Direction-buffer lock bypass diagnostic. */
    PLECS_OUTPUT_FAULT_LATCH,          /* Dll:18 Current hard-protection latch state. */
    PLECS_OUTPUT_DBG,                  /* Dll:19 Forward 100 Hz PR correction in the -0.5...0.5 range. */
    PLECS_OUTPUT_MAX = 40,
} PLECS_OUTPUT_E;

#endif /* __PLECS_CLLC_PORT_H */
