#include "bsp_gpio.h"
#include "section.h"

const bsp_gpio_parm_t bsp_gpio_parm_table[] = {
    GPIO_REG_PARM(BSP_GPIO_LED0, GPIO_PORT_C, 13, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_LED1, GPIO_PORT_C, 02, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_LED2, GPIO_PORT_C, 03, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_LED3, GPIO_PORT_A, 00, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_AC_P2S, GPIO_PORT_A, 04, OUT_PP, 1),
    GPIO_REG_PARM(BSP_GPIO_AC_S2P, GPIO_PORT_A, 05, IPD, 0),
    GPIO_REG_PARM(BSP_GPIO_TEST1, GPIO_PORT_B, 11, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_TEST2, GPIO_PORT_C, 08, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_SYNC, GPIO_PORT_C, 09, OUT_OD, 1),
    GPIO_REG_PARM(BSP_GPIO_TEST3, GPIO_PORT_C, 10, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_RLY_CTRL, GPIO_PORT_C, 11, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_ACIN_N_CTRL, GPIO_PORT_C, 12, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_ACOUTN_CTRL, GPIO_PORT_D, 02, OUT_PP, 0),
    GPIO_REG_PARM(BSP_GPIO_ACOUTL_CTRL, GPIO_PORT_B, 03, OUT_PP, 0),
};

void bsp_gpio_init(void)
{
    uint8_t gpio_num = (uint8_t)(sizeof(bsp_gpio_parm_table) / sizeof(bsp_gpio_parm_t));
    uint8_t i;

    GPIO_REG_Unlock();
    GPIO_SetDebugPort(GPIO_PIN_DEBUG_JTAG, DISABLE);

    for (i = 0; i < gpio_num; i++)
    {
        if (bsp_gpio_parm_table[i].bsp_gpio_table == BSP_GPIO_TEST1)
        {
            continue;
        }

        stc_gpio_init_t stcGpioInit;
        (void)GPIO_StructInit(&stcGpioInit);

        if (bsp_gpio_parm_table[i].def_lv == 0)
        {
            stcGpioInit.u16PinState = PIN_STAT_RST;
        }
        else
        {
            stcGpioInit.u16PinState = PIN_STAT_SET;
        }

        switch (bsp_gpio_parm_table[i].mode)
        {
        case GPIO_MODE_OUT_PP:
            stcGpioInit.u16PinDir = PIN_DIR_OUT;
            stcGpioInit.u16PinDrv = PIN_HIGH_DRV;
            stcGpioInit.u16PinOutputType = PIN_OUT_TYPE_CMOS;
            break;

        case GPIO_MODE_IPD:
            stcGpioInit.u16PinDir = PIN_DIR_IN;
            stcGpioInit.u16PullDown = PIN_PD_ON;
            break;

        case GPIO_MODE_OUT_OD:
            stcGpioInit.u16PinDir = PIN_DIR_OUT;
            stcGpioInit.u16PinDrv = PIN_HIGH_DRV;
            stcGpioInit.u16PinOutputType = PIN_OUT_TYPE_NMOS;
            break;

        default:
            break;
        }

        GPIO_Init(bsp_gpio_parm_table[i].gpio_periph, bsp_gpio_parm_table[i].pin, &stcGpioInit);

        if ((bsp_gpio_parm_table[i].mode == GPIO_MODE_OUT_PP) ||
            (bsp_gpio_parm_table[i].mode == GPIO_MODE_OUT_OD))
        {
            GPIO_OutputCmd(bsp_gpio_parm_table[i].gpio_periph, bsp_gpio_parm_table[i].pin, ENABLE);
        }
        else
        {
            GPIO_OutputCmd(bsp_gpio_parm_table[i].gpio_periph, bsp_gpio_parm_table[i].pin, DISABLE);
        }
    }

    GPIO_REG_Lock();
}

REG_INIT(0, bsp_gpio_init)

void bsp_gpio_set_bit(bsp_gpio_table_e num, uint8_t val)
{
    if (val)
    {
        GPIO_SetPins(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
    }
    else
    {
        GPIO_ResetPins(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
    }
}

void bsp_gpio_get_bit(bsp_gpio_table_e num, uint8_t *val)
{
    *val = GPIO_ReadInputPins(bsp_gpio_parm_table[num].gpio_periph, bsp_gpio_parm_table[num].pin);
}
