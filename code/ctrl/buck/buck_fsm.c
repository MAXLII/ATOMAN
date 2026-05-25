// SPDX-License-Identifier: MIT
/**
 * @file    buck_fsm.c
 * @brief   buck_fsm control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement the buck init, idle, run, and protection-gated state machine
 *          - Latch external start/stop commands and translate them into REG_FSM events
 *          - Coordinate run entry and run exit through the buck HAL callbacks
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
#include "buck_fsm.h"
#include <stddef.h>

static uint32_t fsm_ev = buck_fsm_ev_null;
static buck_fsm_cmd_e fsm_cmd = buck_fsm_cmd_null;
#define p_hal (buck_hal_get_fsm())

void buck_fsm_set_cmd(buck_fsm_cmd_e cmd)
{
    fsm_cmd = cmd;
}

void buck_fsm_set_p_hal(buck_fsm_hal_t *p)
{
    (void)p;
}

static buck_fsm_cmd_e buck_fsm_get_cmd(void)
{
    buck_fsm_cmd_e temp = fsm_cmd;
    fsm_cmd = buck_fsm_cmd_null;
    return temp;
}

static void buck_fsm_init_in(void)
{
    PLECS_LOG("buck_fsm enter init\n");
}

static void buck_fsm_init_exe(void)
{
    if ((p_hal != NULL) &&
        (p_hal->p_enter_run_func != NULL) &&
        (p_hal->p_exit_run_func != NULL))
    {
        PLECS_LOG("buck_fsm init ready, goto idle\n");
        fsm_ev = buck_fsm_ev_to_idle;
    }
}

static uint32_t buck_fsm_init_chk(uint32_t event)
{
    if (event == buck_fsm_ev_to_idle)
    {
        return buck_fsm_sta_idle;
    }
    return 0U;
}

static void buck_fsm_init_out(void)
{
    PLECS_LOG("buck_fsm leave init\n");
}

static void buck_fsm_idle_in(void)
{
    buck_hal_unlock_binding();
    PLECS_LOG("buck_fsm enter idle\n");
}

static void buck_fsm_idle_exe(void)
{
    if (buck_fsm_get_cmd() == buck_fsm_cmd_start)
    {
        if (buck_hal_is_ready() == 0U)
        {
            PLECS_LOG("buck_fsm start rejected by hal binding invalid\n");
            return;
        }

        if (*p_hal->p_latched == 1U)
        {
            PLECS_LOG("buck_fsm start rejected by hard protect latch\n");
            return;
        }

        PLECS_LOG("buck_fsm idle got start, goto run\n");
        fsm_ev = buck_fsm_ev_to_run;
    }
}

static uint32_t buck_fsm_idle_chk(uint32_t event)
{
    if (event == buck_fsm_ev_to_run)
    {
        return buck_fsm_sta_run;
    }
    return 0U;
}

static void buck_fsm_idle_out(void)
{
    buck_hal_lock_binding();
    PLECS_LOG("buck_fsm leave idle\n");
}

static void buck_fsm_run_in(void)
{
    PLECS_LOG("buck_fsm enter run\n");

    if ((p_hal != NULL) &&
        (p_hal->p_enter_run_func != NULL))
    {
        p_hal->p_enter_run_func();
        PLECS_LOG("buck_fsm control prepared\n");
    }
}

static void buck_fsm_run_exe(void)
{
    if (buck_fsm_get_cmd() == buck_fsm_cmd_stop)
    {
        PLECS_LOG("buck_fsm run got stop, goto idle\n");
        fsm_ev = buck_fsm_ev_to_idle;
    }
}

static uint32_t buck_fsm_run_chk(uint32_t event)
{
    if (event == buck_fsm_ev_to_idle)
    {
        return buck_fsm_sta_idle;
    }
    return 0U;
}

static void buck_fsm_run_out(void)
{
    PLECS_LOG("buck_fsm leave run\n");

    if ((p_hal != NULL) &&
        (p_hal->p_exit_run_func != NULL))
    {
        p_hal->p_exit_run_func();
        PLECS_LOG("buck_fsm control stopped\n");
    }
}

REG_FSM(BUCK_FSM, buck_fsm_sta_init, fsm_ev,
        FSM_ENTRY(buck_fsm_sta_init, buck_fsm_init_in, buck_fsm_init_exe, buck_fsm_init_chk, buck_fsm_init_out),
        FSM_ENTRY(buck_fsm_sta_idle, buck_fsm_idle_in, buck_fsm_idle_exe, buck_fsm_idle_chk, buck_fsm_idle_out),
        FSM_ENTRY(buck_fsm_sta_run, buck_fsm_run_in, buck_fsm_run_exe, buck_fsm_run_chk, buck_fsm_run_out), )

buck_run_sta_e buck_fsm_get_run_sta(void)
{
    buck_fsm_sta_e sta = (buck_fsm_sta_e)FSM_GET_STATE(BUCK_FSM);

    if (sta == buck_fsm_sta_init)
    {
        return buck_run_sta_init;
    }
    if (sta == buck_fsm_sta_idle)
    {
        return buck_run_sta_idle;
    }
    return buck_run_sta_run;
}
