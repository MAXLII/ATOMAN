// SPDX-License-Identifier: MIT
/**
 * @file    inv_fsm.h
 * @brief   inv_fsm control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define inverter FSM states, commands, events, and HAL callback contract
 *          - Expose command and HAL-binding APIs for inverter state control
 *          - Provide the state-machine interface used by inverter platform and control modules
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
#ifndef __INV_FSM_H
#define __INV_FSM_H

#include <stdint.h>
#include "inv_hal.h"
#include "section.h"

typedef enum
{
    inv_fsm_sta_null = 0,
    inv_fsm_sta_init = 1,
    inv_fsm_sta_idle,
    inv_fsm_sta_rly_on,
    inv_fsm_sta_run,
    inv_fsm_sta_max,
} inv_fsm_sta_e;

typedef enum
{
    inv_fsm_ev_null = 0,
    inv_fsm_ev_to_idle = 1,
    inv_fsm_ev_to_rly_on,
    inv_fsm_ev_to_run,
} inv_fsm_ev_e;

typedef enum
{
    inv_fsm_cmd_null = 0,
    inv_fsm_cmd_start,
    inv_fsm_cmd_stop,
} inv_fsm_cmd_e;

typedef enum
{
    inv_run_sta_init = 0,
    inv_run_sta_idle,
    inv_run_sta_run,
} inv_run_sta_e;

void inv_fsm_set_cmd(inv_fsm_cmd_e cmd);
inv_run_sta_e inv_fsm_get_run_sta(void);
void inv_fsm_set_p_hal(inv_fsm_hal_t *p);

#endif
