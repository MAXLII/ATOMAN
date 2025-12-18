#ifndef __CTRL_LLC_CHG_H
#define __CTRL_LLC_CHT_H

#include "llc_hardware.h"
#include "my_math.h"

#define CHG_VOLT_LOOP_FREQ_CUT 600.0f
#define CHG_VOLT_LOOP_PM 60.0f
#define CHG_VOLT_LOOP_W_CUT (2.0f * M_PI * CHG_VOLT_LOOP_FREQ_CUT)
#define CHG_VOLT_LOOP_KP (sinf(CHG_VOLT_LOOP_PM * M_PI / 180.0f) * CHG_VOLT_LOOP_W_CUT * CAP_BAT_VALUE)
#define CHG_VOLT_LOOP_KI (CHG_VOLT_LOOP_W_CUT * tanf(CHG_VOLT_LOOP_PM * M_PI / 180.0f))
#define CHG_VOLT_LOOP_UP_LMT 160.0f
#define CHG_VOLT_LOOP_DN_LMT -160.0f
#define CHG_PWR_LMT 500.0f

#define CHG_CURR_LOOP_KP 0.004f
#define CHG_CURR_LOOP_KI (CHG_CURR_LOOP_KP * 2 * M_PI * 3000.0f)
#define CHG_CURR_LOOP_UP_LMT 1.0f
#define CHG_CURR_LOOP_DN_LMT -1.0f

#define LLC_DSG_VOLT_LOOP_FREQ_CUT 600.0f
#define LLC_DSG_VOLT_LOOP_PM 60.0f
#define LLC_DSG_VOLT_LOOP_W_CUT (2.0f * M_PI * LLC_DSG_VOLT_LOOP_FREQ_CUT)
#define LLC_DSG_VOLT_LOOP_KP (sinf(LLC_DSG_VOLT_LOOP_PM * M_PI / 180.0f) * LLC_DSG_VOLT_LOOP_W_CUT * CAP_BUS_VALUE)
#define LLC_DSG_VOLT_LOOP_KI (LLC_DSG_VOLT_LOOP_W_CUT * LLC_DSG_VOLT_LOOP_KP / tanf(LLC_DSG_VOLT_LOOP_PM * M_PI / 180.0f))
#define LLC_DSG_VOLT_LOOP_UP_LMT 160.0f
#define LLC_DSG_VOLT_LOOP_DN_LMT -160.0f

void ctrl_llc_enable(void);

void ctrl_llc_disable(void);

float ctrl_llc_get_v_bus_ref_tag(void);

#endif
