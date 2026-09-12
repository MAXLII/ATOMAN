// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   NPC 5 kHz PLECS duty-output DLL.
 * @details
 *          This file is part of the base digital power framework project.
 *          Update the modulator every 200 us of simulation time and hold duty/enable between updates.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "DllHeader.h"
#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include "plecs.h"
#include "pwm.h"
#include "npc_ctrl.h"
#include "npc_cfg.h"
#include "npc_fsm.h"
#include "npc_hal.h"
#include "npc_platform.h"
#include "shell.h"
#include "frame_tcp_server.h"
#include "plecs_dispatch_lock.h"

#define NPC_TIME_TOLERANCE_S (1.0e-9)               /* 1 ns base tolerance, far below the 200 us control period. */
#define NPC_TWO_PI (6.28318530717958647692)         /* One electrical revolution, radians. */

static float output_frame[PLECS_OUTPUT_MAX]; /* Held 6 duties and bridge enable, published on every callback. */
static double update_origin;                 /* First output timestamp; fixed origin of the 5 kHz grid. */
static uint64_t update_index;                /* Integer index of the next update, avoiding accumulated time rounding. */
static double last_call_time;                /* Detect unsupported simulation-time rollback. */
static bool first_update;                    /* First output establishes the control timing origin. */
static bool initialized;                     /* Initialization succeeded for this simulation. */
static float v_alpha;                        /* Read-only closed-loop alpha command, V. */
static float v_beta;                         /* Read-only closed-loop beta command, V. */
static float v_alpha_pwm;                    /* Alpha command actually passed to PWM this period, V. */
static float v_beta_pwm;                     /* Beta command actually passed to PWM this period, V. */
static float vd_pos_ref = 600.0f;            /* Shell positive d-axis phase-voltage peak reference, V. */
static float vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS; /* Shell soft-start slew, V/s. */
static float vd_pos_ref_act;                 /* Reference actually passed to the controller, V. */
static float trip_current;                   /* Raw phase-current trip magnitude, A. */
static npc_ctrl_monitor_t control_monitor = {0}; /* Read-only copy for Shell and CSV. */
static float theta;                          /* Present-sample external controller angle, rad. */
static float freq_hz = 50.0f;                /* Shell electrical reference frequency, Hz. */
static double phase_cycle;                   /* Shared phase in [0,1), advanced only on control updates. */
static uint8_t run_enable = 1;               /* Shell run request: 0 disabled, 1 enabled. */
static float v_dc_half_min = 20.0f;          /* Shell minimum valid half-bus voltage, V. */
static float applied_half_min;               /* Threshold currently configured in the modulator, V. */
static uint32_t ctrl_ticks;                  /* Number of entered 5 kHz updates, wraps after about 9.9 days. */
static uint32_t ctrl_status;                 /* Last control-chain state code exposed through Shell. */
static uint32_t ctrl_detail;                 /* Failed input index or SVPWM return status, when applicable. */
static uint8_t log_ready;                    /* Runtime file was opened successfully. */
static float v_dc_p;                         /* Present positive bus sample, V. */
static float v_dc_n;                         /* Present negative bus magnitude, V. */
static uint8_t trace_enable = 1u;            /* Arm one 20 s capture; automatically clears on completion. */
static uint8_t trace_state;                  /* 0 idle, 1 armed, 2 writing, 3 complete, 4 I/O failure. */
static uint32_t trace_rows;                  /* Successfully written 5 kHz samples in this capture. */

typedef enum
{
    NPC_APP_BOOT = 0,         /* Initialization in progress. */
    NPC_APP_OFF = 1,          /* Run request disabled. */
    NPC_APP_RUNNING = 2,      /* Controller and modulator produced a valid enabled frame. */
    NPC_APP_REFERENCE = 3,    /* Reference/frequency rejected. */
    NPC_APP_PWM_CONFIG = 4,   /* Half-bus threshold rejected. */
    NPC_APP_INPUT = 5,        /* Host sample is nonfinite or outside float range. */
    NPC_APP_BUS = 6,          /* One measured half bus is below its threshold. */
    NPC_APP_CONTROL = 7,      /* Controller rejected the sample or numerical update. */
    NPC_APP_MODULATION = 8,   /* SVPWM rejected the command. */
    NPC_APP_TIMING = 9,       /* Timestamp rollback, invalid time or missed update. */
    NPC_APP_INIT = 10,        /* Initialization or callback ports unavailable. */
    NPC_APP_STOPPED = 11,     /* Simulation terminated. */
    NPC_APP_OVERCURRENT = 12  /* Latched sampled inductor overcurrent. */
} NPC_APP_STATUS_E;
uint32_t plecs_time_100us;                           /* Shared simulation clock for Shell/Section services. */

