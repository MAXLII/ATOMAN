#include "bsp_pwm.h"

#include "gd32g5x3.h"
#include "section.h"

#define BSP_PWM_TIMER TIMER2
#define BSP_PWM_TIMER_RCU RCU_TIMER2
#define BSP_PWM_TIMER_IRQn TIMER2_IRQn
#define BSP_PWM_IRQ_PRE_PRIORITY 1u
#define BSP_PWM_IRQ_SUB_PRIORITY 0u
#define BSP_PWM_IRQ_FREQ_HZ 30000u
#define BSP_PWM_TIMER_MAX_TICKS 65536u

static void bsp_pwm_timer_period_calc(uint32_t timer_clk_hz,
                                      uint16_t *prescaler,
                                      uint32_t *period)
{
    uint32_t divider;
    uint32_t counter_clk_hz;
    uint32_t ticks;

    if (timer_clk_hz < BSP_PWM_IRQ_FREQ_HZ)
    {
        timer_clk_hz = BSP_PWM_IRQ_FREQ_HZ;
    }

    divider = (timer_clk_hz + ((BSP_PWM_IRQ_FREQ_HZ * BSP_PWM_TIMER_MAX_TICKS) - 1u)) /
              (BSP_PWM_IRQ_FREQ_HZ * BSP_PWM_TIMER_MAX_TICKS);
    if (divider == 0u)
    {
        divider = 1u;
    }
    if (divider > BSP_PWM_TIMER_MAX_TICKS)
    {
        divider = BSP_PWM_TIMER_MAX_TICKS;
    }

    counter_clk_hz = timer_clk_hz / divider;
    ticks = (counter_clk_hz + (BSP_PWM_IRQ_FREQ_HZ / 2u)) / BSP_PWM_IRQ_FREQ_HZ;
    if (ticks == 0u)
    {
        ticks = 1u;
    }
    if (ticks > BSP_PWM_TIMER_MAX_TICKS)
    {
        ticks = BSP_PWM_TIMER_MAX_TICKS;
    }

    *prescaler = (uint16_t)(divider - 1u);
    *period = ticks - 1u;
}

void bsp_pwm_init(void)
{
    timer_parameter_struct timer_initpara;
    uint16_t prescaler;
    uint32_t period;

    bsp_pwm_timer_period_calc(SystemCoreClock, &prescaler, &period);

    rcu_periph_clock_enable(BSP_PWM_TIMER_RCU);
    timer_deinit(BSP_PWM_TIMER);
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = prescaler;
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = period;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0u;

    timer_init(BSP_PWM_TIMER, &timer_initpara);
    timer_update_source_config(BSP_PWM_TIMER, TIMER_UPDATE_SRC_REGULAR);
    timer_counter_value_config(BSP_PWM_TIMER, 0u);
    timer_interrupt_flag_clear(BSP_PWM_TIMER, TIMER_INT_FLAG_UP);
    timer_interrupt_enable(BSP_PWM_TIMER, TIMER_INT_UP);
    nvic_irq_enable(BSP_PWM_TIMER_IRQn,
                    (uint8_t)BSP_PWM_IRQ_PRE_PRIORITY,
                    (uint8_t)BSP_PWM_IRQ_SUB_PRIORITY);
    timer_enable(BSP_PWM_TIMER);
}

REG_INIT(0, bsp_pwm_init)
