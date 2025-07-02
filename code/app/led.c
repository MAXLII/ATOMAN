#include "gpio.h"
#include "section.h"

void led_task(void)
{
    static uint8_t cnt = 0;
    uint8_t sta = (uint8_t)(0x0F << cnt);
    cnt++;
    if (cnt >= 8)
    {
        cnt = 0;
    }
    gpio_set_led1((sta & 0x10) ? 1 : 0);
    gpio_set_led2((sta & 0x20) ? 1 : 0);
    gpio_set_led3((sta & 0x40) ? 1 : 0);
    gpio_set_led4((sta & 0x80) ? 1 : 0);
}

REG_TASK(1000, led_task)
