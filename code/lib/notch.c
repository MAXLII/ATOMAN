#include "notch.h"
#include "string.h"

static void notch_update_coeff(notch_t *p_str)
{
    float ts = p_str->cfg.ts;
    float w0 = p_str->cfg.w0;
    float wb = p_str->cfg.wb;

    float n0 = ts * ts * w0 * w0 + 4;
    float n1 = 2 * ts * ts * w0 * w0 - 8;
    float n2 = ts * ts * w0 * w0 + 4;

    float d0 = ts * ts * w0 * w0 + 2 * ts * wb + 4;
    float d1 = 2 * ts * ts * w0 * w0 - 8;
    float d2 = ts * ts * w0 * w0 - 2 * ts * wb + 4;

    p_str->inter.c0 = n0 / d0;
    p_str->inter.c1 = n1 / d0;
    p_str->inter.c2 = n2 / d0;
    p_str->inter.d1 = d1 / d0;
    p_str->inter.d2 = d2 / d0;
}

void notch_init(notch_t *p_str,
                float w0,
                float wb,
                float ts,
                float *p_val)
{
    p_str->input.p_val = p_val;
    p_str->cfg.ts = ts;
    p_str->cfg.w0 = w0;
    p_str->cfg.wb = wb;

    notch_update_coeff(p_str);
    p_str->inter.x[0] = 0.0f;
    p_str->inter.x[1] = 0.0f;
    p_str->inter.x[2] = 0.0f;
    p_str->inter.y[0] = 0.0f;
    p_str->inter.y[1] = 0.0f;
    p_str->inter.y[2] = 0.0f;
}

void notch_update_freq(notch_t *p_str, float omega)
{
    p_str->cfg.w0 = omega;
    notch_update_coeff(p_str);
}

void notch_cal(notch_t *p_str)
{
    if (p_str->input.p_val == NULL)
    {
        return;
    }
    p_str->inter.x[0] = *p_str->input.p_val;

    p_str->inter.y[0] = p_str->inter.c0 * p_str->inter.x[0] +
                        p_str->inter.c1 * p_str->inter.x[1] +
                        p_str->inter.c2 * p_str->inter.x[2] -
                        p_str->inter.d1 * p_str->inter.y[1] -
                        p_str->inter.d2 * p_str->inter.y[2];

    p_str->inter.y[2] = p_str->inter.y[1];
    p_str->inter.y[1] = p_str->inter.y[0];
    p_str->inter.x[2] = p_str->inter.x[1];
    p_str->inter.x[1] = p_str->inter.x[0];

    p_str->output.val = p_str->inter.y[0];
}
