// SPDX-License-Identifier: MIT
/**
 * @file    buck_ctrl.h
 * @brief   buck_ctrl control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare buck controller public runtime hooks
 *          - Expose controller preparation before entering run state
 *          - Provide the interface shared by buck configuration, HAL binding, and runtime control
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-23
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __BUCK_CTRL_H
#define __BUCK_CTRL_H

#include "buck_hal.h"

void buck_ctrl_set_p_hal(buck_ctrl_hal_t *p);
void buck_ctrl_prepare_run(void);

#endif
