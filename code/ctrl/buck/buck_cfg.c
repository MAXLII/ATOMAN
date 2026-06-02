// SPDX-License-Identifier: MIT
/**
 * @file    buck_cfg.c
 * @brief   buck_cfg control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Maintain active/building buck setpoint buffers with versioned publishing
 *          - Provide setters for run permission, voltage/current references, and power limits
 *          - Keep configuration updates caller-owned and allocation-free for control-loop use
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
#include "buck_cfg.h"
#include <stddef.h>

static buck_ctrl_setpoint_t setpoint_active = {0};
static buck_ctrl_setpoint_t setpoint_building = {
    .run_allowed = 0U,
    .out_volt_ref = BUCK_CTRL_OUT_VOLT_LOOP_REF_TO_CODE(BUCK_CTRL_OUT_VOLT_LOOP_REF_DEFAULT_V),
    .in_volt_lmt = BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_TO_CODE(BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_DEFAULT_V),
    .pwr_lmt = BUCK_CTRL_IN_PWR_LMT_TO_CODE(BUCK_CTRL_IN_PWR_LMT_DEFAULT_W),
    .in_curr_lmt = BUCK_CTRL_IN_CURR_LMT_TO_CODE(BUCK_CTRL_IN_CURR_LMT_DEFAULT_A),
    .out_curr_lmt = BUCK_CTRL_OUT_CURR_LMT_TO_CODE(BUCK_CTRL_OUT_CURR_LMT_DEFAULT_A),
};

static buck_ctrl_timing_t ctrl_timing = {0};

buck_ctrl_setpoint_mgr_t buck_cfg_setpoint_mgr = {
    .active = {
        .p_data = &setpoint_active,
        .version = 0U,
    },
    .building = {
        .p_data = &setpoint_building,
        .version = 0U,
    },
};

static uint8_t buck_cfg_timing_is_valid(const buck_ctrl_timing_t *p_timing)
{
    return (p_timing != NULL) &&
           (p_timing->ctrl_ts > 0.0f) &&
           (p_timing->task_ts > 0.0f) &&
           (p_timing->pwm_cmp_max > 0);
}

void buck_cfg_set_timing(const buck_ctrl_timing_t *p_timing)
{
    if (buck_cfg_timing_is_valid(p_timing) == 0U)
    {
        ctrl_timing.ctrl_ts = 0.0f;
        ctrl_timing.task_ts = 0.0f;
        ctrl_timing.pwm_cmp_max = 0;
        return;
    }

    ctrl_timing = *p_timing;
}

const buck_ctrl_timing_t *buck_cfg_get_timing(void)
{
    return &ctrl_timing;
}

float buck_cfg_get_ctrl_ts(void)
{
    return ctrl_timing.ctrl_ts;
}

float buck_cfg_get_task_ts(void)
{
    return ctrl_timing.task_ts;
}

int32_t buck_cfg_get_pwm_cmp_max(void)
{
    return ctrl_timing.pwm_cmp_max;
}

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

static int32_t buck_cfg_float_to_bipolar_code(float val, float val_abs_max, int32_t code_abs_max)
{
    /* Signed code value before integer conversion. */
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

    if (code >= 0.0f)
    {
        return (int32_t)(code + 0.5f);
    }

    return (int32_t)(code - 0.5f);
}

void buck_cfg_set_p_building(buck_ctrl_setpoint_t *p_data)
{
    if (p_data != NULL)
    {
        buck_cfg_setpoint_mgr.building.p_data = p_data;
    }
}

buck_ctrl_setpoint_t *buck_cfg_get_p_active(void)
{
    return buck_cfg_setpoint_mgr.active.p_data;
}

buck_ctrl_setpoint_t *buck_cfg_get_p_building(void)
{
    return buck_cfg_setpoint_mgr.building.p_data;
}

void buck_cfg_set_run_allowed(uint8_t run_allowed)
{
    if (buck_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    buck_cfg_setpoint_mgr.building.p_data->run_allowed = run_allowed;
}

void buck_cfg_set_pwr_lmt(float pwr_lmt)
{
    if (buck_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    buck_cfg_setpoint_mgr.building.p_data->pwr_lmt =
        buck_cfg_float_to_code(pwr_lmt, BUCK_CTRL_IN_PWR_LMT_MAX_W, BUCK_CTRL_IN_PWR_LMT_CODE_MAX);
}

void buck_cfg_set_out_volt_ref(float out_volt_ref)
{
    if (buck_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    buck_cfg_setpoint_mgr.building.p_data->out_volt_ref =
        buck_cfg_float_to_code(out_volt_ref,
                               BUCK_CTRL_OUT_VOLT_LOOP_REF_MAX_V,
                               BUCK_CTRL_OUT_VOLT_LOOP_REF_CODE_MAX);
}

void buck_cfg_set_in_volt_lmt(float in_volt_lmt)
{
    if (buck_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    buck_cfg_setpoint_mgr.building.p_data->in_volt_lmt =
        buck_cfg_float_to_code(in_volt_lmt,
                               BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_MAX_V,
                               BUCK_CTRL_IN_VOLT_LMT_LOOP_REF_CODE_MAX);
}

void buck_cfg_set_in_curr_lmt(float in_curr_lmt)
{
    if (buck_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    buck_cfg_setpoint_mgr.building.p_data->in_curr_lmt =
        buck_cfg_float_to_bipolar_code(in_curr_lmt,
                                       BUCK_CTRL_IN_CURR_LMT_MAX_A,
                                       BUCK_CTRL_IN_CURR_LMT_CODE_MAX);
}

void buck_cfg_set_out_curr_lmt(float out_curr_lmt)
{
    if (buck_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    buck_cfg_setpoint_mgr.building.p_data->out_curr_lmt =
        buck_cfg_float_to_bipolar_code(out_curr_lmt,
                                       BUCK_CTRL_OUT_CURR_LMT_MAX_A,
                                       BUCK_CTRL_OUT_CURR_LMT_CODE_MAX);
}

void buck_cfg_publish_building(void)
{
    if ((buck_cfg_setpoint_mgr.building.p_data == NULL) ||
        (buck_cfg_setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    buck_cfg_setpoint_mgr.building.version++;
}

void buck_cfg_building_version_inc(void)
{
    buck_cfg_setpoint_mgr.building.version++;
}

uint8_t buck_cfg_is_ready(void)
{
    return (buck_cfg_setpoint_mgr.active.p_data != NULL) &&
           (buck_cfg_setpoint_mgr.building.p_data != NULL) &&
           (buck_cfg_timing_is_valid(&ctrl_timing) != 0U);
}

const buck_ctrl_setpoint_mgr_t *buck_cfg_get_mgr(void)
{
    return &buck_cfg_setpoint_mgr;
}
