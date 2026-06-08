// SPDX-License-Identifier: MIT
/**
 * @file    bsp_adc.h
 * @brief   HC32F558 ADC BSP interface.
 * @details
 *          This file is part of the HC32F558 AC project.
 *
 *          Module responsibilities:
 *          - Define ADC signal to peripheral/channel mappings
 *          - Export direct ADC data register access macros
 *          - Provide ADC initialization entry for hardware and software triggers
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ADC result access is register based and ISR-safe for reads
 *          - Hardware access is abstracted through HC32 LL ADC/AOS/GPIO APIs
 *
 * @author  Max.Li
 * @date    2026-06-06
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BSP_ADC_H
#define BSP_ADC_H

#include <stdint.h>
#include "hc32_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_ADC_SAMPLE_TIME (16U)

typedef enum {
    BSP_ADC_SIG_V_G = 0,
    BSP_ADC_SIG_V_AC_IN,
    BSP_ADC_SIG_V_CAP,
    BSP_ADC_SIG_V_AC_CAP,
    BSP_ADC_SIG_I_L,
    BSP_ADC_SIG_V_BUS,
    BSP_ADC_SIG_I_OUT,
    BSP_ADC_SIG_I_AC_OUT,
    BSP_ADC_SIG_V_OUT,
    BSP_ADC_SIG_V_AC_OUT,
    BSP_ADC_SIG_MAX,
} bsp_adc_signal_e;

typedef struct {
    CM_ADC_TypeDef *adc_periph;
    uint8_t adc_ch;
    uint8_t port;
    uint32_t pin;
    uint8_t sample_time;
} bsp_adc_param_t;

extern const bsp_adc_param_t bsp_adc_param[BSP_ADC_SIG_MAX];

#define BSP_ADC_READ(_sig) (*((&bsp_adc_param[(_sig)].adc_periph->DR0) + bsp_adc_param[(_sig)].adc_ch))

#define BSP_ADC_V_G      BSP_ADC_READ(BSP_ADC_SIG_V_G)
#define BSP_ADC_V_AC_IN  BSP_ADC_READ(BSP_ADC_SIG_V_AC_IN)
#define BSP_ADC_V_CAP    BSP_ADC_READ(BSP_ADC_SIG_V_CAP)
#define BSP_ADC_V_AC_CAP BSP_ADC_READ(BSP_ADC_SIG_V_AC_CAP)
#define BSP_ADC_I_L      BSP_ADC_READ(BSP_ADC_SIG_I_L)
#define BSP_ADC_V_BUS    BSP_ADC_READ(BSP_ADC_SIG_V_BUS)
#define BSP_ADC_I_OUT    BSP_ADC_READ(BSP_ADC_SIG_I_OUT)
#define BSP_ADC_I_AC_OUT BSP_ADC_READ(BSP_ADC_SIG_I_AC_OUT)
#define BSP_ADC_V_OUT    BSP_ADC_READ(BSP_ADC_SIG_V_OUT)
#define BSP_ADC_V_AC_OUT BSP_ADC_READ(BSP_ADC_SIG_V_AC_OUT)

void bsp_adc_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ADC_H */
