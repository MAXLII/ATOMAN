// SPDX-License-Identifier: MIT
/**
 * @file    inv_cfg.c
 * @brief   Inverter int32 configuration module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Maintain active and building integer inverter setpoint buffers
 *          - Convert physical frequency and RMS commands into fixed-point code domains
 *          - Precompute the phase-step gain used by the integer ISR
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Floating-point conversion is outside the control ISR
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
#include "inv_cfg.h"
#include "inv_cfg_fsm.h"

#include <limits.h>
#include <stddef.h>

#include "section.h"

static inv_ctrl_setpoint_t setpoint_active = {0}; /**< Setpoint snapshot consumed by control code. */
static inv_ctrl_setpoint_t setpoint_building = {
    .run_allowed = INV_CFG_DEFAULT_RUN_ALLOWED,
    .freq_hz_q16 = INV_CTRL_FREQ_TO_Q16(INV_CFG_DEFAULT_FREQ_HZ),
    .freq_slew_hzps_q16 = INV_CTRL_FREQ_TO_Q16(INV_CFG_DEFAULT_FREQ_SLEW_HZPS),
    .v_ref_peak_code_q16 = INV_CTRL_RMS_TO_PEAK_CODE_Q16(INV_CFG_DEFAULT_RMS_REF_V),
    .v_ref_slew_code_q16_per_s =
        INV_CTRL_RMS_SLEW_TO_PEAK_CODE_Q16_PER_S(INV_CFG_DEFAULT_RMS_SLEW_VPS),
}; /**< Setpoint buffer modified by non-real-time code. */

static inv_ctrl_setpoint_mgr_t setpoint_mgr = {
    .active = {
        .p_data = &setpoint_active,
        .version = 0U,
    },
    .building = {
        .p_data = &setpoint_building,
        .version = 0U,
    },
}; /**< Versioned setpoint manager. */

static inv_ctrl_timing_t ctrl_timing = {0}; /**< Configured control timing. */
static uint32_t ctrl_freq_hz = 0U;          /**< Rounded control frequency used by integer ramps. */
static uint32_t phase_step_gain_q24 = 0U;  /**< Q24 gain from Q16 Hz to Q32 phase step. */

static uint32_t float_to_u32(float value)
{
    if (value <= 0.0f)
    {
        return 0U;
    }
    if (value >= (float)UINT32_MAX)
    {
        return UINT32_MAX;
    }
    return (uint32_t)(value + 0.5f);
}

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

static uint8_t timing_is_valid(const inv_ctrl_timing_t *p_timing)
{
    return (uint8_t)((p_timing != NULL) &&
                     (p_timing->ctrl_ts > 0.0f) &&
                     (p_timing->ctrl_freq > 0.0f));
}

void inv_cfg_set_timing(const inv_ctrl_timing_t *p_timing)
{
    float phase_gain = 0.0f; /**< Q24 phase-step conversion gain before rounding. */

    if (timing_is_valid(p_timing) == 0U)
    {
        ctrl_timing.ctrl_ts = 0.0f;
        ctrl_timing.ctrl_freq = 0.0f;
        ctrl_freq_hz = 0U;
        phase_step_gain_q24 = 0U;
        return;
    }

    ctrl_timing = *p_timing;
    ctrl_freq_hz = float_to_u32(p_timing->ctrl_freq);
    phase_gain = (65536.0f / p_timing->ctrl_freq) *
                 (float)(1UL << INV_CTRL_PHASE_GAIN_Q_SHIFT);
    phase_step_gain_q24 = float_to_u32(phase_gain);
}

const inv_ctrl_timing_t *inv_cfg_get_timing(void)
{
    return &ctrl_timing;
}

float inv_cfg_get_ctrl_ts(void)
{
    return ctrl_timing.ctrl_ts;
}

uint32_t inv_cfg_get_ctrl_freq_hz(void)
{
    return ctrl_freq_hz;
}

