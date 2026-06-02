// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   PLECS boost application module.
 * @details
 *          This file is part of the BUCK2 project.
 *
 *          Module responsibilities:
 *          - Bind simulated ADC and PWM interface objects to the boost HAL
 *          - Translate the PLECS run input into boost FSM start and stop commands
 *          - Publish the boost FSM state back to the PLECS output vector
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-24
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "app.h"
#include "boost_cfg.h"
#include "boost_fsm.h"
#include "boost_hal.h"
#include "bsp_adc.h"
#include "bsp_pwm.h"
#include "plecs.h"
#include "section.h"
#include "timing.h"

/* Tracks whether the PLECS application has already attempted HAL binding. */
static uint8_t app_boost_hal_bound = 0U;

static uint8_t app_boost_timing_bound = 0U;

/* App-owned high-voltage feedback mirror supplied by the ADC interface. */
static uint32_t app_adc_hv = 0;

/* App-owned low-voltage feedback mirror supplied by the ADC interface. */
static uint32_t app_adc_lv = 0;

/* App-owned A-channel inductor-current feedback mirror supplied by the ADC interface. */
static uint32_t app_adc_ila = 0;

/* App-owned B-channel inductor-current feedback mirror supplied by the ADC interface. */
static uint32_t app_adc_ilb = 0;

static inline void app_update_adc_feedback(void)
{
    app_adc_hv = BSP_ADC_HV;
    app_adc_lv = BSP_ADC_LV;
    app_adc_ila = BSP_ADC_ILA;
    app_adc_ilb = BSP_ADC_ILB;

    plecs_set_output(PLECS_OUTPUT_HV_CODE, (float)app_adc_hv);
    plecs_set_output(PLECS_OUTPUT_LV_CODE, (float)app_adc_lv);
    plecs_set_output(PLECS_OUTPUT_ILA_CODE, (float)app_adc_ila);
    plecs_set_output(PLECS_OUTPUT_ILB_CODE, (float)app_adc_ilb);
}

static void app_adc_feedback_isr(void)
{
    app_update_adc_feedback();
}

REG_INTERRUPT(0, app_adc_feedback_isr)

static void app_bind_boost_hal(void)
{
    if (app_boost_hal_bound != 0U)
    {
        return;
    }

    boost_hal_unlock_binding();

    /* Boost uses LV as input and HV as output in the shared PLECS signal set. */
    boost_hal_set_v_in_ptr(&app_adc_lv);
    boost_hal_set_v_out_ptr(&app_adc_hv);
    boost_hal_set_i_l_ptr(0U, &app_adc_ila);
    boost_hal_set_i_l_ptr(1U, &app_adc_ilb);
    boost_hal_set_pwm_setter(0U, bsp_pwm_set_a_cmp);
    boost_hal_set_pwm_setter(1U, bsp_pwm_set_b_cmp);
    boost_hal_set_pwm_enable(bsp_pwm_enable);
    boost_hal_set_pwm_disable(bsp_pwm_disable);

    app_boost_hal_bound = boost_hal_is_ready();
    if (app_boost_hal_bound != 0U)
    {
        boost_hal_lock_binding();
    }
}

static void app_bind_boost_timing(void)
{
    boost_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
        .task_ts = 100.0e-6f,
        .pwm_ts = PWM_TS,
        .pwm_cmp_max = CTRL_PWM_CMP_MAX,
    };

    if (app_boost_timing_bound != 0U)
    {
        return;
    }

    boost_cfg_set_timing(&timing);
    app_boost_timing_bound = boost_cfg_is_ready();
}

static void app_update_boost_setpoint(void)
{
    boost_cfg_set_pwr_lmt(plecs_get_input(PLECS_INPUT_PWR_LMT));
    boost_cfg_set_in_curr_lmt(plecs_get_input(PLECS_INPUT_IN_CURR_LMT));
    boost_cfg_set_out_curr_lmt(plecs_get_input(PLECS_INPUT_OUT_CURR_LMT));
    boost_cfg_set_out_volt_ref(plecs_get_input(PLECS_INPUT_OUT_VOLT_REF));
    boost_cfg_set_in_volt_lmt(plecs_get_input(PLECS_INPUT_IN_VOLT_LMT));
    boost_cfg_publish_building();
}

static void app_task(void)
{
    /* Run command sampled from the PLECS input vector. */
    uint8_t run_cmd = 0U;

    /* Boost FSM state mirrored to the PLECS output vector. */
    boost_run_sta_e run_sta = boost_run_sta_init;

    app_update_adc_feedback();
    app_bind_boost_timing();
    app_update_boost_setpoint();

    run_cmd = (plecs_get_input(PLECS_INPUT_RUN) > 0.5f) ? 1U : 0U;
    run_sta = boost_fsm_get_run_sta();
    plecs_set_output(PLECS_OUTPUT_RUN_STATE, (float)run_sta);

    if (run_cmd != 0U)
    {
        if (run_sta == boost_run_sta_idle)
        {
            app_bind_boost_hal();
            boost_fsm_set_cmd(boost_fsm_cmd_start);
        }
    }
    else
    {
        app_boost_hal_bound = 0U;
        if (run_sta != boost_run_sta_idle)
        {
            boost_fsm_set_cmd(boost_fsm_cmd_stop);
        }
    }
}

REG_TASK_MS(1, app_task)
