#ifndef __PFC_CTRL_H
#define __PFC_CTRL_H

#include "pfc_hal.h"
#include "hw_params.h"
#include "my_math.h"

#define PFC_CTRL_VOLT_LOOP_FREQ_CUT 20.0f
#define PFC_CTRL_VOLT_LOOP_PM 60.0f
#define PFC_CTRL_VOLT_LOOP_W_CUT (2.0f * M_PI * PFC_CTRL_VOLT_LOOP_FREQ_CUT)
#define PFC_CTRL_VOLT_LOOP_KP (sinf(PFC_CTRL_VOLT_LOOP_PM * M_PI / 180.0f) * PFC_CTRL_VOLT_LOOP_W_CUT * HW_DC_BUS_CAP_VALUE)
#define PFC_CTRL_VOLT_LOOP_KI (PFC_CTRL_VOLT_LOOP_W_CUT * PFC_CTRL_VOLT_LOOP_KP / tanf(PFC_CTRL_VOLT_LOOP_PM * M_PI / 180.0f))

#define PFC_CTRL_CURR_LOOP_FREQ_CUT 2500.0f
#define PFC_CTRL_CURR_LOOP_W_CUT (2.0f * M_PI * PFC_CTRL_CURR_LOOP_FREQ_CUT)
#define PFC_CTRL_CURR_LOOP_PM 60.0f
#define PFC_CTRL_CURR_LOOP_KP (sinf(PFC_CTRL_CURR_LOOP_PM / 180.0f * M_PI) * PFC_CTRL_CURR_LOOP_W_CUT * HW_AC_SIDE_IND_VALUE)
#define PFC_CTRL_CURR_LOOP_KI (PFC_CTRL_CURR_LOOP_KP * PFC_CTRL_CURR_LOOP_W_CUT / tanf(PFC_CTRL_CURR_LOOP_PM / 180.0f * M_PI))

/*
 * PR inner-current-loop design:
 *
 *                 2 * kr * wc * s
 * Gc(s) = kp + -------------------------
 *               s^2 + 2 * wc * s + w0^2
 *
 * with:
 *   wx = 2 * pi * PFC_CTRL_CURR_LOOP_PR_FREQ_CUT
 *   phim = PFC_CTRL_CURR_LOOP_PR_PM
 *   w0 = 2 * pi * 50
 *   wc = 2 * pi * 2
 *   kp ~= L * wx * sin(phim) - 2 * L * wc * cos(phim)
 *   kr ~= L * wx^2 * cos(phim) / (2 * wc)
 */
#define PFC_CTRL_CURR_LOOP_PR_FREQ_CUT 2500.0f
#define PFC_CTRL_CURR_LOOP_PR_W_CUT (2.0f * M_PI * PFC_CTRL_CURR_LOOP_PR_FREQ_CUT)
#define PFC_CTRL_CURR_LOOP_PR_PM 50.0f
#define PFC_CTRL_CURR_LOOP_PR_W0 (2.0f * M_PI * 50.0f)
#define PFC_CTRL_CURR_LOOP_PR_WC (2.0f * M_PI * 2.0f)
#define PFC_CTRL_CURR_LOOP_PR_KP ((HW_AC_SIDE_IND_VALUE * PFC_CTRL_CURR_LOOP_PR_W_CUT * sinf(PFC_CTRL_CURR_LOOP_PR_PM / 180.0f * M_PI)) - (2.0f * HW_AC_SIDE_IND_VALUE * PFC_CTRL_CURR_LOOP_PR_WC * cosf(PFC_CTRL_CURR_LOOP_PR_PM / 180.0f * M_PI)))
#define PFC_CTRL_CURR_LOOP_PR_KR ((HW_AC_SIDE_IND_VALUE * PFC_CTRL_CURR_LOOP_PR_W_CUT * PFC_CTRL_CURR_LOOP_PR_W_CUT * cosf(PFC_CTRL_CURR_LOOP_PR_PM / 180.0f * M_PI)) / (2.0f * PFC_CTRL_CURR_LOOP_PR_WC))

/* Bus-voltage notch filter design parameters. */
#define PFC_CTRL_VBUS_NOTCH_CENTER_RADPS (2.0f * M_PI * 100.0f)
#define PFC_CTRL_VBUS_NOTCH_BANDWIDTH_RADPS (2.0f * M_PI * 10.0f)

/* Outer bus-voltage loop output limits. */
#define PFC_CTRL_VBUS_LOOP_OUT_MAX (60.0f)
#define PFC_CTRL_VBUS_LOOP_OUT_MIN (-3.0f)

/* Inner inductor-current loop output limits. */
#define PFC_CTRL_IND_CURR_LOOP_OUT_MAX (30.0f)
#define PFC_CTRL_IND_CURR_LOOP_OUT_MIN (-30.0f)

/* Startup reference preset used before the controller fully takes over. */
#define PFC_CTRL_STARTUP_VBUS_INIT_BOOST_V (30.0f)
#define PFC_CTRL_VBUS_OV_RELAX_RATIO (1.05f)
#define PFC_CTRL_VBUS_OV_HIST_DECAY (0.90f)

/* Final modulation clamp handled at the PWM command side. */
#define PFC_CTRL_PWM_DUTY_MIN (-0.98f)
#define PFC_CTRL_PWM_DUTY_MAX (0.98f)

/* Grid synchronization parameters. */
#define PFC_CTRL_SOGI_GAIN (1.0f)
#define PFC_CTRL_FLL_GAIN (150.0f)
#define PFC_CTRL_GRID_OMEGA_INIT_RADPS (2.0f * M_PI * 50.0f)

/* Current-reference shaping parameter. */
#define PFC_CTRL_GRID_RMS_NOMINAL_V (230.0f)

void pfc_ctrl_set_p_hal(pfc_ctrl_hal_t *p);
void pfc_ctrl_prepare_run(void);

#endif
