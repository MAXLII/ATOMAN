#include "pi_tustin.h"
#include "my_math.h"
#include "section.h"

void pi_tustin_init(pi_tustin_t *p_str,
                    float kp,
                    float ki,
                    float ts,
                    float up_lmt,
                    float dn_lmt,
                    float *p_ref,
                    float *p_act)
{
    p_str->input.p_ref = p_ref;
    p_str->input.p_act = p_act;
    p_str->inter.a1 = 0.0f;
    p_str->inter.b0 = 0.0f;
    p_str->inter.b1 = 0.0f;
    p_str->inter.b1_inv = 1.0f;
    pi_tustin_update(p_str, kp, ki, ts);
    p_str->inter.up_lmt = up_lmt;
    p_str->inter.dn_lmt = dn_lmt;
    p_str->inter.e[0] = 0.0f;
    p_str->inter.e[1] = 0.0f;
    p_str->inter.u[0] = 0.0f;
    p_str->inter.u[1] = 0.0f;
}

void pi_tustin_cal(pi_tustin_t *p_str)
{
    p_str->inter.e[0] = *p_str->input.p_ref - *p_str->input.p_act;
    const float b0e0 = p_str->inter.b0 * p_str->inter.e[0];
    const float a1u1 = p_str->inter.a1 * p_str->inter.u[1];
    p_str->inter.u[0] = b0e0 +
                        p_str->inter.b1 * p_str->inter.e[1] -
                        a1u1;

    if (unlikely(p_str->inter.u[0] > p_str->inter.up_lmt))
    {
        p_str->inter.u[0] = p_str->inter.up_lmt;
        p_str->inter.e[1] = (p_str->inter.u[0] - b0e0 + a1u1) * p_str->inter.b1_inv;
    }
    else if (unlikely(p_str->inter.u[0] < p_str->inter.dn_lmt))
    {
        p_str->inter.u[0] = p_str->inter.dn_lmt;
        p_str->inter.e[1] = (p_str->inter.u[0] - b0e0 + a1u1) * p_str->inter.b1_inv;
    }
    else
    {
        p_str->inter.e[1] = p_str->inter.e[0];
    }

    p_str->inter.u[1] = p_str->inter.u[0];
    p_str->output.val = p_str->inter.u[0];
}

void pi_tustin_update(pi_tustin_t *p_str,
                      float kp,
                      float ki,
                      float ts)
{
    if (ts * ki >= 2 * kp) // 防止b1大于0导致控制不稳定
    {
        return;
    }

    float n0 = ts * ki + 2 * kp;
    float n1 = ts * ki - 2 * kp;
    float d0 = 2;
    float d1 = -2;
    p_str->inter.a1 = d1 / d0;
    p_str->inter.b0 = n0 / d0;
    p_str->inter.b1 = n1 / d0;
    p_str->inter.b1_inv = 1.0f / p_str->inter.b1;
}

void pi_tustin_reset(pi_tustin_t *p_str)
{
    p_str->inter.e[0] = 0.0f;
    p_str->inter.e[1] = 0.0f;
    p_str->inter.u[0] = 0.0f;
    p_str->inter.u[1] = 0.0f;
}
