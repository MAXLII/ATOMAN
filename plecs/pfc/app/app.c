// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   PLECS PFC application module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Bind simulated ADC and PWM signals to the PFC HAL
 *          - Translate the PLECS run input into PFC FSM start and stop commands
 *          - Publish PFC run state and relay state back to the PLECS output vector
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-19
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
#include "cal_rms.h"
#include "pfc_cfg.h"
#include "pfc_fsm.h"
#include "pfc_hal.h"
#include "plecs.h"
#include "section.h"
#include "timing.h"

#define APP_VBUS_PRECHARGE_ENTER_V (M_SQRT2 * 200.0f)
#define APP_VBUS_NOM_ENTER_V (380.0f)
#define APP_GRID_RMS_MIN_V (180.0f)
#define APP_GRID_RMS_MAX_V (265.0f)
#define APP_GRID_RMS_CROSS_THR_V (1.0f)

static uint8_t app_pfc_hal_bound = 0U;
static uint8_t app_pfc_timing_bound = 0U;

static float app_v_g = 0.0f;
static float app_v_cap = 0.0f;
static float app_v_bus = 0.0f;
static float app_i_l = 0.0f;
static float app_v_g_rms = 0.0f;
static cal_rms_t app_v_g_rms_cal = {0};
static pfc_vbus_sta_e app_vbus_sta = PFC_VBUS_STA_BELOW_INPUT_PEAK;
static uint8_t app_main_rly_is_closed = 0U;

static void app_main_rly_on(void)
{
    app_main_rly_is_closed = 1U;
    plecs_set_output(PLECS_OUTPUT_MAIN_RLY_EN, 1.0f);
}

static void app_main_rly_off(void)
{
    app_main_rly_is_closed = 0U;
    plecs_set_output(PLECS_OUTPUT_MAIN_RLY_EN, 0.0f);
}

static void app_pwm_enable(void)
{
    bsp_pwm_enable();
}

static void app_pwm_disable(void)
{
    bsp_pwm_disable();
}

static float app_calc_duty(float v_pwm, float v_bus, float *p_offset)
{
    float vbus = v_bus;

    if ((vbus < 1.0e-6f) && (vbus > -1.0e-6f))
    {
        vbus = 1.0e-6f;
    }

    if (v_pwm > 0.0f)
    {
        *p_offset = 0.0f;
        return v_pwm / vbus;
    }

    *p_offset = 1.0f;
    return (v_pwm / vbus) + 1.0f;
}

static void app_pwm_set_bridge(float v_pwm, float v_bus)
{
    float offset = 0.0f;
    float duty = app_calc_duty(v_pwm, v_bus, &offset);

    bsp_pwm_set_duty(duty, offset, 1U, 1U, 1U, 1U);
}

static void app_update_feedback(void)
{
    app_v_g = BSP_ADC_V_G;
    app_v_cap = BSP_ADC_V_CAP;
    app_v_bus = BSP_ADC_V_BUS;
    app_i_l = BSP_ADC_I_L;

    cal_rms_master_run(&app_v_g_rms_cal);
    app_v_g_rms = app_v_g_rms_cal.output.rms;

    if (app_v_bus >= APP_VBUS_NOM_ENTER_V)
    {
        app_vbus_sta = PFC_VBUS_STA_IN_REGULATION;
    }
    else if (app_v_bus >= APP_VBUS_PRECHARGE_ENTER_V)
    {
        app_vbus_sta = PFC_VBUS_STA_AT_INPUT_PEAK;
    }
    else
    {
        app_vbus_sta = PFC_VBUS_STA_BELOW_INPUT_PEAK;
    }
}

static uint8_t app_grid_is_ok(void)
{
    return (uint8_t)((app_v_g_rms >= APP_GRID_RMS_MIN_V) &&
                     (app_v_g_rms <= APP_GRID_RMS_MAX_V));
}

static void app_feedback_isr(void)
{
    app_update_feedback();
}

REG_INTERRUPT(0, app_feedback_isr)

static void app_rms_init(void)
{
    cal_rms_init(&app_v_g_rms_cal,
                 CAL_RMS_MASTER,
                 CTRL_TS,
                 APP_GRID_RMS_CROSS_THR_V,
                 &app_v_g,
                 NULL,
                 NULL);
}

REG_INIT(0, app_rms_init)

static void app_bind_pfc_hal(void)
{
    if (app_pfc_hal_bound != 0U)
    {
        return;
    }

    pfc_hal_unlock_binding();
    pfc_hal_set_v_g_ptr(&app_v_g);
    pfc_hal_set_v_cap_ptr(&app_v_cap);
    pfc_hal_set_i_l_ptr(&app_i_l);
    pfc_hal_set_v_bus_ptr(&app_v_bus);
    pfc_hal_set_v_rms_ptr(&app_v_g_rms);
    pfc_hal_set_vbus_sta_ptr(&app_vbus_sta);
    pfc_hal_set_main_rly_is_closed_ptr(&app_main_rly_is_closed);
    pfc_hal_set_pwm_setter(app_pwm_set_bridge);
    pfc_hal_set_pwm_enable(app_pwm_enable);
    pfc_hal_set_pwm_disable(app_pwm_disable);
    pfc_hal_set_main_rly_on_func(app_main_rly_on);
    pfc_hal_set_main_rly_off_func(app_main_rly_off);

    app_pfc_hal_bound = pfc_hal_is_ready();
    if (app_pfc_hal_bound != 0U)
    {
        pfc_hal_lock_binding();
    }
}

static void app_bind_pfc_timing(void)
{
    pfc_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
    };

    if (app_pfc_timing_bound != 0U)
    {
        return;
    }

    pfc_cfg_set_timing(&timing);
    app_pfc_timing_bound = pfc_cfg_is_ready();
}

static void app_update_setpoint(void)
{
    pfc_cfg_set_vbus_ref_v(PFC_CFG_DEFAULT_VBUS_REF_V);
    pfc_cfg_set_vbus_slew_vps(PFC_CFG_DEFAULT_VBUS_SLEW_VPS);
    pfc_cfg_publish_building();
}

static void app_task(void)
{
    uint8_t run_cmd = 0U;
    pfc_run_sta_e run_sta = pfc_fsm_get_run_sta();

    app_update_feedback();
    app_bind_pfc_timing();
    app_update_setpoint();

    run_cmd = (plecs_get_input(PLECS_INPUT_RUN) > 0.5f) ? 1U : 0U;
    plecs_set_output(PLECS_OUTPUT_RUN_STATE, (float)run_sta);

    if ((run_cmd != 0U) && (app_grid_is_ok() != 0U))
    {
        if (run_sta == pfc_run_sta_idle)
        {
            app_bind_pfc_hal();
            pfc_fsm_set_cmd(pfc_fsm_cmd_start);
        }
    }
    else
    {
        app_pfc_hal_bound = 0U;
        if (run_sta != pfc_run_sta_idle)
        {
            pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
        }
    }
}

REG_TASK_MS(1, app_task)
