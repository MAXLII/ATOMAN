#ifndef __CTRL_PFC_H
#define __CTRL_PFC_H

#include "my_math.h"
#include "pfc_hardware.h"

#define PFC_VOLT_LOOP_FREQ_CUT 20.0f
#define PFC_VOLT_LOOP_PM 60.0f
#define PFC_VOTL_LOOP_W_CUT (2.0f * M_PI * PFC_VOLT_LOOP_FREQ_CUT)
#define PFC_VOLT_LOOP_KP (sinf(PFC_VOLT_LOOP_PM * M_PI / 180.0f) * PFC_VOTL_LOOP_W_CUT * CAP_BUS_VALUE)
#define PFC_VOLT_LOOP_KI (PFC_VOTL_LOOP_W_CUT * PFC_VOLT_LOOP_KP / tanf(PFC_VOLT_LOOP_PM * M_PI / 180.0f))

#define PFC_CURR_LOOP_FREQ_CUT 1000.0f
#define PFC_CURR_LOOP_W_CUT (2.0f * M_PI * PFC_CURR_LOOP_FREQ_CUT)
#define PFC_CURR_LOOP_PM 60.0f
#define PFC_CURR_LOOP_KP (sinf(PFC_CURR_LOOP_PM / 180.0f * M_PI) * PFC_CURR_LOOP_W_CUT * L_PFC_VALUE)
#define PFC_CURR_LOOP_KI (PFC_CURR_LOOP_KP * PFC_CURR_LOOP_W_CUT / tanf(PFC_CURR_LOOP_PM / 180.0f * M_PI))

void ctrl_pfc_enable(void);

void ctrl_pfc_disable(void);

float ctrl_pfc_get_v_bus_ref_tag(void);

#endif
