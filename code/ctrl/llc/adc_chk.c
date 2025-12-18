#include "adc_chk.h"
#include "adc.h"
#include "section.h"
#include "buck_hardware.h"
#include "my_math.h"
#include "volt_mt.h"
#include "stdint.h"

float v_bat = 0.0f;
float i_bat = 0.0f;
float i_buck_in = 0.0f;
float v_buck_in = 0.0f;
float pwr_buck_in = 0.0f;

volt_mt_t volt_mt_pv_in = {0};
volt_mt_t volt_mt_bat = {0};

float v_ntc_buck = 0.0f;
float v_ntc_bat = 0.0f;
float v_ntc_pp2 = 0.0f;
float v_ntc_pp1 = 0.0f;

REG_SHELL_VAR(V_BAT, v_bat, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(I_BAT, i_bat, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(I_BUCK_IN, i_buck_in, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_BUCK_IN, v_buck_in, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_AUTO)

REG_SHELL_VAR(V_NTC_BUCK, v_ntc_buck, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_NTC_BAT, v_ntc_bat, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_NTC_PP2, v_ntc_pp2, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_NTC_PP1, v_ntc_pp1, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(PWR_BUCK_IN, pwr_buck_in, SHELL_FP32, 1000.0f, 0.0f, NULL, SHELL_STA_AUTO)

void adc_chk_get_adc_func(void)
{
    uint32_t timeout = 60;
    while (adc_get_ct_is_ok() == 0)
    {
        timeout--;
        if (timeout == 0)
        {
            break;
        }
    }
    adc_clr_ct_is_ok();
    v_bat = adc_get_v_bat();
    i_bat = adc_get_i_bat();
    i_buck_in = adc_get_i_buck_in();
    v_buck_in = adc_get_v_buck_in();
    pwr_buck_in = i_buck_in * v_buck_in;
}

REG_INTERRUPT(1, adc_chk_get_adc_func)

static void adc_chk_init(void)
{
    const volt_cfg_t volt_cfg = {
        .min_vld = PV_VOLT_MIN_VLD,
        .max_vld = PV_VOLT_MAX_VLD,
        .min_exit = PV_VOLT_MIN_EXIT,
        .max_exit = PV_VOLT_MAX_EXIT,
        .min_inst = PV_VOLT_MIN_INST,
        .max_inst = PV_VOLT_MAX_INST,
        .enter_dly = PV_VOLT_ENTER_DLY,
        .exit_dly = PV_VOLT_EXIT_DLY,
    };
    volt_mt_init(&volt_mt_pv_in, &volt_cfg, &v_buck_in);

    const volt_cfg_t bat_volt_cfg = {
        .min_vld = BAT_VOLT_MIN_VLD,
        .max_vld = BAT_VOLT_MAX_VLD,
        .min_exit = BAT_VOLT_MIN_EXIT,
        .max_exit = BAT_VOLT_MAX_EXIT,
        .min_inst = BAT_VOLT_MIN_INST,
        .max_inst = BAT_VOLT_MAX_INST,
        .enter_dly = BAT_VOLT_ENTER_DLY,
        .exit_dly = BAT_VOLT_EXIT_DLY,
    };

    volt_mt_init(&volt_mt_bat, &bat_volt_cfg, &v_bat);
}

REG_INIT(1, adc_chk_init)

static void adc_chk_task(void)
{
    v_ntc_buck = adc_get_ntc_buck();
    v_ntc_bat = adc_get_ntc_bat();
    v_ntc_pp2 = adc_get_ntc_push_pull2();
    v_ntc_pp1 = adc_get_ntc_push_pull1();
    static uint32_t time;
    time++;
    volt_mt_upd(&volt_mt_pv_in, time);
    volt_mt_upd(&volt_mt_bat, time);
}

REG_TASK_MS(1, adc_chk_task)

uint8_t adc_chk_get_pv_in_is_ok(void)
{
    return volt_mt_is_vld(&volt_mt_pv_in);
}

uint8_t adc_chk_get_bat_is_ok(void)
{
    return volt_mt_is_vld(&volt_mt_bat);
}

