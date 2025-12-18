#ifndef __PI_TUSTIN_H
#define __PI_TUSTIN_H

typedef struct
{
    float *p_act;
    float *p_ref;
} pi_tustin_input_t;

typedef struct
{
    float a1;
    float b0;
    float b1;
    float up_lmt;
    float dn_lmt;
    float e[2];
    float u[2];
} pi_tustin_inter_t;

typedef struct
{
    float val;
} pi_tustin_output_t;

typedef struct
{
    pi_tustin_input_t input;
    pi_tustin_inter_t inter;
    pi_tustin_output_t output;
} pi_tustin_t;

void pi_tustin_init(pi_tustin_t *p_str,
                    float kp,
                    float ki,
                    float ts,
                    float up_lmt,
                    float dn_lmt,
                    float *p_ref,
                    float *p_act);

void pi_tustin_cal(pi_tustin_t *p_str);

void pi_tustin_update(pi_tustin_t *p_str, float kp, float ki, float ts);

static inline void pi_tustin_update_b0(pi_tustin_t *p_str, float b0)
{
    p_str->inter.b0 = b0;
}

static inline void pi_tustin_update_b1(pi_tustin_t *p_str, float b1)
{
    p_str->inter.b1 = b1;
}

void pi_tustin_reset(pi_tustin_t *p_str);

#endif
