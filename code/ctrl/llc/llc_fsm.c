// SPDX-License-Identifier: MIT
/**
 * @file    llc_fsm.c
 * @brief   LLC FSM module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement the LLC init, idle, startup, and run state machine
 *          - Latch external start/stop commands and translate them into REG_FSM events
 *          - Coordinate run entry and run exit through the LLC HAL callbacks
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
#include "llc_fsm.h"
#include "llc_cfg.h"
#include "llc_hal.h"
#include "my_math.h"
#include <stddef.h>

typedef enum
{
    llc_fsm_ev_null = 0,
    llc_fsm_ev_to_idle = 1,
    llc_fsm_ev_to_startup,
    llc_fsm_ev_to_run,
} llc_fsm_ev_e;

static uint32_t fsm_ev = llc_fsm_ev_null;
static llc_fsm_cmd_e fsm_cmd = llc_fsm_cmd_null;
static uint32_t startup_cnt = 0U;
static uint8_t is_ups_trig = 0U;

#define p_hal (llc_hal_get_fsm())

void llc_fsm_set_cmd(llc_fsm_cmd_e cmd)
{
    fsm_cmd = cmd;
}

void llc_fsm_emergency_stop(void)
{
    llc_hal_pwm_disable();
    llc_fsm_set_cmd(llc_fsm_cmd_stop);
}

void llc_fsm_set_p_hal(llc_fsm_hal_t *p)
{
    (void)p;
}

static llc_fsm_cmd_e get_cmd(void)
{
    llc_fsm_cmd_e temp = fsm_cmd;
    fsm_cmd = llc_fsm_cmd_null;
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
        fsm_ev = llc_fsm_ev_to_idle;
    }
}

static uint32_t init_chk(uint32_t event)
{
    if (event == llc_fsm_ev_to_idle)
    {
        return llc_fsm_sta_idle;
    }
    return 0U;
}

static void init_out(void)
{
}

static void idle_in(void)
{
    llc_hal_unlock_binding();
}

static void idle_exe(void)
{
    if (get_cmd() == llc_fsm_cmd_start)
    {
        if ((llc_hal_is_ready() == 0U) ||
            (llc_cfg_is_ready() == 0U))
        {
            return;
        }

        fsm_ev = llc_fsm_ev_to_startup;
    }
}

static uint32_t idle_chk(uint32_t event)
{
    if (event == llc_fsm_ev_to_startup)
    {
        return llc_fsm_sta_startup;
    }
    return 0U;
}

static void idle_out(void)
{
    llc_hal_lock_binding();
}

static void startup_in(void)
{
    startup_cnt = llc_cfg_get_startup_delay_ticks();

    if ((p_hal != NULL) &&
        (p_hal->p_enter_run_func != NULL))
    {
        p_hal->p_enter_run_func();
    }
}

static void startup_exe(void)
{
    if (get_cmd() == llc_fsm_cmd_stop)
    {
        fsm_ev = llc_fsm_ev_to_idle;
        return;
    }

    DN_CNT(startup_cnt);
    if (startup_cnt == 0U)
    {
        fsm_ev = llc_fsm_ev_to_run;
    }
}

static uint32_t startup_chk(uint32_t event)
{
    if (event == llc_fsm_ev_to_run)
    {
        return llc_fsm_sta_run;
    }
    if (event == llc_fsm_ev_to_idle)
    {
        return llc_fsm_sta_idle;
    }
    return 0U;
}

static void startup_out(void)
{
}

static void run_in(void)
{
}

static void run_exe(void)
{
    if (get_cmd() == llc_fsm_cmd_stop)
    {
        fsm_ev = llc_fsm_ev_to_idle;
    }
}

static uint32_t run_chk(uint32_t event)
{
    if (event == llc_fsm_ev_to_idle)
    {
        return llc_fsm_sta_idle;
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

REG_FSM(llc_fsm, llc_fsm_sta_init, fsm_ev,
        FSM_ENTRY(llc_fsm_sta_init, init_in, init_exe, init_chk, init_out),
        FSM_ENTRY(llc_fsm_sta_idle, idle_in, idle_exe, idle_chk, idle_out),
        FSM_ENTRY(llc_fsm_sta_startup, startup_in, startup_exe, startup_chk, startup_out),
        FSM_ENTRY(llc_fsm_sta_run, run_in, run_exe, run_chk, run_out), )

llc_run_sta_e llc_fsm_get_run_sta(void)
{
    llc_fsm_sta_e sta = (llc_fsm_sta_e)FSM_GET_STATE(llc_fsm);

    if (sta == llc_fsm_sta_init)
    {
        return llc_run_sta_init;
    }
    if (sta == llc_fsm_sta_idle)
    {
        return llc_run_sta_idle;
    }
    if (sta == llc_fsm_sta_startup)
    {
        return llc_run_sta_startup;
    }
    return llc_run_sta_run;
}

uint8_t llc_fsm_get_is_ups_trig(void)
{
    uint8_t temp = is_ups_trig;
    is_ups_trig = 0U;
    return temp;
}
