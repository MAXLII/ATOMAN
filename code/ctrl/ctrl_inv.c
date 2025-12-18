#include "ctrl_inv.h"
#include "my_math.h"
#include "section.h"
#include "pi_tustin.h"
#include "adc_chk.h"
#include "pwm.h"
#include "string.h"
#include "pfc_fsm.h"

static pi_tustin_t volt_loop_d = {0};
static pi_tustin_t volt_loop_q = {0};

static pi_tustin_t curr_loop_d = {0};
static pi_tustin_t curr_loop_q = {0};

static float v_d_ref = 0.0f;
static float v_d_act = 0.0f;
static float v_q_ref = 0.0f;
static float v_q_act = 0.0f;

static float v_ref = 0.0f;
static float i_d_ref = 0.0f;
static float i_d_act = 0.0f;
static float i_q_ref = 0.0f;
static float i_q_act = 0.0f;

static float volt_buffer[610] = {0};
static float curr_buffer[610] = {0};

static uint32_t period = 0;
static uint32_t period_quarter = 0;

static float sintheta = 0.0f;
static float costheta = 0.0f;
static uint32_t theta = 0;
static uint32_t theta_quarter = 0;

static float v_ref_pk = 0.0f;
static float v_ref_pk_tag = 0.0f;

static uint8_t enable = 0;

static float v_l = 0.0f;
static float vpwm = 0.0f;

static void ctrl_inv_init(void)
{
    pi_tustin_init(&volt_loop_d,
                   INV_VOLT_LOOP_D_KP,
                   INV_VOLT_LOOP_D_KI,
                   CTRL_TS,
                   10.0f,
                   -10.0f,
                   &v_d_ref,
                   &v_d_act);
    pi_tustin_init(&volt_loop_q,
                   INV_VOLT_LOOP_Q_KP,
                   INV_VOLT_LOOP_Q_KI,
                   CTRL_TS,
                   10.0f,
                   -10.0f,
                   &v_q_ref,
                   &v_q_act);

    pi_tustin_init(&curr_loop_d,
                   INV_CURR_LOOP_D_KP,
                   INV_CURR_LOOP_D_KI,
                   CTRL_TS,
                   100.0f,
                   -100.0f,
                   &i_d_ref,
                   &i_d_act);
    pi_tustin_init(&curr_loop_q,
                   INV_CURR_LOOP_Q_KP,
                   INV_CURR_LOOP_Q_KI,
                   CTRL_TS,
                   100.0f,
                   -100.0f,
                   &i_q_ref,
                   &i_q_act);

    period = (uint32_t)(CTRL_FREQ / CTRL_INV_FREQ);
    period_quarter = period / 4;
    v_ref_pk = 0.0f;

    if (pfc_fsm_get_is_ups_to_inv_trig() == 1)
    {
        v_ref_pk = v_ref_pk_tag;
    }

    v_ref_pk_tag = CTRL_INV_RMS * M_SQRT2;
    theta = acosf(v_cap / v_ref_pk) / (2.0f * M_PI) * period;
    theta_quarter = (theta + period - period_quarter) % period;

    v_ref = v_ref_pk * costheta;

    memset(volt_buffer, 0, sizeof(volt_buffer));
    memset(curr_buffer, 0, sizeof(curr_buffer));
}

REG_INIT(1, ctrl_inv_init)

void ctrl_inv_enable(void)
{
    ctrl_inv_init();
    pwm_pfc_enable();
    enable = 1;
}

void ctrl_inv_disable(void)
{
    pwm_pfc_disable();
    enable = 0;
}

static void ctrl_inv_cal(void)
{
    sintheta = sinf((float)theta / (float)period * 2.0f * M_PI);
    costheta = cosf((float)theta / (float)period * 2.0f * M_PI);
}
REG_INTERRUPT(0, ctrl_inv_cal)

static uint32_t ctrl_inv_cnt = 0;
REG_SHELL_VAR(ctrl_inv_cnt, ctrl_inv_cnt, SHELL_UINT32, 10000000, 0, NULL, SHELL_STA_NULL)

void ctrl_inv_func(void)
{
    if (enable == 0)
    {
        return;
    }
    ctrl_inv_cnt++;

    RAMP(v_ref_pk, v_ref_pk_tag, 0.3f / 30.0f);

    v_ref = v_ref_pk * costheta;

    v_d_ref = v_ref_pk;
    v_q_ref = 0.0f;

    volt_buffer[theta] = v_cap;
    curr_buffer[theta] = i_l;

    DQ_CAL(volt_buffer[theta],
           volt_buffer[theta_quarter],
           sintheta,
           costheta,
           v_d_act,
           v_q_act);

    DQ_CAL(curr_buffer[theta],
           curr_buffer[theta_quarter],
           sintheta,
           costheta,
           i_d_act,
           i_q_act);

    pi_tustin_cal(&volt_loop_d);
    pi_tustin_cal(&volt_loop_q);

    i_d_ref = volt_loop_d.output.val;
    i_q_ref = volt_loop_q.output.val;

    pi_tustin_cal(&curr_loop_d);
    pi_tustin_cal(&curr_loop_q);

    v_l = curr_loop_d.output.val * costheta + curr_loop_q.output.val * sintheta;

    vpwm = v_ref + v_l;

    pwm_pfc_set_duty(vpwm, v_bus);

    INC_AND_WRAP(theta, period);
    INC_AND_WRAP(theta_quarter, period);
}

REG_INTERRUPT(3, ctrl_inv_func)

#ifdef IS_PLECS

#include "plecs.h"

void ctrl_inv_scope(void)
{
}

REG_INTERRUPT(8, ctrl_inv_scope)

#endif
