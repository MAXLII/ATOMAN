#include "bsp_init.h"
#include "bsp_gpio.h"
#include "section.h"

void bsp_init(void)
{
    bsp_gpio_init();
}

REG_INIT(bsp_init)
