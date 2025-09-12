#include "bsp_timer.h"
#include "gd32g5x3.h"
#include "section.h"

REG_PERF_BASE_CNT(TIMER_CNT(TIMER5))

void bsp_timer5_init(void)
{
    timer_parameter_struct timer_initpara;

    // 1. 使能TIMER5时钟
    rcu_periph_clock_enable(RCU_TIMER5);

    // 2. 初始化结构体为默认值
    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler = 20; // (215+1)/216MHz = 1us
    timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period = 0xFFFF; // 每计数一次溢出
    timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
    timer_initpara.repetitioncounter = 0;

    // 4. 初始化TIMER5
    timer_init(TIMER5, &timer_initpara);

    // 5. 启动TIMER5
    timer_enable(TIMER5);
}

REG_INIT(0, bsp_timer5_init);
