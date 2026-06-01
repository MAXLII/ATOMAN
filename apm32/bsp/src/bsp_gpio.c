// SPDX-License-Identifier: MIT
/**
 * @file    bsp_gpio.c
 * @brief   APM32 GPIO BSP module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure the demo GPIO output table
 *          - Provide bit set and read helpers used by shared interface code
 *          - Register GPIO initialization with the section startup flow
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_gpio.h"

#include "apm32f402_403_gpio.h"
#include "apm32f402_403_rcm.h"
#include "section.h"

static const bsp_gpio_parm_t bsp_gpio_parm_table[GPIO_TABLE_MAX] = {
    GPIO_REG_PARM(LED1, GPIOB, 7, OUT_PP, 50MHz, 0U),
    GPIO_REG_PARM(LED2, GPIOB, 9, OUT_PP, 50MHz, 0U),
    GPIO_REG_PARM(LED3, GPIOB, 12, OUT_PP, 50MHz, 0U),
    GPIO_REG_PARM(LED4, GPIOC, 13, OUT_PP, 50MHz, 0U),
};

void bsp_gpio_init(void)
{
    GPIO_Config_T gpio_cfg = {0U};
    uint32_t i;

    for (i = 0U; i < (uint32_t)GPIO_TABLE_MAX; ++i)
    {
        RCM_EnableAPB2PeriphClock(bsp_gpio_parm_table[i].periph);

        gpio_cfg.pin = bsp_gpio_parm_table[i].pin;
        gpio_cfg.mode = (GPIO_MODE_T)bsp_gpio_parm_table[i].mode;
        gpio_cfg.speed = (GPIO_SPEED_T)bsp_gpio_parm_table[i].speed;
        GPIO_Config(bsp_gpio_parm_table[i].port, &gpio_cfg);
        GPIO_WriteBitValue(bsp_gpio_parm_table[i].port,
                           bsp_gpio_parm_table[i].pin,
                           bsp_gpio_parm_table[i].def_lv);
    }
}

REG_INIT(0, bsp_gpio_init)

void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val)
{
    if ((uint32_t)num >= (uint32_t)GPIO_TABLE_MAX)
    {
        return;
    }

    GPIO_WriteBitValue(bsp_gpio_parm_table[num].port,
                       bsp_gpio_parm_table[num].pin,
                       (val != 0U) ? 1U : 0U);
}

void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val)
{
    if (((uint32_t)num >= (uint32_t)GPIO_TABLE_MAX) || (val == NULL))
    {
        return;
    }

    *val = GPIO_ReadInputBit(bsp_gpio_parm_table[num].port,
                             bsp_gpio_parm_table[num].pin);
}
