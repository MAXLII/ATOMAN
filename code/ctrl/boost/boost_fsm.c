// SPDX-License-Identifier: MIT
/**
 * @file    boost_fsm.c
 * @brief   boost_fsm control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement the BOOST init, idle, and run state machine
 *          - Latch external start/stop commands and translate them into REG_FSM events
 *          - Coordinate run entry and run exit through the BOOST HAL callbacks
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
#include "boost_fsm.h"
#include <stddef.h>

static uint32_t fsm_ev = boost_fsm_ev_null;
static boost_fsm_cmd_e fsm_cmd = boost_fsm_cmd_null;
#define p_hal (boost_hal_get_fsm())

void boost_fsm_set_cmd(boost_fsm_cmd_e cmd)
{
    fsm_cmd = cmd;
}

void boost_fsm_emergency_stop(void)
{
    boost_hal_pwm_disable();
    boost_fsm_set_cmd(boost_fsm_cmd_stop);
}

void boost_fsm_set_p_hal(boost_fsm_hal_t *p)
{
    (void)p;
}

static boost_fsm_cmd_e get_cmd(void)
{
    boost_fsm_cmd_e temp = fsm_cmd;
    fsm_cmd = boost_fsm_cmd_null;
    return temp;
}

static void init_in(void)
{
}

static void init_exe(void)
{
    if ((p_hal != NULL) &&
        (p_hal->p_enter_run_func != NULL) &&
        (p_hal->p_exit_run_func != NULL))
    {
        fsm_ev = boost_fsm_ev_to_idle;
    }
}

static uint32_t init_chk(uint32_t event)
{
    if (event == boost_fsm_ev_to_idle)
    {
        return boost_fsm_sta_idle;
    }
    return 0U;
}

static void init_out(void)
{
}

static void idle_in(void)
{
    boost_hal_unlock_binding();
}

static void idle_exe(void)
{
    if (get_cmd() == boost_fsm_cmd_start)
    {
        if (boost_hal_is_ready() == 0U)
        {
            return;
        }

        fsm_ev = boost_fsm_ev_to_run;
    }
}

static uint32_t idle_chk(uint32_t event)
{
    if (event == boost_fsm_ev_to_run)
    {
        return boost_fsm_sta_run;
    }
    return 0U;
}

static void idle_out(void)
{
    boost_hal_lock_binding();
}

static void run_in(void)
{
    if ((p_hal != NULL) &&
        (p_hal->p_enter_run_func != NULL))
    {
        p_hal->p_enter_run_func();
    }
}

static void run_exe(void)
{
    if (get_cmd() == boost_fsm_cmd_stop)
    {
        fsm_ev = boost_fsm_ev_to_idle;
    }
}

static uint32_t run_chk(uint32_t event)
{
    if (event == boost_fsm_ev_to_idle)
    {
        return boost_fsm_sta_idle;
    }
    return 0U;
}

static void run_out(void)
{
    if ((p_hal != NULL) &&
        (p_hal->p_exit_run_func != NULL))
    {
        p_hal->p_exit_run_func();
    }
}

REG_FSM(boost_fsm, boost_fsm_sta_init, fsm_ev,
        FSM_ENTRY(boost_fsm_sta_init, init_in, init_exe, init_chk, init_out),
        FSM_ENTRY(boost_fsm_sta_idle, idle_in, idle_exe, idle_chk, idle_out),
        FSM_ENTRY(boost_fsm_sta_run, run_in, run_exe, run_chk, run_out), )

boost_run_sta_e boost_fsm_get_run_sta(void)
{
    boost_fsm_sta_e sta = (boost_fsm_sta_e)FSM_GET_STATE(boost_fsm);

    if (sta == boost_fsm_sta_init)
    {
        return boost_run_sta_init;
    }
    if (sta == boost_fsm_sta_idle)
    {
        return boost_run_sta_idle;
    }
    return boost_run_sta_run;
}
