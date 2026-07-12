// SPDX-License-Identifier: MIT
/**
 * @file    bsp_gpio.c
 * @brief   HC32F558 GPIO BSP implementation.
 * @details
 *          This file is part of the HC32F558 AC project.
 *
 *          Module responsibilities:
 *          - Initialize logical relay and debug GPIO pins
 *          - Apply default output levels before enabling output drivers
 *          - Provide direct GPIO set/get access for the interface layer
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Set/get helpers are task-context APIs
 *          - Hardware access is abstracted through HC32 LL GPIO APIs
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

#include "bsp_gpio.h"
#include "section.h"

const bsp_gpio_param_t bsp_gpio_param_table[BSP_GPIO_MAX] = {
    [BSP_GPIO_LED0]       = {BSP_GPIO_LED0, GPIO_PORT_C, GPIO_PIN_13, BSP_GPIO_MODE_OUT_PP, 0U},
    [BSP_GPIO_LED1]       = {BSP_GPIO_LED1, GPIO_PORT_C, GPIO_PIN_14, BSP_GPIO_MODE_OUT_PP, 0U},
    [BSP_GPIO_LED2]       = {BSP_GPIO_LED2, GPIO_PORT_C, GPIO_PIN_15, BSP_GPIO_MODE_OUT_PP, 0U},
    [BSP_GPIO_LED3]       = {BSP_GPIO_LED3, GPIO_PORT_D, GPIO_PIN_02, BSP_GPIO_MODE_OUT_PP, 0U},
    [BSP_GPIO_MAIN_RLY]   = {BSP_GPIO_MAIN_RLY, GPIO_PORT_D, GPIO_PIN_03, BSP_GPIO_MODE_OUT_PP, 0U},
    [BSP_GPIO_SS_RLY]     = {BSP_GPIO_SS_RLY, GPIO_PORT_D, GPIO_PIN_04, BSP_GPIO_MODE_OUT_PP, 0U},
    [BSP_GPIO_AC_IN_RLY]  = {BSP_GPIO_AC_IN_RLY, GPIO_PORT_D, GPIO_PIN_05, BSP_GPIO_MODE_OUT_PP, 0U},
    [BSP_GPIO_AC_OUT_RLY] = {BSP_GPIO_AC_OUT_RLY, GPIO_PORT_D, GPIO_PIN_06, BSP_GPIO_MODE_OUT_PP, 0U},
    [BSP_GPIO_TEST1]      = {BSP_GPIO_TEST1, GPIO_PORT_D, GPIO_PIN_07, BSP_GPIO_MODE_OUT_PP, 0U},
};

static void bsp_gpio_make_init(const bsp_gpio_param_t *param, stc_gpio_init_t *init)
{
    (void)GPIO_StructInit(init);
    init->u16PinAttr = PIN_ATTR_DIGITAL;
    init->u16PinState = (0U != param->def_level) ? PIN_STAT_SET : PIN_STAT_RST;

    switch (param->mode) {
    case BSP_GPIO_MODE_OUT_PP:
        init->u16PinDir = PIN_DIR_OUT;
        init->u16PinOutputType = PIN_OUT_TYPE_CMOS;
        init->u16PullDown = PIN_PD_OFF;
        init->u16PullUp = PIN_PU_OFF;
        break;

    case BSP_GPIO_MODE_OUT_OD:
        init->u16PinDir = PIN_DIR_OUT;
        init->u16PinOutputType = PIN_OUT_TYPE_NMOS;
        init->u16PullDown = PIN_PD_OFF;
        init->u16PullUp = PIN_PU_OFF;
        break;

    case BSP_GPIO_MODE_IN_PU:
        init->u16PinDir = PIN_DIR_IN;
        init->u16PinInputType = PIN_IN_TYPE_SMT;
        init->u16PullDown = PIN_PD_OFF;
        init->u16PullUp = PIN_PU_ON;
        break;

    case BSP_GPIO_MODE_IN_PD:
    default:
        init->u16PinDir = PIN_DIR_IN;
        init->u16PinInputType = PIN_IN_TYPE_SMT;
        init->u16PullDown = PIN_PD_ON;
        init->u16PullUp = PIN_PU_OFF;
        break;
    }
}

void bsp_gpio_init(void)
{
    uint32_t i;
    stc_gpio_init_t init;

    LL_PERIPH_WE(LL_PERIPH_GPIO);

    for (i = 0UL; i < (uint32_t)BSP_GPIO_MAX; i++) {
        bsp_gpio_make_init(&bsp_gpio_param_table[i], &init);
        (void)GPIO_Init(bsp_gpio_param_table[i].port, bsp_gpio_param_table[i].pin, &init);
        if ((BSP_GPIO_MODE_OUT_PP == bsp_gpio_param_table[i].mode) ||
            (BSP_GPIO_MODE_OUT_OD == bsp_gpio_param_table[i].mode)) {
            GPIO_OutputCmd(bsp_gpio_param_table[i].port, bsp_gpio_param_table[i].pin, ENABLE);
        }
    }

    LL_PERIPH_WP(LL_PERIPH_GPIO);
}

REG_INIT(0, bsp_gpio_init)

void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val)
{
    if ((uint32_t)num >= (uint32_t)BSP_GPIO_MAX) {
        return;
    }

    if (0U != val) {
        GPIO_SetPins(bsp_gpio_param_table[num].port, bsp_gpio_param_table[num].pin);
    } else {
        GPIO_ResetPins(bsp_gpio_param_table[num].port, bsp_gpio_param_table[num].pin);
    }
}

void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val)
{
    if (((uint32_t)num >= (uint32_t)BSP_GPIO_MAX) || (NULL == val)) {
        return;
    }

    *val = (uint8_t)GPIO_ReadInputPins(bsp_gpio_param_table[num].port, bsp_gpio_param_table[num].pin);
}
