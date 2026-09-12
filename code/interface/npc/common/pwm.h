// SPDX-License-Identifier: MIT
/**
 * @file    pwm.h
 * @brief   NPC modulation-to-gate interface.
 * @details
 *          This file is part of the base digital power framework project.
 *          Translate SVPWM dwell ratios into P and N duty pairs for downstream modulation.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef NPC_PWM_H
#define NPC_PWM_H
#include <stdbool.h>
#include "svpwm_3level.h"

/** @param v_dc_half_min Minimum valid voltage of each half bus in V.
 *  @return true when the modulator is initialized; outputs remain disabled. */
bool pwm_init(float v_dc_half_min);
/** @param p_input Coherent voltage snapshot captured at the 5 kHz control update.
 *  @return Modulation status; any failure immediately disables all gates. */
SVPWM_3LEVEL_STATUS_E pwm_update(const svpwm_3level_input_t *p_input);
/** @brief Disable all gates and invalidate modulation output; update is needed to restart. */
void pwm_disable(void);
#endif /* NPC_PWM_H */
