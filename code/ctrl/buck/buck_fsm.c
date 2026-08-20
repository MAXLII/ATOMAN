// SPDX-License-Identifier: MIT
/**
 * @file    buck_fsm.c
 * @brief   buck_fsm control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement the buck init, idle, run, and protection-gated state machine
 *          - Decide when the building configuration becomes visible to the control ISR
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
#include "buck_cfg.h"

#include <stddef.h>
#include <stdatomic.h>

typedef struct
{
    atomic_uint_least32_t sequence;
    atomic_uchar run_allowed;
    atomic_int_least32_t out_volt_ref;
    atomic_int_least32_t in_volt_lmt;
    atomic_int_least32_t pwr_lmt;
    atomic_int_least32_t in_curr_lmt;
    atomic_int_least32_t out_curr_lmt;
} buck_fsm_published_t;

static buck_fsm_published_t published_setpoint = {
    .sequence = ATOMIC_VAR_INIT(0U),
    .run_allowed = ATOMIC_VAR_INIT(0U),
    .out_volt_ref = ATOMIC_VAR_INIT(
        BUCK_CTRL_OUT_VOLT_LOOP_REF_TO_CODE(BUCK_CTRL_OUT_VOLT_LOOP_REF_DEFAULT_V)),
    .in_volt_lmt = ATOMIC_VAR_INIT(
        BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_TO_CODE(BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_DEFAULT_V)),
    .pwr_lmt = ATOMIC_VAR_INIT(BUCK_CTRL_IN_PWR_LMT_TO_CODE(BUCK_CTRL_IN_PWR_LMT_DEFAULT_W)),
    .in_curr_lmt = ATOMIC_VAR_INIT(BUCK_CTRL_IN_CURR_LMT_TO_CODE(BUCK_CTRL_IN_CURR_LMT_DEFAULT_A)),
    .out_curr_lmt = ATOMIC_VAR_INIT(BUCK_CTRL_OUT_CURR_LMT_TO_CODE(BUCK_CTRL_OUT_CURR_LMT_DEFAULT_A)),
};

static uint32_t fsm_ev = buck_fsm_ev_null;
#define p_hal (buck_hal_get_fsm())

static void buck_fsm_publish_building(uint8_t run_allowed)
{
    const buck_ctrl_setpoint_t *p_building = buck_cfg_get_p_building();
    uint_least32_t sequence =
        atomic_load_explicit(&published_setpoint.sequence, memory_order_relaxed);

    atomic_store_explicit(&published_setpoint.sequence, sequence + 1U, memory_order_seq_cst);
    atomic_store_explicit(&published_setpoint.run_allowed,
                          (run_allowed != 0U) ? 1U : 0U,
                          memory_order_relaxed);
    atomic_store_explicit(&published_setpoint.out_volt_ref,
                          p_building->out_volt_ref,
                          memory_order_relaxed);
    atomic_store_explicit(&published_setpoint.in_volt_lmt,
                          p_building->in_volt_lmt,
                          memory_order_relaxed);
    atomic_store_explicit(&published_setpoint.pwr_lmt,
                          p_building->pwr_lmt,
                          memory_order_relaxed);
    atomic_store_explicit(&published_setpoint.in_curr_lmt,
                          p_building->in_curr_lmt,
                          memory_order_relaxed);
    atomic_store_explicit(&published_setpoint.out_curr_lmt,
                          p_building->out_curr_lmt,
                          memory_order_relaxed);
    atomic_store_explicit(&published_setpoint.sequence, sequence + 2U, memory_order_seq_cst);
}

uint8_t buck_fsm_read_published(buck_ctrl_setpoint_t *p_setpoint)
{
    buck_ctrl_setpoint_t snapshot = {0};
    uint_least32_t sequence_before =
        atomic_load_explicit(&published_setpoint.sequence, memory_order_acquire);
    uint_least32_t sequence_after = 0U;

    if ((sequence_before & 1U) != 0U)
    {
        return 0U;
    }

    snapshot.run_allowed =
        (uint8_t)atomic_load_explicit(&published_setpoint.run_allowed, memory_order_relaxed);
    snapshot.out_volt_ref =
        (int32_t)atomic_load_explicit(&published_setpoint.out_volt_ref, memory_order_relaxed);
    snapshot.in_volt_lmt =
        (int32_t)atomic_load_explicit(&published_setpoint.in_volt_lmt, memory_order_relaxed);
    snapshot.pwr_lmt =
        (int32_t)atomic_load_explicit(&published_setpoint.pwr_lmt, memory_order_relaxed);
    snapshot.in_curr_lmt =
        (int32_t)atomic_load_explicit(&published_setpoint.in_curr_lmt, memory_order_relaxed);
    snapshot.out_curr_lmt =
        (int32_t)atomic_load_explicit(&published_setpoint.out_curr_lmt, memory_order_relaxed);

    sequence_after = atomic_load_explicit(&published_setpoint.sequence, memory_order_acquire);
    if (sequence_before != sequence_after)
    {
        return 0U;
    }

    *p_setpoint = snapshot;
    return 1U;
}

static void buck_fsm_init_in(void)
{
    buck_hal_unlock_binding();
    PLECS_LOG("buck_fsm enter init\n");
}

static void buck_fsm_init_exe(void)
{
    extern buck_ctrl_hal_t buck_ctrl_hal;
    extern buck_fsm_hal_t buck_fsm_hal;
    uint32_t ch = 0U;

    if (buck_ctrl_hal.p_v_in == NULL)
    {
        PLECS_LOG("buck_fsm init: buck_ctrl_hal.p_v_in is NULL\n");
        return;
    }

    if (buck_ctrl_hal.p_v_out == NULL)
    {
        PLECS_LOG("buck_fsm init: buck_ctrl_hal.p_v_out is NULL\n");
        return;
    }

    for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
    {
        if (buck_ctrl_hal.p_i_l[ch] == NULL)
        {
            PLECS_LOG("buck_fsm init: buck_ctrl_hal.p_i_l[%u] is NULL\n", (unsigned)ch);
            return;
        }
    }

    for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
    {
        if (buck_ctrl_hal.p_set_pwm_func[ch] == NULL)
        {
            PLECS_LOG("buck_fsm init: buck_ctrl_hal.p_set_pwm_func[%u] is NULL\n", (unsigned)ch);
            return;
        }
    }

    if (buck_ctrl_hal.p_pwm_disable == NULL)
    {
        PLECS_LOG("buck_fsm init: buck_ctrl_hal.p_pwm_disable is NULL\n");
        return;
    }

    if (buck_fsm_hal.p_enter_run_func == NULL)
    {
        PLECS_LOG("buck_fsm init: buck_fsm_hal.p_enter_run_func is NULL\n");
        return;
    }

    if (buck_fsm_hal.p_exit_run_func == NULL)
    {
        PLECS_LOG("buck_fsm init: buck_fsm_hal.p_exit_run_func is NULL\n");
        return;
    }

    PLECS_LOG("buck_fsm init ready, goto idle\n");
    fsm_ev = buck_fsm_ev_to_idle;
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
    buck_fsm_publish_building(0U);
    PLECS_LOG("buck_fsm enter idle\n");
}

static void buck_fsm_idle_exe(void)
{
    if (buck_cfg_get_run_request() != 0U)
    {
        if (buck_hal_hard_protect_is_latched() != 0U)
        {
            PLECS_LOG("buck_fsm start rejected by hard protect latch\n");
            return;
        }

        PLECS_LOG("buck_fsm accepted run request, goto run\n");
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
    buck_fsm_publish_building(0U);
    p_hal->p_enter_run_func();
    PLECS_LOG("buck_fsm control prepared\n");
}

static void buck_fsm_run_exe(void)
{
    if ((buck_cfg_get_run_request() == 0U) ||
        (buck_hal_hard_protect_is_latched() != 0U))
    {
        buck_fsm_publish_building(0U);
        PLECS_LOG("buck_fsm stop condition met, goto idle\n");
        fsm_ev = buck_fsm_ev_to_idle;
        return;
    }

    buck_fsm_publish_building(1U);
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
    p_hal->p_exit_run_func();
    PLECS_LOG("buck_fsm control stopped\n");
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
