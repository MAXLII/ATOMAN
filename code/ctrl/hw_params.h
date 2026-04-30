// SPDX-License-Identifier: MIT
/**
 * @file    hw_params.h
 * @brief   hw_params control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define shared hardware constants used by control-loop calculations
 *          - Centralize DC-bus, AC-side, and buck-boost passive component values
 *          - Provide compile-time plant parameters without introducing hardware access in control code
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
#ifndef __HW_PARAMS_H
#define __HW_PARAMS_H

#define HW_DC_BUS_CAP_VALUE 1300e-6f
#define HW_AC_SIDE_IND_VALUE 440e-6f
#define HW_AC_SIDE_CAP_VALUE 12e-6f

#define HW_BUCK_BOOST_INPUT_CAP_VALUE 0.0f
#define HW_BUCK_BOOST_OUTPUT_CAP_VALUE 0.0f
#define HW_BUCK_BOOST_IND_VALUE 0.0f

#endif
