// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   PLECS inverter application module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Bind simulated ADC and PWM signals to the inverter HAL
 *          - Register inverter commands, setpoints, state, and feedback in Shell
 *          - Translate the Shell RUN request into inverter FSM start and stop commands
 *          - Publish inverter run state and output relay state back to the PLECS output vector
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
#include "frame_tcp_server.h"
#include "inv_cfg.h"
#include "inv_fsm.h"
#include "inv_hal.h"
#include "plecs.h"
#include "section.h"
#include "shell.h"
#include "timing.h"

#define APP_INV_START_VBUS_MIN_V (380.0f)                    /* Default DC-bus start threshold in volts. */
#define APP_INV_FREQ_MAX_HZ (400.0f)                         /* Maximum Shell frequency reference in hertz. */
#define APP_INV_FREQ_SLEW_MAX_HZPS (1000.0f)                /* Maximum frequency slew in hertz per second. */
#define APP_INV_RMS_REF_MAX_V (1000.0f)                     /* Maximum output RMS reference in volts. */
#define APP_INV_RMS_SLEW_MAX_VPS (10000.0f)                 /* Maximum RMS slew in volts per second. */
#define APP_INV_FEEDBACK_MAX (2000.0f)                      /* Maximum displayed analog feedback value. */
#define APP_INV_FEEDBACK_MIN (-APP_INV_FEEDBACK_MAX)        /* Minimum displayed analog feedback value. */
#define APP_INV_RUN_STATE_MAX ((uint32_t)inv_run_sta_run)   /* Maximum public inverter run state. */
#define APP_INV_RUN_STATE_MIN ((uint32_t)inv_run_sta_init)  /* Minimum public inverter run state. */

/* Tracks whether inverter HAL callbacks and feedback pointers are bound. */
static uint8_t app_inv_hal_bound = 0U;
/* Tracks whether inverter control timing has been configured. */
static uint8_t app_inv_timing_bound = 0U;
/* Desired inverter run state toggled through the Shell RUN command. */
static uint8_t app_run_request = 0U;
/* Public inverter run-state mirror for Shell and FRAME. */
static uint32_t app_run_state = (uint32_t)inv_run_sta_init;

/* Output-frequency reference configured through Shell, in hertz. */
static float app_freq_hz = INV_CFG_DEFAULT_FREQ_HZ;
/* Output-frequency slew configured through Shell, in hertz per second. */
static float app_freq_slew_hzps = INV_CFG_DEFAULT_FREQ_SLEW_HZPS;
/* Output RMS voltage reference configured through Shell, in volts. */
static float app_rms_ref_v = INV_CFG_DEFAULT_RMS_REF_V;
/* Output RMS voltage slew configured through Shell, in volts per second. */
static float app_rms_slew_vps = INV_CFG_DEFAULT_RMS_SLEW_VPS;
/* Minimum DC-bus voltage that permits an inverter start request. */
static float app_start_vbus_min_v = APP_INV_START_VBUS_MIN_V;

/* Capacitor-voltage feedback mirrored from the PLECS model. */
static float app_v_cap = 0.0f;
/* DC-bus-voltage feedback mirrored from the PLECS model. */
static float app_v_bus = 0.0f;
/* Inductor-current feedback mirrored from the PLECS model. */
static float app_i_l = 0.0f;
/* FRAME TCP server port exposed as a read-only Shell parameter. */
static uint32_t app_frame_tcp_port = FRAME_TCP_SERVER_PORT;

/**
 * @brief Toggle the desired inverter run state from the Shell RUN command.
 * @param[in] my_printf Shell transport interface associated with the command request.
 */
static void app_run_cmd(DEC_MY_PRINTF)
{
    app_run_request = (app_run_request == 0U) ? 1U : 0U;
    PLECS_LOG("Shell RUN toggled inverter request to %u\n", (unsigned)app_run_request);

    if ((my_printf != NULL) &&
        (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("RUN request=%u\r\n", (unsigned)app_run_request);
    }
}

REG_SHELL_CMD(RUN, app_run_cmd)
REG_SHELL_VAR(FREQ_HZ, app_freq_hz, SHELL_FP32, APP_INV_FREQ_MAX_HZ, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(FREQ_SLEW_HZPS,
              app_freq_slew_hzps,
              SHELL_FP32,
              APP_INV_FREQ_SLEW_MAX_HZPS,
              0.0f,
              NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(RMS_REF_V, app_rms_ref_v, SHELL_FP32, APP_INV_RMS_REF_MAX_V, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(RMS_SLEW_VPS,
              app_rms_slew_vps,
              SHELL_FP32,
              APP_INV_RMS_SLEW_MAX_VPS,
              0.0f,
              NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(START_VBUS_MIN_V,
              app_start_vbus_min_v,
              SHELL_FP32,
              APP_INV_FEEDBACK_MAX,
              0.0f,
              NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(RUN_REQUEST, app_run_request, SHELL_UINT8, 1U, 0U, NULL, SHELL_STA_READ_ONLY)
REG_SHELL_VAR(RUN_STATE,
              app_run_state,
              SHELL_UINT32,
              APP_INV_RUN_STATE_MAX,
              APP_INV_RUN_STATE_MIN,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(V_CAP,
              app_v_cap,
              SHELL_FP32,
              APP_INV_FEEDBACK_MAX,
              APP_INV_FEEDBACK_MIN,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(V_BUS,
              app_v_bus,
              SHELL_FP32,
              APP_INV_FEEDBACK_MAX,
              APP_INV_FEEDBACK_MIN,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(I_L,
              app_i_l,
              SHELL_FP32,
              APP_INV_FEEDBACK_MAX,
              APP_INV_FEEDBACK_MIN,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(FRAME_TCP_PORT,
              app_frame_tcp_port,
              SHELL_UINT32,
              FRAME_TCP_SERVER_PORT,
              FRAME_TCP_SERVER_PORT,
              NULL,
              SHELL_STA_READ_ONLY)

static void app_inv_rly_on(void)
{
    plecs_set_output(PLECS_OUTPUT_INV_RLY, 1.0f);
}

static void app_inv_rly_off(void)
{
    plecs_set_output(PLECS_OUTPUT_INV_RLY, 0.0f);
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
    inv_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
        .ctrl_freq = CTRL_FREQ,
    };

    if (app_inv_timing_bound != 0U)
    {
        return;
    }

    inv_cfg_set_timing(&timing);
    app_inv_timing_bound = inv_cfg_is_ready();
}

static void app_update_setpoint(void)
{
    inv_cfg_set_freq_hz(app_freq_hz);
    inv_cfg_set_freq_slew_hzps(app_freq_slew_hzps);
    inv_cfg_set_rms_ref_v(app_rms_ref_v);
    inv_cfg_set_rms_slew_vps(app_rms_slew_vps);
}

static void app_task(void)
{
    inv_run_sta_e run_sta = inv_fsm_get_run_sta();

    app_update_feedback();
    app_bind_inv_timing();
    app_update_setpoint();

    app_run_state = (uint32_t)run_sta;
    plecs_set_output(PLECS_OUTPUT_RUN_STATE, (float)run_sta);

    if ((app_run_request != 0U) &&
        (app_v_bus >= app_start_vbus_min_v))
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
