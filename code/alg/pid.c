#include "pid.h"
#include <stdbool.h>
#include <stddef.h>

bool pid_init(pid_t *p_str,
              float kp,
              float ki,
              float kd,
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

    p_str->inter.kp = 0.0f;
    p_str->inter.ki = 0.0f;
    p_str->inter.ki_inv = 0.0f;
    p_str->inter.kd = 0.0f;
    p_str->inter.up_lmt = up_lmt;
    p_str->inter.dn_lmt = dn_lmt;
    if (ki > 1e-6f)
    {
        p_str->inter.i_err_up_lmt = up_lmt / ki;
        p_str->inter.i_err_dn_lmt = dn_lmt / ki;
    }
    else
    {
        p_str->inter.i_err_up_lmt = 0.0f;
        p_str->inter.i_err_dn_lmt = 0.0f;
    }

    p_str->inter.err = 0.0f;
    p_str->inter.err_last = 0.0f;
    p_str->inter.err_diff = 0.0f;
    p_str->inter.i_err = 0.0f;
    p_str->output.val = 0.0f;

    return pid_update(p_str, kp, ki, kd);
}

bool pid_update(pid_t *p_str, float kp, float ki, float kd)
{
    if ((p_str == NULL) ||
        (kp < 0.0f) ||
        (ki < 0.0f) ||
        (kd < 0.0f))
    {
        return false;
    }

    p_str->inter.kp = kp;
    p_str->inter.ki = ki;
    p_str->inter.kd = kd;
    p_str->inter.ki_inv = (ki > 0.0f) ? (1.0f / ki) : 0.0f;

    return true;
}

bool pid_cal(pid_t *p_str)
{
    if ((p_str == NULL) ||
        (p_str->input.p_ref == NULL) ||
        (p_str->input.p_act == NULL))
    {
        return false;
    }

    p_str->inter.err = *p_str->input.p_ref - *p_str->input.p_act;
    p_str->inter.err_diff = p_str->inter.err - p_str->inter.err_last;

    const float p_term = p_str->inter.kp * p_str->inter.err;
    const float d_term = p_str->inter.kd * p_str->inter.err_diff;
    const float i_err_prev = p_str->inter.i_err;
    float i_err_next = i_err_prev + p_str->inter.err;

    if (i_err_next > p_str->inter.i_err_up_lmt)
    {
        i_err_next = p_str->inter.i_err_up_lmt;
    }
    else if (i_err_next < p_str->inter.i_err_dn_lmt)
    {
        i_err_next = p_str->inter.i_err_dn_lmt;
    }

    float u = p_term + p_str->inter.ki * i_err_next + d_term;

    if (u > p_str->inter.up_lmt)
    {
        u = p_str->inter.up_lmt;
        if (p_str->inter.err > 0.0f)
        {
            i_err_next = i_err_prev;
        }
    }
    else if (u < p_str->inter.dn_lmt)
    {
        u = p_str->inter.dn_lmt;
        if (p_str->inter.err < 0.0f)
        {
            i_err_next = i_err_prev;
        }
    }

    if (i_err_next > p_str->inter.i_err_up_lmt)
    {
        i_err_next = p_str->inter.i_err_up_lmt;
    }
    else if (i_err_next < p_str->inter.i_err_dn_lmt)
    {
        i_err_next = p_str->inter.i_err_dn_lmt;
    }

    p_str->inter.i_err = i_err_next;
    p_str->inter.err_last = p_str->inter.err;
    p_str->output.val = u;

    return true;
}

void pid_reset(pid_t *p_str)
{
    if (p_str == NULL)
    {
        return;
    }

    p_str->inter.err = 0.0f;
    p_str->inter.err_last = 0.0f;
    p_str->inter.err_diff = 0.0f;
    p_str->inter.i_err = 0.0f;
    p_str->output.val = 0.0f;
}
