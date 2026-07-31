// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   PLECS inverter int32 application adapter.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Convert PLECS physical feedback signals into inverter integer ADC domains
 *          - Bind integer feedback and PWM callbacks to the inverter HAL
 *          - Convert the integer PWM numerator into bridge duty commands
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Floating-point arithmetic is confined to the PLECS boundary
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-08-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "app.h"

#include <limits.h>

#include "bsp_adc.h"
#include "bsp_pwm.h"
#include "inv_cfg.h"
#include "inv_ctrl.h"
#include "inv_fsm.h"
#include "inv_hal.h"
#include "plecs.h"
#include "section.h"
#include "timing.h"

#define APP_INV_START_VBUS_MIN_V (380.0f)
#define APP_PWM_AC_TO_BUS_K_NUM \
    ((float)INV_CTRL_BUS_VOLT_CODE_MAX * INV_CTRL_AC_VOLT_MAX_V)
#define APP_PWM_AC_TO_BUS_K_DEN \
    ((float)INV_CTRL_AC_VOLT_CODE_MAX * INV_CTRL_BUS_VOLT_MAX_V)

static uint8_t hal_bound = 0U;    /**< 1 after all inverter HAL callbacks are bound. */
static uint8_t timing_bound = 0U; /**< 1 after integer timing is configured. */

static float v_cap = 0.0f; /**< PLECS capacitor-voltage feedback in volts. */
static float v_bus = 0.0f; /**< PLECS bus-voltage feedback in volts. */
static float i_l = 0.0f;   /**< PLECS inductor-current feedback in amperes. */
static int32_t v_cap_code = 0; /**< Signed capacitor-voltage ADC code. */
static int32_t v_bus_code = 0; /**< Unsigned bus-voltage ADC code. */
static int32_t i_l_code = 0;   /**< Signed inductor-current ADC code. */

static int32_t float_to_i32(float value)
{
    if (value >= (float)INT32_MAX)
    {
        return INT32_MAX;
    }
    if (value <= (float)INT32_MIN)
    {
        return INT32_MIN;
    }
    return (value >= 0.0f) ? (int32_t)(value + 0.5f) : (int32_t)(value - 0.5f);
}

static int32_t limit_i32(int32_t value, int32_t upper, int32_t lower)
{
    if (value > upper)
    {
        return upper;
    }
    if (value < lower)
    {
        return lower;
    }
    return value;
}

static int32_t ac_voltage_to_code(float voltage)
{
    int32_t code = float_to_i32((voltage / INV_CTRL_AC_VOLT_MAX_V) *
                                (float)INV_CTRL_AC_VOLT_CODE_MAX); /**< Converted AC code. */
    return limit_i32(code, INV_CTRL_AC_VOLT_CODE_MAX, INV_CTRL_AC_VOLT_CODE_MIN);
}

static int32_t bus_voltage_to_code(float voltage)
{
    int32_t code = float_to_i32((voltage / INV_CTRL_BUS_VOLT_MAX_V) *
                                (float)INV_CTRL_BUS_VOLT_CODE_MAX); /**< Converted bus code. */
    return limit_i32(code, INV_CTRL_BUS_VOLT_CODE_MAX, INV_CTRL_BUS_VOLT_CODE_MIN);
}

static int32_t current_to_code(float current)
{
    int32_t code = float_to_i32((current / INV_CTRL_IND_CURR_MAX_A) *
                                (float)INV_CTRL_IND_CURR_CODE_MAX); /**< Converted current code. */
    return limit_i32(code, INV_CTRL_IND_CURR_CODE_MAX, INV_CTRL_IND_CURR_CODE_MIN);
}

static void relay_on(void)
{
    plecs_set_output(PLECS_OUTPUT_INV_RLY, 1.0f);
}

static void relay_off(void)
{
    plecs_set_output(PLECS_OUTPUT_INV_RLY, 0.0f);
}

static void pwm_enable(void)
{
    bsp_pwm_enable();
}

static void pwm_disable(void)
{
    bsp_pwm_disable();
}

