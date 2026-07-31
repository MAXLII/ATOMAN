// SPDX-License-Identifier: MIT
/**
 * @file    trig_i32.h
 * @brief   Integer phase-to-sine/cosine lookup public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Convert a Q32 per-unit phase into Q15 sine and cosine values
 *          - Reconstruct all quadrants from a quarter-wave lookup table
 *          - Apply fixed-point linear interpolation without runtime division
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Runtime path uses no division
 *          - No hardware access
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
#ifndef TRIG_I32_H
#define TRIG_I32_H

#include <stdint.h>

#define TRIG_I32_Q15_ONE (32767)

void trig_i32_sin_cos_q15(uint32_t phase_q32, int32_t *p_sin_q15, int32_t *p_cos_q15);

#endif
