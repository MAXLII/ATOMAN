#ifndef __ADC_CHK_H
#define __ADC_CHK_H

#include "my_math.h"
#include "adc_chk.h"
#include "adc.h"
#include "section.h"
#include "sogi.h"
#include "fll.h"
#include "cal_rms.h"
#include "my_math.h"
#include "chk_grid.h"
#include "notch.h"

#define RLY_ON_SW_TIME 8.0e-3f

#define ADC_CHECK_GRID_RMS_NORMAL_MAX 265.0f
#define ADC_CHECK_GRID_RMS_NORMAL_MIN 176.0f
#define ADC_CHECK_GRID_RMS_ABNORMAL_MAX 270.0f
#define ADC_CHECK_GRID_RMS_ABNORMAL_MIN 170.0f

#define ADC_CHECK_GRID_FREQ_NORMAL_MAX (75.0f)
#define ADC_CHECK_GRID_FREQ_NORMAL_MIN (45.0f)
#define ADC_CHECK_GRID_FREQ_ABNORMAL_MAX (85.0f)
#define ADC_CHECK_GRID_FREQ_ABNORMAL_MIN (35.0f)

#define ADC_CHECK_GRID_JUDGE_TIME TIME_CNT_200MS_IN_1MS
#define ADC_CHECK_GRID_ABNORMAL_TIME TIME_CNT_200MS_IN_1MS

#define ADC_CHECK_BULK_OK_THR 380.0f
#define ADC_CHECK_BULK_NG_THR 350.0f
#define ADC_CHECK_BULK_OK_TIME TIME_CNT_100MS_IN_1MS
#define ADC_CHECK_BULK_NG_TIME TIME_CNT_10MS_IN_1MS

typedef struct
{
    float *p_val;
    float thr;
    float thr_hys;
    uint32_t cnt;
    uint32_t is_ok_time;
    uint8_t is_ok;
} chk_val_t;

uint8_t adc_chk_get_ac_is_ok(void);

uint8_t adc_chk_get_bus_is_ok(void);

uint8_t adc_chk_get_grid_ss_v_bus_is_ok(void);

void adc_chk_set_rly_on_grid_trig(void);
void adc_chk_set_rly_off_grid_trig(void);

extern float i_l;
extern float v_grid;
extern float v_cap;
extern float v_bus;
extern float v_out;
extern float i_out;
extern sogi_t sogi_v_grid;
extern fll_state_t fll_v_grid;
extern cal_rms_t cal_rms_v_g;
extern cal_rms_t cal_rms_i_l;
extern cal_rms_t cal_rms_v_out;
extern cal_rms_t cal_rms_i_out;
extern chk_grid_t chk_grid;
extern notch_t notch_v_bus;
extern float ac_out_pwr_act;
extern float ac_out_pwr_total;
extern float ac_out_pwr_rms;

#endif
