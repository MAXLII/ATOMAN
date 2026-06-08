// SPDX-License-Identifier: MIT
/**
 * @file    bsp_adc.c
 * @brief   HC32F558 ADC BSP implementation.
 * @details
 *          This file is part of the HC32F558 AC project.
 *
 *          Module responsibilities:
 *          - Initialize ADC1/ADC2 synchronous power signal sampling
 *          - Initialize ADC3 software-triggered auxiliary signal sampling
 *          - Route HRPWM1 special compare A event to ADC1 sequence A trigger
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ADC data reads use hardware data registers directly
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

#include "bsp_adc.h"
#include "section.h"

#define BSP_ADC_SYNC_TRIG_SEL (AOS_ADC1_0)
#define BSP_ADC_SYNC_TRIG_EVT (EVT_SRC_HRPWM_INT1)

const bsp_adc_param_t bsp_adc_param[BSP_ADC_SIG_MAX] = {
    [BSP_ADC_SIG_V_G]      = {CM_ADC1, ADC_CH0, GPIO_PORT_A, GPIO_PIN_00, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_V_AC_IN]  = {CM_ADC2, ADC_CH1, GPIO_PORT_A, GPIO_PIN_01, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_V_CAP]    = {CM_ADC1, ADC_CH2, GPIO_PORT_A, GPIO_PIN_02, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_V_AC_CAP] = {CM_ADC2, ADC_CH3, GPIO_PORT_A, GPIO_PIN_07, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_I_L]      = {CM_ADC1, ADC_CH3, GPIO_PORT_A, GPIO_PIN_03, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_V_BUS]    = {CM_ADC2, ADC_CH4, GPIO_PORT_C, GPIO_PIN_04, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_I_OUT]    = {CM_ADC3, ADC_CH5, GPIO_PORT_E, GPIO_PIN_14, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_I_AC_OUT] = {CM_ADC3, ADC_CH6, GPIO_PORT_E, GPIO_PIN_15, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_V_OUT]    = {CM_ADC3, ADC_CH2, GPIO_PORT_E, GPIO_PIN_13, BSP_ADC_SAMPLE_TIME},
    [BSP_ADC_SIG_V_AC_OUT] = {CM_ADC3, ADC_CH3, GPIO_PORT_E, GPIO_PIN_07, BSP_ADC_SAMPLE_TIME},
};

static void bsp_adc_gpio_init(void)
{
    uint32_t i;
    stc_gpio_init_t gpio_init;

    LL_PERIPH_WE(LL_PERIPH_GPIO);

    (void)GPIO_StructInit(&gpio_init);
    gpio_init.u16PinAttr = PIN_ATTR_ANALOG;

    for (i = 0UL; i < (uint32_t)BSP_ADC_SIG_MAX; i++) {
        (void)GPIO_Init(bsp_adc_param[i].port, bsp_adc_param[i].pin, &gpio_init);
    }

    LL_PERIPH_WP(LL_PERIPH_GPIO);
}

static void bsp_adc_unit_init(CM_ADC_TypeDef *unit)
{
    stc_adc_init_t adc_init;

    (void)ADC_StructInit(&adc_init);
    adc_init.u16ScanMode = ADC_MD_SEQA_SINGLESHOT;
    adc_init.u16Resolution = ADC_RESOLUTION_12BIT;
    adc_init.u16DataAlign = ADC_DATAALIGN_RIGHT;
    (void)ADC_Init(unit, &adc_init);
}

static void bsp_adc_channel_init(void)
{
    uint32_t i;

    for (i = 0UL; i < (uint32_t)BSP_ADC_SIG_MAX; i++) {
        ADC_ChCmd(bsp_adc_param[i].adc_periph,
                  ADC_SEQ_A,
                  bsp_adc_param[i].adc_ch,
                  ENABLE);
        ADC_SetSampleTime(bsp_adc_param[i].adc_periph,
                          bsp_adc_param[i].adc_ch,
                          bsp_adc_param[i].sample_time);
    }
}

void bsp_adc_init(void)
{
    LL_PERIPH_WE(LL_PERIPH_FCG);
    FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_ADC1 |
                               FCG3_PERIPH_ADC2 |
                               FCG3_PERIPH_ADC3,
                           ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);
    LL_PERIPH_WP(LL_PERIPH_FCG);

    bsp_adc_gpio_init();

    bsp_adc_unit_init(CM_ADC1);
    bsp_adc_unit_init(CM_ADC2);
    bsp_adc_unit_init(CM_ADC3);
    bsp_adc_channel_init();

    ADC_SyncModeConfig(ADC_SYNC_ADC1_ADC2, ADC_SYNC_SINGLE_PARALLEL_TRIG, 1U);
    ADC_SyncModeCmd(ENABLE);

    ADC_TriggerConfig(CM_ADC1, ADC_SEQ_A, ADC_HARDTRIG_EVT0);
    AOS_SetTriggerEventSrc(BSP_ADC_SYNC_TRIG_SEL, BSP_ADC_SYNC_TRIG_EVT);
    ADC_TriggerCmd(CM_ADC1, ADC_SEQ_A, ENABLE);
}

REG_INIT(0, bsp_adc_init)

static void bsp_adc3_trigger_task(void)
{
    (void)ADC_Start(CM_ADC3);
}

REG_TASK_MS(1, bsp_adc3_trigger_task)
