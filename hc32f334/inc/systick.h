#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

void systick_config(void);
void delay_1ms(uint32_t count);
void delay_decrement(void);
uint32_t systick_gettime_100us(void);

#endif /* SYSTICK_H */
