#include "ctrl_buck.h"
#include "section.h"
#include "pi_tustin.h"
#include "my_math.h"
#include "adc_chk.h"
#include "pwm.h"
#include "mppt.h"

static pi_tustin_t volt_loop = {0};
static pi_tustin_t curr_loop = {0};
static pi_tustin_t vin_lmt_loop = {0};
static float v_bat_ref = 0.0f;
static float v_bat_ref_tag = 0.0f;
static float i_ref = 0.0f;
REG_SHELL_VAR(i_ref, i_ref, SHELL_FP32, 999.9f, -999.9f, NULL, SHELL_STA_NULL)
static uint8_t enable = 0;
static float vpwm = 0.0f;
static float duty = 0.0f;
static uint8_t is_dcm = 0;
static uint32_t is_dcm_cnt = 0;
static mppt_cfg_para_t mppt;
static float i_in_lmt = 0.0f;
REG_SHELL_VAR(i_in_lmt, i_in_lmt, SHELL_FP32, 999.9f, -999.9f, NULL, SHELL_STA_NULL)
static float i_in_lmt_last = 0.0f;

static float curr_loop_kp = 0.0f;
static float curr_loop_ki = 0.0f;

static float volt_loop_kp = 0.0f;
static float volt_loop_ki = 0.0f;

