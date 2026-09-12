// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.h
 * @brief   NPC PLECS duty and enable publisher.
 * @details
 *          This file is part of the base digital power framework project.
 *          Publish 2 state-based duties per phase; PLECS handles complementary gates and dead time.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef PLECS_NPC_BSP_PWM_H
#define PLECS_NPC_BSP_PWM_H
#include <stdbool.h>
#define BSP_PWM_PHASE_COUNT 3u /* Phases A, B and C in publication order. */
#define BSP_PWM_CHANNEL_COUNT (2u * BSP_PWM_PHASE_COUNT) /* Positive and negative duties per phase. */
typedef struct bsp_pwm_phase_duty
{
    float positive_duty; /* P-state fraction, [0,1]; Q1 on-time before dead time. */
    float negative_duty; /* N-state fraction, [0,1]; Q4 on-time before dead time; P+N <= 1. */
} bsp_pwm_phase_duty_t;
/** @param p_duty Complete array of BSP_PWM_PHASE_COUNT phase pairs, ordered A/B/C.
 *  @return true when all duties are published and bridge enable is asserted.
 *  NULL or invalid duty disables the entire bridge. */
bool bsp_pwm_set_duty(const bsp_pwm_phase_duty_t *p_duty);
/** @brief Publish 6 zero duties and deassert the bridge enable. */
void bsp_pwm_disable(void);
#endif /* PLECS_NPC_BSP_PWM_H */
