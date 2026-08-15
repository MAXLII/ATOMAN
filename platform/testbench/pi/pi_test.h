// SPDX-License-Identifier: MIT
/**
 * @file    pi_test.h
 * @brief   PI current-loop testbench fixture definitions.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the ideal 1/(sL) current-loop test environment
 *          - Hold the production PI and SFRA instances used by host tests
 *          - Share deterministic step-response and frequency-sweep state
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Uses the production PI, DFT, and SFRA implementations directly
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

#ifndef PI_TEST_H
#define PI_TEST_H

#include <stdint.h>

#include "pi_tustin.h"
#include "sfra_core.h"

typedef enum
{
    PI_TEST_STEP_RESPONSE_E = 0,
    PI_TEST_SFRA_CLOSED_LOOP_E,
    PI_TEST_SFRA_OPEN_LOOP_E
} PI_TEST_SCENARIO_E;

typedef struct pi_test_fixture
{
    pi_tustin_t dut;
    sfra_t sfra;
    PI_TEST_SCENARIO_E scenario;
    float current_ref_a;
    float current_a;
    float pi_feedback_a;
    float voltage_v;
    float sfra_inject_a;
    float sfra_collect_a;
    float crossover_freq_hz;
    float crossover_magnitude;
    float crossover_phase_deg;
    float max_current_a;
    uint32_t stable_sample_count;
    uint16_t sfra_recorded_point_count;
    uint8_t dut_init_ok;
    uint8_t sfra_init_ok;
    uint8_t stable_reached;
    uint8_t plant_input_valid; /**< 1 when voltage_v is ready to update the next beat's inductor current. */
} pi_test_fixture_t;

#endif /* PI_TEST_H */
