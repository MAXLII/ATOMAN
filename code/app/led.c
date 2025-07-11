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

REG_TASK_MS(100, led_task)

uint8_t u8_temp;
REG_SHELL_VAR(u8_temp, u8_temp, SHELL_UINT8, NULL);

uint16_t u16_temp;
REG_SHELL_VAR(u16_temp, u16_temp, SHELL_UINT16, NULL);

uint32_t u32_temp;
REG_SHELL_VAR(u32_temp, u32_temp, SHELL_UINT32, NULL);

int8_t i8_temp;
REG_SHELL_VAR(i8_temp, i8_temp, SHELL_INT8, NULL);

int16_t i16_temp;
REG_SHELL_VAR(i16_temp, i16_temp, SHELL_INT16, NULL);

int32_t i32_temp;
REG_SHELL_VAR(i32_temp, i32_temp, SHELL_INT32, NULL);

float fp32_temp;
REG_SHELL_VAR(fp32_temp, fp32_temp, SHELL_FP32, NULL);
