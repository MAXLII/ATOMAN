// SPDX-License-Identifier: MIT
/**
 * @file    inv_ctrl.h
 * @brief   Inverter int32 control public interface and scaling definition.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define integer SRFPI and capacitor-current damping gains
 *          - Define current, harmonic, and PWM command limits in their code domains
 *          - Expose inverter controller preparation and HAL compatibility APIs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR data path uses int32_t with bounded int64_t multiply-accumulates
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
#ifndef INV_I32_CTRL_H
#define INV_I32_CTRL_H

#include "hw_params.h"
#include "inv_cfg.h"
#include "inv_hal.h"
#include "my_math.h"

#define INV_CTRL_RATED_POWER_W (6600.0f)
#define INV_CTRL_NOMINAL_RMS_V (230.0f)
#define INV_CTRL_NOMINAL_FREQ_HZ (50.0f)
#define INV_CTRL_FILTER_IND_RES_OHM (0.001f)
#define INV_CTRL_NOMINAL_LOAD_OHM \
    ((INV_CTRL_NOMINAL_RMS_V * INV_CTRL_NOMINAL_RMS_V) / INV_CTRL_RATED_POWER_W)

#define INV_CTRL_INNER_LOOP_BANDWIDTH_HZ (4000.0f)
#define INV_CTRL_INNER_LOOP_BANDWIDTH_RADPS (M_2PI * INV_CTRL_INNER_LOOP_BANDWIDTH_HZ)
#define INV_CTRL_INNER_LOOP_SQRT_TERM                                                              \
    ((2.0f * INV_CTRL_FILTER_IND_RES_OHM * HW_AC_SIDE_CAP_VALUE * INV_CTRL_NOMINAL_LOAD_OHM *      \
      ((INV_CTRL_FILTER_IND_RES_OHM * HW_AC_SIDE_CAP_VALUE * INV_CTRL_NOMINAL_LOAD_OHM) +          \
       HW_AC_SIDE_IND_VALUE)) +                                                                     \
     (HW_AC_SIDE_IND_VALUE * HW_AC_SIDE_IND_VALUE *                                                 \
      (2.0f + (HW_AC_SIDE_CAP_VALUE * HW_AC_SIDE_CAP_VALUE *                                        \
               INV_CTRL_NOMINAL_LOAD_OHM * INV_CTRL_NOMINAL_LOAD_OHM *                              \
               INV_CTRL_INNER_LOOP_BANDWIDTH_RADPS * INV_CTRL_INNER_LOOP_BANDWIDTH_RADPS))))
#define INV_CTRL_INNER_LOOP_K                                                                       \
    ((HW_AC_SIDE_IND_VALUE +                                                                        \
      (INV_CTRL_FILTER_IND_RES_OHM * HW_AC_SIDE_CAP_VALUE * INV_CTRL_NOMINAL_LOAD_OHM) +           \
      sqrtf(INV_CTRL_INNER_LOOP_SQRT_TERM)) /                                                       \
     (HW_AC_SIDE_CAP_VALUE * INV_CTRL_NOMINAL_LOAD_OHM))

#define INV_CTRL_VOLT_LOOP_BANDWIDTH_HZ (1300.0f)
#define INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS (M_2PI * INV_CTRL_VOLT_LOOP_BANDWIDTH_HZ)
#define INV_CTRL_VOLT_LOOP_KP                                                                       \
    (HW_AC_SIDE_CAP_VALUE * INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS *                                    \
     (sqrtf((2.0f * HW_AC_SIDE_IND_VALUE * HW_AC_SIDE_IND_VALUE *                                  \
             INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS * INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS) +             \
            (INV_CTRL_INNER_LOOP_K * INV_CTRL_INNER_LOOP_K)) -                                      \
      (HW_AC_SIDE_IND_VALUE * INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS)) /                                \
     INV_CTRL_INNER_LOOP_K)
#define INV_CTRL_VOLT_LOOP_KI_RATIO (0.55f)
#define INV_CTRL_VOLT_LOOP_KI \
    (INV_CTRL_VOLT_LOOP_KI_RATIO * INV_CTRL_VOLT_LOOP_KP * M_2PI * INV_CTRL_NOMINAL_FREQ_HZ)

#define INV_CTRL_K2_CURR_SHIFT (16U)
#define INV_CTRL_K2_CURR_K ((int32_t)(1L << INV_CTRL_K2_CURR_SHIFT))
#define INV_CTRL_VOLT_PI_GAIN_K                                                                    \
    ((float)INV_CTRL_K2_CURR_K * INV_CTRL_IND_CURR_CODE_PER_A / INV_CTRL_AC_VOLT_CODE_PER_V)
#define INV_CTRL_VOLT_LOOP_KP_I32 (INV_CTRL_VOLT_LOOP_KP * INV_CTRL_VOLT_PI_GAIN_K)
#define INV_CTRL_VOLT_LOOP_KI_I32 (INV_CTRL_VOLT_LOOP_KI * INV_CTRL_VOLT_PI_GAIN_K)
#define INV_CTRL_VOLT_LOOP_OUT_MAX_A (20.0f)
#define INV_CTRL_VOLT_LOOP_OUT_MIN_A (-20.0f)
#define INV_CTRL_VOLT_LOOP_OUT_MAX_I32                                                          \
    ((int32_t)((INV_CTRL_VOLT_LOOP_OUT_MAX_A * INV_CTRL_IND_CURR_CODE_PER_A *                    \
                (float)INV_CTRL_K2_CURR_K) +                                                     \
               0.5f))
#define INV_CTRL_VOLT_LOOP_OUT_MIN_I32 (-INV_CTRL_VOLT_LOOP_OUT_MAX_I32)

#define INV_CTRL_HARMONIC_3_ORDER (3U)
#define INV_CTRL_HARMONIC_5_ORDER (5U)
#define INV_CTRL_HARMONIC_7_ORDER (7U)
#define INV_CTRL_HARMONIC_3_GAIN (0.30f)
#define INV_CTRL_HARMONIC_5_GAIN (0.20f)
#define INV_CTRL_HARMONIC_7_GAIN (0.15f)
#define INV_CTRL_HARMONIC_OUT_MAX_A (8.0f)
#define INV_CTRL_HARMONIC_OUT_MAX_I32 \
    ((int32_t)((INV_CTRL_HARMONIC_OUT_MAX_A * INV_CTRL_IND_CURR_CODE_PER_A) + 0.5f))
#define INV_CTRL_HARMONIC_OUT_MIN_I32 (-INV_CTRL_HARMONIC_OUT_MAX_I32)

#define INV_CTRL_INNER_GAIN_I32                                                                    \
    ((int32_t)((INV_CTRL_INNER_LOOP_K * (float)INV_CTRL_PWM_RELOAD *                               \
                INV_CTRL_AC_VOLT_CODE_PER_V / INV_CTRL_IND_CURR_CODE_PER_A) +                      \
               0.5f))
#define INV_CTRL_CAP_DIFF_GAIN_Q_SHIFT (16U)

void inv_ctrl_set_p_hal(inv_ctrl_hal_t *p_hal);
void inv_ctrl_prepare_run(void);

#endif