static float calculate_duty(int32_t v_pwm_command, int32_t bus_code)
{
    float denominator = 0.0f; /**< Bus-code and reload normalization denominator. */
    float duty = 0.0f;        /**< Signed bridge voltage ratio. */

    if (bus_code <= 0)
    {
        return 0.0f;
    }

    denominator = (float)bus_code * APP_PWM_AC_TO_BUS_K_DEN * (float)INV_CTRL_PWM_RELOAD;
    duty = ((float)v_pwm_command * APP_PWM_AC_TO_BUS_K_NUM) / denominator;
    UP_DN_LMT(duty, 1.0f, -1.0f);
    return duty;
}

static void pwm_set_bridge(int32_t v_pwm_command, int32_t bus_code)
{
    float duty = calculate_duty(v_pwm_command, bus_code); /**< Signed bridge voltage ratio. */
    float duty_fast = 0.0f; /**< Fast-leg duty command. */
    float duty_slow = 0.0f; /**< Slow-leg polarity command. */

    if (duty >= 0.0f)
    {
        duty_fast = duty;
        duty_slow = 0.0f;
    }
    else
    {
        duty_fast = duty + 1.0f;
        duty_slow = 1.0f;
    }
    bsp_pwm_set_duty(duty_fast, duty_slow, 1U, 1U, 1U, 1U);
}

static void update_feedback(void)
{
    v_cap = BSP_ADC_V_CAP;
    v_bus = BSP_ADC_V_BUS;
    i_l = BSP_ADC_I_L;
    v_cap_code = ac_voltage_to_code(v_cap);
    v_bus_code = bus_voltage_to_code(v_bus);
    i_l_code = current_to_code(i_l);
}

static void feedback_isr(void)
{
    update_feedback();
}

REG_INTERRUPT(0, feedback_isr)

static void bind_hal(void)
{
    if (hal_bound != 0U)
    {
        return;
    }
    inv_hal_unlock_binding();
    inv_hal_set_v_cap_ptr(&v_cap_code);
    inv_hal_set_i_l_ptr(&i_l_code);
    inv_hal_set_v_bus_ptr(&v_bus_code);
    inv_hal_set_pwm_setter(pwm_set_bridge);
    inv_hal_set_pwm_enable(pwm_enable);
    inv_hal_set_pwm_disable(pwm_disable);
    inv_hal_set_inv_rly_on_func(relay_on);
    inv_hal_set_inv_rly_off_func(relay_off);
    hal_bound = inv_hal_is_ready();
    if (hal_bound != 0U)
    {
        inv_hal_lock_binding();
    }
}

static void bind_timing(void)
{
    inv_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
        .ctrl_freq = CTRL_FREQ,
    }; /**< PLECS control timing. */

    if (timing_bound != 0U)
    {
        return;
    }
    inv_cfg_set_timing(&timing);
    timing_bound = inv_cfg_is_ready();
}

static void update_setpoint(void)
{
    inv_cfg_set_freq_hz(INV_CFG_DEFAULT_FREQ_HZ);
    inv_cfg_set_freq_slew_hzps(INV_CFG_DEFAULT_FREQ_SLEW_HZPS);
    inv_cfg_set_rms_ref_v(INV_CFG_DEFAULT_RMS_REF_V);
    inv_cfg_set_rms_slew_vps(INV_CFG_DEFAULT_RMS_SLEW_VPS);
    inv_cfg_publish_building();
}

static void app_task(void)
{
    uint8_t run_command = 0U;                  /**< PLECS start request. */
    inv_run_sta_e run_state = inv_fsm_get_run_sta(); /**< Current inverter FSM state. */

    update_feedback();
    bind_timing();
    update_setpoint();
    run_command = (plecs_get_input(PLECS_INPUT_RUN) > 0.5f) ? 1U : 0U;
    plecs_set_output(PLECS_OUTPUT_RUN_STATE, (float)run_state);

    if ((run_command != 0U) &&
        (v_bus >= APP_INV_START_VBUS_MIN_V))
    {
        if (run_state == inv_run_sta_idle)
        {
            bind_hal();
            inv_fsm_set_cmd(inv_fsm_cmd_start);
        }
    }
    else
    {
        hal_bound = 0U;
        if (run_state != inv_run_sta_idle)
        {
            inv_fsm_set_cmd(inv_fsm_cmd_stop);
        }
    }
}

REG_TASK_MS(1, app_task)
