// SPDX-License-Identifier: MIT
/**
 * @file    inv_ctrl.h
 * @brief   inv_ctrl control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define inverter controller HAL signals, references, and loop-control structures
 *          - Expose controller preparation, stepping, and output-generation APIs
 *          - Provide the public interface between inverter control logic and platform HAL callbacks
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __INV_CTRL_H
#define __INV_CTRL_H

#include "inv_hal.h"
#include "hw_params.h"
#include "my_math.h"

#define INV_CTRL_VOLT_LOOP_FREQ_CUT (300.0f)
#define INV_CTRL_VOLT_LOOP_PM (60.0f)

#define INV_CTRL_VOLT_LOOP_D_FREQ_CUT INV_CTRL_VOLT_LOOP_FREQ_CUT
#define INV_CTRL_VOLT_LOOP_D_W_CUT (2.0f * M_PI * INV_CTRL_VOLT_LOOP_D_FREQ_CUT)
#define INV_CTRL_VOLT_LOOP_D_PM INV_CTRL_VOLT_LOOP_PM
#define INV_CTRL_VOLT_LOOP_D_KP (sinf(INV_CTRL_VOLT_LOOP_D_PM / 180.0f * M_PI) * INV_CTRL_VOLT_LOOP_D_W_CUT * HW_AC_SIDE_CAP_VALUE)
#define INV_CTRL_VOLT_LOOP_D_KI (INV_CTRL_VOLT_LOOP_D_KP * INV_CTRL_VOLT_LOOP_D_W_CUT / tanf(INV_CTRL_VOLT_LOOP_D_PM / 180.0f * M_PI))

#define INV_CTRL_VOLT_LOOP_Q_FREQ_CUT INV_CTRL_VOLT_LOOP_FREQ_CUT
#define INV_CTRL_VOLT_LOOP_Q_W_CUT (2.0f * M_PI * INV_CTRL_VOLT_LOOP_Q_FREQ_CUT)
#define INV_CTRL_VOLT_LOOP_Q_PM INV_CTRL_VOLT_LOOP_PM
#define INV_CTRL_VOLT_LOOP_Q_KP (sinf(INV_CTRL_VOLT_LOOP_Q_PM / 180.0f * M_PI) * INV_CTRL_VOLT_LOOP_Q_W_CUT * HW_AC_SIDE_CAP_VALUE)
#define INV_CTRL_VOLT_LOOP_Q_KI (INV_CTRL_VOLT_LOOP_Q_KP * INV_CTRL_VOLT_LOOP_Q_W_CUT / tanf(INV_CTRL_VOLT_LOOP_Q_PM / 180.0f * M_PI))

#define INV_CTRL_CURR_LOOP_FREQ_CUT (1500.0f)
#define INV_CTRL_CURR_LOOP_PM (45.0f)

#define INV_CTRL_CURR_LOOP_D_FREQ_CUT INV_CTRL_CURR_LOOP_FREQ_CUT
#define INV_CTRL_CURR_LOOP_D_W_CUT (2.0f * M_PI * INV_CTRL_CURR_LOOP_D_FREQ_CUT)
#define INV_CTRL_CURR_LOOP_D_PM INV_CTRL_CURR_LOOP_PM
#define INV_CTRL_CURR_LOOP_D_KP (sinf(INV_CTRL_CURR_LOOP_D_PM / 180.0f * M_PI) * INV_CTRL_CURR_LOOP_D_W_CUT * HW_AC_SIDE_IND_VALUE)
#define INV_CTRL_CURR_LOOP_D_KI (0.0f)

#define INV_CTRL_CURR_LOOP_Q_FREQ_CUT INV_CTRL_CURR_LOOP_FREQ_CUT
#define INV_CTRL_CURR_LOOP_Q_W_CUT (2.0f * M_PI * INV_CTRL_CURR_LOOP_Q_FREQ_CUT)
#define INV_CTRL_CURR_LOOP_Q_PM INV_CTRL_CURR_LOOP_PM
#define INV_CTRL_CURR_LOOP_Q_KP (sinf(INV_CTRL_CURR_LOOP_Q_PM / 180.0f * M_PI) * INV_CTRL_CURR_LOOP_Q_W_CUT * HW_AC_SIDE_IND_VALUE)
#define INV_CTRL_CURR_LOOP_Q_KI (0.0f)

void inv_ctrl_set_p_hal(inv_ctrl_hal_t *p);
void inv_ctrl_prepare_run(void);

#endif
