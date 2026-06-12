// SPDX-License-Identifier: MIT
/**
 * @file    llc_fsm.h
 * @brief   LLC FSM public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the simple LLC startup state machine contract
 *          - Expose start, stop, and emergency-stop commands
 *          - Provide public run-state inspection for platform glue
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-10
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __LLC_FSM_H
#define __LLC_FSM_H

#include "llc_hal.h"
#include "section.h"
#include <stdint.h>

typedef enum
{
    llc_fsm_sta_null = 0,
    llc_fsm_sta_init = 1,
    llc_fsm_sta_idle,
    llc_fsm_sta_startup,
    llc_fsm_sta_run,
    llc_fsm_sta_max,
} llc_fsm_sta_e;

typedef enum
{
    llc_fsm_cmd_null = 0,
    llc_fsm_cmd_start,
    llc_fsm_cmd_stop,
} llc_fsm_cmd_e;

typedef enum
{
    llc_run_sta_init = 0,
    llc_run_sta_idle,
    llc_run_sta_startup,
    llc_run_sta_run,
} llc_run_sta_e;

void llc_fsm_set_cmd(llc_fsm_cmd_e cmd);
void llc_fsm_emergency_stop(void);
llc_run_sta_e llc_fsm_get_run_sta(void);
void llc_fsm_set_p_hal(llc_fsm_hal_t *p);
uint8_t llc_fsm_get_is_ups_trig(void);

#endif
