#include "gpio.h"
#include "bsp_gpio.h"
#include "stdint.h"

void gpio_set_led1(uint8_t val)
{
    bsp_gpio_set_bit(LED1, val);
}

void gpio_set_led2(uint8_t val)
{
    bsp_gpio_set_bit(LED2, val);
}

void gpio_set_led3(uint8_t val)
{
    bsp_gpio_set_bit(LED3, val);
}

void gpio_set_led4(uint8_t val)
{
    bsp_gpio_set_bit(LED4, val);
}
