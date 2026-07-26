// SPDX-License-Identifier: MIT
/**
 * @file    cllc_ctrl.h
 * @brief   Bidirectional CLLC controller public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Expose one controller containing forward and reverse control sets
 *          - Accept the run direction latched by the common FSM
 *          - Report loop references, feedbacks, candidates, and normalized output
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Periodic calculation is registered in the common interrupt dispatcher
 *          - Hardware access is abstracted through cllc_hal
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
#ifndef __CLLC_CTRL_H
#define __CLLC_CTRL_H

#include "cllc_cfg.h"

#include <stdint.h>

/** Controller values intended for scope, telemetry, and host verification. */
typedef struct
{
    CLLC_DIRECTION_E direction; /* Direction currently latched for the run. */
    float ref;                  /* Active voltage-loop reference in volts. */
    float fbk;                  /* Active voltage-loop feedback in volts. */
    float current_ref_a;        /* Forward current-limit reference in amperes. */
    float current_fbk_a;        /* Battery/load-current feedback in amperes. */
    float output;               /* Applied normalized modulation command, 0...1. */
    float pr_output;            /* Forward 100 Hz bipolar PR correction after -0.5...0.5 saturation. */
    float voltage_candidate;    /* Forward voltage-loop candidate before competition. */
    float current_candidate;    /* Forward current-loop candidate before competition. */
    uint8_t current_limit_active; /* Nonzero when forward current limiting wins. */
    uint8_t direction_mismatch;   /* Nonzero when setpoint direction changed during run. */
} cllc_ctrl_debug_t;

/**
 * @brief Reinitialize both control sets and latch one run direction.
 * @param direction Direction selected by the FSM before PWM enable.
 */
void cllc_ctrl_prepare_run(CLLC_DIRECTION_E direction);

/**
 * @brief Return whether the controller is executing an enabled run.
 * @return 1 when the ISR is actively controlling, otherwise 0.
 */
uint8_t cllc_ctrl_get_enable(void);

/**
 * @brief Return the direction latched at run entry.
 * @return Forward or reverse direction currently owned by the controller.
 */
CLLC_DIRECTION_E cllc_ctrl_get_direction(void);

/**
 * @brief Copy controller debug values for a slow observer.
 * @param p_debug Caller-owned destination object.
 */
void cllc_ctrl_get_debug(cllc_ctrl_debug_t *p_debug);

#endif /* __CLLC_CTRL_H */
