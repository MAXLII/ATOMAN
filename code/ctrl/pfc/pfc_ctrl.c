#include "pfc_ctrl.h"
#include "pfc_cfg.h"
#include "section.h"
#include "pi_tustin.h"
#include "pr.h"
#include "notch.h"
#include "sogi.h"
#include "fll.h"
#include "my_math.h"
#include "hw_params.h"

/* HAL view of measured signals and hardware actuation hooks. */
#define p_hal (pfc_hal_get_ctrl()) /* p_hal: control-side HAL pointer */

/* Bus-voltage conditioning and outer voltage-loop regulator. */
static notch_t vbus_notch_filter = {0};  /* vbus_notch_filter: bus-voltage notch filter */
static pi_tustin_t vbus_volt_loop = {0}; /* vbus_volt_loop: DC-bus voltage outer PI */
static pi_tustin_t ind_curr_loop = {0};  /* ind_curr_loop: inductor-current inner PI */
static pr_t ind_curr_loop_pr = {0};      /* ind_curr_loop_pr: inductor-current inner PR */

/* Grid-voltage orthogonal signal generator and frequency estimator. */
static sogi_t grid_sogi = {0};     /* grid_sogi: SOGI state for grid voltage */
static fll_state_t grid_fll = {0}; /* grid_fll: FLL state for grid frequency */

/* Slewed bus-voltage reference consumed by the PI loop. */
static float vbus_ref_ramped_v = 0.0f;                                /* vbus_ref_ramped_v: ramped bus-voltage reference */
static float ind_curr_ref_cmd_a = 0.0f;                               /* ind_curr_ref_cmd_a: commanded inductor-current reference */
static float ind_curr_ref_act_a = 0.0f;                               /* ind_curr_ref_act_a: ramped inductor-current reference */
static float ind_curr_fb_a = 0.0f;                                    /* ind_curr_fb_a: inductor-current feedback */
static float ind_curr_ctrl_u_raw_v = 0.0f;                            /* ind_curr_ctrl_u_raw_v: raw PR output */
static float ind_curr_ctrl_u_sat_v = 0.0f;                            /* ind_curr_ctrl_u_sat_v: saturated PR output */
static float pfc_pwm_cmd_v = 0.0f;                                    /* pfc_pwm_cmd_v: PWM voltage command */
static float pfc_pwm_cmd_raw_v = 0.0f;                                /* pfc_pwm_cmd_raw_v: unclamped PWM voltage command */
static float pfc_duty_cmd = 0.0f;                                     /* pfc_duty_cmd: final duty command */
static float pfc_sogi_omega_pending = PFC_CTRL_GRID_OMEGA_INIT_RADPS; /* pfc_sogi_omega_pending: requested SOGI omega update */
static float pfc_pr_w0_pending = PFC_CTRL_GRID_OMEGA_INIT_RADPS;      /* pfc_pr_w0_pending: requested PR center frequency update */
static uint8_t pfc_sogi_update_pending = 0U;                          /* pfc_sogi_update_pending: deferred SOGI update flag */
static uint8_t pfc_pr_update_pending = 0U;                            /* pfc_pr_update_pending: deferred PR update flag */
static uint8_t pfc_ctrl_run_active = 0U;                              /* pfc_ctrl_run_active: latched run gate state */

static const fll_params_t grid_fll_params = {
    /* grid_fll_params: FLL tuning parameters */
    .gamma = PFC_CTRL_FLL_GAIN,
    .omega_init = PFC_CTRL_GRID_OMEGA_INIT_RADPS,
    .ts = CTRL_TS,
};

static inline void pfc_ctrl_request_freq_update(float omega)
{
    pfc_sogi_omega_pending = omega;
    pfc_pr_w0_pending = omega;
    pfc_sogi_update_pending = 1U;
    pfc_pr_update_pending = 1U;
}

static inline void pfc_ctrl_apply_pending_freq_update(void)
{
    if (pfc_sogi_update_pending != 0U)
    {
        sogi_update_frequency((sogi_t *)&grid_sogi, pfc_sogi_omega_pending);
        pfc_sogi_update_pending = 0U;
    }

    if (pfc_pr_update_pending != 0U)
    {
        pr_update_freq(&ind_curr_loop_pr, pfc_pr_w0_pending);
        pfc_pr_update_pending = 0U;
    }
}

