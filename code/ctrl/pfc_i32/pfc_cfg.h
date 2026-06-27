// SPDX-License-Identifier: MIT
/**
 * @file    pfc_cfg.h
 * @brief   PFC int32 configuration public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define PFC int32 setpoint and timing structures
 *          - Convert physical setpoint inputs to integer ADC code domains
 *          - Provide scaling constants shared by the PFC int32 controller
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
#ifndef __PFC_CFG_H
#define __PFC_CFG_H

#include <stddef.h>
#include <stdint.h>
#include "hw_params.h"
#include "my_math.h"

#define PFC_CFG_DEFAULT_VBUS_REF_V (400.0f)
#define PFC_CFG_DEFAULT_VBUS_SLEW_VPS (200.0f)

/* AC voltage feedback: signed +/-400 V mapped to signed 12-bit ADC code. */
#define PFC_CTRL_AC_VOLT_CODE_MAX ((int32_t)((1L << 11) - 1L))
#define PFC_CTRL_AC_VOLT_CODE_MIN (-PFC_CTRL_AC_VOLT_CODE_MAX)
#define PFC_CTRL_AC_VOLT_MAX_V (400.0f)
#define PFC_CTRL_AC_VOLT_MAX_V_INT (400LL)
#define PFC_CTRL_AC_VOLT_CODE_PER_V \
    ((float)PFC_CTRL_AC_VOLT_CODE_MAX / PFC_CTRL_AC_VOLT_MAX_V)

/* DC bus feedback: 0..500 V mapped to unsigned 12-bit ADC code. */
#define PFC_CTRL_BUS_VOLT_CODE_MAX ((int32_t)((1L << 12) - 1L))
#define PFC_CTRL_BUS_VOLT_CODE_MIN (0)
#define PFC_CTRL_BUS_VOLT_MAX_V (500.0f)
#define PFC_CTRL_BUS_VOLT_MAX_V_INT (500LL)
#define PFC_CTRL_BUS_VOLT_CODE_PER_V \
    ((float)PFC_CTRL_BUS_VOLT_CODE_MAX / PFC_CTRL_BUS_VOLT_MAX_V)

#define PFC_CTRL_PWM_AC_TO_BUS_K_NUM \
    ((int64_t)PFC_CTRL_BUS_VOLT_CODE_MAX * (int64_t)PFC_CTRL_AC_VOLT_MAX_V_INT)
#define PFC_CTRL_PWM_AC_TO_BUS_K_DEN \
    ((int64_t)PFC_CTRL_AC_VOLT_CODE_MAX * (int64_t)PFC_CTRL_BUS_VOLT_MAX_V_INT)

/* Inductor-current feedback: signed +/-100 A mapped to signed 14-bit ADC code. */
#define PFC_CTRL_IND_CURR_CODE_MAX ((int32_t)((1L << 13) - 1L))
#define PFC_CTRL_IND_CURR_CODE_MIN (-PFC_CTRL_IND_CURR_CODE_MAX)
#define PFC_CTRL_IND_CURR_MAX_A (100.0f)

#define PFC_CTRL_K4_AC_VOLT_CMD_SHIFT (16U)
#define PFC_CTRL_K4_AC_VOLT_CMD_K ((int32_t)(1L << PFC_CTRL_K4_AC_VOLT_CMD_SHIFT))

/* K1 maps bus-voltage code error to current-code command. */
#define PFC_CTRL_K1_VBUS_PI_GAIN_K                \
    (((float)PFC_CTRL_IND_CURR_CODE_MAX /         \
      PFC_CTRL_IND_CURR_MAX_A) /                  \
     PFC_CTRL_BUS_VOLT_CODE_PER_V)

/* K3 maps current-code error to PWM-reload-scaled AC-voltage command. */
#define PFC_CTRL_K3_CURR_PI_GAIN_K           \
    ((PFC_CTRL_AC_VOLT_CODE_PER_V /          \
      ((float)PFC_CTRL_IND_CURR_CODE_MAX /   \
       PFC_CTRL_IND_CURR_MAX_A)) *           \
     (float)PFC_CTRL_PWM_CMP_MAX)

#define PFC_CTRL_VBUS_REF_TO_CODE(val)                         \
    ((int32_t)(((val) / PFC_CTRL_BUS_VOLT_MAX_V) *             \
                   (float)PFC_CTRL_BUS_VOLT_CODE_MAX +         \
               0.5f))
#define PFC_CTRL_VBUS_SLEW_TO_CODE_PER_S(val)                  \
    ((int32_t)(((val) / PFC_CTRL_BUS_VOLT_MAX_V) *             \
                   (float)PFC_CTRL_BUS_VOLT_CODE_MAX +         \
               0.5f))

#define PFC_CTRL_TS (pfc_cfg_get_ctrl_ts())

