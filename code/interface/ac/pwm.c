#include "pwm.h"

static uint8_t pwm_step = 0;
static float v_pwm_last = 0.0f;
static float v_pwm_tag = 0.0f;
static float v_pwm_act = 0.0f;
static uint8_t v_pwm_i = 0;

void pwm_enable(void)
{
    pwm_step = 0;
    v_pwm_last = 0.0f;
    v_pwm_tag = 0.0f;
    v_pwm_act = 0.0f;
    v_pwm_i = 0U;
}

static inline float pfc_calc_duty(float v_pwm, float v_bus, float *p_offset)
{
    /* 保护：防止除0；你也可以改成直接关PWM并返回 */
    const float vbus = (v_bus > 1e-6f || v_bus < -1e-6f) ? v_bus : 1e-6f;

    if (v_pwm > 0.0f)
    {
        *p_offset = 0.0f;
        return v_pwm / vbus;
    }
    else
    {
        *p_offset = 1.0f;
        return v_pwm / vbus + 1.0f;
    }
}

/* 全开 1,1,1,1 */
static inline void pfc_pwm_apply_full(float v_pwm, float v_bus)
{
    float offset;
    float duty = pfc_calc_duty(v_pwm, v_bus, &offset);
    bsp_pwm_set_duty(duty, offset, 1, 1, 1, 1);
}

/* 过零斜坡阶段：只开一条支路（按你原来的配置） */
static inline void pfc_pwm_apply_ramp(float v_pwm_act, float v_bus)
{
    float offset;
    float duty = pfc_calc_duty(v_pwm_act, v_bus, &offset);

    if (v_pwm_act > 0.0f)
    {
        bsp_pwm_set_duty(duty, offset, 0, 1, 0, 0);
    }
    else
    {
        bsp_pwm_set_duty(duty, offset, 1, 0, 0, 0);
    }
}

static inline uint8_t pfc_is_zero_cross(float v_now, float v_last)
{
    /* 与你原来逻辑一致：乘积<=0 判定过零（包含等于0） */
    return (v_now * v_last) <= 0.0f;
}

void pwm_set_inv(float v_pwm, float v_bus)
{
    float offset;
    float duty = pfc_calc_duty(v_pwm, v_bus, &offset);

    bsp_pwm_set_duty(duty, offset, 1, 1, 1, 1);
}

void pwm_set_pfc(float v_pwm, float v_bus)
{
    switch (pwm_step)
    {
    default:
    case 0:
        // pwm_step = 1;
        pfc_pwm_apply_full(v_pwm, v_bus);
        break;

    case 1:
        if (pfc_is_zero_cross(v_pwm, v_pwm_last))
        {
            pwm_step = 2;
            bsp_pwm_set_duty(0.0f, 0.0f, 0, 0, 0, 0);

            v_pwm_tag = v_pwm;
            v_pwm_act = 0.0f;
            v_pwm_i = 0; /* 建议：显式复位计数器，避免继承旧值 */
        }
        else
        {
            pfc_pwm_apply_full(v_pwm, v_bus);
        }
        break;

    case 2:
        if (v_pwm_i < 5)
        {
            v_pwm_i++;

            /* 线性斜坡：i/5 */
            v_pwm_act = v_pwm_tag * ((float)v_pwm_i / 5.0f);
            pfc_pwm_apply_ramp(v_pwm_act, v_bus);
        }
        else
        {
            pwm_step = 1;
            pfc_pwm_apply_full(v_pwm, v_bus);
        }
        break;
    }
    v_pwm_last = v_pwm;
}
void pwm_disable(void)
{
    bsp_pwm_disable();
}
