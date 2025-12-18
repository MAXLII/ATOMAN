#include "ctrl_llc_chg.h"
#include "section.h"
#include "adc_chk.h"
#include "pi_tustin.h"
#include "my_math.h"
#include "stdint.h"
#include "pwm.h"
#include "z2p2.h"

static pi_tustin_t volt_loop = {0};
static z2p2_t curr_loop = {0};
static float v_bat_ref = 0.0f;
static float v_bat_ref_tag = 3.65f;
static float i_bat_ref;
static float i_bat_ref_tag;
static uint8_t enable = 0;

float v_bat_flt = 0.0f;
float i_bat_flt = 0.0f;

float i_bat_act = 0.0f;

void ctrl_llc_chg_init(void)
{
    pi_tustin_init(&volt_loop,
                   CHG_VOLT_LOOP_KP,
                   CHG_VOLT_LOOP_KI,
                   CTRL_TS,
                   CHG_VOLT_LOOP_UP_LMT,
                   CHG_VOLT_LOOP_DN_LMT,
                   &v_bat_ref,
                   &v_bat_flt);

    z2p2_init(&curr_loop,
              0.2f,
              750,
              9000,
              CTRL_TS,
              CHG_CURR_LOOP_UP_LMT,
              CHG_CURR_LOOP_DN_LMT,
              &i_bat_ref,
              &i_bat_flt);
    v_bat_ref = v_bat;
    i_bat_ref = 0.0f;
}

REG_INIT(1, ctrl_llc_chg_init)

void ctrl_llc_chg_func(void)
{
    static float i_bat_last;
    static float v_bat_last;
    i_bat_act = -i_bat;
    LPF(i_bat_act, i_bat_last, i_bat_flt, CTRL_TS, 2.0f * M_PI * 1000.0f);
    LPF(v_bat, v_bat_last, v_bat_flt, CTRL_TS, 2.0f * M_PI * 50.0f);
    // i_bat_flt = i_bat_act;
    i_bat_last = i_bat_act;
    v_bat_last = v_bat;
    if (enable == 0)
    {
        return;
    }
    RAMP_UP_DN(v_bat_ref, v_bat_ref_tag, 0.2f / 32.0f, 100.0f);
    const float pwr_lmt_curr = CHG_PWR_LMT / v_bat;
    volt_loop.inter.up_lmt = pwr_lmt_curr;
    pi_tustin_cal(&volt_loop);
    MIN(i_bat_ref_tag, volt_loop.output.val, 100.0f);
    RAMP_UP_DN(i_bat_ref, i_bat_ref_tag, 200.0f * CTRL_TS, 200.0f);
    z2p2_cal(&curr_loop);
    pwm_llc_chg_func(curr_loop.output.val);
}

REG_INTERRUPT(2, ctrl_llc_chg_func)

void ctrl_llc_chg_enable(void)
{
    pwm_llc_chg_enable();
    ctrl_llc_chg_init();
    enable = 1;
}

void ctrl_llc_chg_disable(void)
{
    enable = 0;
    pwm_llc_chg_disable();
}

uint8_t ctrl_llc_chg_get_enable(void)
{
    return enable;
}

#ifdef IS_PLECS

#include "plecs.h"

void ctrl_llc_chg_scope(void)
{
}
REG_INTERRUPT(8, ctrl_llc_chg_scope)

#endif
