#ifndef __PID_H
#define __PID_H

#include <stdbool.h>

typedef struct
{
    float *p_act;
    float *p_ref;
} pid_input_t;

typedef struct
{
    float kp;
    float ki;
    float ki_inv;
    float kd;
    float up_lmt;
    float dn_lmt;
    float i_err_up_lmt;
    float i_err_dn_lmt;
    float err;
    float err_last;
    float err_diff;
    float i_err;
} pid_inter_t;

typedef struct
{
    float val;
} pid_output_t;

typedef struct
{
    pid_input_t input;
    pid_inter_t inter;
    pid_output_t output;
} pid_t;

bool pid_init(pid_t *p_str,
              float kp,
              float ki,
              float kd,
              float up_lmt,
              float dn_lmt,
              float *p_ref,
              float *p_act);

bool pid_cal(pid_t *p_str);

bool pid_update(pid_t *p_str, float kp, float ki, float kd);

void pid_reset(pid_t *p_str);

#endif
