#ifndef __CTRL_BUCK_H
#define __CTRL_BUCK_H

#include "my_math.h"
#include "buck_hardware.h"
#include "stdint.h"

#define BUCK_I_IN_LMT 15.0f

#define BUCK_VOLT_LOOP_FREQ_CUT 100.0f
#define BUCK_VOLT_LOOP_W_CUT (2.0f * M_PI * BUCK_VOLT_LOOP_FREQ_CUT)
#define BUCK_VOLT_LOOP_PM 60.0f
#define BUCK_VOLT_LOOP_KP (sinf(BUCK_VOLT_LOOP_PM * M_PI / 180.0f) * BUCK_VOLT_LOOP_W_CUT * BUCK_CAP_BAT_VALUE)
#define BUCK_VOLT_LOOP_KI (BUCK_VOLT_LOOP_W_CUT * BUCK_VOLT_LOOP_KP / tanf(BUCK_VOLT_LOOP_PM * M_PI / 180.0f))

#define BUCK_CURR_LOOP_FREQ_CUT 700.0f
#define BUCK_CURR_LOOP_W_CUT (2.0f * M_PI * BUCK_CURR_LOOP_FREQ_CUT)
#define BUCK_CURR_LOOP_PM 60.0f
#define BUCK_CURR_LOOP_KP (sinf(BUCK_CURR_LOOP_PM * M_PI / 180.0f) *      \
                           (BUCK_CURR_LOOP_W_CUT * BUCK_CURR_LOOP_W_CUT * \
                                BUCK_L_VALUE * BUCK_CAP_BAT_VALUE -       \
                            1.0f) /                                       \
                           (BUCK_CURR_LOOP_W_CUT * BUCK_CAP_BAT_VALUE))
#define BUCK_CURR_LOOP_KI (BUCK_CURR_LOOP_W_CUT * BUCK_CURR_LOOP_KP / tanf(BUCK_CURR_LOOP_PM * M_PI / 180.0f))

#define BUCK_DCM_LOOP_KP 0.5f
#define BUCK_DCM_LOOP_KI 2.5f

#define BUCK_VIN_LMT_DCM_LOOP_KP 0.5f
#define BUCK_VIN_LMT_DCM_LOOP_KI 2.5f

#define BUCK_VIN_LMT_LOOP_FREQ_CUT 750.0f
#define BUCK_VIN_LMT_LOOP_W_CUT (2.0f * M_PI * BUCK_VIN_LMT_LOOP_FREQ_CUT)
#define BUCK_VIN_LMT_LOOP_PM 80.0f
#define BUCK_VIN_LMT_LOOP_KP (sinf(BUCK_VIN_LMT_LOOP_PM * M_PI / 180.0f) * BUCK_VIN_LMT_LOOP_W_CUT * BUCK_CAP_IN_VALUE)
#define BUCK_VIN_LMT_LOOP_KI (BUCK_VIN_LMT_LOOP_W_CUT * BUCK_VIN_LMT_LOOP_KP / tanf(BUCK_VIN_LMT_LOOP_PM * M_PI / 180.0f))

#define BUCK_CHG_PWR_LMT 350.0f
#define BUCK_CHG_CURR_LMT 15.0f

void ctrl_buck_enable(void);

void ctrl_buck_disable(void);

uint8_t ctrl_buck_get_enable(void);

#endif
