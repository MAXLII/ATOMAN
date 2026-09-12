// SPDX-License-Identifier: MIT
/**
 * @file    npc_ctrl.c
 * @brief   Own DSOGI/PI state, sampled protection and soft start; drive PWM through bound HAL.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_ctrl.h"
#include "npc_cfg.h"
#include "npc_fsm.h"
#include "npc_hal.h"
#include "dsogi.h"
#include "section.h"
#include <math.h>
#include <stddef.h>
#include <string.h>
/* These instance types and fixed-address DSOGI bindings are private to control. */
typedef struct npc_ctrl_input
{
    float v_out[3];   /* Output phase voltages A/B/C, V; zero sequence is discarded. */
    float i_l[3];     /* Inductor currents A/B/C, A; bridge to output is positive. */
    float v_dc_p;     /* Positive half-bus voltage, V, strictly positive. */
    float v_dc_n;     /* Negative half-bus magnitude, V, strictly positive. */
    float theta;      /* External present-sample angle, rad; wrap at the caller for precision. */
    float vd_pos_ref; /* Positive d-axis phase-voltage peak reference, V, nonnegative. */
} npc_ctrl_input_t;

typedef struct npc_ctrl_inter
{
    dsogi_t voltage;     /* Voltage sequence extraction; contains internal bindings. */
    dsogi_t current;     /* Current sequence extraction; contains internal bindings. */
    float v_ab[2];       /* DSOGI voltage sources, V. */
    float i_ab[2];       /* DSOGI current sources, A. */
    float integral_v[4]; /* Voltage PI integral outputs, A. */
    float integral_i[4]; /* Current PI integral outputs, V. */
    bool initialized;    /* Configuration accepted; configuration immutable until init. */
} npc_ctrl_inter_t;

typedef struct npc_ctrl
{
    npc_ctrl_cfg_t cfg;       /* Instance-owned fixed configuration. */
    npc_ctrl_inter_t inter;   /* Private runtime state; do not copy an initialized instance. */
    npc_ctrl_output_t output; /* Latest coherent calculation result. */
} npc_ctrl_t;


static npc_ctrl_t controller = {0};
static npc_ctrl_monitor_t monitor = {0};
static npc_ctrl_setpoint_t active_setpoint = {NPC_CFG_DEFAULT_VD_POS_SLEW_VPS, 50.0f, 20.0f, 0U};
static double reference_ramp = 0.0;

/** @param value Candidate number. @param low Inclusive lower bound.
 *  @param high Inclusive upper bound. @return true inside a finite bounded interval. */
static inline bool in_range(float value, float low, float high)
{
    return (value >= low) && /* Reject NaN and low outliers. */
           (value <= high);  /* Reject infinity and high outliers. */
}

/** @param p_cfg Configuration candidate. @return true inside supported arithmetic bounds. */
static bool config_valid(const npc_ctrl_cfg_t *p_cfg)
{
    return in_range(p_cfg->ts, 1.0e-6f, 0.01f) &&               /* DSOGI sample period envelope. */
           in_range(p_cfg->omega, 1.0f, 100000.0f) &&           /* Positive bounded frequency. */
           in_range(p_cfg->omega * p_cfg->ts, 0.001f, 1.0f) &&  /* DSOGI numeric envelope. */
           in_range(p_cfg->sogi_k, 0.1f, 4.0f) &&               /* Supported SOGI damping. */
           in_range(p_cfg->kp_v, 0.0f, 1.0e6f) &&               /* Bound proportional products. */
           in_range(p_cfg->ki_v, 0.0f, 1.0e6f) &&               /* Bound integral products. */
           in_range(p_cfg->kp_i, 0.0f, 1.0e6f) &&               /* Bound proportional products. */
           in_range(p_cfg->ki_i, 0.0f, 1.0e6f) &&               /* Bound integral products. */
           in_range(p_cfg->kaw_v, 0.0f, 1.0e6f) &&              /* Finite back-calculation rate. */
           in_range(p_cfg->kaw_i, 0.0f, 1.0e6f) &&              /* Finite back-calculation rate. */
           in_range(p_cfg->kaw_v * p_cfg->ts, 0.0f, 1.0f) &&    /* Monotonic discrete tracking. */
           in_range(p_cfg->kaw_i * p_cfg->ts, 0.0f, 1.0f) &&    /* Monotonic discrete tracking. */
           in_range(p_cfg->current_peak, 1.0e-6f, 1.0e6f) &&    /* Positive reference budget. */
           in_range(p_cfg->modulation_headroom, 1.0e-6f, 1.0f); /* Physical modulation fraction. */
}

