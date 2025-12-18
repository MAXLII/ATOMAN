#include "adc_chk.h"
#include "adc.h"
#include "section.h"
#include "cal_rms.h"
#include "my_math.h"
#include "chk_grid.h"
#include "notch.h"
#include "rly_on.h"
#include "gpio.h"
#include "data_com.h"
#include "pfc_hardware.h"
#include "ac_loss_det.h"
#include "ctrl_pfc.h"
#include "ctrl_inv.h"
#include "fault.h"

float i_l = 0.0f;
float v_grid = 0.0f;
float v_cap = 0.0f;
float v_bus = 0.0f;
float v_out = 0.0f;
float i_out = 0.0f;
float ac_out_pwr_act = 0.0f;   // AC输出实时功率
float ac_out_pwr_total = 0.0f; // AC输出总功率
float ac_out_pwr_rms = 0.0f;   // AC输出有功功率
cal_rms_t cal_rms_v_g = {0};
cal_rms_t cal_rms_i_l = {0};
cal_rms_t cal_rms_v_cap = {0};
cal_rms_t cal_rms_v_out = {0};
cal_rms_t cal_rms_i_out = {0};
chk_grid_t chk_grid = {0};
ac_loss_det_t ac_loss_det = {0};
static rly_on_t rly_on_grid = {0};

static uint8_t rly_on_grid_trig = 0;
static uint8_t rly_off_grid_trig = 0;
static uint8_t rly_on_grid_is_equal = 0;

static uint8_t ac_is_ok = 0xFF;

static chk_val_t chk_grid_ss_v_bus;
static chk_val_t chk_pfc_ss_v_bus;

