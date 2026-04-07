#ifndef __PWM_H
#define __PWM_H

#include "bsp_pwm.h"
#include "my_math.h"

void pwm_enable(void);

void pwm_set_pfc(float v_pwm, float v_bus);
void pwm_set_inv(float v_pwm, float v_bus);

void pwm_disable(void);

#endif

