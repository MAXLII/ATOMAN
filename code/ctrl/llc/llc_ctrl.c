// SPDX-License-Identifier: MIT
/**
 * @file    llc_ctrl.c
 * @brief   LLC control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement the LLC single voltage-loop control path
 *          - Sample output current directly for monitoring and filter output voltage with a low-pass filter
 *          - Consume HAL measurements and setpoints to generate one LLC PWM command
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
#include "llc_ctrl.h"
#include "llc_cfg.h"
#include "my_math.h"
#include "notch.h"
#include "pi_tustin.h"
#include "section.h"
#include <stddef.h>

#define p_hal (p_ctrl_hal)

static pi_tustin_t volt_loop = {0};
static notch_t v_bus_notch_filter = {0};
static llc_ctrl_hal_t *p_ctrl_hal = NULL;
static llc_ctrl_setpoint_t safe_setpoint = {0};
static llc_ctrl_setpoint_t *p_ctrl_active_setpoint = &safe_setpoint;
static float v_out_ref = 0.0f;
static float i_out_act = 0.0f;
static float v_bus_act = 0.0f;
static float v_bus_notch_comp = 0.0f;
static float v_out_flt = 0.0f;
static float v_out_last = 0.0f;
static uint8_t run_active = 0U;

static uint8_t llc_ctrl_ready(void)
{
    return (uint8_t)((p_hal != NULL) &&
                     (llc_cfg_is_ready() != 0U) &&
                     (p_ctrl_active_setpoint != NULL) &&
                     (p_hal->p_v_out != NULL) &&
                     (p_hal->p_i_out != NULL) &&
                     (p_hal->p_v_bus != NULL) &&
                     (p_hal->p_set_pwm_func != NULL) &&
                     (p_hal->p_pwm_disable != NULL));
}

static inline void llc_ctrl_sample_analog(void)
{
    float v_out_raw = 0.0f;

    /*
     * This ISR sampling layer is the only place that converts HAL raw analog
     * bindings into the clean feedback values consumed by the control loop.
    */
    i_out_act = *p_hal->p_i_out;
    v_bus_act = *p_hal->p_v_bus;

    notch_cal(&v_bus_notch_filter);
    v_bus_notch_comp = (v_bus_notch_filter.output.val - v_bus_act) /
                       LLC_CTRL_OUT_FF_NORM_BASE;

    v_out_raw = *p_hal->p_v_out;
    LPF(v_out_raw,
        v_out_last,
        v_out_flt,
        llc_cfg_get_ctrl_ts(),
        M_2PI * LLC_CTRL_VOUT_LPF_CUTOFF_HZ);
}

static void llc_ctrl_reinit_states(void)
{
    float ctrl_ts = 0.0f;
    llc_ctrl_setpoint_t *p_active_setpoint = NULL;

    p_ctrl_hal = llc_hal_get_ctrl();
    llc_cfg_sync_building_to_active();
    p_active_setpoint = llc_cfg_get_p_active();
    p_ctrl_active_setpoint = (p_active_setpoint != NULL) ? p_active_setpoint : &safe_setpoint;
    run_active = 0U;

    if (llc_ctrl_ready() == 0U)
    {
        return;
    }

    ctrl_ts = llc_cfg_get_ctrl_ts();
    i_out_act = *p_hal->p_i_out;
    v_bus_act = *p_hal->p_v_bus;
    v_bus_notch_comp = 0.0f;
    v_out_flt = *p_hal->p_v_out;
    v_out_last = v_out_flt;
    v_out_ref = v_out_flt;

    notch_init(&v_bus_notch_filter,
               M_2PI * LLC_CTRL_VBUS_NOTCH_CENTER_HZ,
               M_2PI * LLC_CTRL_VBUS_NOTCH_BANDWIDTH_HZ,
               ctrl_ts,
               &v_bus_act);

    (void)pi_tustin_init(&volt_loop,
                         LLC_CTRL_VOLT_LOOP_KP,
                         LLC_CTRL_VOLT_LOOP_KI,
                         ctrl_ts,
                         LLC_CTRL_OUTPUT_UP_LMT,
                         LLC_CTRL_OUTPUT_DN_LMT,
                         &v_out_ref,
                         &v_out_flt);
}

static void llc_ctrl_init(void)
{
    llc_ctrl_reinit_states();
}

REG_INIT(0, llc_ctrl_init)

static void llc_ctrl_force_safe_output(void)
{
    run_active = 0U;
    pi_tustin_reset(&volt_loop);

    if ((p_hal != NULL) &&
        (p_hal->p_pwm_disable != NULL))
    {
        p_hal->p_pwm_disable();
    }
}

/*
 * LLC control block diagram
 *
 * ISR sampling layer:
 *
 *   p_hal->p_i_out -----------------------------> i_out_act
 *   p_hal->p_v_bus -----------------------------> v_bus_act
 *
 *   p_hal->p_v_out -----------------------------> v_out_raw
 *                                                      |
 *                                                      v
 *                                                +-----------+
 *                                                | LPF 50 Hz |
 *                                                +-----+-----+
 *                                                      |
 *                                                      v
 *                                                   v_out_flt
 *
 * Control path:
 *
 *   run_allowed == 0 ---------------------------> disable/reset output state
 *
 *   run_allowed != 0:
 *
 *   setpoint.v_out_ref_v ---------------> v_out_ref
 *                                               |
 *                                           +---v---+
 *   v_out_flt ---------------------------->|  PI   |
 *                                           +---+---+
 *                                               |
 *                                               v
 *                                        volt_loop.output.val
 *                                               |
 *                                               v
 *                                        + v_bus_notch_comp
 *                                               |
 *                                               v
 *                                        p_hal->p_set_pwm_func()
 */
static void llc_ctrl_isr(void)
{
    llc_ctrl_setpoint_t *p_setpoint = p_ctrl_active_setpoint;

    if (llc_ctrl_ready() == 0U)
    {
        return;
    }

    llc_cfg_sync_building_to_active();
    p_setpoint = p_ctrl_active_setpoint;
    llc_ctrl_sample_analog();

    if (p_setpoint->run_allowed == 0U)
    {
        if (run_active != 0U)
        {
            llc_ctrl_force_safe_output();
        }
        return;
    }

    if (run_active == 0U)
    {
        v_out_ref = v_out_flt;
        pi_tustin_reset(&volt_loop);
        run_active = 1U;
    }

    v_out_ref = p_setpoint->v_out_ref_v;

    (void)pi_tustin_cal(&volt_loop);
    p_hal->p_set_pwm_func(volt_loop.output.val + v_bus_notch_comp);
}

REG_INTERRUPT(2, llc_ctrl_isr)

void llc_ctrl_set_p_hal(llc_ctrl_hal_t *p)
{
    (void)p;
}

void llc_ctrl_prepare_run(void)
{
    llc_ctrl_reinit_states();
}

uint8_t llc_ctrl_get_enable(void)
{
    if (p_ctrl_active_setpoint == NULL)
    {
        return 0U;
    }

    return p_ctrl_active_setpoint->run_allowed;
}

void llc_ctrl_get_pi_debug(llc_ctrl_pi_debug_t *p_debug)
{
    if (p_debug == NULL)
    {
        return;
    }

    p_debug->ref = v_out_ref;
    p_debug->fbk = v_out_flt;
    p_debug->out = volt_loop.output.val;
    p_debug->out_ff_norm = v_bus_notch_comp;
}
