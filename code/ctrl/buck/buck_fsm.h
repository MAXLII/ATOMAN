// SPDX-License-Identifier: MIT
/**
 * @file    buck_fsm.h
 * @brief   buck_fsm control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the internal Buck lifecycle states and transition events
 *          - Consume configuration run requests and protection state
 *          - Expose read-only access to the FSM-published control snapshot
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
#ifndef __BUCK_FSM_H
#define __BUCK_FSM_H

#include "buck_cfg.h"
#include "buck_hal.h"
#include "section.h"
#include <stdint.h>

typedef enum
{
    buck_fsm_sta_null = 0,
    buck_fsm_sta_init = 1,
    buck_fsm_sta_idle,
    buck_fsm_sta_run,
    buck_fsm_sta_max,
} buck_fsm_sta_e;

typedef enum
{
    buck_fsm_ev_null = 0,
    buck_fsm_ev_to_idle = 1,
    buck_fsm_ev_to_run,
} buck_fsm_ev_e;

buck_run_sta_e buck_fsm_get_run_sta(void);
uint8_t buck_fsm_read_published(buck_ctrl_setpoint_t *p_setpoint);

#endif
