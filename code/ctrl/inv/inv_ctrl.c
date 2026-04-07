#include "inv_ctrl.h"
#include "inv_cfg.h"
#include "section.h"
#include "pi_tustin.h"
#include "string.h"

#define p_hal (inv_hal_get_ctrl())

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
static uint32_t theta = 0;
static uint32_t theta_quarter = 0;

static float sintheta = 0.0f;
static float costheta = 1.0f;
static float phase_pu = 0.0f;
static float freq_hz_ramped = 0.0f;
static float v_ref_pk = 0.0f;
static float v_ref_pk_tag = 0.0f;
static float v_l = 0.0f;
static float vpwm = 0.0f;
static uint8_t inv_ctrl_run_active = 0U;
static uint8_t inv_ctrl_first_run_cycle = 0U;

static float inv_ctrl_limit_freq_hz(float freq_hz)
{
    if (freq_hz < 1.0f)
    {
        return 1.0f;
    }

    if (freq_hz > (CTRL_FREQ / 4.0f))
    {
        return CTRL_FREQ / 4.0f;
    }

    return freq_hz;
}

static void inv_ctrl_update_timing_by_freq(float freq_hz)
{
    float freq_hz_limited = inv_ctrl_limit_freq_hz(freq_hz);

    period = (uint32_t)(CTRL_FREQ / freq_hz_limited);
    if (period < 4U)
    {
        period = 4U;
    }
    if (period > (uint32_t)(sizeof(volt_buffer) / sizeof(volt_buffer[0])))
    {
        period = (uint32_t)(sizeof(volt_buffer) / sizeof(volt_buffer[0]));
    }

    period_quarter = period / 4U;
    theta = ((uint32_t)(phase_pu * (float)period)) % period;
    theta_quarter = (theta + period - period_quarter) % period;
}

static void inv_ctrl_reinit_states(void)
{
    inv_ctrl_setpoint_t *p_active_setpoint = inv_cfg_get_p_active();

    if ((p_hal == NULL) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_v_cap == NULL) ||
        (p_hal->p_i_l == NULL) ||
        (p_hal->p_v_bus == NULL))
    {
        PLECS_LOG("inv_ctrl reinit skipped: hal invalid\n");
        return;
    }

    pi_tustin_init(&volt_loop_d,
                   INV_CTRL_VOLT_LOOP_D_KP,
                   INV_CTRL_VOLT_LOOP_D_KI,
                   CTRL_TS,
                   50.0f,
                   -50.0f,
                   &v_d_ref,
                   &v_d_act);
    pi_tustin_init(&volt_loop_q,
                   INV_CTRL_VOLT_LOOP_Q_KP,
                   INV_CTRL_VOLT_LOOP_Q_KI,
                   CTRL_TS,
                   50.0f,
                   -50.0f,
                   &v_q_ref,
                   &v_q_act);

    pi_tustin_init(&curr_loop_d,
                   INV_CTRL_CURR_LOOP_D_KP,
                   INV_CTRL_CURR_LOOP_D_KI,
                   CTRL_TS,
                   100.0f,
                   -100.0f,
                   &i_d_ref,
                   &i_d_act);
    pi_tustin_init(&curr_loop_q,
                   INV_CTRL_CURR_LOOP_Q_KP,
                   INV_CTRL_CURR_LOOP_Q_KI,
                   CTRL_TS,
                   100.0f,
                   -100.0f,
                   &i_q_ref,
                   &i_q_act);

    phase_pu = 0.0f;
    freq_hz_ramped = inv_ctrl_limit_freq_hz(p_active_setpoint->freq_hz);
    inv_ctrl_update_timing_by_freq(freq_hz_ramped);
    sintheta = 0.0f;
    costheta = 1.0f;
    v_ref = 0.0f;
    v_ref_pk = 0.0f;
    v_ref_pk_tag = p_active_setpoint->rms_ref_v * M_SQRT2;
    v_l = 0.0f;
    vpwm = 0.0f;

    memset(volt_buffer, 0, sizeof(volt_buffer));
    memset(curr_buffer, 0, sizeof(curr_buffer));
    PLECS_LOG("inv_ctrl reinit done: freq=%.3f freq_slew=%.3f rms=%.3f rms_slew=%.3f period=%u\n",
              p_active_setpoint->freq_hz,
              p_active_setpoint->freq_slew_hzps,
              p_active_setpoint->rms_ref_v,
              p_active_setpoint->rms_slew_vps,
              period);
}

