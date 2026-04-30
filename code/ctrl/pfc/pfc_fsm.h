// SPDX-License-Identifier: MIT
/**
 * @file    pfc_fsm.h
 * @brief   pfc_fsm control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define PFC FSM states, commands, events, and HAL callback contract
 *          - Expose command and HAL-binding entry points for PFC state control
 *          - Provide the state-machine interface used by PFC platform and control modules
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
#ifndef __PFC_FSM_H
#define __PFC_FSM_H

#include <stdint.h>
#include "pfc_hal.h"
#include "section.h"

typedef enum
{
    pfc_fsm_sta_null = 0,
    pfc_fsm_sta_init = 1,
    pfc_fsm_sta_idle,
    pfc_fsm_sta_soft_start,
    pfc_fsm_sta_main_rly,
    pfc_fsm_sta_run,
    pfc_fsm_sta_max,
} pfc_fsm_sta_e;

typedef enum
{
    pfc_fsm_ev_null = 0,
    pfc_fsm_ev_to_idle = 1,
    pfc_fsm_ev_to_soft_start,
    pfc_fsm_ev_to_main_rly,
    pfc_fsm_ev_to_run,
} pfc_fsm_ev_e;

typedef enum
{
    pfc_fsm_cmd_null = 0,
    pfc_fsm_cmd_start,
    pfc_fsm_cmd_stop,
} pfc_fsm_cmd_e;

typedef enum
{
    pfc_run_sta_init = 0,
    pfc_run_sta_idle,
    pfc_run_sta_run,
} pfc_run_sta_e;

void pfc_fsm_set_cmd(pfc_fsm_cmd_e cmd);
pfc_run_sta_e pfc_fsm_get_run_sta(void);
void pfc_fsm_set_p_hal(pfc_fsm_hal_t *p);

#endif
