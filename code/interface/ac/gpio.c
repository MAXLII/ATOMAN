#include "gpio.h"

void gpio_set_ac_in_rly_sta(uint8_t sta)
{
    bsp_gpio_set_bit(LED3, sta);
}

void gpio_set_ac_out_rly_sta(uint8_t sta)
{
    bsp_gpio_set_bit(LED4, sta);
}

void gpio_set_ss_rly_sta(uint8_t sta)
{
    bsp_gpio_set_bit(LED2, sta);
}

void gpio_set_main_rly_sta(uint8_t sta)
{
    bsp_gpio_set_bit(LED1, sta);
}
