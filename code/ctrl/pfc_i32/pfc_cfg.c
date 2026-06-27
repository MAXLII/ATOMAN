// SPDX-License-Identifier: MIT
/**
 * @file    pfc_cfg.c
 * @brief   PFC int32 configuration module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Maintain active/building PFC int32 setpoint buffers
 *          - Convert physical PFC references and slew rates to integer code values
 *          - Store timing used by the int32 ISR
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "pfc_cfg.h"
#include <stddef.h>

static pfc_ctrl_setpoint_t setpoint_active = {0};
static pfc_ctrl_setpoint_t setpoint_building = {
    .run_allowed = 0U,
    .vbus_ref = PFC_CTRL_VBUS_REF_TO_CODE(PFC_CFG_DEFAULT_VBUS_REF_V),
    .vbus_slew_code_per_s = PFC_CTRL_VBUS_SLEW_TO_CODE_PER_S(PFC_CFG_DEFAULT_VBUS_SLEW_VPS),
};

pfc_ctrl_setpoint_mgr_t pfc_cfg_setpoint_mgr = {
    .active = {
        .p_data = &setpoint_active,
        .version = 0U,
    },
    .building = {
        .p_data = &setpoint_building,
        .version = 0U,
    },
};

static pfc_ctrl_timing_t ctrl_timing = {0};

static uint8_t pfc_cfg_timing_is_valid(const pfc_ctrl_timing_t *p_timing)
{
    return (uint8_t)((p_timing != NULL) &&
                     (p_timing->ctrl_ts > 0.0f) &&
                     (p_timing->ctrl_freq_hz > 0U));
}

void pfc_cfg_set_timing(const pfc_ctrl_timing_t *p_timing)
{
    if (pfc_cfg_timing_is_valid(p_timing) == 0U)
    {
        ctrl_timing.ctrl_ts = 0.0f;
        ctrl_timing.ctrl_freq_hz = 0U;
        return;
    }

    ctrl_timing = *p_timing;
}

const pfc_ctrl_timing_t *pfc_cfg_get_timing(void)
{
    return &ctrl_timing;
}

float pfc_cfg_get_ctrl_ts(void)
{
    return ctrl_timing.ctrl_ts;
}

uint32_t pfc_cfg_get_ctrl_freq_hz(void)
{
    return ctrl_timing.ctrl_freq_hz;
}

void pfc_cfg_set_p_building(pfc_ctrl_setpoint_t *p_data)
{
    if (p_data != NULL)
    {
        pfc_cfg_setpoint_mgr.building.p_data = p_data;
    }
}

pfc_ctrl_setpoint_t *pfc_cfg_get_p_active(void)
{
    return pfc_cfg_setpoint_mgr.active.p_data;
}

pfc_ctrl_setpoint_t *pfc_cfg_get_p_building(void)
{
    return pfc_cfg_setpoint_mgr.building.p_data;
}

void pfc_cfg_set_run_allowed(uint8_t run_allowed)
{
    if (pfc_cfg_setpoint_mgr.building.p_data != NULL)
    {
        pfc_cfg_setpoint_mgr.building.p_data->run_allowed = run_allowed;
    }
}

void pfc_cfg_set_vbus_ref_v(float vbus_ref_v)
{
    if (pfc_cfg_setpoint_mgr.building.p_data != NULL)
    {
        pfc_cfg_setpoint_mgr.building.p_data->vbus_ref =
            PFC_CTRL_VBUS_REF_TO_CODE(vbus_ref_v);
    }
}

void pfc_cfg_set_vbus_slew_vps(float vbus_slew_vps)
{
    if (pfc_cfg_setpoint_mgr.building.p_data != NULL)
    {
        pfc_cfg_setpoint_mgr.building.p_data->vbus_slew_code_per_s =
            PFC_CTRL_VBUS_SLEW_TO_CODE_PER_S(vbus_slew_vps);
    }
}

void pfc_cfg_publish_building(void)
{
    if ((pfc_cfg_setpoint_mgr.building.p_data == NULL) ||
        (pfc_cfg_setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    pfc_cfg_setpoint_mgr.building.version++;
    *pfc_cfg_setpoint_mgr.active.p_data = *pfc_cfg_setpoint_mgr.building.p_data;
    pfc_cfg_setpoint_mgr.active.version = pfc_cfg_setpoint_mgr.building.version;
}

void pfc_cfg_building_version_inc(void)
{
    pfc_cfg_setpoint_mgr.building.version++;
}

uint8_t pfc_cfg_is_ready(void)
{
    return (uint8_t)((pfc_cfg_setpoint_mgr.active.p_data != NULL) &&
                     (pfc_cfg_setpoint_mgr.building.p_data != NULL) &&
                     (pfc_cfg_timing_is_valid(&ctrl_timing) != 0U));
}

const pfc_ctrl_setpoint_mgr_t *pfc_cfg_get_mgr(void)
{
    return &pfc_cfg_setpoint_mgr;
}
