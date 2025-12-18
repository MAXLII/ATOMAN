#ifndef __ADC_CHK_H
#define __ADC_CHK_H

#include "my_math.h"
#include "stdint.h"

#define PV_VOLT_MIN_VLD 10.0f
#define PV_VOLT_MAX_VLD 60.0f
#define PV_VOLT_MIN_EXIT 8.0f
#define PV_VOLT_MAX_EXIT 62.0f
#define PV_VOLT_MIN_INST 7.0f
#define PV_VOLT_MAX_INST 65.0f
#define PV_VOLT_ENTER_DLY TIME_CNT_100MS_IN_1MS
#define PV_VOLT_EXIT_DLY TIME_CNT_100MS_IN_1MS

#define BAT_VOLT_MIN_VLD 2.5f
#define BAT_VOLT_MAX_VLD 3.85f
#define BAT_VOLT_MIN_EXIT 2.3f
#define BAT_VOLT_MAX_EXIT 4.0f
#define BAT_VOLT_MIN_INST 2.1f
#define BAT_VOLT_MAX_INST 4.3f
#define BAT_VOLT_ENTER_DLY TIME_CNT_100MS_IN_1MS
#define BAT_VOLT_EXIT_DLY TIME_CNT_100MS_IN_1MS

extern float v_bat;
extern float i_bat;
extern float i_buck_in;
extern float v_buck_in;
extern float pwr_buck_in;

uint8_t adc_chk_get_pv_in_is_ok(void);

uint8_t adc_chk_get_bat_is_ok(void);

#endif
