// SPDX-License-Identifier: MIT
/**
 * @file    bsp_adc.h
 * @brief   PLECS boost ADC adapter public interface.
 * @details
 *          This file is part of the BUCK2 project.
 *
 *          Module responsibilities:
 *          - Define raw ADC code macros for the PLECS simulation project
 *          - Convert physical PLECS inputs to bounded uint32_t ADC code values
 *          - Keep the BSP ADC layer stateless and free of interrupt registration
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
/* Header guard for the PLECS boost ADC BSP macro interface. */
#ifndef __BSP_ADC_H

/* Header guard marker for the PLECS boost ADC BSP macro interface. */
#define __BSP_ADC_H

#include "plecs.h"
#include <stdint.h>

/* Lower bound of the voltage sampling physical domain. */
#define BSP_ADC_VOLT_MIN_V (0.0f)

/* Upper bound of the voltage sampling physical domain. */
#define BSP_ADC_VOLT_MAX_V (60.0f)

/* Maximum raw ADC code for voltage sampling macros. */
#define BSP_ADC_VOLT_CODE_MAX (4095UL)

/* Lower bound of the inductor-current sampling physical domain. */
#define BSP_ADC_IND_CURR_MIN_A (-100.0f)

/* Upper bound of the inductor-current sampling physical domain. */
#define BSP_ADC_IND_CURR_MAX_A (100.0f)

/* Maximum raw ADC code for inductor-current sampling macros. */
#define BSP_ADC_IND_CURR_CODE_MAX (4096UL * 8UL - 1UL)

/* Convert a bounded physical input value into an unsigned ADC raw code. */
static inline uint32_t bsp_adc_physical_to_code(float val, float min, float max, uint32_t code_max)
{
    /* Raw ADC code before integer conversion. */
    float code = 0.0f;

    /* Physical span used by the normalization calculation. */
    float span = 0.0f;

    if ((max <= min) || (code_max == 0UL))
    {
        return 0U;
    }

    if (val <= min)
    {
        return 0U;
    }

    if (val >= max)
    {
        return code_max;
    }

    span = max - min;
    code = ((val - min) / span) * (float)code_max;

    return (uint32_t)code;
}

/* High-voltage input raw ADC code calculated from the PLECS signal. */
#define BSP_ADC_HV                                             \
    (bsp_adc_physical_to_code(plecs_get_input(PLECS_INPUT_HV), \
                              BSP_ADC_VOLT_MIN_V,              \
                              BSP_ADC_VOLT_MAX_V,              \
                              BSP_ADC_VOLT_CODE_MAX))

/* Low-voltage output raw ADC code calculated from the PLECS signal. */
#define BSP_ADC_LV                                             \
    (bsp_adc_physical_to_code(plecs_get_input(PLECS_INPUT_LV), \
                              BSP_ADC_VOLT_MIN_V,              \
                              BSP_ADC_VOLT_MAX_V,              \
                              BSP_ADC_VOLT_CODE_MAX))

/* A-channel inductor-current raw ADC code calculated from the PLECS signal. */
#define BSP_ADC_ILA                                             \
    (bsp_adc_physical_to_code(plecs_get_input(PLECS_INPUT_ILA), \
                              BSP_ADC_IND_CURR_MIN_A,           \
                              BSP_ADC_IND_CURR_MAX_A,           \
                              BSP_ADC_IND_CURR_CODE_MAX))

/* B-channel inductor-current raw ADC code calculated from the PLECS signal. */
#define BSP_ADC_ILB                                             \
    (bsp_adc_physical_to_code(plecs_get_input(PLECS_INPUT_ILB), \
                              BSP_ADC_IND_CURR_MIN_A,           \
                              BSP_ADC_IND_CURR_MAX_A,           \
                              BSP_ADC_IND_CURR_CODE_MAX))

#endif