static void npc_ctrl_reset_states(npc_ctrl_t *p_ctrl)
{
    if (p_ctrl == NULL)
    {
        return;
    }
    dsogi_reset(&p_ctrl->inter.voltage);                                         /* Clear voltage observer history. */
    dsogi_reset(&p_ctrl->inter.current);                                         /* Clear current observer history. */
    (void)memset(p_ctrl->inter.integral_v, 0, sizeof(p_ctrl->inter.integral_v)); /* Clear outer loops. */
    (void)memset(p_ctrl->inter.integral_i, 0, sizeof(p_ctrl->inter.integral_i)); /* Clear inner loops. */
    p_ctrl->output = (npc_ctrl_output_t){0};                                     /* Require a new valid calculation before use. */
}

static bool npc_ctrl_init_states(npc_ctrl_t *p_ctrl, const npc_ctrl_cfg_t *p_cfg)
{
    npc_ctrl_cfg_t cfg = {0};       /* Snapshot before clearing permits self-configuration reinit. */
    dsogi_cfg_t observer_cfg = {0}; /* Common fixed-frequency observer parameters. */
    if (p_ctrl == NULL)
    {
        return false;
    }
    if (p_cfg != NULL)
    {
        cfg = *p_cfg;
    }
    (void)memset(p_ctrl, 0, sizeof(*p_ctrl));
    if (config_valid(&cfg) == false)
    {
        return false;
    }
    p_ctrl->cfg = cfg;
    observer_cfg = (dsogi_cfg_t){.ts = cfg.ts, .k = cfg.sogi_k, .omega_min = cfg.omega, .omega_max = cfg.omega};
    if ((dsogi_init(&p_ctrl->inter.voltage, &observer_cfg, &p_ctrl->inter.v_ab[0],
                    &p_ctrl->inter.v_ab[1], &p_ctrl->cfg.omega) == false) || /* Bind voltage sources. */
        (dsogi_init(&p_ctrl->inter.current, &observer_cfg, &p_ctrl->inter.i_ab[0],
                    &p_ctrl->inter.i_ab[1], &p_ctrl->cfg.omega) == false)) /* Bind current sources. */
    {
        (void)memset(p_ctrl, 0, sizeof(*p_ctrl));
        return false;
    }
    p_ctrl->inter.initialized = true;
    return true;
}

/** @param p_abc Phase snapshot. @param p_ab Amplitude-invariant stationary components. */
static inline void clarke(const float *p_abc, float *p_ab)
{
    p_ab[0] = (2.0f * p_abc[0] - p_abc[1] - p_abc[2]) / 3.0f;
    p_ab[1] = (p_abc[1] - p_abc[2]) * 0.5773502691896258f;
}

/** @param p_seq Separated observer output. @param cosine Present angle cosine.
 *  @param sine Present angle sine. @param p_dq Output [d+,q+,d-,q-]. */
static inline void park(const dsogi_output_t *p_seq, float cosine, float sine, float *p_dq)
{
    p_dq[0] = cosine * p_seq->alpha_pos + sine * p_seq->beta_pos;
    p_dq[1] = -sine * p_seq->alpha_pos + cosine * p_seq->beta_pos;
    p_dq[2] = cosine * p_seq->alpha_neg - sine * p_seq->beta_neg;
    p_dq[3] = sine * p_seq->alpha_neg + cosine * p_seq->beta_neg;
}

/** @param p_input Coherent sample. @return true within supported arithmetic bounds. */
static inline bool input_valid(const npc_ctrl_input_t *p_input)
{
    for (uint32_t axis = 0u; axis < 3u; ++axis) /* Phase index A/B/C. */
    {
        if ((in_range(p_input->v_out[axis], -1.0e6f, 1.0e6f) == false) || /* Bound voltage arithmetic. */
            (in_range(p_input->i_l[axis], -1.0e6f, 1.0e6f) == false))     /* Bound current arithmetic. */
        {
            return false;
        }
    }
    return in_range(p_input->v_dc_p, 1.0e-6f, 1.0e6f) &&  /* Require energized positive bus. */
           in_range(p_input->v_dc_n, 1.0e-6f, 1.0e6f) &&  /* Require energized negative bus. */
           in_range(p_input->vd_pos_ref, 0.0f, 1.0e6f) && /* Nonnegative voltage amplitude. */
           (isfinite(p_input->theta) != 0);               /* Never pass nonfinite phase to trigonometry. */
}

