#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "gd32g5x3.h"
#include "gd32g5x3_rcu.h"

/*!
    \brief      set GPIO mode
    \param[in]  gpio_periph: GPIOx(x = A,B,C,D,E,F,G)
                only one parameter can be selected which is shown as below:
      \arg        GPIOx(x = A,B,C,D,E,F,G)
    \param[in]  mode: gpio pin mode
                only one parameter can be selected which is shown as below:
      \arg        GPIO_MODE_INPUT: input mode
      \arg        GPIO_MODE_OUTPUT: output mode
      \arg        GPIO_MODE_AF: alternate function mode
      \arg        GPIO_MODE_ANALOG: analog mode
    \param[in]  pull_up_down: gpio pin with pull-up or pull-down resistor
                only one parameter can be selected which is shown as below:
      \arg        GPIO_PUPD_NONE: floating mode, no pull-up and pull-down resistors
      \arg        GPIO_PUPD_PULLUP: with pull-up resistor
      \arg        GPIO_PUPD_PULLDOWN:with pull-down resistor
    \param[in]  pin: GPIO pin
                one or more parameters can be selected which are shown as below:
      \arg        GPIO_PIN_x(x=0..15), GPIO_PIN_ALL
    \param[out] none
    \retval     none
*/

/*!
    \brief      set GPIO output type and speed
    \param[in]  gpio_periph: GPIOx(x = A,B,C,D,E,F,G)
                only one parameter can be selected which is shown as below:
      \arg        GPIOx(x = A,B,C,D,E,F,G)
    \param[in]  otype: gpio pin output mode
                only one parameter can be selected which is shown as below:
      \arg        GPIO_OTYPE_PP: push pull mode
      \arg        GPIO_OTYPE_OD: open drain mode
    \param[in]  speed: gpio pin output max speed
                only one parameter can be selected which is shown as below:
      \arg        GPIO_OSPEED_12MHZ: output max speed 12MHz
      \arg        GPIO_OSPEED_60MHZ: output max speed 60MHz
      \arg        GPIO_OSPEED_85MHZ: output max speed 85MHz
      \arg        GPIO_OSPEED_100_220MHZ: output max speed 100/220MHz
    \param[in]  pin: GPIO pin
                one or more parameters can be selected which are shown as below:
      \arg        GPIO_PIN_x(x=0..15), GPIO_PIN_ALL
    \param[out] none
    \retval     none
*/

#define GPIO_REG_PARM(name, gpx, _pin, _mode, _otype, pud, _speed) { \
    .bsp_gpio_table = name,                                          \
    .gpio_periph = gpx,                                              \
    .mode = GPIO_MODE_##_mode,                                       \
    .pull_up_down = GPIO_PUPD_##pud,                                 \
    .pin = GPIO_PIN_##_pin,                                          \
    .speed = GPIO_OSPEED##_speed,                                    \
    .otype = GPIO_OTYPE_##_otype,                                    \
    .rcu_periph = RCU_##gpx,                                         \
}

typedef enum
{
    LED1,
    LED2,
    LED3,
    LED4,
    GPIO_TABLE_MAX
} bsp_gpio_table_e;

typedef struct
{
    bsp_gpio_table_e bsp_gpio_table;
    uint32_t gpio_periph;
    uint32_t mode;
    uint32_t pull_up_down;
    uint32_t pin;
    uint32_t speed;
    uint8_t otype;
    rcu_periph_enum rcu_periph;
} bsp_gpio_parm_t;

void bsp_gpio_init(void);
void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val);
void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val);

#endif
