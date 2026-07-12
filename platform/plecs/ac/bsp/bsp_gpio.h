#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "plecs.h"

typedef enum
{
    BSP_GPIO_AC_OUT_RLY = PLECS_OUTPUT_AC_OUT_RLY_EN,
    BSP_GPIO_MAIN_RLY = PLECS_OUTPUT_MAIN_RLY_EN,
    BSP_GPIO_AC_IN_RLY = BSP_GPIO_MAIN_RLY,
    BSP_GPIO_SS_RLY = BSP_GPIO_AC_OUT_RLY,
    PWM_ACT = BSP_GPIO_AC_OUT_RLY,
    BSP_GPIO_MAX,
} bsp_gpio_table_e;

void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val);
void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val);

#endif
