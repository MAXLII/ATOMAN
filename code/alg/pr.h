#ifndef __PR_H
#define __PR_H

typedef struct
{
    float err;
} pr_input_t;

typedef struct
{
    float kp;
    float kr;
    float w0;
    float wc;
    float ts;
    float lmt;
} pr_cfg_t;

typedef struct
{
    float a1;
    float a2;
    float b0;
    float b1;
    float b2;

    float e[3];
    float u[3];
} pr_inter_t;

typedef struct
{
    float val;
} pr_output_t;

typedef struct
{
    pr_input_t input;
    pr_cfg_t cfg;
    pr_inter_t inter;
    pr_output_t output;
} pr_t;

void pr_init(pr_t *p_str, pr_cfg_t *p_cfg);

void pr_cal(pr_t *p_str);

void pr_update_freq(pr_t *p_str, float omega);

#endif
