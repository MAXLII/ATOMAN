// SPDX-License-Identifier: MIT
/**
 * @file    boost_cfg.h
 * @brief   boost_cfg control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare BOOST setpoint data structures and manager handles
 *          - Expose APIs for staging, publishing, and reading BOOST control references
 *          - Define scaling constants and loop parameters used by the controller
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
/* Header guard for the BOOST configuration interface. */
#ifndef __BOOST_CFG_H

/* Header guard marker for boost_cfg.h. */
#define __BOOST_CFG_H

#include <stddef.h>
#include <stdint.h>
#include "my_math.h"
#include "boost_hw_param.h"

typedef struct
{
    float ctrl_ts;
    float task_ts;
    float pwm_ts;
    int32_t pwm_cmp_max;
} boost_ctrl_timing_t;

void boost_cfg_set_timing(const boost_ctrl_timing_t *p_timing);
const boost_ctrl_timing_t *boost_cfg_get_timing(void);
float boost_cfg_get_ctrl_ts(void);
float boost_cfg_get_task_ts(void);
float boost_cfg_get_pwm_ts(void);
int32_t boost_cfg_get_pwm_cmp_max(void);

/* Output voltage-loop reference maximum integer code. */
#define BOOST_CTRL_OUT_VOLT_LOOP_REF_CODE_MAX ((int32_t)(0x1000 - 1))

/* Output voltage-loop reference full-scale physical value. */
#define BOOST_CTRL_OUT_VOLT_LOOP_REF_MAX_V (60.0f)

/* Output voltage-loop startup reference value. */
#define BOOST_CTRL_OUT_VOLT_LOOP_REF_DEFAULT_V (12.0f)

/* Convert an output voltage-loop reference to the loop reference code domain. */
#define BOOST_CTRL_OUT_VOLT_LOOP_REF_TO_CODE(val)                 \
    ((int32_t)(((val) / BOOST_CTRL_OUT_VOLT_LOOP_REF_MAX_V) *     \
               (float)BOOST_CTRL_OUT_VOLT_LOOP_REF_CODE_MAX))

/* Input voltage-limit loop reference maximum integer code. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_REF_CODE_MAX ((int32_t)(0x1000 - 1))

/* Input voltage-limit loop reference full-scale physical value. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_REF_MAX_V (60.0f)

/* Input voltage-limit loop startup reference value. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_REF_DEFAULT_V (24.0f)

/* Convert an input voltage-limit reference to the loop reference code domain. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_REF_TO_CODE(val)                 \
    ((int32_t)(((val) / BOOST_CTRL_IN_VOLT_LMT_LOOP_REF_MAX_V) *     \
               (float)BOOST_CTRL_IN_VOLT_LMT_LOOP_REF_CODE_MAX))

/* Input power-limit path reference maximum integer code. */
#define BOOST_CTRL_IN_PWR_LMT_CODE_MAX ((int32_t)((0x4000L * 0x1000L) - 1L))

/* Input power-limit path reference full-scale physical value. */
#define BOOST_CTRL_IN_PWR_LMT_MAX_W (1500.0f)

/* Input power-limit path startup reference value. */
#define BOOST_CTRL_IN_PWR_LMT_DEFAULT_W (1500.0f)

/* Convert an input power-limit reference to the path reference code domain. */
#define BOOST_CTRL_IN_PWR_LMT_TO_CODE(val)                 \
    ((int32_t)(((val) / BOOST_CTRL_IN_PWR_LMT_MAX_W) *     \
               (float)BOOST_CTRL_IN_PWR_LMT_CODE_MAX))

/* Input current-limit path reference positive endpoint code. */
#define BOOST_CTRL_IN_CURR_LMT_CODE_MAX ((int32_t)(0x4000 - 1))

/* Input current-limit path reference negative endpoint code. */
#define BOOST_CTRL_IN_CURR_LMT_CODE_MIN (-BOOST_CTRL_IN_CURR_LMT_CODE_MAX)

/* Input current-limit path reference full-scale physical value. */
#define BOOST_CTRL_IN_CURR_LMT_MAX_A (100.0f)

/* Input current-limit path startup reference value. */
#define BOOST_CTRL_IN_CURR_LMT_DEFAULT_A (100.0f)