/* PWM reload domain for up-down counting: period counts split across up and down half-cycles. */
#define PFC_CTRL_PWM_CLK_HZ (120000000LL)
#define PFC_CTRL_PWM_HR_MUL (32LL)
#define PFC_CTRL_PWM_FREQ_HZ (30000LL)
#define PFC_CTRL_PWM_PERIOD_COUNTS \
    ((PFC_CTRL_PWM_CLK_HZ * PFC_CTRL_PWM_HR_MUL) / PFC_CTRL_PWM_FREQ_HZ)
#define PFC_CTRL_PWM_UP_DOWN_HALF_CYCLES (2LL)
#define PFC_CTRL_PWM_RELOAD \
    ((int32_t)(PFC_CTRL_PWM_PERIOD_COUNTS / PFC_CTRL_PWM_UP_DOWN_HALF_CYCLES))
#define PFC_CTRL_PWM_CMP_MAX (PFC_CTRL_PWM_RELOAD)
#define PFC_CTRL_PWM_CMP_MIN (-PFC_CTRL_PWM_RELOAD)

#define PFC_CTRL_SOGI_COEFF_Q_SHIFT (20U)
#define PFC_CTRL_SOGI_COEFF_Q ((int32_t)(1L << PFC_CTRL_SOGI_COEFF_Q_SHIFT))
#define PFC_CTRL_SOGI_GAIN (1.0f)
#define PFC_CTRL_GRID_OMEGA_INIT_MRADPS ((int32_t)(314159))
#define PFC_CTRL_GRID_OMEGA_MIN_MRADPS ((int32_t)(188496))
#define PFC_CTRL_GRID_OMEGA_MAX_MRADPS ((int32_t)(439822))
#define PFC_CTRL_FLL_GAIN (150.0f)
#define PFC_CTRL_FLL_GAIN_Q_SHIFT (20U)
#define PFC_CTRL_FLL_GAIN_Q ((int32_t)(1L << PFC_CTRL_FLL_GAIN_Q_SHIFT))
#define PFC_CTRL_FLL_I_ERR_MAX_MRADPS ((int32_t)(188496))
#define PFC_CTRL_FLL_I_ERR_MIN_MRADPS (-PFC_CTRL_FLL_I_ERR_MAX_MRADPS)

#define PFC_CTRL_GRID_RMS_NOMINAL_V (230.0f)
#define PFC_CTRL_GRID_RMS_NOMINAL_CODE \
    ((int32_t)(PFC_CTRL_GRID_RMS_NOMINAL_V * PFC_CTRL_AC_VOLT_CODE_PER_V + 0.5f))
#define PFC_CTRL_VOLT_LOOP_TS (0.01f)
#define PFC_CTRL_VOLT_LOOP_PM (60.0f / 180.0f * M_PI)
#define PFC_CTRL_VOLT_LOOP_WCUT (M_2PI * 20.0f)
#define PFC_CTRL_VOLT_LOOP_OBJ (HW_DC_BUS_CAP_VALUE)
#define PFC_CTRL_VOLT_LOOP_KP      \
    (sinf(PFC_CTRL_VOLT_LOOP_PM) * \
     PFC_CTRL_VOLT_LOOP_WCUT *     \
     PFC_CTRL_VOLT_LOOP_OBJ *      \
     (float)PFC_CTRL_K1_VBUS_PI_GAIN_K)
#define PFC_CTRL_VOLT_LOOP_KI        \
    ((sinf(PFC_CTRL_VOLT_LOOP_PM) *  \
      PFC_CTRL_VOLT_LOOP_WCUT *      \
      PFC_CTRL_VOLT_LOOP_OBJ *       \
      PFC_CTRL_VOLT_LOOP_WCUT /      \
      tanf(PFC_CTRL_VOLT_LOOP_PM)) * \
     (float)PFC_CTRL_K1_VBUS_PI_GAIN_K)

#define PFC_CTRL_VBUS_LOOP_OUT_MAX_A (60.0f)
#define PFC_CTRL_VBUS_LOOP_OUT_MIN_A (-3.0f)
#define PFC_CTRL_VBUS_LOOP_OUT_MAX_RAW                         \
    ((int32_t)((PFC_CTRL_VBUS_LOOP_OUT_MAX_A /                 \
                PFC_CTRL_IND_CURR_MAX_A) *                     \
                   (float)PFC_CTRL_IND_CURR_CODE_MAX +         \
               0.5f))
#define PFC_CTRL_VBUS_LOOP_OUT_MIN_RAW                         \
    ((int32_t)((PFC_CTRL_VBUS_LOOP_OUT_MIN_A /                 \
                PFC_CTRL_IND_CURR_MAX_A) *                     \
                   (float)PFC_CTRL_IND_CURR_CODE_MAX -         \
               0.5f))
#define PFC_CTRL_VBUS_LOOP_OUT_MAX (PFC_CTRL_VBUS_LOOP_OUT_MAX_RAW)
#define PFC_CTRL_VBUS_LOOP_OUT_MIN (PFC_CTRL_VBUS_LOOP_OUT_MIN_RAW)