static float v_out_a; /* Sampled phase A output voltage, V. */
static float v_out_b; /* Sampled phase B output voltage, V. */
static float v_out_c; /* Sampled phase C output voltage, V. */
static float i_l_a;   /* Sampled phase A inductor current, A. */
static float i_l_b;   /* Sampled phase B inductor current, A. */
static float i_l_c;   /* Sampled phase C inductor current, A. */

REG_SHELL_VAR(V_OUT_A, v_out_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_OUT_B, v_out_b, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_OUT_C, v_out_c, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(I_L_A, i_l_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(I_L_B, i_l_b, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(I_L_C, i_l_c, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(V_ALPHA, v_alpha, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_BETA, v_beta, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_ALPHA_PWM, v_alpha_pwm, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_BETA_PWM, v_beta_pwm, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VD_POS_REF, vd_pos_ref, SHELL_FP32, 1000000.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VD_POS_SLEW, vd_pos_slew_vps, SHELL_FP32, 1000000.0f, 0.001f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VD_POS_REF_ACT, vd_pos_ref_act, SHELL_FP32, 1000000.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TRIP_CURRENT, trip_current, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
/* Legacy amplitude names now address the same balanced positive-sequence reference. */
REG_SHELL_VAR(V_ALPHA_AMP, vd_pos_ref, SHELL_FP32, 1000000.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_BETA_AMP, vd_pos_ref, SHELL_FP32, 1000000.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(THETA, theta, SHELL_FP32, 6.283186f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VD_POS, control_monitor.output.v_dq[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VQ_POS, control_monitor.output.v_dq[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VD_NEG, control_monitor.output.v_dq[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VQ_NEG, control_monitor.output.v_dq[3], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(ID_POS, control_monitor.output.i_dq[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(IQ_POS, control_monitor.output.i_dq[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(ID_NEG, control_monitor.output.i_dq[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(IQ_NEG, control_monitor.output.i_dq[3], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(ID_POS_REF, control_monitor.output.i_ref[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(IQ_POS_REF, control_monitor.output.i_ref[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(ID_NEG_REF, control_monitor.output.i_ref[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(IQ_NEG_REF, control_monitor.output.i_ref[3], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(FREQ_HZ, freq_hz, SHELL_FP32, 795.0f, 1.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(RUN_ENABLE, run_enable, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_DC_HALF_MIN, v_dc_half_min, SHELL_FP32, 1000000.0f, 0.001f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CTRL_TICKS, ctrl_ticks, SHELL_UINT32, UINT32_MAX, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CTRL_STATUS, ctrl_status, SHELL_UINT32, 12u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CTRL_DETAIL, ctrl_detail, SHELL_UINT32, UINT32_MAX, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(LOG_READY, log_ready, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TRACE_ENABLE, trace_enable, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TRACE_STATE, trace_state, SHELL_UINT8, 4u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TRACE_ROWS, trace_rows, SHELL_UINT32, 100000u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_DC_P, v_dc_p, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(V_DC_N, v_dc_n, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)

/** @param status New control-chain state. @param detail Failed channel or modulation return value.
 *  @param time_s Simulation timestamp of the transition. */
static void report_status(NPC_APP_STATUS_E status, uint32_t detail, double time_s)
{
    if ((ctrl_status == (uint32_t)status) && /* Avoid printing the same failure every 200 us. */
        (ctrl_detail == detail))             /* Changed failure details still need a record. */
    {
        return;
    }
    ctrl_status = (uint32_t)status;
    ctrl_detail = detail;
    PLECS_LOG("NPC t=%.9f ticks=%lu status=%lu detail=%lu run=%u ref=%.6g freq=%.6g "
              "vdc_p=%.6g vdc_n=%.6g half_min=%.6g alpha=%.6g beta=%.6g\n",
              time_s, (unsigned long)ctrl_ticks, (unsigned long)ctrl_status, (unsigned long)detail,
              (unsigned int)run_enable, (double)vd_pos_ref, (double)freq_hz,
              (double)v_dc_p, (double)v_dc_n, (double)v_dc_half_min, (double)v_alpha, (double)v_beta);
}

/** @param value DLL double input. @return true for finite double values without narrowing. */
static inline bool finite_value(double value)
{
    return (value >= -DBL_MAX) && /* Excludes negative infinity and NaN. */
           (value <= DBL_MAX);    /* Excludes positive infinity. */
}

/** @param alpha Alpha voltage. @param beta Beta voltage. @param vdc_p Positive half bus.
 * @param vdc_n Negative half bus. @return Detailed PWM status, zero on success. */
static uint32_t platform_pwm_write(float alpha, float beta, float vdc_p, float vdc_n)
{
    const svpwm_3level_input_t input = {.v_alpha = alpha, .v_beta = beta, .v_dc_p = vdc_p, .v_dc_n = vdc_n};
    SVPWM_3LEVEL_STATUS_E status = pwm_update(&input);
    if (status == SVPWM_3LEVEL_OK)
    {
        v_alpha_pwm = alpha;
        v_beta_pwm = beta;
        return 0U; /* HAL success is zero; SVPWM success is not zero. */
    }
    /* NOT_READY is zero in the library and must never become HAL success. */
    return (status == SVPWM_3LEVEL_NOT_READY) ? UINT32_MAX : (uint32_t)status;
}
static void platform_pwm_disable(void)
{
    pwm_disable();
    v_alpha_pwm = 0.0f;
    v_beta_pwm = 0.0f;
}
/* Bind physical samples and the modulation action while the dispatcher is stopped. */
static void bind_control_hal(void)
{
    float *p_voltages[3] = {&v_out_a, &v_out_b, &v_out_c};
    float *p_currents[3] = {&i_l_a, &i_l_b, &i_l_c};
    npc_hal_unlock_binding();
    for (uint32_t phase = 0U; phase < 3U; ++phase)
    {
        npc_hal_set_v_out_ptr(phase, p_voltages[phase]);
        npc_hal_set_i_l_ptr(phase, p_currents[phase]);
    }
    npc_hal_set_v_dc_p_ptr(&v_dc_p);
    npc_hal_set_v_dc_n_ptr(&v_dc_n);
    npc_hal_set_theta_ptr(&theta);
    npc_hal_set_vd_pos_ref_ptr(&vd_pos_ref);
    npc_hal_set_pwm_setter(platform_pwm_write);
    npc_hal_set_pwm_disable(platform_pwm_disable);
    npc_hal_lock_binding();
}
static void disable_control(void)
{
    npc_ctrl_stop();
    control_monitor = *npc_ctrl_get_monitor();
    v_alpha = 0.0f;
    v_beta = 0.0f;
    vd_pos_ref_act = 0.0f;
}

/** @param p_state Current callback whose output array receives the held frame. */
static void publish(const struct SimulationState *p_state)
{
    if (p_state->outputs != NULL)
    {
        for (uint32_t i = 0u; i < (uint32_t)PLECS_OUTPUT_MAX; ++i) /* Output channel index. */
        {
            p_state->outputs[i] = (double)output_frame[i];
        }
    }
}

void plecs_set_output(PLECS_OUTPUT_E num, float val)
{
    if (((int)num >= 0) &&        /* Reject negative channel identifiers. */
        (num < PLECS_OUTPUT_MAX)) /* Keep BSP writes inside the held frame. */
    {
        output_frame[num] = val;
    }
}

void plecsSetSizes(struct SimulationSizes *p_sizes)
{
    if (p_sizes != NULL)
    {
        p_sizes->numInputs = (int)PLECS_INPUT_MAX;
        p_sizes->numOutputs = (int)PLECS_OUTPUT_MAX;
        p_sizes->numStates = 0;
        p_sizes->numParameters = 0;
    }
}

void plecsStart(struct SimulationState *p_state)
{
    npc_ctrl_cfg_t cfg = npc_cfg_default(); /* Platform uses the shared validated candidate parameters. */
    frame_tcp_server_stop();
    bind_control_hal();
    log_ready = (npc_log_start() == 1) ? 1u : 0u;
    ctrl_status = UINT32_MAX;
    ctrl_detail = 0u;
    ctrl_ticks = 0u;
    trace_enable = 1u;
    trace_state = 1u;
    trace_rows = 0u;
    PLECS_LOG("NPC APP build=%s %s kp_v=%.9g ki_v=%.9g kp_i=%.9g ki_i=%.9g "
              "current_peak=%.9g overvoltage_trip=off dll_delay_samples=0 trace_hz=5000\n",
              __DATE__, __TIME__, (double)cfg.kp_v, (double)cfg.ki_v,
              (double)cfg.kp_i, (double)cfg.ki_i, (double)cfg.current_peak);
    trip_current = NPC_CFG_TRIP_CURRENT_FACTOR * cfg.current_peak;
    v_dc_p = 0.0f;
    v_dc_n = 0.0f;
    plecs_dispatch_lock_start();
    initialized = false;
    first_update = true;
    disable_control();
    if (p_state == NULL)
    {
        return;
    }
    publish(p_state);
    if (finite_value(p_state->time) == false)
    {
        report_status(NPC_APP_TIMING, 0u, p_state->time);
        p_state->errorMessage = "NPC requires finite start time.";
        return;
    }
    v_alpha = 0.0f;
    v_beta = 0.0f;
    v_out_a = 0.0f;
    v_out_b = 0.0f;
    v_out_c = 0.0f;
    i_l_a = 0.0f;
    i_l_b = 0.0f;
    i_l_c = 0.0f;
    vd_pos_ref = 0.0f;
    vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS;
    theta = 0.0f;
    freq_hz = 50.0f;
    phase_cycle = 0.0;
    run_enable = 0u;
    v_dc_half_min = 20.0f;
    applied_half_min = v_dc_half_min;
    __atomic_store_n(&plecs_time_100us, 0u, __ATOMIC_RELAXED);
    initialized = pwm_init(applied_half_min);
    if (npc_cfg_set_ctrl_ts((float)PLECS_NPC_CONTROL_PERIOD_S) == 0U) initialized = false;
    section_init(); /* Registers lifecycle tasks and the 5 kHz controller callback. */
    if ((npc_hal_is_ready() == 0U) || (npc_cfg_is_ready() == 0U) ||
        (npc_ctrl_get_monitor()->status == 7U)) initialized = false;
    last_call_time = p_state->time;
    update_origin = p_state->time;
    update_index = 0u;
    if (initialized == false)
    {
        report_status(NPC_APP_INIT, 0u, p_state->time);
        p_state->errorMessage = "NPC controller or PWM initialization failed.";
    }
    else
    {
        report_status(NPC_APP_OFF, 0u, p_state->time);
        frame_tcp_server_start();
    }
}

/* The controller's external phase input uses the same published frequency as its DSOGIs. */
static void generate_reference(void)
{
    npc_ctrl_setpoint_t setpoint = {NPC_CFG_DEFAULT_VD_POS_SLEW_VPS, 50.0f, 20.0f, 0U};
    (void)npc_fsm_read_published(&setpoint);
    theta = (float)(NPC_TWO_PI * phase_cycle);
    phase_cycle += (double)setpoint.freq_hz * PLECS_NPC_CONTROL_PERIOD_S;
    if (phase_cycle >= 1.0) phase_cycle -= 1.0;
}

/** @param value Host feedback sample. @return Float sample, or NaN for invalid/out-of-range input. */
static inline float sample_feedback(double value)
{
    if ((finite_value(value) == false) || /* Invalid host sample. */
        (fabs(value) > FLT_MAX))          /* Prevent float conversion overflow. */
    {
        return nanf("");
    }
    return (float)value;
}

/** @param p_state Current 5 kHz host snapshot; no control state lives in app. */
static void update(const struct SimulationState *p_state)
{
    ++ctrl_ticks;
    v_dc_p = sample_feedback(p_state->inputs[PLECS_INPUT_V_DC_P]);
    v_dc_n = sample_feedback(p_state->inputs[PLECS_INPUT_V_DC_N]);
    v_out_a = sample_feedback(p_state->inputs[PLECS_INPUT_V_OUT_A]);
    v_out_b = sample_feedback(p_state->inputs[PLECS_INPUT_V_OUT_B]);
    v_out_c = sample_feedback(p_state->inputs[PLECS_INPUT_V_OUT_C]);
    i_l_a = sample_feedback(p_state->inputs[PLECS_INPUT_I_L_A]);
    i_l_b = sample_feedback(p_state->inputs[PLECS_INPUT_I_L_B]);
    i_l_c = sample_feedback(p_state->inputs[PLECS_INPUT_I_L_C]);
    /* Shell staging is serialized with this dispatcher; cfg is the application API. */
    if ((npc_cfg_set_vd_pos_slew_vps(vd_pos_slew_vps) == 0U) ||
        (npc_cfg_set_freq_hz(freq_hz) == 0U) ||
        (npc_cfg_set_run_request(run_enable) == 0U))
    {
        (void)npc_cfg_set_run_request(0U);
        disable_control();
        report_status(NPC_APP_REFERENCE, 0U, p_state->time);
        return;
    }
    if (v_dc_half_min != applied_half_min)
    {
        if ((npc_cfg_set_v_dc_half_min(v_dc_half_min) == 0U) || !pwm_init(v_dc_half_min))
        {
            disable_control();
            report_status(NPC_APP_PWM_CONFIG, 0U, p_state->time);
            return;
        }
        applied_half_min = v_dc_half_min;
    }
    generate_reference();
    section_interrupt();
    control_monitor = *npc_ctrl_get_monitor();
    v_alpha = control_monitor.output.v_alpha;
    v_beta = control_monitor.output.v_beta;
    vd_pos_ref_act = control_monitor.vd_pos_ref_act;
    if ((control_monitor.status == NPC_CTRL_OVERCURRENT) &&
        (ctrl_status != control_monitor.status))
    {
        PLECS_LOG("NPC trip t=%.9f status=%lu phase=%lu i=[%.6g %.6g %.6g] "
                  "v=[%.6g %.6g %.6g] i_trip=%.6g\n",
                  p_state->time, (unsigned long)control_monitor.status, (unsigned long)control_monitor.detail,
                  (double)i_l_a, (double)i_l_b, (double)i_l_c,
                  (double)v_out_a, (double)v_out_b, (double)v_out_c,
                  (double)trip_current);
    }
    report_status((NPC_APP_STATUS_E)control_monitor.status, control_monitor.detail, p_state->time);
}

/** @param time_s Timestamp of the coherent post-control snapshot, before Shell task execution. */
static void trace_sample(double time_s)
{
    const float values[] = {
        (float)run_enable, (float)ctrl_status, (float)ctrl_detail, v_dc_p, v_dc_n,
        v_out_a, v_out_b, v_out_c, i_l_a, i_l_b, i_l_c, theta, vd_pos_ref, vd_pos_ref_act,
        control_monitor.output.v_dq[0], control_monitor.output.v_dq[1], control_monitor.output.v_dq[2], control_monitor.output.v_dq[3],
        control_monitor.output.i_dq[0], control_monitor.output.i_dq[1], control_monitor.output.i_dq[2], control_monitor.output.i_dq[3],
        control_monitor.output.i_ref[0], control_monitor.output.i_ref[1], control_monitor.output.i_ref[2], control_monitor.output.i_ref[3],
        v_alpha, v_beta, v_alpha_pwm, v_beta_pwm,
        control_monitor.integral_v[0], control_monitor.integral_v[1], control_monitor.integral_v[2], control_monitor.integral_v[3],
        control_monitor.integral_i[0], control_monitor.integral_i[1], control_monitor.integral_i[2], control_monitor.integral_i[3],
        control_monitor.output.current_limited ? 1.0f : 0.0f, control_monitor.output.voltage_limited ? 1.0f : 0.0f,
        output_frame[0], output_frame[1], output_frame[2], output_frame[3], output_frame[4], output_frame[5], output_frame[6]};
    static const char header[] =
        "time_s,run,status,detail,vdc_p,vdc_n,va,vb,vc,ia,ib,ic,theta,ref,ref_actual,"
        "vd_pos,vq_pos,vd_neg,vq_neg,id_pos,iq_pos,id_neg,iq_neg,"
        "id_pos_ref,iq_pos_ref,id_neg_ref,iq_neg_ref,alpha,beta,alpha_pwm,beta_pwm,"
        "integral_vdp,integral_vqp,integral_vdn,integral_vqn,integral_idp,integral_iqp,integral_idn,integral_iqn,"
        "current_limited,voltage_limited,duty_a_pos,duty_a_neg,duty_b_pos,duty_b_neg,duty_c_pos,duty_c_neg,pwm_enable";
    if (trace_enable != 1u)
    {
        if (trace_state == 2u)
        {
            npc_trace_stop();
            trace_state = 0u;
        }
        return;
    }
    if (trace_state != 2u)
    {
        trace_state = 1u;
        if (run_enable != 1u)
        {
            return;
        }
        trace_rows = 0u;
        if (npc_trace_start(header) == 0)
        {
            trace_state = 4u;
            trace_enable = 0u;
            PLECS_LOG("NPC trace open failed t=%.9f\n", time_s);
            return;
        }
        trace_state = 2u;
        PLECS_LOG("NPC trace started t=%.9f samples=100000 rate=5000 Hz\n", time_s);
    }
    if (npc_trace_write(time_s, values, sizeof(values) / sizeof(values[0])) == 0)
    {
        trace_state = 4u;
        trace_enable = 0u;
        PLECS_LOG("NPC trace write failed t=%.9f rows=%lu\n", time_s, (unsigned long)trace_rows);
        return;
    }
    ++trace_rows;
    if (trace_rows >= 100000u)
    {
        npc_trace_stop();
        trace_state = 3u;
        trace_enable = 0u;
        PLECS_LOG("NPC trace complete t=%.9f rows=%lu\n", time_s, (unsigned long)trace_rows);
    }
}

/** @param p_state Current callback; caller holds the shared Shell dispatch lock. */
static void output_locked(struct SimulationState *p_state)
{
    double time_tolerance = NPC_TIME_TOLERANCE_S; /* Absolute-time representation allowance, seconds. */
    double next_update_time = 0.0;                /* Next scheduled update computed directly from its index. */
    if (p_state == NULL)
    {
        return;
    }
    if ((initialized == false) ||    /* Start must initialize the modulator. */
        (p_state->inputs == NULL) || /* Require the 8 declared inputs. */
        (p_state->outputs == NULL))  /* Require the 7 declared outputs. */
    {
        report_status(NPC_APP_INIT, 1u, p_state->time);
        disable_control();
        p_state->errorMessage = "NPC PWM is not initialized or its ports are unavailable.";
        publish(p_state);
        return;
    }
    time_tolerance += 8.0 * DBL_EPSILON * fmax(fabs(p_state->time), fabs(last_call_time));
    if ((finite_value(p_state->time) == false) ||            /* Reject invalid simulation timestamps. */
        (p_state->time < (last_call_time - time_tolerance))) /* Rollback cannot restore private DLL state. */
    {
        report_status(NPC_APP_TIMING, 1u, p_state->time);
        disable_control();
        initialized = false;
        p_state->errorMessage = "NPC requires finite, monotonic simulation time.";
        publish(p_state);
        return;
    }
    last_call_time = p_state->time;
    if (first_update == true)
    {
        update_origin = p_state->time;
        update_index = 0u;
        first_update = false;
    }
    next_update_time = update_origin + (double)update_index * PLECS_NPC_CONTROL_PERIOD_S;
    if (p_state->time > (next_update_time + time_tolerance))
    {
        report_status(NPC_APP_TIMING, 2u, p_state->time);
        disable_control();
        initialized = false;
        p_state->errorMessage = "NPC missed a 200 us update: set DLL sample time to 2e-4 s or an exact subdivision.";
        publish(p_state);
        return;
    }
    if (p_state->time >= (next_update_time - time_tolerance))
    {
        update(p_state);
        trace_sample(p_state->time);
        (void)__atomic_add_fetch(&plecs_time_100us, 2u, __ATOMIC_RELAXED);
        run_task(); /* Mark due tasks and execute them, including the 1 ms Shell waveform reporter. */
        ++update_index;
    }
    publish(p_state);
}

void plecsOutput(struct SimulationState *p_state)
{
    frame_tcp_server_dispatch_enter();
    output_locked(p_state);
    frame_tcp_server_dispatch_exit();
}

void plecsTerminate(struct SimulationState *p_state)
{
    frame_tcp_server_stop();
    report_status(NPC_APP_STOPPED, 0u, last_call_time);
    (void)npc_cfg_set_run_request(0U);
    npc_log_stop();
    log_ready = 0u;
    plecs_dispatch_lock_stop();
    disable_control();
    initialized = false;
    if (p_state != NULL)
    {
        publish(p_state);
    }
}
