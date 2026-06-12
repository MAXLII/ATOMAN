// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   PLECS LLC application module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Bind simulated analog and PWM interface objects to the LLC HAL
 *          - Translate the PLECS run input into LLC FSM start and stop commands
 *          - Publish PWM commands and filtered debug values back to the PLECS output vector
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
#include "app.h"
#include "bsp_adc.h"
#include "bsp_pwm.h"
#include "llc_cfg.h"
#include "llc_ctrl.h"
#include "llc_fsm.h"
#include "llc_hal.h"
#include "plecs.h"
#include "section.h"
#include "timing.h"

static uint8_t app_llc_hal_bound = 0U;
static uint8_t app_llc_timing_bound = 0U;
static float app_adc_v_out = 0.0f;
static float app_adc_i_out = 0.0f;
static float app_adc_v_bus = 0.0f;

static void app_update_llc_setpoint(void)
{
    float v_out_ref = plecs_get_input(PLECS_INPUT_V_OUT_REF);

    llc_cfg_set_v_out_ref(v_out_ref);
    llc_cfg_publish_building();
}

static inline void app_update_adc_feedback(void)
{
    app_adc_v_out = BSP_ADC_V_OUT;
    app_adc_i_out = BSP_ADC_I_OUT;
    app_adc_v_bus = BSP_ADC_V_BUS;
    app_update_llc_setpoint();
}

static void app_adc_feedback_isr(void)
{
    app_update_adc_feedback();
}

REG_INTERRUPT(0, app_adc_feedback_isr)

static void app_bind_llc_hal(void)
{
    if (app_llc_hal_bound != 0U)
    {
        return;
    }

    llc_hal_unlock_binding();

    llc_hal_set_v_out_ptr(&app_adc_v_out);
    llc_hal_set_i_out_ptr(&app_adc_i_out);
    llc_hal_set_v_bus_ptr(&app_adc_v_bus);
    llc_hal_set_pwm_setter(bsp_pwm_set_duty);
    llc_hal_set_pwm_enable(bsp_pwm_enable);
    llc_hal_set_pwm_disable(bsp_pwm_disable);

    app_llc_hal_bound = llc_hal_is_ready();
    if (app_llc_hal_bound != 0U)
    {
        llc_hal_lock_binding();
    }
}

static void app_bind_llc_timing(void)
{
    llc_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
        .task_ts = 100.0e-6f,
        .startup_delay_ticks = 1U,
    };

    if (app_llc_timing_bound != 0U)
    {
        return;
    }

    llc_cfg_set_timing(&timing);
    app_llc_timing_bound = llc_cfg_is_ready();
}

static void app_update_llc_debug_output(void)
{
    llc_ctrl_pi_debug_t pi_debug = {0};

    llc_ctrl_get_pi_debug(&pi_debug);
    plecs_set_output(PLECS_OUTPUT_PI_REF, pi_debug.ref);
    plecs_set_output(PLECS_OUTPUT_PI_FBK, pi_debug.fbk);
    plecs_set_output(PLECS_OUTPUT_PI_OUT, pi_debug.out);
    plecs_set_output(PLECS_OUTPUT_V_BUS_NOTCH, pi_debug.out_ff_norm);
}

REG_INTERRUPT(8, app_update_llc_debug_output)

static void app_task(void)
{
    uint8_t run_cmd = 0U;
    llc_run_sta_e run_sta = llc_run_sta_init;

    app_update_adc_feedback();
    app_bind_llc_timing();

    run_cmd = (plecs_get_input(PLECS_INPUT_RUN) > 0.5f) ? 1U : 0U;
    run_sta = llc_fsm_get_run_sta();

    if (run_cmd != 0U)
    {
        if (run_sta == llc_run_sta_idle)
        {
            app_bind_llc_hal();
            llc_fsm_set_cmd(llc_fsm_cmd_start);
        }
    }
    else
    {
        app_llc_hal_bound = 0U;
        if (run_sta != llc_run_sta_idle)
        {
            llc_fsm_set_cmd(llc_fsm_cmd_stop);
        }
    }
}

REG_TASK_MS(1, app_task)
