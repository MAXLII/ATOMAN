#ifndef __HYS_CMP_H
#define __HYS_CMP_H

#include "stdint.h"

typedef struct
{
    float *p_val;
} hys_cmp_input_t;

typedef struct
{
    float thr;
    float thr_hys;
    uint32_t time;
    uint32_t time_hys;
    uint8_t (*p_cmp_func)(float val, float thr);
    uint8_t (*p_cmp_hys_func)(float val, float thr);
} hys_cmp_cfg_t;

typedef struct
{
    hys_cmp_cfg_t *p_cfg;
    uint32_t cnt;
} hys_cmp_inter_t;

typedef struct
{
    uint8_t is_protect;
} hys_cmp_output_t;

typedef struct
{
    hys_cmp_input_t input;
    hys_cmp_inter_t inter;
    hys_cmp_output_t output;
} hys_cmp_t;

void hys_cmp_init(hys_cmp_t *p_str, float *p_val, hys_cmp_cfg_t *p_cfg);

void hys_cmp_func(hys_cmp_t *p_str);

uint8_t cmp_gt(float val, float thr);

uint8_t cmp_lt(float val, float thr);

#endif
