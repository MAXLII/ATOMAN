// SPDX-License-Identifier: MIT
/**
 * @file    cllc_hw_param.h
 * @brief   CLLC hardware and rated-plant parameter definitions.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the bidirectional CLLC resonant-tank component values
 *          - Define battery, bus, power, current, and output-capacitor ratings
 *          - Provide derived resonant-frequency and bus-reference formulas
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Contains no hardware register access
 *          - Values match the MATLAB FHA controller-design model
 *
 * @author  Max.Li
 * @date    2026-07-26
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __CLLC_HW_PARAM_H
#define __CLLC_HW_PARAM_H

#include <math.h>

/* Resonant tank shown in the CLLC schematic. */
#define CLLC_HW_PRIMARY_RESONANT_IND_H             (40.0e-6f)
#define CLLC_HW_PRIMARY_RESONANT_CAP_F             (80.0e-9f)
#define CLLC_HW_PRIMARY_MAGNETIZING_IND_H          (200.0e-6f)
#define CLLC_HW_SECONDARY_RESONANT_IND_H           (0.625e-6f)
#define CLLC_HW_SECONDARY_RESONANT_CAP_F           (5.12e-6f)
#define CLLC_HW_TRANSFORMER_TURNS_RATIO            (8.0f)

/* DC-side capacitors and product ratings. */
#define CLLC_HW_BATTERY_OUTPUT_CAP_F               (10.0e-3f)
#define CLLC_HW_BUS_OUTPUT_CAP_F                   (1360.0e-6f)
#define CLLC_HW_RATED_POWER_W                      (6600.0f)
#define CLLC_HW_RATED_BATTERY_CURRENT_A            (150.0f)
#define CLLC_HW_FORWARD_CURRENT_LIMIT_A            (1.10f * CLLC_HW_RATED_BATTERY_CURRENT_A)
#define CLLC_HW_BATTERY_VOLTAGE_MIN_V              (24.0f)
#define CLLC_HW_BATTERY_VOLTAGE_NOMINAL_V          (48.0f)
#define CLLC_HW_BATTERY_VOLTAGE_MAX_V              (72.0f)
#define CLLC_HW_BUS_VOLTAGE_MIN_V                  (400.0f)
#define CLLC_HW_BUS_VOLTAGE_NOMINAL_V              (450.0f)
#define CLLC_HW_BUS_VOLTAGE_MAX_V                  (500.0f)

/* Forward pre-regulated bus law retained from the MATLAB design. */
#define CLLC_HW_FORWARD_BUS_GAIN                    (8.0f)
#define CLLC_HW_FORWARD_BUS_OFFSET_V                (25.0f)
#define CLLC_HW_FORWARD_BUS_MIN_V                   (370.0f)
#define CLLC_HW_MAX_FLOAT(a, b)                     (((a) > (b)) ? (a) : (b))
#define CLLC_HW_FORWARD_BUS_REF_V(v_battery_ref_v)  \
    CLLC_HW_MAX_FLOAT(CLLC_HW_FORWARD_BUS_MIN_V,    \
                      (CLLC_HW_FORWARD_BUS_GAIN * (v_battery_ref_v)) + \
                          CLLC_HW_FORWARD_BUS_OFFSET_V)

/* Resonant and modulation-frequency limits. */
#define CLLC_HW_TWO_PI                              (6.2831853071795864769f)
#define CLLC_HW_PRIMARY_RESONANT_FREQ_HZ            \
    (1.0f / (CLLC_HW_TWO_PI *                       \
             sqrtf(CLLC_HW_PRIMARY_RESONANT_IND_H * CLLC_HW_PRIMARY_RESONANT_CAP_F)))
#define CLLC_HW_FORWARD_MAX_FREQ_HZ                 (2.0f * CLLC_HW_PRIMARY_RESONANT_FREQ_HZ)
#define CLLC_HW_REVERSE_MIN_FREQ_HZ                 (32000.0f)
#define CLLC_HW_FORWARD_PSM_PFM_TRANSITION_U         (0.241267563f)
#define CLLC_HW_REVERSE_PSM_PFM_TRANSITION_U         (0.733686703f)
#define CLLC_HW_MAX_PHASE_SHIFT_DUTY                 (0.5f)

#endif /* __CLLC_HW_PARAM_H */
