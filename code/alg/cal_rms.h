#ifndef __CAL_RMS_H
#define __CAL_RMS_H

#include "stdint.h"

#define CAL_RMS_FREQ_MIN 20.0f

typedef enum
{
    CAL_RMS_STA_IDLE,
    CAL_RMS_STA_IS_POS,
    CAL_RMS_STA_IS_NEG,
    CAL_RMS_STA_CAL,
} CAL_RMS_STA_E;

typedef enum
{
    CAL_RMS_ROLE_NULL,
    CAL_RMS_MASTER,
    CAL_RMS_SLAVE,
} CAL_RMS_ROLE_E;

typedef struct
{
    float *p_val;
    uint8_t *p_is_cal;
    uint8_t *p_is_run;
} cal_rms_input_t;

typedef struct
{
    float ts;
    CAL_RMS_ROLE_E role;
    float cross_thr;
    float cnt_max;
} cal_rms_cfg_t;

typedef struct
{
    uint32_t cnt;
    CAL_RMS_STA_E sta;
    float square_sum;
} cal_rms_inter_t;

typedef struct
{
    float rms;
    float freq;
    uint8_t is_run;
    uint8_t is_cal;
} cal_rms_output_t;

typedef struct
{
    cal_rms_input_t input;
    cal_rms_cfg_t cfg;
    cal_rms_inter_t inter;
    cal_rms_output_t output;
} cal_rms_t;

void reset_calculation(cal_rms_t *str);

void cal_rms_init(cal_rms_t *str,
                  CAL_RMS_ROLE_E role,
                  float ts,
                  float cross_thr,
                  float *p_val,
                  uint8_t *p_is_cal,
                  uint8_t *p_is_run);

void cal_rms_master_run(cal_rms_t *str);

void cal_rms_slave_run(cal_rms_t *str);

#endif
