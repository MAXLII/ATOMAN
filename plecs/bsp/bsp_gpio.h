#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "plecs.h"

typedef enum
{
    LED1 = PLECS_OUTPUT_LED1,
    LED2 = PLECS_OUTPUT_LED2,
    LED3 = PLECS_OUTPUT_LED3,
    LED4 = PLECS_OUTPUT_LED4,
    BSP_GPIO_MAX,
} bsp_gpio_table_e;

void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val);
void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val);

#endif