/* Convert an input current-limit reference to the signed path reference code domain. */
#define BOOST_CTRL_IN_CURR_LMT_TO_CODE(val)                                      \
    ((int32_t)(((val) / BOOST_CTRL_IN_CURR_LMT_MAX_A) *                          \
               (float)BOOST_CTRL_IN_CURR_LMT_CODE_MAX))

/* Output current-limit path reference positive endpoint code. */
#define BOOST_CTRL_OUT_CURR_LMT_CODE_MAX ((int32_t)(0x4000 - 1))

/* Output current-limit path reference negative endpoint code. */
#define BOOST_CTRL_OUT_CURR_LMT_CODE_MIN (-BOOST_CTRL_OUT_CURR_LMT_CODE_MAX)

/* Output current-limit path reference full-scale physical value. */
#define BOOST_CTRL_OUT_CURR_LMT_MAX_A (100.0f)

/* Output current-limit path startup reference value. */
#define BOOST_CTRL_OUT_CURR_LMT_DEFAULT_A (100.0f)

/* Convert an output current-limit reference to the signed path reference code domain. */
#define BOOST_CTRL_OUT_CURR_LMT_TO_CODE(val)                                      \
    ((int32_t)(((val) / BOOST_CTRL_OUT_CURR_LMT_MAX_A) *                          \
               (float)BOOST_CTRL_OUT_CURR_LMT_CODE_MAX))

/* Integer-control K2 shift used by the current-feedback domain. */
#define BOOST_CTRL_K2_IND_CURR_FB_SHIFT (16U)

/* Integer-control K2 selected as the current-feedback shift gain. */
#define BOOST_CTRL_K2_IND_CURR_FB_K ((int32_t)(1L << BOOST_CTRL_K2_IND_CURR_FB_SHIFT))

/* Integer-control K4 selected as the output-voltage feedforward and PWM compare gain. */
#define BOOST_CTRL_K4_V_OUT_FF_K (BOOST_CTRL_CMP_MAX)

/* Integer-control K1 numerator derived from current and voltage code scales. */
#define BOOST_CTRL_K1_OUT_VOLT_PI_GAIN_K_NUM      \
    ((int64_t)BOOST_CTRL_K2_IND_CURR_FB_K *       \
     (int64_t)BOOST_CTRL_IN_CURR_LMT_CODE_MAX *   \
     (int64_t)BOOST_CTRL_OUT_VOLT_LOOP_REF_MAX_V)

/* Integer-control K1 denominator derived from current and voltage code scales. */
#define BOOST_CTRL_K1_OUT_VOLT_PI_GAIN_K_DEN      \
    ((int64_t)BOOST_CTRL_IN_CURR_LMT_MAX_A *      \
     (int64_t)BOOST_CTRL_OUT_VOLT_LOOP_REF_CODE_MAX)

/* Integer-control K1 used as the output-voltage PI coefficient gain. */
#define BOOST_CTRL_K1_OUT_VOLT_PI_GAIN_K                          \
    ((int32_t)((float)BOOST_CTRL_K1_OUT_VOLT_PI_GAIN_K_NUM /      \
               (float)BOOST_CTRL_K1_OUT_VOLT_PI_GAIN_K_DEN))

/* Integer-control K3 numerator for raw-current error to K4 voltage domain. */
#define BOOST_CTRL_K3_IND_CURR_PI_GAIN_K_NUM       \
    ((int64_t)BOOST_CTRL_K4_V_OUT_FF_K *           \
     (int64_t)BOOST_CTRL_OUT_VOLT_LOOP_REF_CODE_MAX * \
     (int64_t)BOOST_CTRL_IN_CURR_LMT_MAX_A)

/* Integer-control K3 denominator for raw-current error to K4 voltage domain. */
#define BOOST_CTRL_K3_IND_CURR_PI_GAIN_K_DEN      \
    ((int64_t)BOOST_CTRL_OUT_VOLT_LOOP_REF_MAX_V * \
     (int64_t)BOOST_CTRL_IN_CURR_LMT_CODE_MAX)

/* Integer-control K3 used as the inductor-current PI coefficient gain. */
#define BOOST_CTRL_K3_IND_CURR_PI_GAIN_K                          \
    ((int32_t)((float)BOOST_CTRL_K3_IND_CURR_PI_GAIN_K_NUM /      \
               (float)BOOST_CTRL_K3_IND_CURR_PI_GAIN_K_DEN))

