// SPDX-License-Identifier: MIT
/**
 * @file    llc_cfg.h
 * @brief   LLC control configuration public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare LLC setpoint data structures and manager handles
 *          - Expose APIs for staging, publishing, and reading control references
 *          - Define voltage-loop tuning, startup, filter, and limit parameters used by the controller
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
#ifndef __LLC_CFG_H
#define __LLC_CFG_H

#include <stdint.h>
#include "my_math.h"
#include <stddef.h>

typedef struct
{
    float ctrl_ts;
    float task_ts;
    uint32_t startup_delay_ticks;
} llc_ctrl_timing_t;

#define LLC_CTRL_VOLT_LOOP_FREQ_CUT_HZ (500.0f)
#define LLC_CTRL_VOLT_LOOP_W_CUT (M_2PI * LLC_CTRL_VOLT_LOOP_FREQ_CUT_HZ)
#define LLC_CTRL_VOLT_LOOP_PM (60.0f / 180.0f * M_PI)
#define LLC_CTRL_VOLT_LOOP_KP 0.005f
#define LLC_CTRL_VOLT_LOOP_KI   \
    (LLC_CTRL_VOLT_LOOP_KP *    \
     LLC_CTRL_VOLT_LOOP_W_CUT / \
     tanf(LLC_CTRL_VOLT_LOOP_PM))

#define LLC_CTRL_OUTPUT_UP_LMT (1.0f)
#define LLC_CTRL_OUTPUT_DN_LMT (0.0f)
#define LLC_CTRL_VOUT_REF_DEFAULT_V (3.6f)
#define LLC_CTRL_VOUT_LPF_CUTOFF_HZ (50.0f)
#define LLC_CTRL_VBUS_NOTCH_CENTER_HZ (100.0f)
#define LLC_CTRL_VBUS_NOTCH_BANDWIDTH_HZ (50.0f)
#define LLC_CTRL_OUT_FF_NORM_BASE (800.0f)

typedef struct
{
    uint8_t run_allowed;
    float v_out_ref_v;
} llc_ctrl_setpoint_t;

typedef struct
{
    llc_ctrl_setpoint_t *p_data;
    unsigned int version;
} llc_ctrl_setpoint_buf_t;

typedef struct
{
    llc_ctrl_setpoint_buf_t active;
    llc_ctrl_setpoint_buf_t building;
} llc_ctrl_setpoint_mgr_t;

extern llc_ctrl_setpoint_mgr_t llc_cfg_setpoint_mgr;

void llc_cfg_set_timing(const llc_ctrl_timing_t *p_timing);
const llc_ctrl_timing_t *llc_cfg_get_timing(void);
float llc_cfg_get_ctrl_ts(void);
float llc_cfg_get_task_ts(void);
uint32_t llc_cfg_get_startup_delay_ticks(void);
void llc_cfg_set_p_building(llc_ctrl_setpoint_t *p_data);
llc_ctrl_setpoint_t *llc_cfg_get_p_active(void);
llc_ctrl_setpoint_t *llc_cfg_get_p_building(void);
void llc_cfg_set_run_allowed(uint8_t run_allowed);
void llc_cfg_set_v_out_ref(float v_out_ref_v);
void llc_cfg_publish_building(void);
void llc_cfg_building_version_inc(void);
uint8_t llc_cfg_is_ready(void);
const llc_ctrl_setpoint_mgr_t *llc_cfg_get_mgr(void);

static inline void llc_cfg_sync_building_to_active(void)
{
    if ((llc_cfg_setpoint_mgr.building.p_data == NULL) ||
        (llc_cfg_setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    if (llc_cfg_setpoint_mgr.active.version != llc_cfg_setpoint_mgr.building.version)
    {
        *llc_cfg_setpoint_mgr.active.p_data = *llc_cfg_setpoint_mgr.building.p_data;
        llc_cfg_setpoint_mgr.active.version = llc_cfg_setpoint_mgr.building.version;
    }
}

#endif
