// SPDX-License-Identifier: MIT
/**
 * @file    bsp_adc.c
 * @brief   PLECS CLLC ADC adapter implementation.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Map CLLC BSP battery-voltage reads to one DLL input
 *          - Map CLLC BSP battery-current reads to one DLL input
 *          - Map CLLC BSP bus-voltage reads to one DLL input
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Contains no filtering or cached state
 *          - PLECS common bridge validates input indexes
 *
 * @author  Max.Li
 * @date    2026-07-26
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "bsp_adc.h"

#include "plecs.h"

float bsp_adc_get_battery_voltage(void)
{
    return plecs_get_input(PLECS_INPUT_V_BATTERY);
}

float bsp_adc_get_battery_current(void)
{
    return plecs_get_input(PLECS_INPUT_I_BATTERY);
}

float bsp_adc_get_bus_voltage(void)
{
    return plecs_get_input(PLECS_INPUT_V_BUS);
}