static bool FUNC_RAM npc_ctrl_cal(npc_ctrl_t *p_ctrl, const npc_ctrl_input_t *p_input)
{
    float ev[4] = {0};      /* Voltage errors [d+,q+,d-,q-], V. */
    float ei[4] = {0};      /* Current errors [d+,q+,d-,q-], A. */
    float iraw[4] = {0};    /* Unconstrained current references, A. */
    float uraw[4] = {0};    /* Unconstrained voltage commands, V. */
    float cosine = 0.0f;    /* Same-sample phase cosine. */
    float sine = 0.0f;      /* Same-sample phase sine. */
    float magnitude = 0.0f; /* Worst combined sequence-current magnitude, A. */
    float scale_i = 1.0f;   /* Common current-reference scaling. */
    float scale_u = 1.0f;   /* Common sequence-voltage scaling. */
    float alpha = 0.0f;     /* Combined stationary voltage, V. */
    float beta = 0.0f;      /* Combined stationary voltage, V. */
    float phase_b = 0.0f;   /* Reconstructed phase B voltage, V. */
    float phase_c = 0.0f;   /* Reconstructed phase C voltage, V. */
    float span = 0.0f;      /* Required phase-voltage span, V. */
    float budget = 0.0f;    /* Available DC span, V. */
    if (p_ctrl == NULL)
    {
        return false;
    }
    if ((p_ctrl->inter.initialized == false) || /* Require successful initialization. */
        (p_input == NULL))                      /* Require a complete sample snapshot. */
    {
        npc_ctrl_reset_states(p_ctrl);
        return false;
    }
    if (input_valid(p_input) == false)
    {
        npc_ctrl_reset_states(p_ctrl);
        return false;
    }
    clarke(p_input->v_out, p_ctrl->inter.v_ab);         /* Convert sampled output voltage. */
    clarke(p_input->i_l, p_ctrl->inter.i_ab);           /* Convert sampled inductor current. */
    if ((dsogi_cal(&p_ctrl->inter.voltage) == false) || /* Extract voltage sequences. */
        (dsogi_cal(&p_ctrl->inter.current) == false))   /* Extract current sequences. */
    {
        npc_ctrl_reset_states(p_ctrl);
        return false;
    }
    cosine = cosf(p_input->theta);
    sine = sinf(p_input->theta);
    park(&p_ctrl->inter.voltage.output, cosine, sine, p_ctrl->output.v_dq); /* Positive/negative voltage dq. */
    park(&p_ctrl->inter.current.output, cosine, sine, p_ctrl->output.i_dq); /* Positive/negative current dq. */
    for (uint32_t axis = 0u; axis < 4u; ++axis)                             /* Sequence-axis index. */
    {
        ev[axis] = -p_ctrl->output.v_dq[axis];
        if (axis == 0u)
        {
            ev[axis] += p_input->vd_pos_ref;
        }
        iraw[axis] = p_ctrl->cfg.kp_v * ev[axis] + p_ctrl->inter.integral_v[axis];
    }
    magnitude = hypotf(iraw[0], iraw[1]) + hypotf(iraw[2], iraw[3]);
    if (magnitude > p_ctrl->cfg.current_peak)
    {
        scale_i = p_ctrl->cfg.current_peak / magnitude;
    }
    for (uint32_t axis = 0u; axis < 4u; ++axis) /* Sequence-axis index. */
    {
        p_ctrl->output.i_ref[axis] = iraw[axis] * scale_i;
        ei[axis] = p_ctrl->output.i_ref[axis] - p_ctrl->output.i_dq[axis];
        uraw[axis] = p_ctrl->cfg.kp_i * ei[axis] + p_ctrl->inter.integral_i[axis];
    }
    alpha = cosine * (uraw[0] + uraw[2]) + sine * (uraw[3] - uraw[1]);
    beta = sine * (uraw[0] - uraw[2]) + cosine * (uraw[1] + uraw[3]);
    phase_b = -0.5f * alpha + 0.8660254037844386f * beta;
    phase_c = -0.5f * alpha - 0.8660254037844386f * beta;
    span = fmaxf(alpha, fmaxf(phase_b, phase_c)) - fminf(alpha, fminf(phase_b, phase_c));
    budget = p_ctrl->cfg.modulation_headroom * (p_input->v_dc_p + p_input->v_dc_n);
    if (span > budget)
    {
        scale_u = budget / span;
    }
    p_ctrl->output.v_alpha = alpha * scale_u;
    p_ctrl->output.v_beta = beta * scale_u;
    for (uint32_t axis = 0u; axis < 4u; ++axis) /* Update integrals after both outputs and limits. */
    {
        p_ctrl->output.u_dq[axis] = uraw[axis] * scale_u;
        p_ctrl->inter.integral_v[axis] += p_ctrl->cfg.ts * (p_ctrl->cfg.ki_v * ev[axis] +
                                                            p_ctrl->cfg.kaw_v * (p_ctrl->output.i_ref[axis] - iraw[axis]));
        p_ctrl->inter.integral_i[axis] += p_ctrl->cfg.ts * (p_ctrl->cfg.ki_i * ei[axis] +
                                                            p_ctrl->cfg.kaw_i * (p_ctrl->output.u_dq[axis] - uraw[axis]));
        if ((isfinite(p_ctrl->inter.integral_v[axis]) == 0) || /* Reject arithmetic overflow. */
            (isfinite(p_ctrl->inter.integral_i[axis]) == 0) || /* Reject arithmetic overflow. */
            (isfinite(p_ctrl->output.i_ref[axis]) == 0) ||     /* Reject nonfinite reference. */
            (isfinite(p_ctrl->output.u_dq[axis]) == 0))        /* Reject nonfinite modulation command. */
        {
            npc_ctrl_reset_states(p_ctrl);
            return false;
        }
    }
    if ((isfinite(p_ctrl->output.v_alpha) == 0) || /* Check final reconstructed command. */
        (isfinite(p_ctrl->output.v_beta) == 0))    /* Check final reconstructed command. */
    {
        npc_ctrl_reset_states(p_ctrl);
        return false;
    }
    p_ctrl->output.current_limited = scale_i < 1.0f;
    p_ctrl->output.voltage_limited = scale_u < 1.0f;
    p_ctrl->output.valid = true;
    return true;
}

