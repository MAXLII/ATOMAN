#include "gpio.h"

void gpio_set_ac_in_rly_sta(uint8_t sta)
{
    bsp_gpio_set_bit(BSP_GPIO_AC_IN_RLY, sta);
}

void gpio_set_ac_out_rly_sta(uint8_t sta)
{
    bsp_gpio_set_bit(BSP_GPIO_AC_OUT_RLY, sta);
}

void gpio_set_ss_rly_sta(uint8_t sta)
{
    gpio_set_ac_out_rly_sta(sta);
}

void gpio_set_main_rly_sta(uint8_t sta)
{
    gpio_set_ac_in_rly_sta(sta);
}
