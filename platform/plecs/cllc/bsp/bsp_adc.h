// SPDX-License-Identifier: MIT
/**
 * @file    bsp_adc.h
 * @brief   PLECS CLLC ADC adapter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Read battery voltage from the PLECS DLL input vector
 *          - Read battery current from the PLECS DLL input vector
 *          - Read high-voltage-bus voltage from the PLECS DLL input vector
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Values are physical floating-point quantities
 *          - Reads are stateless and suitable for control-interrupt use
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
#ifndef __PLECS_CLLC_BSP_ADC_H
#define __PLECS_CLLC_BSP_ADC_H

/** @brief Read battery voltage from PLECS. @return Battery voltage in volts. */
float bsp_adc_get_battery_voltage(void);

/** @brief Read battery current from PLECS. @return Current magnitude in amperes. */
float bsp_adc_get_battery_current(void);

/** @brief Read high-voltage bus from PLECS. @return Bus voltage in volts. */
float bsp_adc_get_bus_voltage(void);

#endif /* __PLECS_CLLC_BSP_ADC_H */