/* Control-loop sample time supplied by boost_cfg_set_timing(). */
#define BOOST_CTRL_TS (boost_cfg_get_ctrl_ts())

/* Slow control-task sample time supplied by boost_cfg_set_timing(). */
#define BOOST_CTRL_TASK_TS (boost_cfg_get_task_ts())

/* Common PI tuning formula: kp = sin(PM) * wcut * OBJ, ki = kp * wcut / tan(PM). */
/* K keeps kp/ki as scaled float values before pi_tustin_i32_update generates int32_t b0/b1. */

/* Output voltage-loop phase-margin setting. */
#define BOOST_CTRL_OUT_VOLT_LOOP_PM (60.0f / 180.0f * M_PI)

/* Output voltage-loop angular cutoff-frequency setting. */
#define BOOST_CTRL_OUT_VOLT_LOOP_WCUT (M_2PI * 1200.0f)

/* Output voltage-loop controlled object parameter. */
#define BOOST_CTRL_OUT_VOLT_LOOP_OBJ (BOOST_HW_OUT_CAP_TOTAL)

/* Output voltage-loop PI gain scaling factor. */
#define BOOST_CTRL_OUT_VOLT_LOOP_PI_GAIN_K (BOOST_CTRL_K1_OUT_VOLT_PI_GAIN_K)

/* Output voltage-loop output maximum current-domain code. */
#define BOOST_CTRL_OUT_VOLT_LOOP_OUT_CODE_MAX (BOOST_CTRL_OUT_CURR_LMT_CODE_MAX)

/* Output voltage-loop output full-scale physical value. */
#define BOOST_CTRL_OUT_VOLT_LOOP_OUT_MAX_A (100.0f)

/* Output voltage-loop proportional gain before coefficient generation. */
#define BOOST_CTRL_OUT_VOLT_LOOP_KP      \
    (sinf(BOOST_CTRL_OUT_VOLT_LOOP_PM) * \
     BOOST_CTRL_OUT_VOLT_LOOP_WCUT *     \
     BOOST_CTRL_OUT_VOLT_LOOP_OBJ *      \
     (float)BOOST_CTRL_OUT_VOLT_LOOP_PI_GAIN_K)

/* Output voltage-loop integral gain before coefficient generation. */
#define BOOST_CTRL_OUT_VOLT_LOOP_KI        \
    ((sinf(BOOST_CTRL_OUT_VOLT_LOOP_PM) *  \
      BOOST_CTRL_OUT_VOLT_LOOP_WCUT *      \
      BOOST_CTRL_OUT_VOLT_LOOP_OBJ *       \
      BOOST_CTRL_OUT_VOLT_LOOP_WCUT /      \
      tanf(BOOST_CTRL_OUT_VOLT_LOOP_PM)) * \
     (float)BOOST_CTRL_OUT_VOLT_LOOP_PI_GAIN_K)

/* Output voltage-loop upper limit expressed in the current physical domain. */
#define BOOST_CTRL_OUT_VOLT_LOOP_UP_LMT_A (100.0f)

/* Output voltage-loop lower limit expressed in the current physical domain. */
#define BOOST_CTRL_OUT_VOLT_LOOP_DN_LMT_A (-10.0f)

/* Output voltage-loop upper limit converted to the raw current-code domain. */
#define BOOST_CTRL_OUT_VOLT_LOOP_UP_LMT_RAW                       \
    ((int32_t)((BOOST_CTRL_OUT_VOLT_LOOP_UP_LMT_A /               \
                BOOST_CTRL_OUT_VOLT_LOOP_OUT_MAX_A) *             \
               (float)BOOST_CTRL_OUT_VOLT_LOOP_OUT_CODE_MAX))

/* Output voltage-loop lower limit converted to the raw current-code domain. */
#define BOOST_CTRL_OUT_VOLT_LOOP_DN_LMT_RAW                       \
    ((int32_t)((BOOST_CTRL_OUT_VOLT_LOOP_DN_LMT_A /               \
                BOOST_CTRL_OUT_VOLT_LOOP_OUT_MAX_A) *             \
               (float)BOOST_CTRL_OUT_VOLT_LOOP_OUT_CODE_MAX))

