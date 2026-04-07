#include "bb_ol.h"
#include <stddef.h>
#include "my_math.h"

static inline float bb_ol_safe_sqrt(float x)
{
    return (x > 0.0f) ? sqrtf(x) : 0.0f;
}

static inline float bb_ol_safe_div(float numerator, float denominator)
{
    return (fabsf(denominator) > 1e-6f) ? (numerator / denominator) : (numerator / 1e-6f);
}

static inline float bb_ol_limit_duty(float duty)
{
    if (duty < 0.0f)
    {
        return 0.0f;
    }
    if (duty > 1.0f)
    {
        return 1.0f;
    }
    return duty;
}

static float bb_ol_calc_boost_dcm_duty(float i_ref, float t_s, float v_in, float v_out, float l)
{
    float factor = (2.0f * i_ref * t_s * l * (v_out - v_in)) / (v_in * v_out);
    float result = bb_ol_safe_sqrt(factor);
    return bb_ol_limit_duty(bb_ol_safe_div(result, t_s));
}

static float bb_ol_calc_buck_dcm_duty(float i_ref, float t_s, float v_in, float v_out, float l)
{
    float denominator = (v_in * (v_in - v_out)) / l;
    float factor = (2.0f * i_ref * t_s * v_out) / denominator;
    float result = bb_ol_safe_sqrt(factor);
    return bb_ol_limit_duty(bb_ol_safe_div(result, t_s));
}

static float bb_ol_calc_buck_boost_dcm_ton(float d,
                                           float t,
                                           float w,
                                           float v_in,
                                           float v_out,
                                           float l,
                                           float i_ref)
{
    float w_v_in = w - v_in;
    float w_v_in_3_v_out = w_v_in - 3.0f * v_out;
    float term1 = 2.0f * l * i_ref * w_v_in * w_v_in_3_v_out;
    float temp = w * w_v_in * v_in + (w * w - w * v_in + v_in * v_in) * v_out;
    float term2 = d * d * t * temp;
    float sqrt_term = bb_ol_safe_sqrt(t * v_out * (term1 + term2));
    float numerator = d * t * (w + v_in) * (w_v_in - v_out) + 2.0f * sqrt_term;
    float denominator = w_v_in * w_v_in_3_v_out;

    return bb_ol_safe_div(numerator, denominator);
}

static float bb_ol_check_buck_duty(float w, float t, float l, float v_in, float v_out, float i_ref)
{
    float numerator = 2.0f * l * i_ref * v_out;
    float denominator = t * ((w + v_in) * (w + v_in) + (w - v_in) * v_out);
    return bb_ol_limit_duty(2.0f * bb_ol_safe_sqrt(bb_ol_safe_div(numerator, denominator)));
}

static void bb_ol_calc_buck_boost_dcm_duty(float v_in,
                                           float v_out,
                                           float i_ref,
                                           float t,
                                           float l,
                                           float *p_buck_duty,
                                           float *p_boost_duty)
{
    float w = 0.0f;
    float ton = 0.0f;

    if ((p_buck_duty == NULL) ||
        (p_boost_duty == NULL))
    {
        return;
    }

    if (i_ref < 0.0f)
    {
        *p_buck_duty = 0.0f;
        *p_boost_duty = 0.0f;
        return;
    }

    w = (v_in > v_out) ? (v_in - v_out) : 0.0f;

    if (w > 0.0f)
    {
        *p_buck_duty = bb_ol_check_buck_duty(w, t, l, v_in, v_out, i_ref);
        if (*p_buck_duty > BB_OL_FIX_DUTY)
        {
            *p_buck_duty = BB_OL_FIX_DUTY;
        }
    }
    else
    {
        *p_buck_duty = BB_OL_FIX_DUTY;
    }

    ton = bb_ol_calc_buck_boost_dcm_ton(*p_buck_duty, t, w, v_in, v_out, l, i_ref);
    *p_boost_duty = bb_ol_limit_duty(bb_ol_safe_div(ton, t));
}

