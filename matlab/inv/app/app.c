// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   MATLAB inverter application module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Bind simulated ADC and PWM signals to the inverter HAL
 *          - Translate the MATLAB run input into inverter FSM start and stop commands
 *          - Publish inverter run state and output relay state back to the MATLAB output vector
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
#include "inv_cfg.h"
#include "inv_fsm.h"
#include "inv_hal.h"
#include "sim_sfunc.h"
#include "section.h"
#include "timing.h"
#include "bsp_pwm.h"

#define APP_INV_START_VBUS_MIN_V (380.0f)

static uint8_t app_inv_hal_bound = 0U;
static uint8_t app_inv_timing_bound = 0U;

static float app_v_cap = 0.0f;
static float app_v_bus = 0.0f;
static float app_i_l = 0.0f;

static void app_inv_rly_on(void)
{
    sim_set_output(SIM_OUTPUT_INV_RLY, 1.0f);
}

static void app_inv_rly_off(void)
{
    sim_set_output(SIM_OUTPUT_INV_RLY, 0.0f);
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
    app_v_cap = BSP_ADC_V_CAP;
    app_v_bus = BSP_ADC_V_BUS;
    app_i_l = BSP_ADC_I_L;
}

static void app_feedback_isr(void)
{
    app_update_feedback();
}

REG_INTERRUPT(0, app_feedback_isr)

static void app_bind_inv_hal(void)
{
    if (app_inv_hal_bound != 0U)
    {
        return;
    }

    inv_hal_unlock_binding();
    inv_hal_set_v_cap_ptr(&app_v_cap);
    inv_hal_set_i_l_ptr(&app_i_l);
    inv_hal_set_v_bus_ptr(&app_v_bus);
    inv_hal_set_pwm_setter(app_pwm_set_bridge);
    inv_hal_set_pwm_enable(app_pwm_enable);
    inv_hal_set_pwm_disable(app_pwm_disable);
    inv_hal_set_inv_rly_on_func(app_inv_rly_on);
    inv_hal_set_inv_rly_off_func(app_inv_rly_off);

    app_inv_hal_bound = inv_hal_is_ready();
    if (app_inv_hal_bound != 0U)
    {
        inv_hal_lock_binding();
    }
}

static void app_bind_inv_timing(void)
{
    inv_ctrl_timing_t timing = {CTRL_TS, CTRL_FREQ};

    if (app_inv_timing_bound != 0U)
    {
        return;
    }

    inv_cfg_set_timing(&timing);
    app_inv_timing_bound = inv_cfg_is_ready();
}

static void app_update_setpoint(void)
{
    inv_cfg_set_freq_hz(INV_CFG_DEFAULT_FREQ_HZ);
    inv_cfg_set_freq_slew_hzps(INV_CFG_DEFAULT_FREQ_SLEW_HZPS);
    inv_cfg_set_rms_ref_v(INV_CFG_DEFAULT_RMS_REF_V);
    inv_cfg_set_rms_slew_vps(INV_CFG_DEFAULT_RMS_SLEW_VPS);
    inv_cfg_publish_building();
}

static void app_task(void)
{
    uint8_t run_cmd = 0U;
    inv_run_sta_e run_sta = inv_fsm_get_run_sta();

    app_update_feedback();
    app_bind_inv_timing();
    app_update_setpoint();

    run_cmd = (sim_get_input(SIM_INPUT_RUN) > 0.5f) ? 1U : 0U;
    sim_set_output(SIM_OUTPUT_RUN_STATE, (float)run_sta);

    if ((run_cmd != 0U) && (app_v_bus >= APP_INV_START_VBUS_MIN_V))
    {
        if (run_sta == inv_run_sta_idle)
        {
            app_bind_inv_hal();
            inv_fsm_set_cmd(inv_fsm_cmd_start);
        }
    }
    else
    {
        app_inv_hal_bound = 0U;
        if (run_sta != inv_run_sta_idle)
        {
            inv_fsm_set_cmd(inv_fsm_cmd_stop);
        }
    }
}

REG_TASK_MS(1, app_task)
