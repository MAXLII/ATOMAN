// SPDX-License-Identifier: MIT
/**
 * @file    pfc_ctrl.h
 * @brief   PFC int32 controller public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare PFC int32 controller runtime hooks
 *          - Expose controller preparation before entering run state
 *          - Provide the interface shared by PFC int32 configuration and HAL binding
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __PFC_I32_CTRL_H
#define __PFC_I32_CTRL_H

#include "pfc_hal.h"

void pfc_i32_ctrl_set_p_hal(pfc_i32_ctrl_hal_t *p);
void pfc_i32_ctrl_prepare_run(void);

#endif
