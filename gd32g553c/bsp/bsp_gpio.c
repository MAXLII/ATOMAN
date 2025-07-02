#include "bsp_gpio.h"

const bsp_gpio_parm_t bsp_gpio_parm_table[] = {
    GPIO_REG_PARM(LED1, GPIOB, 7, OUTPUT, PP, NONE, _60MHZ),
    GPIO_REG_PARM(LED2, GPIOB, 9, OUTPUT, PP, NONE, _60MHZ),
    GPIO_REG_PARM(LED3, GPIOB, 1, OUTPUT, PP, NONE, _60MHZ),
    GPIO_REG_PARM(LED4, GPIOC, 13, OUTPUT, PP, NONE, _60MHZ),
};

void bsp_gpio_init(void)
{
    uint8_t gpio_num = 0;
    gpio_num = sizeof(bsp_gpio_parm_table) / sizeof(bsp_gpio_parm_t);
    uint8_t i;

    for (i = 0; i < gpio_num; i++)
    {
        rcu_periph_clock_enable(bsp_gpio_parm_table[i].rcu_periph);
        gpio_mode_set(bsp_gpio_parm_table[i].gpio_periph,
                      bsp_gpio_parm_table[i].mode,
                      bsp_gpio_parm_table[i].pull_up_down,
                      bsp_gpio_parm_table[i].pin);
        if (bsp_gpio_parm_table[i].mode == GPIO_MODE_OUTPUT)
        {
            gpio_output_options_set(bsp_gpio_parm_table[i].gpio_periph,
                                    bsp_gpio_parm_table[i].otype,
                                    bsp_gpio_parm_table[i].speed,
                                    bsp_gpio_parm_table[i].pin);
        }
    }
}

void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val)
{
    if (val)
    {
        gpio_bit_set(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
    }
    else
    {
        gpio_bit_reset(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
    }
}

void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val)
{
    *val = gpio_input_bit_get(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
}
