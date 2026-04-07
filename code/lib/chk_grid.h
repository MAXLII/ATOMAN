#ifndef __CHK_GRID_H
#define __CHK_GRID_H
#include "stdint.h"

typedef struct
{
    float *p_rms;
    float *p_freq;
} chk_grid_input_t;

typedef struct
{
    float max;
    float min;
} chk_grid_lmt_t;

typedef struct
{
    chk_grid_lmt_t normal;
    chk_grid_lmt_t abnormal;
} chk_grid_normal_t;

typedef struct
{
    uint32_t judge_time;
    uint32_t abnormale_time;
    chk_grid_normal_t rms;
    chk_grid_normal_t freq;
} chk_grid_cfg_t;

typedef struct
{
    uint32_t is_ok_cnt;
    uint32_t abnormal_cnt;
} chk_grid_inter_t;

typedef struct
{
    uint8_t is_ok;
} chk_grid_output_t;

typedef struct
{
    chk_grid_input_t input;
    chk_grid_cfg_t cfg;
    chk_grid_inter_t inter;
    chk_grid_output_t output;
} chk_grid_t;

void chk_grid_init(chk_grid_t *p_str,
                   float *p_rms,
                   float *p_freq,
                   uint32_t judge_time,
                   uint32_t abnormale_time,
                   float rms_normal_max,
                   float rms_normal_min,
                   float rms_abnormal_max,
                   float rms_abnormal_min,
                   float freq_normal_max,
                   float freq_normal_min,
                   float freq_abnormal_max,
                   float freq_abnormal_min);

void chk_grid_func(chk_grid_t *p_str);

void chk_grid_reset(chk_grid_t *p_str);

#endif
