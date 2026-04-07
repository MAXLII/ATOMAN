#ifndef __GPIO_H
#define __GPIO_H

#include "bsp_gpio.h"
#include <stdint.h>

void gpio_set_ss_rly_sta(uint8_t sta);

void gpio_set_main_rly_sta(uint8_t sta);

void gpio_set_ac_in_rly_sta(uint8_t sta);

void gpio_set_ac_out_rly_sta(uint8_t sta);

#endif
