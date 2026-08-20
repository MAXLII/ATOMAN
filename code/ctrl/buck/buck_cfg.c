// SPDX-License-Identifier: MIT
/**
 * @file    buck_cfg.c
 * @brief   Buck configuration module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Store the application-visible Buck parameters and run request
 *          - Convert physical parameters into controller code domains
 *          - Provide read access to the complete building configuration
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Configuration publication and lifecycle decisions belong to the FSM
 *          - Hardware access is abstracted through the Buck HAL
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
#include "buck_cfg.h"
#include "buck_fsm.h"

static buck_cfg_t buck_cfg = {
    .out_volt_ref = BUCK_CTRL_OUT_VOLT_LOOP_REF_DEFAULT_V,
    .in_volt_lmt = BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_DEFAULT_V,
    .pwr_lmt = BUCK_CTRL_IN_PWR_LMT_DEFAULT_W,
    .in_curr_lmt = BUCK_CTRL_IN_CURR_LMT_DEFAULT_A,
    .out_curr_lmt = BUCK_CTRL_OUT_CURR_LMT_DEFAULT_A,
    .run_request = 0U,
};

static buck_ctrl_setpoint_t setpoint_building = {
    .run_allowed = 0U,
    .out_volt_ref = BUCK_CTRL_OUT_VOLT_LOOP_REF_TO_CODE(BUCK_CTRL_OUT_VOLT_LOOP_REF_DEFAULT_V),
    .in_volt_lmt = BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_TO_CODE(BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_DEFAULT_V),
    .pwr_lmt = BUCK_CTRL_IN_PWR_LMT_TO_CODE(BUCK_CTRL_IN_PWR_LMT_DEFAULT_W),
    .in_curr_lmt = BUCK_CTRL_IN_CURR_LMT_TO_CODE(BUCK_CTRL_IN_CURR_LMT_DEFAULT_A),
    .out_curr_lmt = BUCK_CTRL_OUT_CURR_LMT_TO_CODE(BUCK_CTRL_OUT_CURR_LMT_DEFAULT_A),
};

static int32_t buck_cfg_float_to_code(float val, float val_max, int32_t code_max)
{
    float code = 0.0f;

    if ((val <= 0.0f) || (val_max <= 0.0f) || (code_max <= 0))
    {
        return 0;
    }
    if (val >= val_max)
    {
        return code_max;
    }

    code = (val / val_max) * (float)code_max;
    return (int32_t)(code + 0.5f);
}

static int32_t buck_cfg_float_to_bipolar_code(float val,
                                              float val_abs_max,
                                              int32_t code_abs_max)
{
    float code = 0.0f;

    if ((val_abs_max <= 0.0f) || (code_abs_max <= 0))
    {
        return 0;
    }
    if (val >= val_abs_max)
    {
        return code_abs_max;
    }
    if (val <= -val_abs_max)
    {
        return -code_abs_max;
    }

    code = (val / val_abs_max) * (float)code_abs_max;
    return (code >= 0.0f) ? (int32_t)(code + 0.5f) : (int32_t)(code - 0.5f);
}

uint8_t buck_cfg_set_out_volt_ref(float out_volt_ref)
{
    int32_t code = 0;

    if (out_volt_ref != out_volt_ref)
    {
        return 0U;
    }

    code = buck_cfg_float_to_code(out_volt_ref,
                                  BUCK_CTRL_OUT_VOLT_LOOP_REF_MAX_V,
                                  BUCK_CTRL_OUT_VOLT_LOOP_REF_CODE_MAX);
    buck_cfg.out_volt_ref = out_volt_ref;
    setpoint_building.out_volt_ref = code;
    return 1U;
}

uint8_t buck_cfg_set_in_volt_lmt(float in_volt_lmt)
{
    int32_t code = 0;

    if (in_volt_lmt != in_volt_lmt)
    {
        return 0U;
    }

    code = buck_cfg_float_to_code(in_volt_lmt,
                                  BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_MAX_V,
                                  BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_CODE_MAX);
    buck_cfg.in_volt_lmt = in_volt_lmt;
    setpoint_building.in_volt_lmt = code;
    return 1U;
}

uint8_t buck_cfg_set_pwr_lmt(float pwr_lmt)
{
    int32_t code = 0;

    if (pwr_lmt != pwr_lmt)
    {
        return 0U;
    }

    code = buck_cfg_float_to_code(pwr_lmt,
                                  BUCK_CTRL_IN_PWR_LMT_MAX_W,
                                  BUCK_CTRL_IN_PWR_LMT_CODE_MAX);
    buck_cfg.pwr_lmt = pwr_lmt;
    setpoint_building.pwr_lmt = code;
    return 1U;
}

uint8_t buck_cfg_set_in_curr_lmt(float in_curr_lmt)
{
    int32_t code = 0;

    if (in_curr_lmt != in_curr_lmt)
    {
        return 0U;
    }

    code = buck_cfg_float_to_bipolar_code(in_curr_lmt,
                                          BUCK_CTRL_IN_CURR_LMT_MAX_A,
                                          BUCK_CTRL_IN_CURR_LMT_CODE_MAX);
    buck_cfg.in_curr_lmt = in_curr_lmt;
    setpoint_building.in_curr_lmt = code;
    return 1U;
}

uint8_t buck_cfg_set_out_curr_lmt(float out_curr_lmt)
{
    int32_t code = 0;

    if (out_curr_lmt != out_curr_lmt)
    {
        return 0U;
    }

    code = buck_cfg_float_to_bipolar_code(out_curr_lmt,
                                          BUCK_CTRL_OUT_CURR_LMT_MAX_A,
                                          BUCK_CTRL_OUT_CURR_LMT_CODE_MAX);
    buck_cfg.out_curr_lmt = out_curr_lmt;
    setpoint_building.out_curr_lmt = code;
    return 1U;
}

uint8_t buck_cfg_set_run_request(uint8_t run_request)
{
    buck_cfg.run_request = (run_request != 0U) ? 1U : 0U;
    return 1U;
}

buck_run_sta_e buck_cfg_get_run_state(void)
{
    return buck_fsm_get_run_sta();
}

uint8_t buck_cfg_get_run_request(void)
{
    return buck_cfg.run_request;
}

const buck_ctrl_setpoint_t *buck_cfg_get_p_building(void)
{
    return &setpoint_building;
}