static void inv_ctrl_init(void)
{
    PLECS_LOG("inv_ctrl init\n");
    inv_ctrl_run_active = 0U;
    inv_ctrl_first_run_cycle = 1U;
    inv_ctrl_reinit_states();
}

static void inv_ctrl_cal_theta(void)
{
    if (freq_hz_ramped <= 0.0f)
    {
        return;
    }

    inv_ctrl_update_timing_by_freq(freq_hz_ramped);
    sintheta = sinf(phase_pu * 2.0f * M_PI);
    costheta = cosf(phase_pu * 2.0f * M_PI);
}

REG_INTERRUPT(0, inv_ctrl_cal_theta)

static void inv_ctrl_isr(void)
{
    inv_ctrl_setpoint_t *p_active_setpoint = inv_cfg_get_p_active();

    if ((p_hal == NULL) ||
        (inv_cfg_is_ready() == 0U) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_v_cap == NULL) ||
        (p_hal->p_i_l == NULL) ||
        (p_hal->p_v_bus == NULL) ||
        (p_hal->p_set_pwm_func == NULL) ||
        (p_hal->p_pwm_disable == NULL))
    {
        return;
    }

    inv_cfg_sync_building_to_active();

    if (p_active_setpoint->run_allowed == 0U)
    {
        if (inv_ctrl_run_active != 0U)
        {
            p_hal->p_pwm_disable();
            inv_ctrl_run_active = 0U;
            inv_ctrl_first_run_cycle = 1U;
        }
        return;
    }

    if (inv_ctrl_run_active == 0U)
    {
        inv_ctrl_first_run_cycle = 1U;
    }

    inv_ctrl_run_active = 1U;

    if (inv_ctrl_first_run_cycle != 0U)
    {
        freq_hz_ramped = inv_ctrl_limit_freq_hz(p_active_setpoint->freq_hz);
        inv_ctrl_update_timing_by_freq(freq_hz_ramped);
        inv_ctrl_first_run_cycle = 0U;
    }
    else if (p_active_setpoint->freq_slew_hzps > 0.0f)
    {
        RAMP(freq_hz_ramped,
             p_active_setpoint->freq_hz,
             p_active_setpoint->freq_slew_hzps * CTRL_TS);
    }
    else
    {
        freq_hz_ramped = p_active_setpoint->freq_hz;
    }
    freq_hz_ramped = inv_ctrl_limit_freq_hz(freq_hz_ramped);

    v_ref_pk_tag = p_active_setpoint->rms_ref_v * M_SQRT2;
    RAMP(v_ref_pk, v_ref_pk_tag, p_active_setpoint->rms_slew_vps * M_SQRT2 * CTRL_TS);

    v_ref = v_ref_pk * costheta;

    v_d_ref = v_ref_pk;
    v_q_ref = 0.0f;

    volt_buffer[theta] = *p_hal->p_v_cap;
    curr_buffer[theta] = *p_hal->p_i_l;

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
    p_hal->p_set_pwm_func(vpwm, *p_hal->p_v_bus);

    phase_pu += freq_hz_ramped * CTRL_TS;
    while (phase_pu >= 1.0f)
    {
        phase_pu -= 1.0f;
    }
}

REG_INTERRUPT(3, inv_ctrl_isr)

void inv_ctrl_set_p_hal(inv_ctrl_hal_t *p)
{
    (void)p;
}

void inv_ctrl_prepare_run(void)
{
    PLECS_LOG("inv_ctrl prepare run\n");
    inv_ctrl_run_active = 0U;
    inv_ctrl_first_run_cycle = 1U;
    inv_ctrl_reinit_states();
}