#define PFC_CTRL_CURR_LOOP_PM (50.0f / 180.0f * M_PI)
#define PFC_CTRL_CURR_LOOP_WCUT (M_2PI * 2500.0f)
#define PFC_CTRL_CURR_LOOP_OBJ (HW_AC_SIDE_IND_VALUE)
#define PFC_CTRL_CURR_LOOP_KP      \
    (sinf(PFC_CTRL_CURR_LOOP_PM) * \
     PFC_CTRL_CURR_LOOP_WCUT *     \
     PFC_CTRL_CURR_LOOP_OBJ *      \
     (float)PFC_CTRL_K3_CURR_PI_GAIN_K)
#define PFC_CTRL_CURR_LOOP_KI        \
    ((sinf(PFC_CTRL_CURR_LOOP_PM) *  \
      PFC_CTRL_CURR_LOOP_WCUT *      \
      PFC_CTRL_CURR_LOOP_OBJ *       \
      PFC_CTRL_CURR_LOOP_WCUT /      \
      tanf(PFC_CTRL_CURR_LOOP_PM)) * \
     (float)PFC_CTRL_K3_CURR_PI_GAIN_K)

#define PFC_CTRL_CURR_LOOP_OUT_MAX_V (30.0f)
#define PFC_CTRL_CURR_LOOP_OUT_MIN_V (-30.0f)
#define PFC_CTRL_CURR_LOOP_OUT_MAX_RAW                         \
    ((int32_t)((PFC_CTRL_CURR_LOOP_OUT_MAX_V /                 \
                PFC_CTRL_AC_VOLT_MAX_V) *                      \
                   (float)PFC_CTRL_AC_VOLT_CODE_MAX +          \
               0.5f))
#define PFC_CTRL_CURR_LOOP_OUT_MIN_RAW                         \
    ((int32_t)((PFC_CTRL_CURR_LOOP_OUT_MIN_V /                 \
                PFC_CTRL_AC_VOLT_MAX_V) *                      \
                   (float)PFC_CTRL_AC_VOLT_CODE_MAX -          \
               0.5f))
#define PFC_CTRL_CURR_LOOP_OUT_MAX                             \
    ((int32_t)((int64_t)PFC_CTRL_CURR_LOOP_OUT_MAX_RAW *       \
               (int64_t)PFC_CTRL_PWM_CMP_MAX))
#define PFC_CTRL_CURR_LOOP_OUT_MIN                             \
    ((int32_t)((int64_t)PFC_CTRL_CURR_LOOP_OUT_MIN_RAW *       \
               (int64_t)PFC_CTRL_PWM_CMP_MAX))

typedef struct
{
    float ctrl_ts;
    uint32_t ctrl_freq_hz;
} pfc_ctrl_timing_t;

typedef struct
{
    uint8_t run_allowed;
    int32_t vbus_ref;
    int32_t vbus_slew_code_per_s;
} pfc_ctrl_setpoint_t;

typedef struct
{
    pfc_ctrl_setpoint_t *p_data;
    unsigned int version;
} pfc_ctrl_setpoint_buf_t;

typedef struct
{
    pfc_ctrl_setpoint_buf_t active;
    pfc_ctrl_setpoint_buf_t building;
} pfc_ctrl_setpoint_mgr_t;

extern pfc_ctrl_setpoint_mgr_t pfc_cfg_setpoint_mgr;

void pfc_cfg_set_timing(const pfc_ctrl_timing_t *p_timing);
const pfc_ctrl_timing_t *pfc_cfg_get_timing(void);
float pfc_cfg_get_ctrl_ts(void);
uint32_t pfc_cfg_get_ctrl_freq_hz(void);
void pfc_cfg_set_p_building(pfc_ctrl_setpoint_t *p_data);
pfc_ctrl_setpoint_t *pfc_cfg_get_p_active(void);
pfc_ctrl_setpoint_t *pfc_cfg_get_p_building(void);
void pfc_cfg_set_run_allowed(uint8_t run_allowed);
void pfc_cfg_set_vbus_ref_v(float vbus_ref_v);
void pfc_cfg_set_vbus_slew_vps(float vbus_slew_vps);
void pfc_cfg_publish_building(void);
void pfc_cfg_building_version_inc(void);
uint8_t pfc_cfg_is_ready(void);
const pfc_ctrl_setpoint_mgr_t *pfc_cfg_get_mgr(void);

static inline void pfc_cfg_sync_building_to_active(void)
{
    if ((pfc_cfg_setpoint_mgr.building.p_data == NULL) ||
        (pfc_cfg_setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    if (pfc_cfg_setpoint_mgr.active.version != pfc_cfg_setpoint_mgr.building.version)
    {
        *pfc_cfg_setpoint_mgr.active.p_data = *pfc_cfg_setpoint_mgr.building.p_data;
        pfc_cfg_setpoint_mgr.active.version = pfc_cfg_setpoint_mgr.building.version;
    }
}

#endif
