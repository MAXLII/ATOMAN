#ifndef __BSP_TIMER_H
#define __BSP_TIMER_H

#include <stdint.h>

void bsp_timer_init(void);
uint32_t bsp_timer_cnt_get(void);

#endif
