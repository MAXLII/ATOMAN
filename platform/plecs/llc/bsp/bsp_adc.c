// SPDX-License-Identifier: MIT
/**
 * @file    bsp_adc.c
 * @brief   PLECS LLC ADC adapter module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Keep a C translation unit for the PLECS BSP ADC module
 *          - Leave analog input reads in the public macros declared by bsp_adc.h
 *          - Avoid interrupt registration and cached measurement state in the BSP layer
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
#include "bsp_adc.h"
