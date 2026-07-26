// SPDX-License-Identifier: MIT
/**
 * @file    adc.h
 * @brief   CLLC analog-feedback interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Expose physical battery-voltage feedback to CLLC application glue
 *          - Expose physical battery-current feedback to CLLC application glue
 *          - Expose physical high-voltage-bus feedback to CLLC application glue
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Read functions are safe for control-interrupt use after BSP setup
 *          - Platform-specific acquisition remains in bsp_adc
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
#ifndef __CLLC_ADC_H
#define __CLLC_ADC_H

/** @brief Read the battery-port voltage. @return Physical voltage in volts. */
float cllc_adc_get_battery_voltage(void);

/** @brief Read the battery/load current magnitude. @return Physical current in amperes. */
float cllc_adc_get_battery_current(void);

/** @brief Read the high-voltage DC bus. @return Physical voltage in volts. */
float cllc_adc_get_bus_voltage(void);

#endif /* __CLLC_ADC_H */