void npc_ctrl_stop(void)
{
    const npc_ctrl_hal_t *p_hal = npc_hal_get_ctrl();
    if (p_hal->p_pwm_disable != NULL) p_hal->p_pwm_disable();
    npc_ctrl_reset_states(&controller);
    reference_ramp = 0.0;
    monitor = (npc_ctrl_monitor_t){.status = NPC_CTRL_OFF};
}
void npc_ctrl_prepare_run(void)
{
    npc_ctrl_cfg_t cfg = npc_cfg_default();
    npc_ctrl_stop();
    (void)npc_fsm_read_published(&active_setpoint);
    cfg.ts = npc_cfg_get_ctrl_ts();
    cfg.omega = (float)(6.28318530717958647692 * (double)active_setpoint.freq_hz);
    if (!npc_ctrl_init_states(&controller, &cfg)) monitor.status = NPC_CTRL_CONTROL;
}
static void npc_ctrl_init(void) { npc_ctrl_prepare_run(); }
REG_INIT(2, npc_ctrl_init)
const npc_ctrl_monitor_t *npc_ctrl_get_monitor(void) { return &monitor; }

/* Invalid feedback never acknowledges a trip. Only a healthy stop request clears it. */
static bool npc_ctrl_check_protection(const npc_ctrl_input_t *p_input)
{
    bool healthy = isfinite(p_input->v_dc_p) && isfinite(p_input->v_dc_n);
    uint32_t fault = 0U;
    uint32_t phase_fault = 0U;
    for (uint32_t phase = 0U; phase < 3U; ++phase)
    {
        uint32_t candidate = 0U;
        if (!isfinite(p_input->i_l[phase]) || !isfinite(p_input->v_out[phase])) healthy = false;
        if (fabsf(p_input->i_l[phase]) > NPC_CFG_TRIP_CURRENT_FACTOR * controller.cfg.current_peak) candidate = NPC_CTRL_OVERCURRENT;
        if (candidate != 0U)
        {
            healthy = false;
            if (fault == 0U) { fault = candidate; phase_fault = phase; }
        }
    }
    if ((fault != 0U) && (npc_cfg_get_run_request() != 0U))
        npc_hal_hard_protect_trip(fault, phase_fault);
    if (healthy && (npc_cfg_get_run_request() == 0U)) npc_hal_hard_protect_clear();
    if (npc_hal_hard_protect_is_latched() != 0U)
    {
        npc_ctrl_stop();
        monitor.status = npc_hal_get_fault();
        monitor.detail = npc_hal_get_fault_phase();
        return true;
    }
    return false;
}

