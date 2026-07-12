#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#include "plecs.h"
#include "my_math.h"

void bsp_pwm_enable(void);

void bsp_pwm_disable(void);

void bsp_pwm_set_duty(float duty_fast, float duty_slow, uint8_t up_en_fast, uint8_t dn_en_fast, uint8_t up_en_slow, uint8_t dn_en_slow);

#endif
