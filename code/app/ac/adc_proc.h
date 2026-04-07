#ifndef __ADC_PROC_H
#define __ADC_PROC_H

#include "section.h"
#include "adc.h"
#include "cal_rms.h"
#include "rly_on.h"
#include "my_math.h"
#include "gpio.h"
#include "rly_on.h"
#include "pfc_fsm.h"
#include "hys_cmp.h"
#include "ac_loss_det.h"
#include "chk_grid.h"

/*
 * Main relay close delay used by rly_on module.
 * Unit: second.
 */
#define MAIN_RLY_ON_DLY (8.0e-3f)

/* ===== VBUS PRECHARGE (after relay close) =====
 * Enter threshold/time: used to assert that precharge reached near input peak.
 * Exit threshold/time:  hysteresis branch to avoid state chatter.
 */
#define PFC_VBUS_PRECHARGE_ENTER (M_SQRT2 * 200.0f)
#define PFC_VBUS_PRECHARGE_EXIT (PFC_VBUS_PRECHARGE_ENTER - 30.0f)

#define PFC_VBUS_PRECHARGE_ENTER_TIME TIME_CNT_200MS_IN_1MS
#define PFC_VBUS_PRECHARGE_EXIT_TIME TIME_CNT_10MS_IN_1MS

/* ===== VBUS NOMINAL (normal operating range) =====
 * Nominal window for "in-regulation" bus classification.
 */
#define PFC_VBUS_NOM_ENTER (380.0f)
#define PFC_VBUS_NOM_EXIT (350.0f)
#define PFC_VBUS_NOM_ENTER_TIME TIME_CNT_100MS_IN_1MS
#define PFC_VBUS_NOM_EXIT_TIME TIME_CNT_10MS_IN_1MS

/* ===== GRID CHECK (RMS/FREQ WINDOW + TIME) =====
 * chk_grid_func() runs in REG_TASK(1), i.e. 100 us period.
 * Therefore judge/abnormal times are defined in 100 us counters.
 */
#define PFC_GRID_RMS_NORMAL_MIN_V (180.0f)
#define PFC_GRID_RMS_NORMAL_MAX_V (265.0f)
#define PFC_GRID_RMS_ABNORMAL_MIN_V (175.0f)
#define PFC_GRID_RMS_ABNORMAL_MAX_V (270.0f)

#define PFC_GRID_FREQ_NORMAL_MIN_HZ (45.0f)
#define PFC_GRID_FREQ_NORMAL_MAX_HZ (65.0f)
#define PFC_GRID_FREQ_ABNORMAL_MIN_HZ (40.0f)
#define PFC_GRID_FREQ_ABNORMAL_MAX_HZ (70.0f)

/* 500 ms stable-inside-normal-window required before asserting grid OK. */
#define PFC_GRID_JUDGE_TIME TIME_CNT_500MS_IN_100US
/* 10 ms inside-abnormal-window required before de-asserting grid OK. */
#define PFC_GRID_ABNORMAL_TIME TIME_CNT_10MS_IN_100US

/* ADC instantaneous signals and derived status exported for other modules. */
extern float v_g;
extern float v_cap;
extern float v_bus;
extern float i_l;
extern float i_out;
extern float v_g_rms;
extern float v_cap_rms;
extern float i_l_rms;
extern float i_out_rms;
extern pfc_vbus_sta_e vbus_sta;
extern uint8_t main_rly_is_closed;

uint8_t adc_proc_get_ac_is_ok(void);

/* Relay control wrappers used by PFC FSM HAL mapping. */
void adc_proc_main_rly_on(void);
void adc_proc_main_rly_off(void);

#endif