/* Output voltage-loop upper limit converted to the K2 current domain. */
#define BOOST_CTRL_OUT_VOLT_LOOP_UP_LMT                         \
    ((int32_t)((int64_t)BOOST_CTRL_OUT_VOLT_LOOP_UP_LMT_RAW *   \
               (int64_t)BOOST_CTRL_K2_IND_CURR_FB_K))

/* Output voltage-loop lower limit converted to the K2 current domain. */
#define BOOST_CTRL_OUT_VOLT_LOOP_DN_LMT                         \
    ((int32_t)((int64_t)BOOST_CTRL_OUT_VOLT_LOOP_DN_LMT_RAW *   \
               (int64_t)BOOST_CTRL_K2_IND_CURR_FB_K))

/* Output-voltage feedforward gain used by the compare-command calculation. */
#define BOOST_CTRL_OUT_VOLT_LOOP_V_OUT_FF_K (BOOST_CTRL_K4_V_OUT_FF_K)

/* Input voltage-limit loop phase-margin setting. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_PM (45.0f / 180.0f * M_PI)

/* Input voltage-limit loop angular cutoff-frequency setting. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_WCUT (M_2PI * 500.0f)

/* Input voltage-limit loop controlled object parameter. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_OBJ (BOOST_HW_IN_CAP_TOTAL)

/* Input voltage-limit loop PI gain scaling factor. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_PI_GAIN_K (BOOST_CTRL_OUT_VOLT_LOOP_PI_GAIN_K)

/* Input voltage-limit loop output maximum current-domain code. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_OUT_CODE_MAX (BOOST_CTRL_IN_CURR_LMT_CODE_MAX)

/* Input voltage-limit loop output full-scale physical value. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_OUT_MAX_A (100.0f)

/* Input voltage-limit loop proportional gain before coefficient generation. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_KP      \
    (sinf(BOOST_CTRL_IN_VOLT_LMT_LOOP_PM) * \
     BOOST_CTRL_IN_VOLT_LMT_LOOP_WCUT *     \
     BOOST_CTRL_IN_VOLT_LMT_LOOP_OBJ *      \
     (float)BOOST_CTRL_IN_VOLT_LMT_LOOP_PI_GAIN_K)

/* Input voltage-limit loop integral gain before coefficient generation. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_KI        \
    ((sinf(BOOST_CTRL_IN_VOLT_LMT_LOOP_PM) *  \
      BOOST_CTRL_IN_VOLT_LMT_LOOP_WCUT *      \
      BOOST_CTRL_IN_VOLT_LMT_LOOP_OBJ *       \
      BOOST_CTRL_IN_VOLT_LMT_LOOP_WCUT /      \
      tanf(BOOST_CTRL_IN_VOLT_LMT_LOOP_PM)) * \
     (float)BOOST_CTRL_IN_VOLT_LMT_LOOP_PI_GAIN_K)

/* Input voltage-limit loop upper limit expressed in the current physical domain. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_UP_LMT_A (100.0f)

/* Input voltage-limit loop lower limit expressed in the current physical domain. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_DN_LMT_A (-10.0f)

/* Input voltage-limit loop upper limit converted to the raw current-code domain. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_UP_LMT_RAW                       \
    ((int32_t)((BOOST_CTRL_IN_VOLT_LMT_LOOP_UP_LMT_A /               \
                BOOST_CTRL_IN_VOLT_LMT_LOOP_OUT_MAX_A) *             \
               (float)BOOST_CTRL_IN_VOLT_LMT_LOOP_OUT_CODE_MAX))

/* Input voltage-limit loop lower limit converted to the raw current-code domain. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_DN_LMT_RAW                       \
    ((int32_t)((BOOST_CTRL_IN_VOLT_LMT_LOOP_DN_LMT_A /               \
                BOOST_CTRL_IN_VOLT_LMT_LOOP_OUT_MAX_A) *             \
               (float)BOOST_CTRL_IN_VOLT_LMT_LOOP_OUT_CODE_MAX))

/* Input voltage-limit loop upper limit converted to the K2 current domain. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_UP_LMT                         \
    ((int32_t)((int64_t)BOOST_CTRL_IN_VOLT_LMT_LOOP_UP_LMT_RAW *   \
               (int64_t)BOOST_CTRL_IND_CURR_LOOP_FB_K))

/* Input voltage-limit loop lower limit converted to the K2 current domain. */
#define BOOST_CTRL_IN_VOLT_LMT_LOOP_DN_LMT                         \
    ((int32_t)((int64_t)BOOST_CTRL_IN_VOLT_LMT_LOOP_DN_LMT_RAW *   \
               (int64_t)BOOST_CTRL_IND_CURR_LOOP_FB_K))

