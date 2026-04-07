/*
 * Proportional-Resonant controller transfer function:
 *
 *                 2 * kr * wc * s
 * Gc(s) = kp + -------------------------
 *               s^2 + 2 * wc * s + w0^2
 *
 * Parameter approximation relations:
 *
 *               L * wx^2 * cos(phim)
 * kr ~= --------------------------------
 *                    2 * wc
 *
 * kp ~= L * wx * sin(phim) - 2 * L * wc * cos(phim)
 *
 * for wc << wx:
 *
 * kp ~= L * wx * sin(phim)
 *
 * where:
 *   kp : proportional gain
 *   kr : resonant gain
 *   wx : cutoff angular frequency
 *   wc : resonant bandwidth
 *   w0 : resonant center angular frequency
 *   phim : phase margin
 */

#include "pr.h"
#include "my_math.h"
#include <stddef.h>

bool pr_init(pr_t *p_str,
             float kp,
             float kr,
             float w0,
             float wc,
             float ts,
             float up_lmt,
             float dn_lmt,
             float *p_ref,
             float *p_act)
{
    if ((p_str == NULL) ||
        (p_ref == NULL) ||
        (p_act == NULL))
    {
        return false;
    }

    p_str->input.p_ref = p_ref;
    p_str->input.p_act = p_act;

    p_str->inter.kp = kp;
    p_str->inter.kr = kr;
    p_str->inter.w0 = w0;
    p_str->inter.wc = wc;
    p_str->inter.ts = ts;
    p_str->inter.a1 = 0.0f;
    p_str->inter.a2 = 0.0f;
    p_str->inter.b0 = 0.0f;
    p_str->inter.b1 = 0.0f;
    p_str->inter.b2 = 0.0f;
    p_str->inter.up_lmt = up_lmt;
    p_str->inter.dn_lmt = dn_lmt;

    pr_reset(p_str);
    return pr_update_freq(p_str, w0);
}

bool pr_cal(pr_t *p_str)
{
    if ((p_str == NULL) ||
        (p_str->input.p_ref == NULL) ||
        (p_str->input.p_act == NULL))
    {
        return false;
    }

    float e0 = *p_str->input.p_ref - *p_str->input.p_act;
    float e1 = p_str->inter.e[0];
    float e2 = p_str->inter.e[1];
    float u1 = p_str->inter.u[0];
    float u2 = p_str->inter.u[1];
    float u_raw = p_str->inter.b0 * e0 +
                  p_str->inter.b1 * e1 +
                  p_str->inter.b2 * e2 -
                  p_str->inter.a1 * u1 -
                  p_str->inter.a2 * u2;
    float u_sat = u_raw;
    bool is_saturated = false;
    bool hold_state = false;

    UP_DN_LMT(u_sat, p_str->inter.up_lmt, p_str->inter.dn_lmt);
    is_saturated = (u_sat != u_raw);

    /*
     * Freeze state propagation only when saturation would be driven deeper
     * by the current error direction. This keeps the PR state from winding up
     * during hard startup/limit conditions while still allowing recovery.
     */
    if (is_saturated)
    {
        if (((u_raw > p_str->inter.up_lmt) && (e0 > 0.0f)) ||
            ((u_raw < p_str->inter.dn_lmt) && (e0 < 0.0f)))
        {
            hold_state = true;
        }
    }

    if (hold_state == false)
    {
        p_str->inter.e[2] = p_str->inter.e[1];
        p_str->inter.e[1] = p_str->inter.e[0];
        p_str->inter.e[0] = e0;

        p_str->inter.u[2] = p_str->inter.u[1];
        p_str->inter.u[1] = p_str->inter.u[0];
        p_str->inter.u[0] = u_sat;
    }

    p_str->output.raw = u_raw;
    p_str->output.sat = u_sat;
    p_str->output.val = u_sat;
    p_str->output.is_saturated = is_saturated;

    return true;
}

bool pr_update_freq(pr_t *p_str, float omega)
{
    if ((p_str == NULL) ||
        (p_str->inter.kp < 0.0f) ||
        (p_str->inter.kr < 0.0f) ||
        (p_str->inter.wc <= 0.0f) ||
        (p_str->inter.ts <= 0.0f) ||
        (omega <= 0.0f))
    {
        return false;
    }

    p_str->inter.w0 = omega;

    float ts = p_str->inter.ts;
    float kp = p_str->inter.kp;
    float kr = p_str->inter.kr;
    float w0 = p_str->inter.w0;
    float wc = p_str->inter.wc;

    // 分子系数 (从 z^2 到 z^0)
    float n0 = ts * ts * kp * w0 * w0 + 4 * ts * kp * wc + 4 * ts * kr * wc + 4 * kp;
    float n1 = 2 * ts * ts * kp * w0 * w0 - 8 * kp;
    float n2 = ts * ts * kp * w0 * w0 - 4 * ts * kp * wc - 4 * ts * kr * wc + 4 * kp;

    // 分母系数 (从 z^2 到 z^0)
    float d0 = ts * ts * w0 * w0 + 4 * ts * wc + 4;
    float d1 = 2 * ts * ts * w0 * w0 - 8;
    float d2 = ts * ts * w0 * w0 - 4 * ts * wc + 4;

    if (d0 == 0.0f)
    {
        return false;
    }

    p_str->inter.a1 = d1 / d0;
    p_str->inter.a2 = d2 / d0;
    p_str->inter.b0 = n0 / d0;
    p_str->inter.b1 = n1 / d0;
    p_str->inter.b2 = n2 / d0;
    return true;
}

void pr_reset(pr_t *p_str)
{
    if (p_str == NULL)
    {
        return;
    }

    p_str->inter.e[0] = 0.0f;
    p_str->inter.e[1] = 0.0f;
    p_str->inter.e[2] = 0.0f;
    p_str->inter.u[0] = 0.0f;
    p_str->inter.u[1] = 0.0f;
    p_str->inter.u[2] = 0.0f;
    p_str->output.raw = 0.0f;
    p_str->output.sat = 0.0f;
    p_str->output.val = 0.0f;
    p_str->output.is_saturated = false;
}
