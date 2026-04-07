#include "bsp_pwm.h"

void bsp_pwm_enable(void)
{
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_UP_EN, 1.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DN_EN, 1.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_UP_EN, 1.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DN_EN, 1.0f);
}

void bsp_pwm_disable(void)
{
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DUTY, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_UP_EN, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DN_EN, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_UP_EN, 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DN_EN, 0.0f);
}

void bsp_pwm_set_duty(float duty_fast,
                      float duty_slow,
                      uint8_t up_en_fast,
                      uint8_t dn_en_fast,
                      uint8_t up_en_slow,
                      uint8_t dn_en_slow)
{
    UP_DN_LMT(duty_fast, 1.0f, 0.0f);
    UP_DN_LMT(duty_slow, 1.0f, 0.0f);

    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DUTY, duty_fast);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DUTY, duty_slow);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_UP_EN, up_en_fast ? 1.0f : 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_FAST_DN_EN, dn_en_fast ? 1.0f : 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_UP_EN, up_en_slow ? 1.0f : 0.0f);
    plecs_set_output(PLECS_OUTPUT_PWM_SLOW_DN_EN, dn_en_slow ? 1.0f : 0.0f);
}