/* Called exactly once per configured control period, after platform sampling. */
static void FUNC_RAM npc_ctrl_isr(void)
{
    const npc_ctrl_hal_t *p_hal = npc_hal_get_ctrl();
    npc_ctrl_input_t input = {0};
    uint32_t pwm_status = 0U;
    float omega = 0.0f;
    double ramp_step = 0.0; /* Published slew rate times the control period. */
    float amplitude = 0.0f; /* Same-cycle external amplitude target from HAL. */
    if (npc_hal_is_ready() == 0U)
    {
        npc_ctrl_stop();
        monitor.status = NPC_CTRL_BINDING;
        return;
    }
    (void)npc_fsm_read_published(&active_setpoint);
    for (uint32_t phase = 0U; phase < 3U; ++phase)
    {
        input.v_out[phase] = *p_hal->p_v_out[phase];
        input.i_l[phase] = *p_hal->p_i_l[phase];
    }
    input.v_dc_p = *p_hal->p_v_dc_p;
    input.v_dc_n = *p_hal->p_v_dc_n;
    input.theta = *p_hal->p_theta;
    amplitude = *p_hal->p_vd_pos_ref;
    ramp_step = (double)active_setpoint.vd_pos_slew_vps * (double)npc_cfg_get_ctrl_ts();
    if (npc_ctrl_check_protection(&input)) return;
    /* Stop is effective on the next ISR even before the 1 ms FSM publishes. */
    if ((active_setpoint.run_allowed == 0U) || (npc_cfg_get_run_request() == 0U))
    {
        npc_ctrl_stop();
        return;
    }
    if (!in_range(amplitude, 0.0f, 1.0e6f) ||
        !in_range(input.theta, 0.0f, 6.283186f))
    {
        npc_ctrl_stop();
        monitor.status = NPC_CTRL_REFERENCE;
        return;
    }
    omega = (float)(6.28318530717958647692 * (double)active_setpoint.freq_hz);
    if ((!controller.inter.initialized) || (controller.cfg.omega != omega)) npc_ctrl_prepare_run();
    input.vd_pos_ref = (float)reference_ramp;
    if ((input.v_dc_p < active_setpoint.v_dc_half_min) || (input.v_dc_n < active_setpoint.v_dc_half_min))
    {
        npc_ctrl_stop();
        monitor.status = NPC_CTRL_BUS;
        return;
    }
    /* Preserve DLL input diagnostic ordering: bus P/N, voltage A/B/C, current A/B/C. */
    {
        const float samples[8] = {input.v_dc_p, input.v_dc_n,
            input.v_out[0], input.v_out[1], input.v_out[2],
            input.i_l[0], input.i_l[1], input.i_l[2]};
        for (uint32_t channel = 0U; channel < 8U; ++channel)
        {
            if (!isfinite(samples[channel]))
            {
                npc_ctrl_stop();
                monitor.status = NPC_CTRL_INPUT;
                monitor.detail = channel;
                return;
            }
        }
    }
    if (!input_valid(&input))
    {
        npc_ctrl_stop();
        monitor.status = NPC_CTRL_CONTROL;
        return;
    }
    if (!npc_ctrl_cal(&controller, &input))
    {
        npc_ctrl_stop();
        monitor.status = NPC_CTRL_CONTROL;
        return;
    }
    /* No command pipeline here: the platform PWM setter receives this sample's result. */
    pwm_status = p_hal->p_set_pwm_func(controller.output.v_alpha, controller.output.v_beta,
                                    input.v_dc_p, input.v_dc_n);
    if (pwm_status != 0U)
    {
        npc_ctrl_stop();
        monitor.status = NPC_CTRL_PWM;
        monitor.detail = pwm_status;
        return;
    }
    monitor.output = controller.output;
    (void)memcpy(monitor.integral_v, controller.inter.integral_v, sizeof(monitor.integral_v));
    (void)memcpy(monitor.integral_i, controller.inter.integral_i, sizeof(monitor.integral_i));
    monitor.vd_pos_ref_act = input.vd_pos_ref;
    monitor.status = NPC_CTRL_RUNNING;
    monitor.detail = 0U;
    /* Preserve the existing post-output soft-start update and double accumulator. */
    if (reference_ramp < (double)amplitude)
        reference_ramp = fmin(reference_ramp + ramp_step, (double)amplitude);
    else reference_ramp = fmax(reference_ramp - ramp_step, (double)amplitude);
}
REG_INTERRUPT(3, npc_ctrl_isr)