static inline void pfc_ctrl_reset_loops(void)
{
    pi_tustin_reset(&vbus_volt_loop);
    pi_tustin_reset(&ind_curr_loop);
    pr_reset(&ind_curr_loop_pr);
    ind_curr_ref_cmd_a = 0.0f;
    ind_curr_ref_act_a = 0.0f;
    ind_curr_fb_a = 0.0f;
    ind_curr_ctrl_u_raw_v = 0.0f;
    ind_curr_ctrl_u_sat_v = 0.0f;
    pfc_pwm_cmd_raw_v = 0.0f;
    pfc_pwm_cmd_v = 0.0f;
    pfc_duty_cmd = 0.0f;
}

static inline void pfc_ctrl_force_safe_output(void)
{
    ind_curr_ref_cmd_a = 0.0f;
    ind_curr_ref_act_a = 0.0f;
    ind_curr_ctrl_u_raw_v = 0.0f;
    ind_curr_ctrl_u_sat_v = 0.0f;
    pfc_pwm_cmd_raw_v = 0.0f;
    pfc_pwm_cmd_v = 0.0f;
    pfc_duty_cmd = 0.0f;

    if ((p_hal != NULL) &&
        (p_hal->p_pwm_disable != NULL))
    {
        p_hal->p_pwm_disable();
    }
}

static inline float pfc_ctrl_get_startup_vbus_ref_init(void)
{
    return *p_hal->p_v_bus + PFC_CTRL_STARTUP_VBUS_INIT_BOOST_V;
}

static inline float pfc_ctrl_calc_ind_curr_ref(void)
{
    float grid_rms_v = *p_hal->p_v_rms;             /* grid_rms_v: measured grid RMS voltage */
    float grid_rms_sq_v2 = grid_rms_v * grid_rms_v; /* grid_rms_sq_v2: squared grid RMS voltage */

    DN_LMT(grid_rms_sq_v2, 0.001f);

    /*
     * Inductor-current reference is composed of:
     * 1) an active-power term following the in-phase SOGI output;
     * 2) an input-capacitor compensation term on the quadrature axis.
     */
    return (vbus_volt_loop.output.val *
            PFC_CTRL_GRID_RMS_NOMINAL_V *
            grid_sogi.osg_u[0] /
            grid_rms_sq_v2) -
           (grid_sogi.osg_qu[0] * grid_fll.omega * HW_AC_SIDE_CAP_VALUE);
}

static inline float pfc_ctrl_apply_pwm_clamp(float pwm_cmd_raw_v)
{
    float v_bus = *p_hal->p_v_bus;

    DN_LMT(v_bus, 1.0f);

    pfc_duty_cmd = pwm_cmd_raw_v / v_bus;
    UP_DN_LMT(pfc_duty_cmd, PFC_CTRL_PWM_DUTY_MAX, PFC_CTRL_PWM_DUTY_MIN);

    return pfc_duty_cmd * v_bus;
}

static inline void pfc_ctrl_relax_vbus_loop_hist_if_ov(void)
{
    float vbus_ref_target = vbus_ref_ramped_v * PFC_CTRL_VBUS_OV_RELAX_RATIO;

    if (*p_hal->p_v_bus > vbus_ref_target)
    {
        vbus_volt_loop.inter.u[0] *= PFC_CTRL_VBUS_OV_HIST_DECAY;
        vbus_volt_loop.inter.u[1] *= PFC_CTRL_VBUS_OV_HIST_DECAY;
        vbus_volt_loop.inter.e[0] *= PFC_CTRL_VBUS_OV_HIST_DECAY;
        vbus_volt_loop.inter.e[1] *= PFC_CTRL_VBUS_OV_HIST_DECAY;
        vbus_volt_loop.output.val = vbus_volt_loop.inter.u[0];
    }
}

