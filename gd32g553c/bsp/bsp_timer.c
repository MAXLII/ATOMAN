#include "bsp_timer.h"

#include "gd32g5x3.h"
#include "section.h"

REG_PERF_BASE_CNT((uint32_t *)(uintptr_t)(TIMER1 + 0x00000024u))

#define BSP_TIMER_CNT_FREQ_HZ 2000000u

void bsp_timer_init(void)
{
    timer_parameter_struct timer_initpara;
    uint32_t timer_clk_hz;

    rcu_periph_clock_enable(RCU_TIMER1);
    timer_deinit(TIMER1);
    timer_struct_para_init(&timer_initpara);

    /*
     * In this project APB1 follows HCLK, so TIMER1 clock is derived from
     * SystemCoreClock. Set the prescaler so one counter step equals 0.5us.
     */
    timer_clk_hz = SystemCoreClock;
    if (timer_clk_hz < BSP_TIMER_CNT_FREQ_HZ)
    {
        timer_clk_hz = BSP_TIMER_CNT_FREQ_HZ;
    }

    timer_initpara.prescaler = (uint16_t)((timer_clk_hz / BSP_TIMER_CNT_FREQ_HZ) - 1u);
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = 0xFFFFFFFFu;
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0u;

    timer_init(TIMER1, &timer_initpara);
    timer_counter_value_config(TIMER1, 0u);
    timer_enable(TIMER1);
}

REG_INIT(0, bsp_timer_init)
