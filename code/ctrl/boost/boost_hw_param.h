// SPDX-License-Identifier: MIT
/**
 * @file    boost_hw_param.h
 * @brief   BOOST hardware parameter definitions.
 * @details
 *          This file is part of the BOOST2 project.
 *
 *          Module responsibilities:
 *          - Define BOOST power-stage capacitor and inductor constants
 *          - Keep component count, per-device value, and total equivalent value explicit
 *          - Provide hardware parameters for control calculation and design checks
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-24
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __BOOST_HW_PARAM_H
#define __BOOST_HW_PARAM_H

#define BOOST_HW_IN_CAP_PER (330.0e-6f)
#define BOOST_HW_IN_CAP_PCS (4U)
#define BOOST_HW_IN_CAP_TOTAL (BOOST_HW_IN_CAP_PER * (float)BOOST_HW_IN_CAP_PCS)

#define BOOST_HW_OUT_CAP_PER (1000.0e-6f)
#define BOOST_HW_OUT_CAP_PCS (4U)
#define BOOST_HW_OUT_CAP_TOTAL (BOOST_HW_OUT_CAP_PER * (float)BOOST_HW_OUT_CAP_PCS)

#define BOOST_HW_IND (1.5e-6f)

#endif
