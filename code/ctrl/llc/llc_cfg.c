// SPDX-License-Identifier: MIT
/**
 * @file    llc_cfg.c
 * @brief   LLC control configuration module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Maintain active/building LLC setpoint buffers with versioned publishing
 *          - Provide setters for run permission and output-voltage reference
 *          - Keep configuration updates caller-owned and allocation-free for control-loop use
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
#include "llc_cfg.h"
#include <stddef.h>

static llc_ctrl_setpoint_t setpoint_active = {0};
static llc_ctrl_setpoint_t setpoint_building = {
    .run_allowed = 0U,
    .v_out_ref_v = LLC_CTRL_VOUT_REF_DEFAULT_V,
};
static llc_ctrl_timing_t ctrl_timing = {0};

llc_ctrl_setpoint_mgr_t llc_cfg_setpoint_mgr = {
    .active = {
        .p_data = &setpoint_active,
        .version = 0U,
    },
    .building = {
        .p_data = &setpoint_building,
        .version = 0U,
    },
};

static uint8_t llc_cfg_timing_is_valid(const llc_ctrl_timing_t *p_timing)
{
    return (uint8_t)((p_timing != NULL) &&
                     (p_timing->ctrl_ts > 0.0f) &&
                     (p_timing->task_ts > 0.0f));
}

void llc_cfg_set_timing(const llc_ctrl_timing_t *p_timing)
{
    if (llc_cfg_timing_is_valid(p_timing) == 0U)
    {
        ctrl_timing.ctrl_ts = 0.0f;
        ctrl_timing.task_ts = 0.0f;
        ctrl_timing.startup_delay_ticks = 0U;
        return;
    }

    ctrl_timing = *p_timing;
}

const llc_ctrl_timing_t *llc_cfg_get_timing(void)
{
    return &ctrl_timing;
}

float llc_cfg_get_ctrl_ts(void)
{
    return ctrl_timing.ctrl_ts;
}

float llc_cfg_get_task_ts(void)
{
    return ctrl_timing.task_ts;
}

uint32_t llc_cfg_get_startup_delay_ticks(void)
{
    if (ctrl_timing.startup_delay_ticks == 0U)
    {
        return (uint32_t)((0.2f / ctrl_timing.ctrl_ts) + 0.5f);
    }

    return ctrl_timing.startup_delay_ticks;
}

void llc_cfg_set_p_building(llc_ctrl_setpoint_t *p_data)
{
    if (p_data != NULL)
    {
        llc_cfg_setpoint_mgr.building.p_data = p_data;
    }
}

llc_ctrl_setpoint_t *llc_cfg_get_p_active(void)
{
    return llc_cfg_setpoint_mgr.active.p_data;
}

llc_ctrl_setpoint_t *llc_cfg_get_p_building(void)
{
    return llc_cfg_setpoint_mgr.building.p_data;
}

void llc_cfg_set_run_allowed(uint8_t run_allowed)
{
    if (llc_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    llc_cfg_setpoint_mgr.building.p_data->run_allowed = run_allowed;
}

void llc_cfg_set_v_out_ref(float v_out_ref_v)
{
    if (llc_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }

    llc_cfg_setpoint_mgr.building.p_data->v_out_ref_v = v_out_ref_v;
}

void llc_cfg_publish_building(void)
{
    if ((llc_cfg_setpoint_mgr.building.p_data == NULL) ||
        (llc_cfg_setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    llc_cfg_setpoint_mgr.building.version++;
}

void llc_cfg_building_version_inc(void)
{
    llc_cfg_setpoint_mgr.building.version++;
}

uint8_t llc_cfg_is_ready(void)
{
    return (uint8_t)((llc_cfg_setpoint_mgr.active.p_data != NULL) &&
                     (llc_cfg_setpoint_mgr.building.p_data != NULL) &&
                     (llc_cfg_timing_is_valid(&ctrl_timing) != 0U));
}

const llc_ctrl_setpoint_mgr_t *llc_cfg_get_mgr(void)
{
    return &llc_cfg_setpoint_mgr;
}
