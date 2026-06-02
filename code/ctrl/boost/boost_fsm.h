// SPDX-License-Identifier: MIT
/**
 * @file    boost_fsm.h
 * @brief   boost_fsm control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define BOOST FSM states, commands, events, and HAL callback contract
 *          - Expose command and HAL-binding entry points for the BOOST state machine
 *          - Provide the public state-machine interface used by platform and control glue
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-23
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __BOOST_FSM_H
#define __BOOST_FSM_H

#include "boost_hal.h"
#include "section.h"
#include <stdint.h>

typedef enum
{
    boost_fsm_sta_null = 0,
    boost_fsm_sta_init = 1,
    boost_fsm_sta_idle,
    boost_fsm_sta_run,
    boost_fsm_sta_max,
} boost_fsm_sta_e;

typedef enum
{
    boost_fsm_ev_null = 0,
    boost_fsm_ev_to_idle = 1,
    boost_fsm_ev_to_run,
} boost_fsm_ev_e;

typedef enum
{
    boost_fsm_cmd_null = 0,
    boost_fsm_cmd_start,
    boost_fsm_cmd_stop,
} boost_fsm_cmd_e;

typedef enum
{
    boost_run_sta_init = 0,
    boost_run_sta_idle,
    boost_run_sta_run,
} boost_run_sta_e;

void boost_fsm_set_cmd(boost_fsm_cmd_e cmd);
void boost_fsm_emergency_stop(void);
boost_run_sta_e boost_fsm_get_run_sta(void);
void boost_fsm_set_p_hal(boost_fsm_hal_t *p);

#endif
