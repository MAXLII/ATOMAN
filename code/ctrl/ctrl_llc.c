#include "ctrl_llc.h"
#include "section.h"
#include "adc_chk.h"
#include "pi_tustin.h"
#include "my_math.h"
#include "stdint.h"
#include "pwm.h"

static pi_tustin_t vbat_loop = {0};
static pi_tustin_t ibat_loop = {0};
static pi_tustin_t vbus_loop = {0};
static float v_bat_ref = 0.0f;
static float v_bat_ref_tag = 3.65f;
static float i_bat_ref;
static float v_bus_ref = 0.0f;
static float v_bus_ref_tag = 0.0f;
static uint8_t enable = 0;

void ctrl_llc_init(void)
{
    pi_tustin_init(&vbat_loop,
                   CHG_VOLT_LOOP_KP,
                   CHG_VOLT_LOOP_KI,
                   CTRL_TS,
                   CHG_VOLT_LOOP_UP_LMT,
                   CHG_VOLT_LOOP_DN_LMT,
                   &v_bat_ref,
                   &v_bat);

    pi_tustin_init(&ibat_loop,
                   CHG_CURR_LOOP_KP,
                   CHG_CURR_LOOP_KI,
                   CTRL_TS,
                   CHG_CURR_LOOP_UP_LMT,
                   CHG_CURR_LOOP_DN_LMT,
                   &i_bat_ref,
                   &i_bat);

    pi_tustin_init(&vbus_loop,
                   LLC_DSG_VOLT_LOOP_KP,
                   LLC_DSG_VOLT_LOOP_KI,
                   CTRL_TS,
                   LLC_DSG_VOLT_LOOP_UP_LMT,
                   LLC_DSG_VOLT_LOOP_DN_LMT,
                   &v_bus_ref,
                   &v_bus);

    v_bat_ref = v_bat;
    v_bus_ref = v_bus;
    v_bus_ref_tag = 0.0f;
}

REG_INIT(1, ctrl_llc_init)

void ctrl_llc_func(void)
{
    if (enable == 0)
    {
        return;
    }
    v_bus_ref_tag = v_bat * TF_TURNS_RATIO * 2.0f + 15.0f;
    RAMP(v_bus_ref, v_bus_ref_tag, 1.0f / 32.0f);
    RAMP(v_bat_ref, v_bat_ref_tag, 0.1f / 32.0f);
    pi_tustin_cal(&vbus_loop);
    vbat_loop.inter.up_lmt = -vbus_loop.output.val;
    pi_tustin_cal(&vbat_loop);
    const float pwr_lmt_curr = CHG_PWR_LMT / v_bat;
    MIN(i_bat_ref, vbat_loop.output.val, pwr_lmt_curr);
    pi_tustin_cal(&ibat_loop);
    pwm_llc_func(ibat_loop.output.val);
}

REG_INTERRUPT(2, ctrl_llc_func)

void ctrl_llc_enable(void)
{
    ctrl_llc_init();
    pwm_llc_enable();
    enable = 1;
}

void ctrl_llc_disable(void)
{
    enable = 0;
    pwm_llc_disable();
}

float ctrl_llc_get_v_bus_ref_tag(void)
{
    return v_bus_ref_tag;
}

#ifdef IS_PLECS

#include "plecs.h"

void ctrl_llc_scope(void)
{
}
REG_INTERRUPT(8, ctrl_llc_scope)

#endif
