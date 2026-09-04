#ifndef __BSP_TIMER_H
#define __BSP_TIMER_H

#include <stdint.h>

void bsp_timer_init(void);
uint32_t bsp_timer_jitter_count_get(void);
uint32_t bsp_timer_jitter_clock_hz_get(void);
uint32_t bsp_timer_jitter_period_ticks_get(void);

#endif
