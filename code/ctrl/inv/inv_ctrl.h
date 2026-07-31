// SPDX-License-Identifier: MIT
/**
 * @file    inv_ctrl.h
 * @brief   inv_ctrl control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define inverter controller HAL signals, references, and loop-control structures
 *          - Expose controller preparation, stepping, and output-generation APIs
 *          - Provide the public interface between inverter control logic and platform HAL callbacks
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __INV_CTRL_H
#define __INV_CTRL_H

#include "inv_hal.h"
#include "hw_params.h"
#include "my_math.h"

#define INV_CTRL_RATED_POWER_W (6600.0f)
#define INV_CTRL_NOMINAL_RMS_V (230.0f)
#define INV_CTRL_NOMINAL_FREQ_HZ (50.0f)
#define INV_CTRL_FILTER_IND_RES_OHM (0.001f)
#define INV_CTRL_NOMINAL_LOAD_OHM \
    ((INV_CTRL_NOMINAL_RMS_V * INV_CTRL_NOMINAL_RMS_V) / INV_CTRL_RATED_POWER_W)

#define INV_CTRL_INNER_LOOP_BANDWIDTH_HZ (4000.0f)
#define INV_CTRL_INNER_LOOP_BANDWIDTH_RADPS (M_2PI * INV_CTRL_INNER_LOOP_BANDWIDTH_HZ)
#define INV_CTRL_INNER_LOOP_SQRT_TERM                                                         \
    ((2.0f * INV_CTRL_FILTER_IND_RES_OHM * HW_AC_SIDE_CAP_VALUE * INV_CTRL_NOMINAL_LOAD_OHM * \
      ((INV_CTRL_FILTER_IND_RES_OHM * HW_AC_SIDE_CAP_VALUE * INV_CTRL_NOMINAL_LOAD_OHM) +     \
       HW_AC_SIDE_IND_VALUE)) +                                                               \
     (HW_AC_SIDE_IND_VALUE * HW_AC_SIDE_IND_VALUE *                                           \
      (2.0f +                                                                                 \
       (HW_AC_SIDE_CAP_VALUE * HW_AC_SIDE_CAP_VALUE *                                         \
        INV_CTRL_NOMINAL_LOAD_OHM * INV_CTRL_NOMINAL_LOAD_OHM *                               \
        INV_CTRL_INNER_LOOP_BANDWIDTH_RADPS * INV_CTRL_INNER_LOOP_BANDWIDTH_RADPS))))
#define INV_CTRL_INNER_LOOP_K                                                            \
    ((HW_AC_SIDE_IND_VALUE +                                                             \
      (INV_CTRL_FILTER_IND_RES_OHM * HW_AC_SIDE_CAP_VALUE * INV_CTRL_NOMINAL_LOAD_OHM) + \
      sqrtf(INV_CTRL_INNER_LOOP_SQRT_TERM)) /                                            \
     (HW_AC_SIDE_CAP_VALUE * INV_CTRL_NOMINAL_LOAD_OHM))

#define INV_CTRL_VOLT_LOOP_BANDWIDTH_HZ (1300.0f)
#define INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS (M_2PI * INV_CTRL_VOLT_LOOP_BANDWIDTH_HZ)
#define INV_CTRL_VOLT_LOOP_KP                                                           \
    (HW_AC_SIDE_CAP_VALUE * INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS *                        \
     (sqrtf((2.0f * HW_AC_SIDE_IND_VALUE * HW_AC_SIDE_IND_VALUE *                       \
             INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS * INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS) + \
            (INV_CTRL_INNER_LOOP_K * INV_CTRL_INNER_LOOP_K)) -                          \
      (HW_AC_SIDE_IND_VALUE * INV_CTRL_VOLT_LOOP_BANDWIDTH_RADPS)) /                    \
     INV_CTRL_INNER_LOOP_K)
#define INV_CTRL_VOLT_LOOP_KI_RATIO (0.55f)
#define INV_CTRL_VOLT_LOOP_KI \
    (INV_CTRL_VOLT_LOOP_KI_RATIO * INV_CTRL_VOLT_LOOP_KP * M_2PI * INV_CTRL_NOMINAL_FREQ_HZ)
#define INV_CTRL_VOLT_LOOP_OUT_MAX_A (20.0f)
#define INV_CTRL_VOLT_LOOP_OUT_MIN_A (-20.0f)

#define INV_CTRL_HARMONIC_3_ORDER (3.0f)
#define INV_CTRL_HARMONIC_5_ORDER (5.0f)
#define INV_CTRL_HARMONIC_7_ORDER (7.0f)
#define INV_CTRL_HARMONIC_3_GAIN (0.30f)
#define INV_CTRL_HARMONIC_5_GAIN (0.20f)
#define INV_CTRL_HARMONIC_7_GAIN (0.15f)
#define INV_CTRL_HARMONIC_OUT_MAX_A (8.0f)
#define INV_CTRL_HARMONIC_OUT_MIN_A (-8.0f)

void inv_ctrl_set_p_hal(inv_ctrl_hal_t *p);
void inv_ctrl_prepare_run(void);

#endif