REG_SHELL_VAR(curr_loop_kp, curr_loop_kp, SHELL_FP32, 10000.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(curr_loop_ki, curr_loop_ki, SHELL_FP32, 10000.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(volt_loop_kp, volt_loop_kp, SHELL_FP32, 10000.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(volt_loop_ki, volt_loop_ki, SHELL_FP32, 10000.0f, 0.0f, NULL, SHELL_STA_NULL)

static void set_mppt_ref(float val)
{
    SetMpptVoltRef(&mppt, val);
}

static float get_mppt_ref(void)
{
    return GetMpptVoltRef(&mppt);
}

static float mppt_get_volt_fdk(void)
{
    return v_buck_in;
}

static float mppt_get_pwr_fdk(void)
{
    return pwr_buck_in;
}

static void mppt_init(void)
{

    mppt.setMpptRef = set_mppt_ref;
    mppt.getMpptRef = get_mppt_ref;
    mppt.getMpptVoltFdk = mppt_get_volt_fdk;
    mppt.getMpptPwrFdk = mppt_get_pwr_fdk;
    mppt.mpptStartVolt = v_buck_in;
    mppt.stepDeltaVolt = 0.1f;
    mppt.fastStepDeltaVolt = 1.0f;
    mppt.midStepDeltaVolt = 0.5f;
    mppt.slowStepDeltaVolt = 0.2f;
    mppt.fastStepVoltThr = 1.0f;
    mppt.slowStepVoltThr = 0.3f;
    mppt.mpptLoseCtrVoltThres = 2.0f;
    mppt.voltRef = v_buck_in;
    mppt.pwrStep1 = 0.0f;
    mppt.pwrStep2 = 0.0f;
    mppt.pwrStep3 = 0.0f;
    mppt.mpptVoc = v_buck_in;
    mppt.mpptVoltUplimit = 58.0f;
    mppt.mpptVoltDnlimit = 10.0f;
    mppt.mpptLoseCtrTimeCnt = 0;
    mppt.mpptTimeThres = 50;
    mppt.mpptLoseCtrTimeThres = TIME_CNT_1S_IN_1MS / mppt.mpptTimeThres;
    mppt.mpptEnableFlg = 0;
    mppt.mpptPauseFlg = 0;
    mppt.mpptTimeCnt = 0;
    mppt.mpptSubStep = MPPT_DISTURB;
    mppt.mpptDir = 0;
}

void ctrl_buck_init(void)
{
    pi_tustin_init(&volt_loop,
                   BUCK_VOLT_LOOP_KP,
                   BUCK_VOLT_LOOP_KI,
                   CTRL_TS,
                   BUCK_CHG_CURR_LMT,
                   -10.0f,
                   &v_bat_ref,
                   &v_bat);

    pi_tustin_init(&vin_lmt_loop,
                   BUCK_VIN_LMT_LOOP_KP,
                   BUCK_VIN_LMT_LOOP_KI,
                   CTRL_TS,
                   BUCK_CHG_CURR_LMT,
                   -10.0f,
                   &v_buck_in,
                   &mppt.voltRef);

    pi_tustin_init(&curr_loop,
                   BUCK_CURR_LOOP_KP,
                   BUCK_CURR_LOOP_KI,
                   CTRL_TS,
                   30.0f,
                   0.0f,
                   &i_ref,
                   &i_buck_in);

    v_bat_ref = v_bat;
    is_dcm = 0;
    is_dcm_cnt = 0;
    v_bat_ref_tag = 3.65f;
    v_bat_ref = v_bat;
    curr_loop.inter.e[0] = 0.0f;
    curr_loop.inter.e[1] = 0.0f;
    curr_loop.inter.u[0] = v_bat;
    curr_loop.inter.u[1] = v_bat;
    i_buck_in = 0.0f;
    i_in_lmt = BUCK_I_IN_LMT;
    i_in_lmt_last = BUCK_I_IN_LMT;

    mppt_init();
    MpptEnable(&mppt);
    SetMpptVoc(&mppt, v_buck_in);
    float start_volt = v_buck_in * 0.8f;
    DN_LMT(start_volt, 10.0f);
    SetMpptStartVolt(&mppt, start_volt);
    volt_loop_kp = BUCK_VOLT_LOOP_KP;
    volt_loop_ki = BUCK_VOLT_LOOP_KI;
    curr_loop_kp = BUCK_CURR_LOOP_KP;
    curr_loop_ki = BUCK_CURR_LOOP_KI;
}

REG_INIT(1, ctrl_buck_init)

void ctrl_buck_enable(void)
{
    ctrl_buck_init();
    enable = 1;
}

void ctrl_buck_disable(void)
{
    enable = 0;
    pwm_buck_disable();
}

uint8_t ctrl_buck_get_enable(void)
{
    return enable;
}

// 限定 duty 范围
static inline float limit_duty(float duty)
{
    if (duty < 0.0f)
        return 0.0f;
    if (duty > 1.0f)
        return 1.0f;
    return duty;
}

// 安全开方
static inline float safe_sqrtf(float x)
{
    return (x > 0.0f) ? sqrtf(x) : 0.0f;
}

// 安全除法
static inline float safe_divf(float numerator, float denominator)
{
    return (fabsf(denominator) > 1e-6f) ? (numerator / denominator) : (numerator / 1e-6f);
}

REG_SHELL_VAR(vin_lmt_out, vin_lmt_loop.output.val, SHELL_FP32, 0.0F, 0.0F, NULL, SHELL_STA_NULL)

void ctrl_buck_func(void)
{
    if (enable == 0)
    {
        return;
    }
    RAMP(v_bat_ref, v_bat_ref_tag, 0.01f / 30.0f);
    pi_tustin_cal(&vin_lmt_loop);
    i_ref = vin_lmt_loop.output.val;
    volt_loop.inter.up_lmt = i_ref;
    pi_tustin_cal(&volt_loop);
    i_ref = volt_loop.output.val;
    i_in_lmt = safe_divf(BUCK_CHG_PWR_LMT, v_buck_in);
    MIN(i_ref, i_ref, i_in_lmt);
    pi_tustin_cal(&curr_loop);
    vpwm = curr_loop.output.val;
    duty = safe_divf(vpwm, v_buck_in);
    pwm_buck_func(duty, 1, is_dcm ? 0 : 1);
}

REG_INTERRUPT(2, ctrl_buck_func)

static void mppt_task(void)
{
    MpptProcess(&mppt);
    mppt.voltRef = 12.0f;
}

REG_TASK_MS(1, mppt_task)

#ifdef IS_PLECS

#include "plecs.h"

void ctrl_buck_scope(void)
{
}

REG_INTERRUPT(8, ctrl_buck_scope)

#endif
