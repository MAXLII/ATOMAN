// SPDX-License-Identifier: MIT
/**
 * @file    llc_ctrl.h
 * @brief   LLC control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare LLC controller runtime hooks
 *          - Expose controller preparation before entering run state
 *          - Provide the interface shared by LLC configuration, HAL binding, and runtime control
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-10
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __LLC_CTRL_H
#define __LLC_CTRL_H

#include "llc_hal.h"
#include <stdint.h>

typedef struct
{
    float ref;
    float fbk;
    float out;
    float out_ff_norm;
} llc_ctrl_pi_debug_t;

void llc_ctrl_set_p_hal(llc_ctrl_hal_t *p);
void llc_ctrl_prepare_run(void);
uint8_t llc_ctrl_get_enable(void);
void llc_ctrl_get_pi_debug(llc_ctrl_pi_debug_t *p_debug);

#endif
