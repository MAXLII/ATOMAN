// SPDX-License-Identifier: MIT
/**
 * @file    adc.c
 * @brief   CLLC analog-feedback interface implementation.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Forward battery-voltage reads to the active BSP
 *          - Forward battery-current reads to the active BSP
 *          - Forward high-voltage-bus reads to the active BSP
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Contains no filtering or unit conversion
 *          - BSP values are already expressed in physical units
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
#include "adc.h"

#include "bsp_adc.h"

float cllc_adc_get_battery_voltage(void)
{
    return bsp_adc_get_battery_voltage();
}

float cllc_adc_get_battery_current(void)
{
    return bsp_adc_get_battery_current();
}

float cllc_adc_get_bus_voltage(void)
{
    return bsp_adc_get_bus_voltage();
}
