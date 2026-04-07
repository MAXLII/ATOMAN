#ifndef __BB_MODE_H
#define __BB_MODE_H

#include <stdint.h>

#define BB_MODE_SSW_TO_BB_MODE_THR (0.95f)
#define BB_MODE_BB_MODE_TO_SSW_THR (0.85f)
#define BB_MODE_DUTY_SUM (1.9f)
#define BB_MODE_DUTY_MAX (0.97f)
#define BB_MODE_BUCK_BOOST_FIXED_BOOST_DUTY (0.93f)

typedef enum
{
    BB_MODE_BUCK = 0,
    BB_MODE_BOOST,
    BB_MODE_BUCK_BOOST,
} bb_mode_e;

typedef struct
{
    float *p_v_l;
    float *p_v_in;
    float *p_v_out;
} bb_mode_input_t;

typedef struct
{
    bb_mode_e mode;
} bb_mode_inter_t;

typedef struct
{
    float buck_duty;
    float boost_duty;
    uint8_t is_half_freq;
} bb_mode_output_t;

typedef struct
{
    bb_mode_input_t input;
    bb_mode_inter_t inter;
    bb_mode_output_t output;
} bb_mode_t;

void bb_mode_init(bb_mode_t *p_mode,
                  float *p_v_l,
                  float *p_v_in,
                  float *p_v_out,
                  bb_mode_e mode);
void bb_mode_func(bb_mode_t *p_mode);

#endif