REG_SHELL_VAR(V_BUS, v_bus, SHELL_FP32, 1000.0f, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_OUT_RMS, cal_rms_v_out.output.rms, SHELL_FP32, 1000.0f, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(I_OUT_RMS, cal_rms_i_out.output.rms, SHELL_FP32, 1000.0f, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(I_L_RMS, cal_rms_i_l.output.rms, SHELL_FP32, 1000.0f, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_IN_RMS, cal_rms_v_g.output.rms, SHELL_FP32, 1000.0f, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_CAP_RMS, cal_rms_v_cap.output.rms, SHELL_FP32, 1000.0f, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_IN_FREQ, cal_rms_v_g.output.freq, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_OUT_RAW, v_out, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_OUT_RAW, i_out, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_L_RAW, i_l, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_IN_RAW, v_grid, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_CAP_RAW, v_cap, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_NULL)

REG_SHELL_VAR(BUS_IS_OK, chk_pfc_ss_v_bus.is_ok, SHELL_UINT8, 255, 0, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(AC_IS_OK, ac_is_ok, SHELL_UINT8, 255, 0, NULL, SHELL_STA_AUTO)

uint8_t adc_chk_get_ac_is_ok(void)
{
    return ac_is_ok;
}

uint8_t adc_chk_get_bus_is_ok(void)
{
    return chk_pfc_ss_v_bus.is_ok;
}

uint8_t adc_chk_get_grid_ss_v_bus_is_ok(void)
{
    return chk_grid_ss_v_bus.is_ok;
}

void adc_chk_set_rly_on_grid_trig(void)
{
    rly_on_grid_trig = 1;
}

void adc_chk_set_rly_off_grid_trig(void)
{
    rly_off_grid_trig = 1;
}

static void rly_on_grid_func(void)
{
    gpio_set_grid_rly(1);
}

static void rly_off_grid_func(void)
{
    gpio_set_grid_rly(0);
}

static void chk_val_init(chk_val_t *p_str,
                         float *p_val,
                         uint32_t time)
{
    p_str->p_val = p_val;
    p_str->is_ok_time = time;
    p_str->is_ok = 0;
    p_str->cnt = 0;
}

static void chk_val_func(chk_val_t *p_str,
                         float thr,
                         float thr_hys)
{
    p_str->thr = thr;
    p_str->thr_hys = thr_hys;

    if ((p_str->is_ok == 0) &&
        (*p_str->p_val > p_str->thr))
    {
        p_str->cnt++;
    }
    else if ((p_str->is_ok == 1) &&
             (*p_str->p_val < p_str->thr_hys))
    {
        p_str->cnt = 0;
    }

    if (p_str->cnt >= p_str->is_ok_time)
    {
        p_str->is_ok = 1;
    }
    else
    {
        p_str->is_ok = 0;
    }
}

REG_SHELL_VAR(GRID_RLY_FSM, rly_on_grid.inter.sta, SHELL_UINT8, 255, 0, NULL, SHELL_STA_NULL)

void adc_chk_init(void)
{
    cal_rms_init(&cal_rms_v_cap,
                 CAL_RMS_MASTER,
                 CTRL_TS,
                 20.0f,
                 &v_cap,
                 NULL,
                 NULL);

    cal_rms_init(&cal_rms_v_g,
                 CAL_RMS_MASTER,
                 0.0001f,
                 20.0f,
                 &v_grid,
                 NULL,
                 NULL);

    cal_rms_init(&cal_rms_i_l,
                 CAL_RMS_SLAVE,
                 CTRL_TS,
                 5.0f,
                 &i_l,
                 &cal_rms_v_cap.output.is_cal,
                 &cal_rms_v_cap.output.is_run);

    cal_rms_init(&cal_rms_v_out,
                 CAL_RMS_MASTER,
                 0.0001f,
                 20.0f,
                 &v_out,
                 NULL,
                 NULL);

    cal_rms_init(&cal_rms_i_out,
                 CAL_RMS_SLAVE,
                 0.0001f,
                 5.0f,
                 &i_out,
                 &cal_rms_v_out.output.is_cal,
                 &cal_rms_v_out.output.is_run);

    chk_grid_init(&chk_grid,
                  &cal_rms_v_g.output.rms,
                  &cal_rms_v_g.output.freq,
                  ADC_CHECK_GRID_JUDGE_TIME,
                  ADC_CHECK_GRID_ABNORMAL_TIME,
                  ADC_CHECK_GRID_RMS_NORMAL_MAX,
                  ADC_CHECK_GRID_RMS_NORMAL_MIN,
                  ADC_CHECK_GRID_RMS_ABNORMAL_MAX,
                  ADC_CHECK_GRID_RMS_ABNORMAL_MIN,
                  ADC_CHECK_GRID_FREQ_NORMAL_MAX,
                  ADC_CHECK_GRID_FREQ_NORMAL_MIN,
                  ADC_CHECK_GRID_FREQ_ABNORMAL_MAX,
                  ADC_CHECK_GRID_FREQ_ABNORMAL_MIN);

    rly_on_init(&rly_on_grid,
                &rly_on_grid_trig,
                &rly_off_grid_trig,
                &rly_on_grid_is_equal,
                &cal_rms_v_g.output.freq,
                CTRL_FREQ,
                RLY_ON_SW_TIME,
                rly_on_grid_func,
                rly_off_grid_func);

    ac_loss_det_init(&ac_loss_det,
                     &v_grid,
                     &chk_grid.output.is_ok);

    chk_val_init(&chk_grid_ss_v_bus,
                 &v_bus,
                 TIME_CNT_200MS_IN_1MS);

    chk_val_init(&chk_pfc_ss_v_bus,
                 &v_bus,
                 TIME_CNT_200MS_IN_1MS);
}

REG_INIT(1, adc_chk_init)

void adc_chk_get_adc(void)
{
    volatile uint32_t timeout = 60;
    while (adc_get_ct_is_ok() == 0)
    {
        timeout--;
        if (timeout == 0)
        {
            fault_set_bit(FAULT_STA_SAMPLE_ERR);
            ctrl_pfc_disable();
            ctrl_inv_disable();
            break;
        }
    }
    adc_clr_ct_is_ok();
    i_l = adc_get_i_l();
    v_grid = adc_get_v_grid();
    v_cap = adc_get_v_cap();
    v_bus = adc_get_v_bus();
    v_out = adc_get_v_out();
    i_out = adc_get_i_out();
}

static void adc_chk_cal_rms()
{
    cal_rms_master_run(&cal_rms_v_cap);
    cal_rms_slave_run(&cal_rms_i_l);
}

static void adc_chk_v_grid_cap_equal(void)
{
    if (fabsf(v_grid - v_cap) < 8.0f)
    {
        rly_on_grid_is_equal = 1;
    }
    else
    {
        rly_on_grid_is_equal = 0;
    }
}

static void adc_chk_func(void)
{
    adc_chk_get_adc();
    adc_chk_cal_rms();
    adc_chk_v_grid_cap_equal();
    rly_on_func(&rly_on_grid);
}

REG_INTERRUPT(1, adc_chk_func)

static inline void adc_chk_v_bus_func()
{
    float thr = cal_rms_v_g.output.rms * 1.414f * 0.9f;
    float thr_hys = cal_rms_v_g.output.rms * 1.414f * 0.8f;

    DN_LMT(thr, 60.0f);
    DN_LMT(thr_hys, 40.0f);

    chk_val_func(&chk_grid_ss_v_bus,
                 thr,
                 thr_hys);

    thr = data_com_get_v_bat() * TF_TURNS_RATIO * 2.0f - 10.0f;
    thr_hys = data_com_get_v_bat() * TF_TURNS_RATIO * 2.0f - 40.0f;

    DN_LMT(thr, ADC_CHECK_BULK_OK_THR);
    DN_LMT(thr_hys, ADC_CHECK_BULK_NG_THR);

    chk_val_func(&chk_pfc_ss_v_bus,
                 thr,
                 thr_hys);
}

static void adc_check_task(void)
{
    adc_chk_v_bus_func();
}

REG_TASK_MS(1, adc_check_task)

static uint8_t ac_loss_det_obs = 0;
static uint8_t chk_grid_is_ok_obs = 0;
static float v_g_rms = 0.0f;
static float v_g_freq = 0.0f;
static uint8_t v_g_trig = 0;

REG_SHELL_VAR(ac_loss_det_obs, ac_loss_det_obs, SHELL_UINT8, 255, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(chk_grid_is_ok_obs, chk_grid_is_ok_obs, SHELL_UINT8, 255, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(v_g_rms, v_g_rms, SHELL_FP32, 9999.9f, -9999.9f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(v_g_freq, v_g_freq, SHELL_FP32, 9999.9f, -9999.9f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(v_g_trig, v_g_trig, SHELL_UINT8, 255, 0, NULL, SHELL_STA_NULL)

static void ac_is_ok_func(void)
{
    static uint8_t ac_loss_last = 0;
    static uint8_t chk_grid_last = 0;

    if (((ac_loss_last == 0) &&
         (ac_loss_det.output.is_loss == 1)) ||
        (chk_grid.output.is_ok == 0))
    {
        if (v_g_trig == 1)
        {
            v_g_trig = 2;
            ac_loss_det_obs = ac_loss_det.output.is_loss;
            chk_grid_is_ok_obs = chk_grid.output.is_ok;
            v_g_rms = *chk_grid.input.p_rms;
            v_g_freq = *chk_grid.input.p_freq;
        }
        ac_is_ok = 0;
    }
    else if ((chk_grid_last == 0) &&
             (chk_grid.output.is_ok == 1))
    {
        if (v_g_trig == 0)
        {
            v_g_trig = 1;
        }
        ac_is_ok = 1;
    }

    if ((chk_grid_last == 1) &&
        (chk_grid.output.is_ok == 0))
    {
        ac_loss_det_reset(&ac_loss_det);
    }

    if ((ac_loss_last == 0) &&
        (ac_loss_det.output.is_loss == 1))
    {
        chk_grid_reset(&chk_grid);
    }

    chk_grid_last = chk_grid.output.is_ok;
    ac_loss_last = ac_loss_det.output.is_loss;
}

static void adc_check_task_100us(void)
{
    ac_loss_det_func(&ac_loss_det);
    ac_is_ok_func();
    cal_rms_master_run(&cal_rms_v_g);
    cal_rms_master_run(&cal_rms_v_out);
    cal_rms_slave_run(&cal_rms_i_out);

    static float ac_out_pwr_sum = 0.0f;
    static uint32_t ac_out_pwr_cnt = 0;
    ac_out_pwr_act = v_out * i_out;
    if ((cal_rms_v_out.output.is_cal == 1) &&
        (ac_out_pwr_cnt != 0))
    {
        ac_out_pwr_rms = ac_out_pwr_sum / ac_out_pwr_cnt;
        ac_out_pwr_cnt = 0;
        ac_out_pwr_sum = 0.0f;
    }
    ac_out_pwr_cnt++;
    ac_out_pwr_sum += ac_out_pwr_act;

    ac_out_pwr_total = cal_rms_v_out.output.rms *
                       cal_rms_i_out.output.rms;
}

REG_TASK(1, adc_check_task_100us)

#ifdef IS_PLECS
#include "plecs.h"

static void adc_chk_scope(void)
{
}

REG_INTERRUPT(8, adc_chk_scope)

#endif