static inline void pfc_ctrl_run_current_loop(float i_ref_abs_lmt_a)
{
    float target_ref_a = 0.0f;

    /* Run the outer voltage loop once per control interrupt. */
    pfc_ctrl_relax_vbus_loop_hist_if_ov();
    pi_tustin_cal(&vbus_volt_loop);

    /* Convert bus-loop output into the instantaneous grid-current reference. */
    target_ref_a = pfc_ctrl_calc_ind_curr_ref();
    UP_DN_LMT(target_ref_a, i_ref_abs_lmt_a, -i_ref_abs_lmt_a);
    ind_curr_ref_cmd_a = target_ref_a;

    ind_curr_ref_act_a = ind_curr_ref_cmd_a;

    /* Match the current-feedback sign convention used by the legacy controller. */
    ind_curr_fb_a = -*p_hal->p_i_l;

    /* Run the PR current loop and keep both raw and saturated outputs for debug. */
    pr_cal(&ind_curr_loop_pr);
    ind_curr_ctrl_u_raw_v = ind_curr_loop_pr.output.raw;
    ind_curr_ctrl_u_sat_v = ind_curr_loop_pr.output.sat;

    /* Clamp the final modulation command rather than only clipping the PR output. */
    pfc_pwm_cmd_raw_v = *p_hal->p_v_cap - ind_curr_ctrl_u_sat_v;
    pfc_pwm_cmd_v = pfc_ctrl_apply_pwm_clamp(pfc_pwm_cmd_raw_v);

    p_hal->p_set_pwm_func(pfc_pwm_cmd_v, *p_hal->p_v_bus);
}

static inline void pfc_ctrl_reinit_states(void)
{
    if ((p_hal == NULL) ||
        (p_hal->p_v_bus == NULL) ||
        (p_hal->p_v_g == NULL) ||
        (p_hal->p_v_rms == NULL) ||
        (p_hal->p_i_l == NULL) ||
        (p_hal->p_v_cap == NULL) ||
        (p_hal->p_main_rly_is_closed == NULL))
    {
        return;
    }

    /* Reject the dominant bus ripple before feeding the outer loop. */
    notch_init(&vbus_notch_filter,
               PFC_CTRL_VBUS_NOTCH_CENTER_RADPS,
               PFC_CTRL_VBUS_NOTCH_BANDWIDTH_RADPS,
               CTRL_TS,
               p_hal->p_v_bus);

    /* The PI output is the current-amplitude command handled downstream. */
    pi_tustin_init(&vbus_volt_loop,
                   PFC_CTRL_VOLT_LOOP_KP,
                   PFC_CTRL_VOLT_LOOP_KI,
                   CTRL_TS,
                   PFC_CTRL_VBUS_LOOP_OUT_MAX,
                   PFC_CTRL_VBUS_LOOP_OUT_MIN,
                   &vbus_ref_ramped_v,
                   &vbus_notch_filter.output.val);

    /*
     * Current loop regulates the inductor current.
     * The sensed current polarity is inverted here to match the controller sign.
     */
    pi_tustin_init(&ind_curr_loop,
                   PFC_CTRL_CURR_LOOP_KP,
                   PFC_CTRL_CURR_LOOP_KI,
                   CTRL_TS,
                   PFC_CTRL_IND_CURR_LOOP_OUT_MAX,
                   PFC_CTRL_IND_CURR_LOOP_OUT_MIN,
                   &ind_curr_ref_act_a,
                   &ind_curr_fb_a);

    pr_init(&ind_curr_loop_pr,
            PFC_CTRL_CURR_LOOP_PR_KP,
            PFC_CTRL_CURR_LOOP_PR_KR,
            grid_fll_params.omega_init,
            PFC_CTRL_CURR_LOOP_PR_WC,
            CTRL_TS,
            PFC_CTRL_IND_CURR_LOOP_OUT_MAX,
            PFC_CTRL_IND_CURR_LOOP_OUT_MIN,
            &ind_curr_ref_act_a,
            &ind_curr_fb_a);

    /* Track grid phase with SOGI, and let FLL adapt the center frequency. */
    sogi_init(&grid_sogi,
              CTRL_TS,
              grid_fll_params.omega_init,
              PFC_CTRL_SOGI_GAIN,
              p_hal->p_v_g);

    fll_init(&grid_fll,
             &grid_fll_params,
             &grid_sogi.osg_u[0],
             &grid_sogi.osg_qu[0],
             &grid_sogi.err);

    vbus_ref_ramped_v = pfc_ctrl_get_startup_vbus_ref_init();
    pfc_ctrl_request_freq_update(grid_fll_params.omega_init);
    pfc_ctrl_reset_loops();
}

