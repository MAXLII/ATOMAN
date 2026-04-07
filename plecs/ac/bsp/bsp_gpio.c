#include "bsp_gpio.h"
#include "plecs.h"

void bsp_gpio_get_bit(bsp_gpio_table_e gpio, uint8_t *val)
{
    *val = (uint8_t)plecs_get_input((PLECS_INPUT_E)gpio);
}

void bsp_gpio_set_bit(bsp_gpio_table_e gpio, uint8_t status)
{
    plecs_set_output((PLECS_OUTPUT_E)gpio, (float)status);
}
