#ifndef __BSP_GPIO_H__
#define __BSP_GPIO_H__

#include "hc32_ll.h"
#include "hc32_ll_gpio.h"

#define GPIO_MODE_OUT_PP 0
#define GPIO_MODE_IPD 1
#define GPIO_MODE_OUT_OD 2
// 其他类型定义按实际需要增加

// 以下定义中，speed与rcu_periph用不到，直接给0
#define GPIO_REG_PARM(_name, _gpx, _pin, _mode, _def_lv) { \
    .bsp_gpio_table = _name,                               \
    .gpio_periph = _gpx,                                   \
    .mode = GPIO_MODE_##_mode,                             \
    .speed = 0,                                            \
    .pin = GPIO_PIN_##_pin,                                \
    .rcu_periph = 0,                                       \
    .def_lv = _def_lv,                                     \
}

typedef enum
{
    BSP_GPIO_LED0,
    BSP_GPIO_LED1,
    BSP_GPIO_LED2,
    BSP_GPIO_LED3,
    BSP_GPIO_AC_P2S,
    BSP_GPIO_AC_S2P,
    BSP_GPIO_TEST1,
    BSP_GPIO_TEST2,
    BSP_GPIO_SYNC,
    BSP_GPIO_TEST3,
    BSP_GPIO_RLY_CTRL,
    BSP_GPIO_ACIN_N_CTRL,
    BSP_GPIO_ACOUTN_CTRL,
    BSP_GPIO_ACOUTL_CTRL,
    GPIO_TABLE_MAX,
} bsp_gpio_table_e;

#define LED1 BSP_GPIO_LED0
#define LED2 BSP_GPIO_LED1
#define LED3 BSP_GPIO_LED2
#define LED4 BSP_GPIO_LED3

typedef struct
{
    bsp_gpio_table_e bsp_gpio_table;
    uint32_t gpio_periph;
    uint32_t mode;
    uint32_t speed;
    uint32_t pin;
    uint32_t rcu_periph;
    uint8_t def_lv;
} bsp_gpio_parm_t;

void bsp_gpio_init(void);
void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val);
void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val);

#endif
