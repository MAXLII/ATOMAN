#ifndef __NOTCH_H
#define __NOTCH_H

typedef struct
{
    float *p_val;
} notch_input_t;

typedef struct
{
    float w0;
    float wb;
    float ts;
} notch_cfg_t;

typedef struct
{
    float c0;
    float c1;
    float c2;
    float d1;
    float d2;
    float x[3];
    float y[3];
} notch_inter_t;

typedef struct
{
    float val;
} notch_output_t;

typedef struct
{
    notch_input_t input;
    notch_cfg_t cfg;
    notch_inter_t inter;
    notch_output_t output;
} notch_t;

void notch_init(notch_t *p_str, notch_cfg_t *p_cfg, float *p_val);

void notch_update_freq(notch_t *p_str, float omega);

void notch_cal(notch_t *p_str);

#endif
