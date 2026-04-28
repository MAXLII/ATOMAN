#include "systick.h"
#include "hc32f3xx.h"

static volatile uint32_t delay;
volatile uint32_t sys_tick_100us;

void systick_config(void)
{
    SystemCoreClockUpdate();

    if (SysTick_Config(SystemCoreClock / 10000U)) {
        while (1) {
        }
    }

    NVIC_SetPriority(SysTick_IRQn, 0U);
}

void delay_1ms(uint32_t count)
{
    delay = count;

    while (0U != delay) {
    }
}

void delay_decrement(void)
{
    sys_tick_100us++;

    if (0U != delay) {
        delay--;
    }
}

uint32_t systick_gettime_100us(void)
{
    return sys_tick_100us;
}
