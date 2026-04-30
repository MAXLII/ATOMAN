// SPDX-License-Identifier: MIT
/**
 * @file    bb_fsm.h
 * @brief   bb_fsm control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define buck-boost FSM states, commands, events, and HAL callback contract
 *          - Expose command and HAL-binding entry points for the buck-boost state machine
 *          - Provide the public state-machine interface used by platform and control glue
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __BB_FSM_H
#define __BB_FSM_H

#include <stdint.h>
#include "bb_hal.h"
#include "section.h"

typedef enum
{
    bb_fsm_sta_null = 0,
    bb_fsm_sta_init = 1,
    bb_fsm_sta_idle,
    bb_fsm_sta_run,
    bb_fsm_sta_max,
} bb_fsm_sta_e;

typedef enum
{
    bb_fsm_ev_null = 0,
    bb_fsm_ev_to_idle = 1,
    bb_fsm_ev_to_run,
} bb_fsm_ev_e;

typedef enum
{
    bb_fsm_cmd_null = 0,
    bb_fsm_cmd_start,
    bb_fsm_cmd_stop,
} bb_fsm_cmd_e;

typedef enum
{
    bb_run_sta_init = 0,
    bb_run_sta_idle,
    bb_run_sta_run,
} bb_run_sta_e;

/**
 * @brief Post an external command to the buck-boost FSM.
 * @param cmd Command to be consumed on the next FSM execution step.
 * @return None.
 */
void bb_fsm_set_cmd(bb_fsm_cmd_e cmd);

/**
 * @brief Get the coarse run state exposed to the rest of the system.
 * @param None.
 * @return One of init, idle, or run.
 */
bb_run_sta_e bb_fsm_get_run_sta(void);

/**
 * @brief Bind the FSM HAL callbacks.
 * @param p Pointer to the FSM HAL object. Passing NULL detaches the callbacks.
 * @return None.
 */
void bb_fsm_set_p_hal(bb_fsm_hal_t *p);

#endif