/* Inductor-current loop phase-margin setting. */
#define BOOST_CTRL_IND_CURR_LOOP_PM (45.0f / 180.0f * M_PI)

/* Inductor-current loop angular cutoff-frequency setting. */
#define BOOST_CTRL_IND_CURR_LOOP_WCUT (M_2PI * 6000.0f)

/* Inductor-current loop controlled object parameter. */
#define BOOST_CTRL_IND_CURR_LOOP_OBJ (BOOST_HW_IND)

/* Inductor-current loop PI gain scaling factor. */
#define BOOST_CTRL_IND_CURR_LOOP_PI_GAIN_K (BOOST_CTRL_K3_IND_CURR_PI_GAIN_K)

/* Inductor-current loop reference maximum current-domain code. */
#define BOOST_CTRL_IND_CURR_LOOP_REF_CODE_MAX (BOOST_CTRL_IN_CURR_LMT_CODE_MAX)

/* Inductor-current loop reference full-scale physical value. */
#define BOOST_CTRL_IND_CURR_LOOP_REF_MAX_A (100.0f)

/* Inductor-current loop feedback gain applied before PI calculation. */
#define BOOST_CTRL_IND_CURR_LOOP_FB_K (BOOST_CTRL_K2_IND_CURR_FB_K)

/* Inductor-current loop output maximum integer code. */
#define BOOST_CTRL_IND_CURR_LOOP_OUT_CODE_MAX ((int32_t)(0x1000 - 1))

/* Inductor-current loop output full-scale physical value. */
#define BOOST_CTRL_IND_CURR_LOOP_OUT_MAX_V (60.0f)

/* Inductor-current loop proportional gain before coefficient generation. */
#define BOOST_CTRL_IND_CURR_LOOP_KP      \
    (sinf(BOOST_CTRL_IND_CURR_LOOP_PM) * \
     BOOST_CTRL_IND_CURR_LOOP_WCUT *     \
     BOOST_CTRL_IND_CURR_LOOP_OBJ *      \
     (float)BOOST_CTRL_IND_CURR_LOOP_PI_GAIN_K)

/* Inductor-current loop integral gain before coefficient generation. */
#define BOOST_CTRL_IND_CURR_LOOP_KI        \
    ((sinf(BOOST_CTRL_IND_CURR_LOOP_PM) *  \
      BOOST_CTRL_IND_CURR_LOOP_WCUT *      \
      BOOST_CTRL_IND_CURR_LOOP_OBJ *       \
      BOOST_CTRL_IND_CURR_LOOP_WCUT /      \
      tanf(BOOST_CTRL_IND_CURR_LOOP_PM)) * \
     (float)BOOST_CTRL_IND_CURR_LOOP_PI_GAIN_K)

/* Inductor-current loop upper limit expressed in the voltage physical domain. */
#define BOOST_CTRL_IND_CURR_LOOP_UP_LMT_V (60.0f)

/* Inductor-current loop lower limit expressed in the voltage physical domain. */
#define BOOST_CTRL_IND_CURR_LOOP_DN_LMT_V (-60.0f)

/* Inductor-current loop upper limit converted to the raw voltage-code domain. */
#define BOOST_CTRL_IND_CURR_LOOP_UP_LMT_RAW                       \
    ((int32_t)((BOOST_CTRL_IND_CURR_LOOP_UP_LMT_V /               \
                BOOST_CTRL_IND_CURR_LOOP_OUT_MAX_V) *             \
               (float)BOOST_CTRL_IND_CURR_LOOP_OUT_CODE_MAX))

/* Inductor-current loop lower limit converted to the raw voltage-code domain. */
#define BOOST_CTRL_IND_CURR_LOOP_DN_LMT_RAW                       \
    ((int32_t)((BOOST_CTRL_IND_CURR_LOOP_DN_LMT_V /               \
                BOOST_CTRL_IND_CURR_LOOP_OUT_MAX_V) *             \
               (float)BOOST_CTRL_IND_CURR_LOOP_OUT_CODE_MAX))

