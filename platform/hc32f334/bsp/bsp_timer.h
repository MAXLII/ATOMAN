#ifndef __BSP_TIMER_H__
#define __BSP_TIMER_H__

#include "hc32_ll.h"
#include "hc32_ll_tmra.h"

int32_t bsp_timer_init(void);
uint32_t bsp_timer_get_perf_cnt(void);

#endif
