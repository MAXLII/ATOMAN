// SPDX-License-Identifier: MIT
/**
 * @file    bsp_adc.h
 * @brief   PLECS inverter ADC adapter module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Map PLECS input signals to inverter ADC feedback names
 *          - Provide capacitor voltage, bus voltage, and inductor-current feedback macros
 *          - Keep inverter simulation ADC access behind the BSP naming boundary
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-19
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef PLECS_INV_BSP_ADC_H
#define PLECS_INV_BSP_ADC_H

#include "plecs.h"

#define BSP_ADC_V_CAP plecs_get_input(PLECS_INPUT_V_CAP)
#define BSP_ADC_I_L plecs_get_input(PLECS_INPUT_I_L)
#define BSP_ADC_V_BUS plecs_get_input(PLECS_INPUT_V_BUS)

#endif
