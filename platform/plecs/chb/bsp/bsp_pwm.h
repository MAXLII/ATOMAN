// SPDX-License-Identifier: MIT
/**
 * @file bsp_pwm.h
 * @brief Publish three CHB duties and the common PWM enable to PLECS.
 * @details Input duties are normalized to 0..1 by the CHB interface layer.
 *          Relay output is controlled separately by the platform FSM adapter.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef PLECS_CHB_BSP_PWM_H
#define PLECS_CHB_BSP_PWM_H

#include <stdint.h>

#define BSP_PWM_CELL_COUNT 3u /* Three cascaded H bridges. */

/**
 * @brief Publish duties and enable delayed together by one control cycle.
 * @param p_duty Three leg-A duties in CHB1, CHB2, CHB3 order, each in 0..1.
 * @param enable 1 enables all bridges; 0 disables all bridges and resets cached duties to 0.5.
 */
void bsp_pwm_set(const float p_duty[BSP_PWM_CELL_COUNT], uint8_t enable);

#endif /* PLECS_CHB_BSP_PWM_H */
