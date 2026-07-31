// SPDX-License-Identifier: MIT
/**
 * @file    inv_cfg.h
 * @brief   Inverter int32 configuration public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define inverter integer ADC, current, frequency, and PWM code domains
 *          - Convert physical setpoints into fixed-point values before ISR consumption
 *          - Publish versioned setpoint buffers and control timing data
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR reads only integer setpoint members
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
#ifndef INV_I32_CFG_H
#define INV_I32_CFG_H

#include <stdint.h>

#define INV_CFG_DEFAULT_RUN_ALLOWED (0U)
#define INV_CFG_DEFAULT_FREQ_HZ (50.0f)
#define INV_CFG_DEFAULT_FREQ_SLEW_HZPS (10.0f)
#define INV_CFG_DEFAULT_RMS_REF_V (230.0f)
#define INV_CFG_DEFAULT_RMS_SLEW_VPS (212.0f)

#define INV_CTRL_AC_VOLT_CODE_MAX ((int32_t)((1L << 11) - 1L))
#define INV_CTRL_AC_VOLT_CODE_MIN (-INV_CTRL_AC_VOLT_CODE_MAX)
#define INV_CTRL_AC_VOLT_MAX_V (400.0f)
#define INV_CTRL_AC_VOLT_CODE_PER_V ((float)INV_CTRL_AC_VOLT_CODE_MAX / INV_CTRL_AC_VOLT_MAX_V)

#define INV_CTRL_BUS_VOLT_CODE_MAX ((int32_t)((1L << 12) - 1L))
#define INV_CTRL_BUS_VOLT_CODE_MIN (0)
#define INV_CTRL_BUS_VOLT_MAX_V (500.0f)

#define INV_CTRL_IND_CURR_CODE_MAX ((int32_t)((1L << 13) - 1L))
#define INV_CTRL_IND_CURR_CODE_MIN (-INV_CTRL_IND_CURR_CODE_MAX)
#define INV_CTRL_IND_CURR_MAX_A (100.0f)
#define INV_CTRL_IND_CURR_CODE_PER_A ((float)INV_CTRL_IND_CURR_CODE_MAX / INV_CTRL_IND_CURR_MAX_A)

#define INV_CTRL_PWM_RELOAD (64000)
#define INV_CTRL_FREQ_Q_SHIFT (16U)
#define INV_CTRL_FREQ_Q_ONE (1UL << INV_CTRL_FREQ_Q_SHIFT)
#define INV_CTRL_REF_Q_SHIFT (16U)
#define INV_CTRL_REF_Q_ONE (1UL << INV_CTRL_REF_Q_SHIFT)
#define INV_CTRL_PHASE_GAIN_Q_SHIFT (24U)
#define INV_CTRL_SQRT2 (1.41421356237f)

#define INV_CTRL_FREQ_TO_Q16(value) \
    ((uint32_t)(((value) * (float)INV_CTRL_FREQ_Q_ONE) + 0.5f))
#define INV_CTRL_RMS_TO_PEAK_CODE_Q16(value)                                         \
    ((int32_t)((((value) * INV_CTRL_SQRT2 * INV_CTRL_AC_VOLT_CODE_PER_V) *           \
                (float)INV_CTRL_REF_Q_ONE) +                                          \
               0.5f))
#define INV_CTRL_RMS_SLEW_TO_PEAK_CODE_Q16_PER_S(value)                              \
    ((uint32_t)((((value) * INV_CTRL_SQRT2 * INV_CTRL_AC_VOLT_CODE_PER_V) *          \
                 (float)INV_CTRL_REF_Q_ONE) +                                         \
                0.5f))

typedef struct
{
    uint8_t run_allowed;                 /**< 1 permits PWM control execution. */
    uint32_t freq_hz_q16;                /**< Fundamental frequency in Q16 Hz. */
    uint32_t freq_slew_hzps_q16;         /**< Frequency slew in Q16 Hz/s. */
    int32_t v_ref_peak_code_q16;         /**< Peak voltage reference in Q16 AC-voltage codes. */
    uint32_t v_ref_slew_code_q16_per_s;  /**< Peak voltage slew in Q16 AC-voltage codes/s. */
} inv_ctrl_setpoint_t;

typedef struct
{
    float ctrl_ts;   /**< Control sample time used by non-ISR coefficient design. */
    float ctrl_freq; /**< Control frequency in hertz. */
} inv_ctrl_timing_t;

typedef struct
{
    inv_ctrl_setpoint_t *p_data; /**< Setpoint storage owned by the caller or module. */
    uint32_t version;            /**< Publication version. */
} inv_ctrl_setpoint_buf_t;

typedef struct
{
    inv_ctrl_setpoint_buf_t active;   /**< Buffer consumed by control code. */
    inv_ctrl_setpoint_buf_t building; /**< Buffer updated by non-real-time code. */
} inv_ctrl_setpoint_mgr_t;

void inv_cfg_set_timing(const inv_ctrl_timing_t *p_timing);
const inv_ctrl_timing_t *inv_cfg_get_timing(void);
float inv_cfg_get_ctrl_ts(void);
uint32_t inv_cfg_get_ctrl_freq_hz(void);
uint32_t inv_cfg_get_phase_step_gain_q24(void);

void inv_cfg_set_p_building(inv_ctrl_setpoint_t *p_data);
inv_ctrl_setpoint_t *inv_cfg_get_p_active(void);
inv_ctrl_setpoint_t *inv_cfg_get_p_building(void);
void inv_cfg_set_run_allowed(uint8_t run_allowed);
void inv_cfg_set_freq_hz(float freq_hz);
void inv_cfg_set_freq_slew_hzps(float freq_slew_hzps);
void inv_cfg_set_rms_ref_v(float rms_ref_v);
void inv_cfg_set_rms_slew_vps(float rms_slew_vps);
void inv_cfg_publish_building(void);
void inv_cfg_building_version_inc(void);
uint8_t inv_cfg_is_ready(void);
void inv_cfg_sync_building_to_active(void);
const inv_ctrl_setpoint_mgr_t *inv_cfg_get_mgr(void);

#endif
