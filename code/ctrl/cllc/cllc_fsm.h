// SPDX-License-Identifier: MIT
/**
 * @file    cllc_fsm.h
 * @brief   Bidirectional CLLC common FSM public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define one lifecycle state machine shared by forward and reverse control
 *          - Latch the selected direction before bridge enable
 *          - Coordinate startup delay, stop, hard fault, and fault reset
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Direction cannot change while startup or run is active
 *          - Hard protection disables PWM before the scheduled FSM observes the latch
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
#ifndef __CLLC_FSM_H
#define __CLLC_FSM_H

#include "cllc_cfg.h"

#include <stdint.h>

/** Internal lifecycle states registered with the common section FSM runner. */
typedef enum
{
    CLLC_FSM_STATE_NULL = 0,    /* Invalid sentinel used by the section FSM. */
    CLLC_FSM_STATE_INIT = 1,    /* Wait for internal HAL lifecycle callbacks. */
    CLLC_FSM_STATE_IDLE,        /* Permit binding/configuration and wait for start. */
    CLLC_FSM_STATE_STARTUP,     /* Enable selected direction and wait bridge settling time. */
    CLLC_FSM_STATE_RUN,         /* Execute the selected controller in the fast ISR. */
    CLLC_FSM_STATE_FAULT,       /* Hold PWM disabled until an explicit reset command. */
    CLLC_FSM_STATE_MAX
} CLLC_FSM_STATE_E;

/** One-shot commands accepted by the common CLLC FSM. */
typedef enum
{
    CLLC_FSM_CMD_NULL = 0, /* No pending command. */
    CLLC_FSM_CMD_START,    /* Start the direction staged in the active setpoint. */
    CLLC_FSM_CMD_STOP,     /* Stop either direction and return to idle. */
    CLLC_FSM_CMD_RESET     /* Clear a hard fault and return to idle. */
} CLLC_FSM_CMD_E;

/** Coarse state exposed to platform/application code. */
typedef enum
{
    CLLC_RUN_STATE_INIT = 0, /* Module is waiting for initialization. */
    CLLC_RUN_STATE_IDLE,     /* Module is stopped and may be rebound. */
    CLLC_RUN_STATE_STARTUP,  /* Selected bridge is in startup delay. */
    CLLC_RUN_STATE_RUN,      /* Fast control is active. */
    CLLC_RUN_STATE_FAULT     /* Hard-protection latch is active. */
} CLLC_RUN_STATE_E;

/** @brief Post a one-shot lifecycle command. @param command Start, stop, reset, or null. */
void cllc_fsm_set_cmd(CLLC_FSM_CMD_E command);
/** @brief Disable PWM immediately and latch hard protection. */
void cllc_fsm_emergency_stop(void);
/** @brief Get the public lifecycle state. @return Init, idle, startup, run, or fault. */
CLLC_RUN_STATE_E cllc_fsm_get_run_state(void);
/** @brief Get the direction latched at the latest accepted start. @return Forward or reverse. */
CLLC_DIRECTION_E cllc_fsm_get_direction(void);

#endif /* __CLLC_FSM_H */