static void pfc_ctrl_init(void)
{
    pfc_ctrl_run_active = 0U;
    pfc_ctrl_reinit_states();
}

static void pfc_ctrl_isr(void)
{
    pfc_ctrl_setpoint_t *p_active_setpoint = pfc_cfg_get_p_active(); /* p_active_setpoint: active PFC setpoint */

    if ((p_hal == NULL) ||
        (pfc_cfg_is_ready() == 0) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_i_l == NULL) ||
        (p_hal->p_v_cap == NULL) ||
        (p_hal->p_v_g == NULL) ||
        (p_hal->p_v_bus == NULL) ||
        (p_hal->p_pwm_disable == NULL) ||
        (p_hal->p_set_pwm_func == NULL))
    {
        return;
    }

    /* Advance the staged setpoint from building to active in one place. */
    pfc_cfg_sync_building_to_active();

    /* Update orthogonal grid components and online frequency estimate. */
    pfc_ctrl_apply_pending_freq_update();
    sogi_cal((sogi_t *)&grid_sogi);
    if ((p_active_setpoint->run_allowed != 0U) &&
        (*p_hal->p_main_rly_is_closed == 1U))
    {
        fll_cal((fll_state_t *)&grid_fll);
    }
    else
    {
        grid_fll.omega = PFC_CTRL_GRID_OMEGA_INIT_RADPS;
    }
    notch_cal((notch_t *)&vbus_notch_filter);

    /* Keep controller states frozen until the upper layer allows run and the
     * main relay has been confirmed closed.
     */
    if ((p_active_setpoint->run_allowed == 0U) ||
        (*p_hal->p_main_rly_is_closed == 0U))
    {
        if (pfc_ctrl_run_active != 0U)
        {
            vbus_ref_ramped_v = pfc_ctrl_get_startup_vbus_ref_init();
            pfc_ctrl_reset_loops();
            pfc_ctrl_force_safe_output();
            pfc_ctrl_run_active = 0U;
        }
        return;
    }

    pfc_ctrl_run_active = 1U;

    /* Apply a slew-rate limit to the bus-reference command. */
    RAMP(vbus_ref_ramped_v, p_active_setpoint->vbus_ref_v, p_active_setpoint->vbus_slew_vps * CTRL_TS);

    pfc_ctrl_run_current_loop(PFC_CTRL_VBUS_LOOP_OUT_MAX);
}

REG_INTERRUPT(3, pfc_ctrl_isr)

static void pfc_ctrl_task(void)
{
    static uint32_t grid_freq_last = 0U; /* grid_freq_last: last quantized requested omega */
    uint32_t grid_freq_now = 0U;
    float target_omega = PFC_CTRL_GRID_OMEGA_INIT_RADPS;

    if ((p_hal != NULL) &&
        (pfc_cfg_is_ready() != 0U) &&
        (pfc_cfg_get_p_active() != NULL) &&
        (pfc_cfg_get_p_active()->run_allowed != 0U) &&
        (p_hal->p_main_rly_is_closed != NULL) &&
        (*p_hal->p_main_rly_is_closed == 1U))
    {
        target_omega = grid_fll.omega;
    }

    grid_freq_now = ((uint32_t)target_omega) >> 1;

    if (grid_freq_now != grid_freq_last)
    {
        pfc_ctrl_request_freq_update(target_omega);
        grid_freq_last = grid_freq_now;
    }
}

REG_TASK(1, pfc_ctrl_task)

void pfc_ctrl_set_p_hal(pfc_ctrl_hal_t *p)
{
    (void)p;
}

void pfc_ctrl_prepare_run(void)
{
    pfc_ctrl_run_active = 0U;
    pfc_ctrl_reinit_states();
}
