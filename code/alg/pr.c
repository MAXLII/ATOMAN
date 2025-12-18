#include "pr.h"
#include "my_math.h"

void pr_init(pr_t *p_str, pr_cfg_t *p_cfg)
{
    p_str->cfg = *p_cfg;

    pr_update_freq(p_str, p_str->cfg.w0);

    p_str->inter.e[0] = 0.0f;
    p_str->inter.e[1] = 0.0f;
    p_str->inter.e[2] = 0.0f;
    p_str->inter.u[0] = 0.0f;
    p_str->inter.u[1] = 0.0f;
    p_str->inter.u[2] = 0.0f;
}

void pr_cal(pr_t *p_str)
{
    p_str->inter.e[2] = p_str->inter.e[1];
    p_str->inter.e[1] = p_str->inter.e[0];
    p_str->inter.e[0] = p_str->input.err;

    p_str->inter.u[0] = p_str->inter.b0 * p_str->inter.e[0] +
                        p_str->inter.b1 * p_str->inter.e[1] +
                        p_str->inter.b2 * p_str->inter.e[2] -
                        p_str->inter.a1 * p_str->inter.u[1] -
                        p_str->inter.a2 * p_str->inter.u[2];

    UP_DN_LMT(p_str->inter.u[0], p_str->cfg.lmt, -p_str->cfg.lmt);

    p_str->inter.u[2] = p_str->inter.u[1];
    p_str->inter.u[1] = p_str->inter.u[0];
    p_str->output.val = p_str->inter.u[0];
}

void pr_update_freq(pr_t *p_str, float omega)
{
    p_str->cfg.w0 = omega;

    float ts = p_str->cfg.ts;
    float kp = p_str->cfg.kp;
    float kr = p_str->cfg.kr;
    float w0 = p_str->cfg.w0;
    float wc = p_str->cfg.wc;

    // 分子系数 (从 z^2 到 z^0)
    float n0 = ts * ts * kp * w0 * w0 + 4 * ts * kp * wc + 4 * ts * kr * wc + 4 * kp;
    float n1 = 2 * ts * ts * kp * w0 * w0 - 8 * kp;
    float n2 = ts * ts * kp * w0 * w0 - 4 * ts * kp * wc - 4 * ts * kr * wc + 4 * kp;

    // 分母系数 (从 z^2 到 z^0)
    float d0 = ts * ts * w0 * w0 + 4 * ts * wc + 4;
    float d1 = 2 * ts * ts * w0 * w0 - 8;
    float d2 = ts * ts * w0 * w0 - 4 * ts * wc + 4;

    p_str->inter.a1 = d1 / d0;
    p_str->inter.a2 = d2 / d0;
    p_str->inter.b0 = n0 / d0;
    p_str->inter.b1 = n1 / d0;
    p_str->inter.b2 = n2 / d0;
}