uint32_t inv_cfg_get_phase_step_gain_q24(void)
{
    return phase_step_gain_q24;
}

void inv_cfg_set_p_building(inv_ctrl_setpoint_t *p_data)
{
    if (p_data != NULL)
    {
        setpoint_mgr.building.p_data = p_data;
    }
}

inv_ctrl_setpoint_t *inv_cfg_get_p_active(void)
{
    return setpoint_mgr.active.p_data;
}

inv_ctrl_setpoint_t *inv_cfg_get_p_building(void)
{
    return setpoint_mgr.building.p_data;
}

void inv_cfg_set_run_allowed(uint8_t run_allowed)
{
    if (setpoint_mgr.building.p_data != NULL)
    {
        setpoint_mgr.building.p_data->run_allowed = (run_allowed != 0U) ? 1U : 0U;
    }
}

void inv_cfg_set_freq_hz(float freq_hz)
{
    if (setpoint_mgr.building.p_data != NULL)
    {
        setpoint_mgr.building.p_data->freq_hz_q16 = float_to_u32(freq_hz * (float)INV_CTRL_FREQ_Q_ONE);
    }
}

void inv_cfg_set_freq_slew_hzps(float freq_slew_hzps)
{
    if (setpoint_mgr.building.p_data != NULL)
    {
        setpoint_mgr.building.p_data->freq_slew_hzps_q16 =
            float_to_u32(freq_slew_hzps * (float)INV_CTRL_FREQ_Q_ONE);
    }
}

void inv_cfg_set_rms_ref_v(float rms_ref_v)
{
    float peak_code_q16 = 0.0f; /**< Peak reference converted to Q16 AC-voltage codes. */

    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }
    peak_code_q16 = rms_ref_v * INV_CTRL_SQRT2 * INV_CTRL_AC_VOLT_CODE_PER_V *
                    (float)INV_CTRL_REF_Q_ONE;
    setpoint_mgr.building.p_data->v_ref_peak_code_q16 = float_to_i32(peak_code_q16);
}

void inv_cfg_set_rms_slew_vps(float rms_slew_vps)
{
    float slew_code_q16 = 0.0f; /**< Peak-reference slew in Q16 AC-voltage codes/s. */

    if (setpoint_mgr.building.p_data == NULL)
    {
        return;
    }
    slew_code_q16 = rms_slew_vps * INV_CTRL_SQRT2 * INV_CTRL_AC_VOLT_CODE_PER_V *
                    (float)INV_CTRL_REF_Q_ONE;
    setpoint_mgr.building.p_data->v_ref_slew_code_q16_per_s = float_to_u32(slew_code_q16);
}

void inv_cfg_publish_building(void)
{
    if ((setpoint_mgr.building.p_data == NULL) ||
        (setpoint_mgr.active.p_data == NULL))
    {
        return;
    }
    setpoint_mgr.building.version++;
    *setpoint_mgr.active.p_data = *setpoint_mgr.building.p_data;
    setpoint_mgr.active.version = setpoint_mgr.building.version;
}

uint8_t inv_cfg_is_ready(void)
{
    return (uint8_t)((setpoint_mgr.active.p_data != NULL) &&
                     (setpoint_mgr.building.p_data != NULL) &&
                     (timing_is_valid(&ctrl_timing) != 0U) &&
                     (ctrl_freq_hz != 0U) &&
                     (phase_step_gain_q24 != 0U));
}

void inv_cfg_sync_building_to_active(void)
{
    if ((setpoint_mgr.building.p_data != NULL) &&
        (setpoint_mgr.active.p_data != NULL) &&
        (setpoint_mgr.active.version != setpoint_mgr.building.version))
    {
        *setpoint_mgr.active.p_data = *setpoint_mgr.building.p_data;
        setpoint_mgr.active.version = setpoint_mgr.building.version;
    }
}

const inv_ctrl_setpoint_mgr_t *inv_cfg_get_mgr(void)
{
    return &setpoint_mgr;
}
