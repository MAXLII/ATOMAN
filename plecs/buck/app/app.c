// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   PLECS buck application module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Bind simulated ADC and PWM interface objects to the buck HAL
 *          - Translate the PLECS run input into buck FSM start and stop commands
 *          - Publish the buck FSM state back to the PLECS output vector
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
#include "bsp_adc.h"
#include "bsp_pwm.h"
#include "buck_cfg.h"
#include "buck_fsm.h"
#include "buck_hal.h"
#include "plecs.h"
#include "section.h"
#include "timing.h"

/* Inductor-current ADC center code used to convert offset-binary samples to signed control codes. */
#define APP_ADC_IND_CURR_CODE_CENTER ((int32_t)0x4000)

/* Tracks whether the PLECS application has already attempted HAL binding. */
static uint8_t app_buck_hal_bound = 0U;

static uint8_t app_buck_timing_bound = 0U;

/* App-owned high-voltage feedback mirror supplied by the ADC interface. */
static int32_t app_adc_hv = 0;

/* App-owned low-voltage feedback mirror supplied by the ADC interface. */
static int32_t app_adc_lv = 0;

/* App-owned A-channel inductor-current feedback mirror supplied by the ADC interface. */
static int32_t app_adc_ila = 0;

/* App-owned B-channel inductor-current feedback mirror supplied by the ADC interface. */
static int32_t app_adc_ilb = 0;

static inline void app_update_adc_feedback(void)
{
    app_adc_hv = (int32_t)BSP_ADC_HV;
    app_adc_lv = (int32_t)BSP_ADC_LV;
    app_adc_ila = (int32_t)BSP_ADC_ILA - APP_ADC_IND_CURR_CODE_CENTER;
    app_adc_ilb = (int32_t)BSP_ADC_ILB - APP_ADC_IND_CURR_CODE_CENTER;

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

static void app_bind_buck_hal(void)
{
    if (app_buck_hal_bound != 0U)
    {
        return;
    }

    /* ADC feedback is supplied by the interface layer through app-owned mirrors. */
    buck_hal_set_v_in_ptr(&app_adc_hv);
    buck_hal_set_v_out_ptr(&app_adc_lv);
    buck_hal_set_i_l_ptr(0U, &app_adc_ila);
    buck_hal_set_i_l_ptr(1U, &app_adc_ilb);
    buck_hal_set_pwm_setter(0U, bsp_pwm_set_a_cmp);
    buck_hal_set_pwm_setter(1U, bsp_pwm_set_b_cmp);
    buck_hal_set_pwm_disable(bsp_pwm_disable);

    app_buck_hal_bound = buck_hal_is_ready();
}

static void app_bind_buck_timing(void)
{
    buck_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
        .task_ts = 100.0e-6f,
        .pwm_cmp_max = CTRL_PWM_CMP_MAX,
    };

    if (app_buck_timing_bound != 0U)
    {
        return;
    }

    buck_cfg_set_timing(&timing);
    app_buck_timing_bound = buck_cfg_is_ready();
}

static void app_update_buck_setpoint(void)
{
    buck_cfg_set_pwr_lmt(plecs_get_input(PLECS_INPUT_PWR_LMT));
    buck_cfg_set_in_curr_lmt(plecs_get_input(PLECS_INPUT_IN_CURR_LMT));
    buck_cfg_set_out_curr_lmt(plecs_get_input(PLECS_INPUT_OUT_CURR_LMT));
    buck_cfg_set_out_volt_ref(plecs_get_input(PLECS_INPUT_OUT_VOLT_REF));
    buck_cfg_publish_building();
}

static void app_task(void)
{
    /* Run command sampled from the PLECS input vector. */
    uint8_t run_cmd = 0U;

    /* Buck FSM state mirrored to the PLECS output vector. */
    buck_run_sta_e run_sta = buck_run_sta_init;

    app_update_adc_feedback();
    app_bind_buck_timing();
    app_update_buck_setpoint();

    run_cmd = (plecs_get_input(PLECS_INPUT_RUN) > 0.5f) ? 1U : 0U;
    run_sta = buck_fsm_get_run_sta();
    plecs_set_output(PLECS_OUTPUT_RUN_STATE, (float)run_sta);

    if (run_cmd != 0U)
    {
        if (run_sta == buck_run_sta_idle)
        {
            app_bind_buck_hal();
            buck_fsm_set_cmd(buck_fsm_cmd_start);
        }
    }
    else
    {
        app_buck_hal_bound = 0U;
        if (run_sta != buck_run_sta_idle)
        {
            buck_fsm_set_cmd(buck_fsm_cmd_stop);
        }
    }
}

REG_TASK_MS(1, app_task)
