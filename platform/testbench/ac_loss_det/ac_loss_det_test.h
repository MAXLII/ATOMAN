// SPDX-License-Identifier: MIT
/**
 * @file    ac_loss_det_test.h
 * @brief   AC loss detector testbench fixture definitions.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Define deterministic AC loss detector test scenarios
 *          - Hold caller-owned DUT inputs, state, and process-recording data
 *          - Reference the production ac_loss_det public interface directly
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Uses the production ac_loss_det_t instance without copying the DUT
 *          - Runs only on the host testbench and is never called from an ISR
 *
 * @author  Max.Li
 * @date    2026-08-16
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef AC_LOSS_DET_TEST_H
#define AC_LOSS_DET_TEST_H

#include <stdint.h>

#include "ac_loss_det.h"

/**
 * @brief Input behavior selected by each registered test case.
 */
typedef enum
{
    AC_LOSS_DET_TEST_AC_UNAVAILABLE_E = 0, /**< Positive voltage while the AC-valid input is inactive. */
    AC_LOSS_DET_TEST_HEALTHY_WAVE_E,       /**< Repeating periodic voltage establishing healthy AC. */
    AC_LOSS_DET_TEST_FROZEN_WAVE_E,        /**< Healthy voltage followed by a frozen zero input. */
    AC_LOSS_DET_TEST_RESET_E               /**< Direct exercise of the public reset operation. */
} AC_LOSS_DET_TEST_SCENARIO_E;

/**
 * @brief Complete fixture shared by the DUT callbacks and case callbacks.
 */
typedef struct
{
    ac_loss_det_t dut;                      /**< Production AC loss detector instance under test. */
    float voltage_v;                        /**< Simulated instantaneous AC input voltage, in V. */
    uint8_t ac_is_ok;                       /**< Simulated normalized upstream AC-valid input. */
    AC_LOSS_DET_TEST_SCENARIO_E scenario;   /**< Input behavior selected for the active case. */
    uint32_t sample_index;                  /**< Number of voltage samples applied to the DUT. */
    uint32_t last_recorded_state;           /**< State value emitted by the previous record callback. */
    uint32_t last_recorded_ovf_diff_count;  /**< Difference counter emitted by the previous record callback. */
    uint8_t last_recorded_loss;             /**< Loss flag emitted by the previous record callback. */
    uint8_t healthy_seen;                   /**< 1 after the DUT has declared the AC input healthy. */
} ac_loss_det_test_fixture_t;

#endif /* AC_LOSS_DET_TEST_H */