void bb_ol_init(bb_ol_t *p_ol,
                bb_ol_mode_e *p_mode,
                float *p_i_ref,
                float *p_pwm_ts,
                float *p_v_in,
                float *p_v_out,
                float *p_l)
{
    if (p_ol == NULL)
    {
        return;
    }

    p_ol->input.p_mode = p_mode;
    p_ol->input.p_i_ref = p_i_ref;
    p_ol->input.p_pwm_ts = p_pwm_ts;
    p_ol->input.p_v_in = p_v_in;
    p_ol->input.p_v_out = p_v_out;
    p_ol->input.p_l = p_l;

    p_ol->output.buck_duty = 0.0f;
    p_ol->output.buck_up_en = 0U;
    p_ol->output.buck_dn_en = 0U;
    p_ol->output.boost_duty = 0.0f;
    p_ol->output.boost_up_en = 0U;
    p_ol->output.boost_dn_en = 0U;
}

void bb_ol_func(bb_ol_t *p_ol)
{
    if ((p_ol == NULL) ||
        (p_ol->input.p_mode == NULL) ||
        (p_ol->input.p_i_ref == NULL) ||
        (p_ol->input.p_pwm_ts == NULL) ||
        (p_ol->input.p_v_in == NULL) ||
        (p_ol->input.p_v_out == NULL) ||
        (p_ol->input.p_l == NULL))
    {
        return;
    }

    const bb_ol_mode_e mode = *p_ol->input.p_mode;
    float i_ref = *p_ol->input.p_i_ref;
    const float pwm_ts = *p_ol->input.p_pwm_ts;
    const float v_in = *p_ol->input.p_v_in;
    const float v_out = *p_ol->input.p_v_out;
    const float l = *p_ol->input.p_l;

    switch (mode)
    {
    case BB_OL_MODE_BOOST:
        DN_LMT(i_ref, 0.0f);
        p_ol->output.buck_duty = 1.0f;
        p_ol->output.boost_duty = 1.0f - bb_ol_calc_boost_dcm_duty(i_ref, pwm_ts, v_in, v_out, l);
        p_ol->output.buck_dn_en = 1U;
        p_ol->output.buck_up_en = 1U;
        p_ol->output.boost_dn_en = 1U;
        p_ol->output.boost_up_en = 0U;
        break;

    case BB_OL_MODE_BUCK:
        DN_LMT(i_ref, 0.0f);
        p_ol->output.buck_duty = bb_ol_calc_buck_dcm_duty(i_ref, pwm_ts, v_in, v_out, l);
        p_ol->output.boost_duty = 1.0f;
        p_ol->output.buck_dn_en = 0U;
        p_ol->output.buck_up_en = 1U;
        p_ol->output.boost_dn_en = 1U;
        p_ol->output.boost_up_en = 1U;
        break;

    case BB_OL_MODE_BUCK_BOOST:
        bb_ol_calc_buck_boost_dcm_duty(v_in,
                                       v_out,
                                       i_ref,
                                       pwm_ts,
                                       l,
                                       &p_ol->output.buck_duty,
                                       &p_ol->output.boost_duty);
        p_ol->output.boost_duty = 1.0f - p_ol->output.boost_duty;
        p_ol->output.buck_dn_en = 0U;
        p_ol->output.buck_up_en = 1U;
        p_ol->output.boost_dn_en = 1U;
        p_ol->output.boost_up_en = 0U;
        break;

    default:
        p_ol->output.buck_duty = 0.0f;
        p_ol->output.buck_up_en = 0U;
        p_ol->output.buck_dn_en = 0U;
        p_ol->output.boost_duty = 0.0f;
        p_ol->output.boost_up_en = 0U;
        p_ol->output.boost_dn_en = 0U;
        break;
    }

    p_ol->output.buck_duty = bb_ol_limit_duty(p_ol->output.buck_duty);
    p_ol->output.boost_duty = bb_ol_limit_duty(p_ol->output.boost_duty);
}
