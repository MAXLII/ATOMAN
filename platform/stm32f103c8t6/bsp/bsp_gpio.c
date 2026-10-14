// SPDX-License-Identifier: MIT
/**
 * @file bsp_gpio.c
 * @brief PB3 indicator driver.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Disable JTAG while retaining SWD and initialize the schematic's PB3 LED
 *          Design notes:
 *          - C11 compatible; no dynamic allocation in platform drivers
 *          - Hardware access belongs to the BSP; framework bindings belong to src
 * @author Max.Li
 * @date 2026-10-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */
#include "bsp_gpio.h"
#include "stm32f10x.h"
void bsp_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0}; /* Active-low DS0 output configuration. */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
    GPIO_SetBits(GPIOB, GPIO_Pin_3);
    gpio.GPIO_Pin   = GPIO_Pin_3;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &gpio);
}
void bsp_gpio_led_toggle(void)
{
    GPIOB->ODR ^= GPIO_Pin_3;
}
