#ifndef __CTRL_LLC_CHG_H
#define __CTRL_LLC_CHT_H

#include "llc_hardware.h"
#include "my_math.h"
#include "stdint.h"

#define CHG_VOLT_LOOP_FREQ_CUT 300.0f
#define CHG_VOLT_LOOP_W_CUT (2.0f * M_PI * CHG_VOLT_LOOP_FREQ_CUT)
#define CHG_VOLT_LOOP_PM 60.0f
#define CHG_VOLT_LOOP_KP (sinf(CHG_VOLT_LOOP_PM / 180.0f * M_PI) * CHG_VOLT_LOOP_W_CUT * CAP_BAT_VALUE)
#define CHG_VOLT_LOOP_KI (CHG_VOLT_LOOP_W_CUT * CHG_VOLT_LOOP_KP / tanf(CHG_VOLT_LOOP_PM / 180.0f * M_PI))
#define CHG_VOLT_LOOP_UP_LMT 160.0f
#define CHG_VOLT_LOOP_DN_LMT -10.0f
#define CHG_PWR_LMT 500.0f

#define CHG_CURR_LOOP_KP 0.0004f
#define CHG_CURR_LOOP_KI CHG_CURR_LOOP_KP * 2 * M_PI * 25.0f / tanf(60.0f / 180.0f * M_PI)
#define CHG_CURR_LOOP_UP_LMT 1.0f
#define CHG_CURR_LOOP_DN_LMT 0.0f

void ctrl_llc_chg_enable(void);

void ctrl_llc_chg_disable(void);

uint8_t ctrl_llc_chg_get_enable(void);

#endif
