#include "ctrl_pfc.h"
#include "section.h"
#include "pi_tustin.h"
#include "adc_chk.h"
#include "pwm.h"
#include "data_com.h"
#include "sogi.h"
#include "fll.h"
#include "notch.h"
#include "scope.h"

static pi_tustin_t volt_loop = {0};
static pi_tustin_t curr_loop = {0};
static uint8_t enable = 0;
static uint8_t enable_obs = 0;
static float v_bus_ref = 0.0f;
static float i_ref = 0.0f;
static float v_pwm = 0.0f;
static float v_bus_ref_tag = 0.0f;
static float i_l_neg;
sogi_t sogi_v_grid = {0};
fll_state_t fll_v_grid = {0};
notch_t notch_v_bus = {0};
notch_cfg_t notch_cfg = {
    .ts = CTRL_TS,
    .w0 = 2.0f * M_PI * 100.0f,
    .wb = 2.0F * M_PI * 10.0f,
};

static fll_params_t v_grid_fll_params = {
    .gamma = 150,
    .omega_init = 2.0f * M_PI * 50.0f,
    .ts = CTRL_TS,
};

REG_SCOPE(PFC_OBS, 450, 10,
          i_l_ref_obs,
          i_l_act_obs,
          v_l_obs,
          v_grid_obs,
          sogi_p_obs,
          sogi_q_obs,
          v_cap_obs)

static void pfc_obs_trig(DEC_MY_PRINTF)
{
    (void)my_printf;
    SCOPE_TRIGGER(PFC_OBS);
}

REG_SHELL_CMD(PFC_OBS_TRIG, pfc_obs_trig)

void ctrl_pfc_init(void)
{
    pi_tustin_init(&volt_loop,
                   PFC_VOLT_LOOP_KP,
                   PFC_VOLT_LOOP_KI,
                   CTRL_TS,
                   5.0f,
                   -5.0f,
                   &v_bus_ref,
                   &notch_v_bus.output.val);

    pi_tustin_init(&curr_loop,
                   PFC_CURR_LOOP_KP,
                   PFC_CURR_LOOP_KI,
                   CTRL_TS,
                   30.0f,
                   -30.0f,
                   &i_ref,
                   &i_l_neg);

    sogi_init(&sogi_v_grid,
              CTRL_TS,
              2.0f * M_PI * 50.0f,
              1.0f,
              &v_grid);

    fll_init(&fll_v_grid,
             &v_grid_fll_params,
             &sogi_v_grid.osg_u[0],
             &sogi_v_grid.osg_qu[0],
             &sogi_v_grid.err);
    v_bus_ref = v_bus;

    notch_init(&notch_v_bus,
               &notch_cfg,
               &v_bus);
}

REG_INIT(1, ctrl_pfc_init)

void ctrl_pfc_enable(void)
{
    ctrl_pfc_init();
    pwm_pfc_enable();
    enable = 1;
}

void ctrl_pfc_disable(void)
{
    enable = 0;
    pwm_pfc_disable();
    SCOPE_TRIGGER(PFC_OBS);
}

REG_SHELL_VAR(PFC_ENABLE, enable_obs, SHELL_UINT8, 1, 0, NULL, SHELL_STA_AUTO)

static float v_bus_ref_dbg = 0.0f;
REG_SHELL_VAR(v_bus_ref_dbg, v_bus_ref_dbg, SHELL_FP32, 430.0f, 0.0f, NULL, SHELL_STA_NULL)

static uint32_t ctrl_pfc_cnt = 0;
REG_SHELL_VAR(ctrl_pfc_cnt, ctrl_pfc_cnt, SHELL_UINT32, 10000000, 0, NULL, SHELL_STA_NULL)

void ctrl_pfc_func(void)
{
    if (enable == 0)
    {
        i_l_ref_obs = *curr_loop.input.p_ref;
        i_l_act_obs = *curr_loop.input.p_act;
        v_l_obs = curr_loop.output.val;
        v_grid_obs = v_grid;
        sogi_p_obs = sogi_v_grid.osg_u[0];
        sogi_q_obs = sogi_v_grid.osg_qu[0];
        v_cap_obs = v_cap;
        SCOPE_RUN(PFC_OBS);
        return;
    }

    ctrl_pfc_cnt++;

    i_l_neg = -i_l;

    sogi_cal(&sogi_v_grid);
    fll_cal(&fll_v_grid);
    notch_cal(&notch_v_bus);

    v_bus_ref_tag = data_com_get_v_bat() * TF_TURNS_RATIO * 2.0f * 1.05f;
    if (v_bus_ref_dbg > 300.0f)
    {
        v_bus_ref_tag = v_bus_ref_dbg;
    }
    UP_DN_LMT(v_bus_ref_tag, 430.0f, 380.0f);
    RAMP(v_bus_ref, v_bus_ref_tag, 40.0f * CTRL_TS);
    pi_tustin_cal(&volt_loop);

    float v_g_rms_sq = cal_rms_v_g.output.rms * cal_rms_v_g.output.rms;
    DN_LMT(v_g_rms_sq, 0.001f);
    float v_g_rms_sq_inv = 1.0f / v_g_rms_sq;

    i_ref = (volt_loop.output.val *
             230.0f *
             sogi_v_grid.osg_u[0] *
             v_g_rms_sq_inv) -
            (sogi_v_grid.osg_qu[0] * 2.0f * M_PI * 50.0f * CAP_INV_VALUE);

    pi_tustin_cal(&curr_loop);

    v_pwm = v_cap - curr_loop.output.val;
    pwm_pfc_set_duty(v_pwm, v_bus);

    i_l_ref_obs = *curr_loop.input.p_ref;
    i_l_act_obs = *curr_loop.input.p_act;
    v_l_obs = curr_loop.output.val;
    v_grid_obs = v_grid;
    sogi_p_obs = sogi_v_grid.osg_u[0];
    ;
    sogi_q_obs = sogi_v_grid.osg_qu[0];
    ;
    v_cap_obs = v_cap;
    SCOPE_RUN(PFC_OBS);
}

REG_INTERRUPT(3, ctrl_pfc_func)

float ctrl_pfc_get_v_bus_ref_tag(void)
{
    return v_bus_ref_tag;
}

static void sogi_omega_update(void)
{
    static uint32_t fll_v_grid_omega_u32_last = 0;
    uint32_t fll_v_grid_omega_u32 = ((uint32_t)fll_v_grid.omega) >> 1;
    if (fll_v_grid_omega_u32 != fll_v_grid_omega_u32_last)
    {
        sogi_update_frequency(&sogi_v_grid, fll_v_grid.omega);
        fll_v_grid_omega_u32_last = fll_v_grid_omega_u32;
    }
}

#include "scope.h"

static void ctrl_pfc_task(void)
{
    enable_obs = enable;
    chk_grid_func(&chk_grid);
    sogi_omega_update();
    notch_update_freq(&notch_v_bus, 2 * M_PI * cal_rms_v_g.output.freq * 2.0f);
}

REG_TASK_MS(1, ctrl_pfc_task)
