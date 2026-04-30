// SPDX-License-Identifier: MIT
/**
 * @file    bb_ctrl.h
 * @brief   bb_ctrl control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define buck-boost controller HAL callbacks, measurements, and setpoint structures
 *          - Expose controller preparation, stepping, and protection-check APIs
 *          - Provide the interface shared by buck-boost configuration, HAL binding, and runtime control
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
#ifndef __BB_CTRL_H
#define __BB_CTRL_H

#include "bb_hal.h"
#include "hw_params.h"
#include "my_math.h"

#define BB_CTRL_BUCK_TO_BUCK_BOOST_THR (0.9f)
#define BB_CTRL_BUCK_BOOST_TO_BUCK_THR (0.85f)
#define BB_CTRL_BUCK_BOOST_TO_BOOST_THR (1.17f)
#define BB_CTRL_BOOST_TO_BUCK_BOOST_THR (1.11f)

#define BB_CTRL_OUT_VOLT_LOOP_FREQ_CUT (500.0f)
#define BB_CTRL_OUT_VOLT_LOOP_PM (60.0f)
#define BB_CTRL_OUT_VOLT_LOOP_W_CUT (2.0f * M_PI * BB_CTRL_OUT_VOLT_LOOP_FREQ_CUT)
#define BB_CTRL_OUT_VOLT_LOOP_KP (sinf(BB_CTRL_OUT_VOLT_LOOP_PM * M_PI / 180.0f) * BB_CTRL_OUT_VOLT_LOOP_W_CUT * HW_BUCK_BOOST_OUTPUT_CAP_VALUE)
#define BB_CTRL_OUT_VOLT_LOOP_KI (BB_CTRL_OUT_VOLT_LOOP_W_CUT * BB_CTRL_OUT_VOLT_LOOP_KP / tanf(BB_CTRL_OUT_VOLT_LOOP_PM * M_PI / 180.0f))
#define BB_CTRL_OUT_VOLT_LOOP_UP_LMT (50.0f)
#define BB_CTRL_OUT_VOLT_LOOP_DN_LMT (-5.0f)

#define BB_CTRL_IN_VOLT_LMT_LOOP_FREQ_CUT (500.0f)
#define BB_CTRL_IN_VOLT_LMT_LOOP_PM (60.0f)
#define BB_CTRL_IN_VOLT_LMT_LOOP_W_CUT (2.0f * M_PI * BB_CTRL_IN_VOLT_LMT_LOOP_FREQ_CUT)
#define BB_CTRL_IN_VOLT_LMT_LOOP_KP (sinf(BB_CTRL_IN_VOLT_LMT_LOOP_PM * M_PI / 180.0f) * BB_CTRL_IN_VOLT_LMT_LOOP_W_CUT * HW_BUCK_BOOST_INPUT_CAP_VALUE)
#define BB_CTRL_IN_VOLT_LMT_LOOP_KI (BB_CTRL_IN_VOLT_LMT_LOOP_W_CUT * BB_CTRL_IN_VOLT_LMT_LOOP_KP / tanf(BB_CTRL_IN_VOLT_LMT_LOOP_PM * M_PI / 180.0f))
#define BB_CTRL_IN_VOLT_LMT_LOOP_UP_LMT (50.0f)
#define BB_CTRL_IN_VOLT_LMT_LOOP_DN_LMT (-5.0f)

#define BB_CTRL_IN_CURR_LOOP_FREQ_CUT (500.0f)
#define BB_CTRL_IN_CURR_LOOP_PM (60.0f)
#define BB_CTRL_IN_CURR_LOOP_W_CUT (2.0f * M_PI * BB_CTRL_IN_CURR_LOOP_FREQ_CUT)
#define BB_CTRL_IN_CURR_LOOP_KP (sinf(BB_CTRL_IN_CURR_LOOP_PM * M_PI / 180.0f) * BB_CTRL_IN_CURR_LOOP_W_CUT * HW_BUCK_BOOST_IND_VALUE)
#define BB_CTRL_IN_CURR_LOOP_KI (BB_CTRL_IN_CURR_LOOP_W_CUT * BB_CTRL_IN_CURR_LOOP_KP / tanf(BB_CTRL_IN_CURR_LOOP_PM * M_PI / 180.0f))
#define BB_CTRL_IN_CURR_LOOP_UP_LMT (50.0f)
#define BB_CTRL_IN_CURR_LOOP_DN_LMT (-5.0f)

#define BB_CTRL_OUT_CURR_LOOP_FREQ_CUT (500.0f)
#define BB_CTRL_OUT_CURR_LOOP_PM (60.0f)
#define BB_CTRL_OUT_CURR_LOOP_W_CUT (2.0f * M_PI * BB_CTRL_OUT_CURR_LOOP_FREQ_CUT)
#define BB_CTRL_OUT_CURR_LOOP_KP (sinf(BB_CTRL_OUT_CURR_LOOP_PM * M_PI / 180.0f) * BB_CTRL_OUT_CURR_LOOP_W_CUT * HW_BUCK_BOOST_IND_VALUE)
#define BB_CTRL_OUT_CURR_LOOP_KI (BB_CTRL_OUT_CURR_LOOP_W_CUT * BB_CTRL_OUT_CURR_LOOP_KP / tanf(BB_CTRL_OUT_CURR_LOOP_PM * M_PI / 180.0f))
#define BB_CTRL_OUT_CURR_LOOP_UP_LMT (50.0f)
#define BB_CTRL_OUT_CURR_LOOP_DN_LMT (-5.0f)

#define BB_CTRL_IND_CURR_LOOP_FREQ_CUT (2500.0f)
#define BB_CTRL_IND_CURR_LOOP_PM (45.0f)
#define BB_CTRL_IND_CURR_LOOP_W_CUT (2.0f * M_PI * BB_CTRL_IND_CURR_LOOP_FREQ_CUT)
#define BB_CTRL_IND_CURR_LOOP_KP (sinf(BB_CTRL_IND_CURR_LOOP_PM * M_PI / 180.0f) * BB_CTRL_IND_CURR_LOOP_W_CUT * HW_BUCK_BOOST_IND_VALUE)
#define BB_CTRL_IND_CURR_LOOP_KI (BB_CTRL_IND_CURR_LOOP_W_CUT * BB_CTRL_IND_CURR_LOOP_KP / tanf(BB_CTRL_IND_CURR_LOOP_PM * M_PI / 180.0f))
#define BB_CTRL_IND_CURR_LOOP_UP_LMT (50.0f)
#define BB_CTRL_IND_CURR_LOOP_DN_LMT (-50.0f)

/**
 * @brief Bind the control module to its HAL object.
 * @param p Pointer to the HAL instance. Passing NULL detaches the controller.
 * @return None.
 */
void bb_ctrl_set_p_hal(bb_ctrl_hal_t *p);

/**
 * @brief Reinitialize controller internal states before entering run.
 * @param None.
 * @return None.
 */
void bb_ctrl_prepare_run(void);

#endif