/* Inductor-current loop upper limit converted to the K4 voltage domain. */
#define BOOST_CTRL_IND_CURR_LOOP_UP_LMT                          \
    ((int32_t)((int64_t)BOOST_CTRL_IND_CURR_LOOP_UP_LMT_RAW *    \
               (int64_t)BOOST_CTRL_K4_V_OUT_FF_K))

/* Inductor-current loop lower limit converted to the K4 voltage domain. */
#define BOOST_CTRL_IND_CURR_LOOP_DN_LMT                          \
    ((int32_t)((int64_t)BOOST_CTRL_IND_CURR_LOOP_DN_LMT_RAW *    \
               (int64_t)BOOST_CTRL_K4_V_OUT_FF_K))

/* Maximum PWM compare command accepted by the BOOST controller. */
#define BOOST_CTRL_CMP_MAX (boost_cfg_get_pwm_cmp_max())

/* Minimum PWM compare command accepted by the BOOST controller. */
#define BOOST_CTRL_CMP_MIN (0)

typedef struct
{
    /* Run permission consumed by the control ISR. */
    uint8_t run_allowed;
    /* Output voltage reference in the voltage-code domain. */
    int32_t out_volt_ref;
    /* Input voltage limit reference in the voltage-code domain. */
    int32_t in_volt_lmt;
    /* Input power limit in the power-code domain. */
    int32_t pwr_lmt;
    /* Input current limit in the current-code domain. */
    int32_t in_curr_lmt;
    /* Output current limit in the current-code domain. */
    int32_t out_curr_lmt;
} boost_ctrl_setpoint_t;

typedef struct
{
    boost_ctrl_setpoint_t *p_data;
    unsigned int version;
} boost_ctrl_setpoint_buf_t;

typedef struct
{
    boost_ctrl_setpoint_buf_t active;
    boost_ctrl_setpoint_buf_t building;
} boost_ctrl_setpoint_mgr_t;

extern boost_ctrl_setpoint_mgr_t boost_cfg_setpoint_mgr;

/* Setter APIs accept physical SI-unit values and store integer codes in the building setpoint. */
void boost_cfg_set_p_building(boost_ctrl_setpoint_t *p_data);
boost_ctrl_setpoint_t *boost_cfg_get_p_active(void);
boost_ctrl_setpoint_t *boost_cfg_get_p_building(void);
void boost_cfg_set_run_allowed(uint8_t run_allowed);
void boost_cfg_set_pwr_lmt(float pwr_lmt);
void boost_cfg_set_out_volt_ref(float out_volt_ref);
void boost_cfg_set_in_volt_lmt(float in_volt_lmt);
void boost_cfg_set_in_curr_lmt(float in_curr_lmt);
void boost_cfg_set_out_curr_lmt(float out_curr_lmt);
void boost_cfg_publish_building(void);
void boost_cfg_building_version_inc(void);
uint8_t boost_cfg_is_ready(void);
const boost_ctrl_setpoint_mgr_t *boost_cfg_get_mgr(void);

/* Called from the control side to atomically consume the latest published building setpoint. */
static inline void boost_cfg_sync_building_to_active(void)
{
    if ((boost_cfg_setpoint_mgr.building.p_data == NULL) ||
        (boost_cfg_setpoint_mgr.active.p_data == NULL))
    {
        return;
    }

    if (boost_cfg_setpoint_mgr.active.version != boost_cfg_setpoint_mgr.building.version)
    {
        *boost_cfg_setpoint_mgr.active.p_data = *boost_cfg_setpoint_mgr.building.p_data;
        boost_cfg_setpoint_mgr.active.version = boost_cfg_setpoint_mgr.building.version;
    }
}

/* Fast ISR-side setpoint sync; caller guarantees active/building pointers are valid. */
static inline void boost_cfg_sync_building_to_active_fast(void)
{
    if (boost_cfg_setpoint_mgr.active.version != boost_cfg_setpoint_mgr.building.version)
    {
        *boost_cfg_setpoint_mgr.active.p_data = *boost_cfg_setpoint_mgr.building.p_data;
        boost_cfg_setpoint_mgr.active.version = boost_cfg_setpoint_mgr.building.version;
    }
}

#endif
