// SPDX-License-Identifier: MIT
/**
 * @file app.c
 * @brief Bind the CHB controller to eight PLECS samples and FRAME commands.
 * @details The DLL supplies eight physical samples; the shared CHB observer owns
 *          the PLL, harmonic extraction and current quadrature calculations.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_cfg.h"
#include "chb_ctrl.h"
#include "chb_fsm.h"
#include "chb_hal.h"
#include "chb_protect.h"
#include "bsp_pwm.h"
#include "plecs.h"
#include "pwm.h"
#include "section.h"
#include "shell.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

#define CHB_APP_CURRENT_TRIP_A  200.0f
#define CHB_APP_BUS_MIN_V       500.0f
#define CHB_APP_BUS_MAX_V       4200.0f
#define CHB_APP_LOAD_MIN_OHM    1.0f
#define CHB_APP_LOAD_OFF_OHM    1.0e6f /* Near-open load: 10.24 W at 3200 V; avoids ill-conditioned variable resistors. */
#define CHB_APP_LOAD_TASK_MS    1u
#define CHB_APP_MONITOR_TASK_MS 1u /* Shell-only magnitudes do not participate in control. */

static float    grid_v;                   /* Instantaneous grid voltage at the DLL input. */
static float    grid_frequency_rate_hz_s; /* Measured frequency change rate, Hz/s. */
static float    grid_phase_error_rad;     /* Normalized PLL phase detector error, rad. */
static float    harmonic_feedback_weight; /* Confidence in stationary harmonic-current estimates, 0..1. */
static float    grid_rms_v;               /* Shell copy of observed grid RMS, V. */
static float    grid_hz;                  /* Shell copy of observed grid frequency, Hz. */
static float    theta_rad;                /* Shell copy of observed grid angle, rad. */
static float    i_alpha_a;                /* Mean input inductor current supplied by the model, A. */
static float    i_beta_a;                 /* Shell copy of observed quadrature current, A. */
static float    bus_v[CHB_CELL_COUNT];
static float    bus_i[CHB_CELL_COUNT]; /* 负载支路电流，母线流向负载为正，不含电容电流。 */
static float    grid_harmonic_peak_v[CHB_HARMONIC_COUNT];    /* Observed harmonic magnitudes for shell. */
static float    current_harmonic_peak_a[CHB_HARMONIC_COUNT]; /* Observed sampled-current harmonic peaks. */
static uint8_t  run_request;                /* FRAME writable; applied by the 1 ms app task. */
static uint32_t run_state;                  /* FRAME readable FSM state. */
static float    duty[CHB_CELL_COUNT];       /* Last modulation frame for FRAME inspection. */
static float    v_pwm_v[CHB_CELL_COUNT];    /* 各桥最终调制电压，V。 */
static float    load_r_ohm[CHB_CELL_COUNT]; /* FRAME staging values; LOAD_APPLY captures all three. */
static float    load_pending_ohm[CHB_CELL_COUNT]; /* Snapshot consumed by the dedicated load task. */
static uint8_t  load_apply_pending;               /* Shell and tasks run in the same PLECS scheduler context. */

REG_SHELL_VAR(RUN_REQUEST, run_request, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(RUN_STATE, run_state, SHELL_UINT32, 5u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_V, grid_v, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(GRID_RMS_V, grid_rms_v, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H3_PEAK_V, grid_harmonic_peak_v[0], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H5_PEAK_V, grid_harmonic_peak_v[1], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H7_PEAK_V, grid_harmonic_peak_v[2], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H9_PEAK_V, grid_harmonic_peak_v[3], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_H3_PEAK_A, current_harmonic_peak_a[0], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_H5_PEAK_A, current_harmonic_peak_a[1], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_H7_PEAK_A, current_harmonic_peak_a[2], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_H9_PEAK_A, current_harmonic_peak_a[3], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_HZ, grid_hz, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_FREQ_RATE_HZ_S, grid_frequency_rate_hz_s, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_PHASE_ERROR_RAD, grid_phase_error_rad, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(HARM_FEEDBACK_WEIGHT, harmonic_feedback_weight, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(THETA_RAD, theta_rad, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_L, i_alpha_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(I_BETA_A, i_beta_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(BUS_1_V, bus_v[0], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_2_V, bus_v[1], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_3_V, bus_v[2], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_1_I, bus_i[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(BUS_2_I, bus_i[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(BUS_3_I, bus_i[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DUTY_1, duty[0], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DUTY_2, duty[1], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DUTY_3, duty[2], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VPWM_1_V, v_pwm_v[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VPWM_2_V, v_pwm_v[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VPWM_3_V, v_pwm_v[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(LOAD_R1, load_r_ohm[0], SHELL_FP32, CHB_APP_LOAD_OFF_OHM, CHB_APP_LOAD_MIN_OHM, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(LOAD_R2, load_r_ohm[1], SHELL_FP32, CHB_APP_LOAD_OFF_OHM, CHB_APP_LOAD_MIN_OHM, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(LOAD_R3, load_r_ohm[2], SHELL_FP32, CHB_APP_LOAD_OFF_OHM, CHB_APP_LOAD_MIN_OHM, NULL,
              SHELL_STA_NULL)

/** @brief Validate and snapshot a load step at the simulation command boundary. */
static void load_apply_command(shell_core_io_t *p_io)
{
    (void)p_io;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        if (!(    (load_r_ohm[cell] >= CHB_APP_LOAD_MIN_OHM)
               && (load_r_ohm[cell] <= CHB_APP_LOAD_OFF_OHM)))
        {
            PLECS_LOG("LOAD_APPLY rejected: resistance outside 1..1e6 ohm\n");
            return;
        }
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        load_pending_ohm[cell] = load_r_ohm[cell];
    }
    load_apply_pending = 1u;
}
REG_SHELL_CMD(LOAD_APPLY, load_apply_command)

static void clear_fault_command(shell_core_io_t *p_io)
{
    (void)p_io;
    (void)chb_fsm_clear_fault();
    (void)chb_protect_clear_latch();
}
REG_SHELL_CMD(CLEAR_FAULT, clear_fault_command)

static void pwm_write_cell(uint32_t                cell,
                           float                   v_pwm,
                           float                   bus_voltage,
                           chb_pwm_deadtime_flag_t deadtime_flag)
{
    const float compensated = bsp_pwm_deadtime_compensate(v_pwm / bus_voltage, (int8_t)deadtime_flag);
    v_pwm_v[cell]           = v_pwm;
    duty[cell] = 0.5f * (fminf(1.0f, fmaxf(-1.0f, compensated)) + 1.0f);
    chb_pwm_set_cell(cell, v_pwm, bus_voltage, deadtime_flag);
}

static void pwm_write_cell_1(float v_pwm, float bus_voltage, chb_pwm_deadtime_flag_t deadtime_flag)
{
    pwm_write_cell(0u, v_pwm, bus_voltage, deadtime_flag);
}

static void pwm_write_cell_2(float v_pwm, float bus_voltage, chb_pwm_deadtime_flag_t deadtime_flag)
{
    pwm_write_cell(1u, v_pwm, bus_voltage, deadtime_flag);
}

static void pwm_write_cell_3(float v_pwm, float bus_voltage, chb_pwm_deadtime_flag_t deadtime_flag)
{
    pwm_write_cell(2u, v_pwm, bus_voltage, deadtime_flag);
    const chb_observer_sample_t *p_observed = chb_observer_get_sample();

    if ((plecs_time_100us % 5u) == 0u)
    {
        PLECS_LOG("PWM vg=%g il=%g ib=%g th=%g bus=%g,%g,%g vcmd=%g duty=%g,%g,%g vpwm=%g,%g,%g\n",
                  (double)grid_v,
                  (double)i_alpha_a,
                  (double)p_observed->i_beta_a,
                  (double)p_observed->theta_rad,
                  (double)bus_v[0],
                  (double)bus_v[1],
                  (double)bus_v[2],
                  (double)(v_pwm_v[0] + v_pwm_v[1] + v_pwm_v[2]),
                  (double)duty[0],
                  (double)duty[1],
                  (double)duty[2],
                  (double)v_pwm_v[0],
                  (double)v_pwm_v[1],
                  (double)v_pwm_v[2]);
    }
}

static void pwm_disable(void)
{
    chb_pwm_disable();

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        duty[cell]    = 0.0f;
        v_pwm_v[cell] = 0.0f;
    }
}

static void soft_start_relay_on(void)
{
    plecs_set_output(PLECS_OUTPUT_SOFT_START_RELAY, 1.0f);
}

static void soft_start_relay_off(void)
{
    plecs_set_output(PLECS_OUTPUT_SOFT_START_RELAY, 0.0f);
}

static void main_relay_on(void)
{
    plecs_set_output(PLECS_OUTPUT_MAIN_RELAY, 1.0f);
}

static void main_relay_off(void)
{
    plecs_set_output(PLECS_OUTPUT_MAIN_RELAY, 0.0f);
}

static void app_init(void)
{
    const chb_ctrl_hal_t binding = {
        .p_grid_v    = &grid_v,
        .p_i_alpha_a = &i_alpha_a,
        .p_bus_v = {&bus_v[0], &bus_v[1], &bus_v[2]},
        .p_load_i_a = {&bus_i[0], &bus_i[1], &bus_i[2]},
        .p_set_pwm_func[0]      = pwm_write_cell_1,
        .p_set_pwm_func[1]      = pwm_write_cell_2,
        .p_set_pwm_func[2]      = pwm_write_cell_3,
        .p_pwm_disable          = pwm_disable,
        .p_soft_start_relay_on  = soft_start_relay_on,
        .p_soft_start_relay_off = soft_start_relay_off,
        .p_main_relay_on        = main_relay_on,
        .p_main_relay_off       = main_relay_off,
    };
    chb_ctrl_cfg_t cfg = chb_cfg_default();            /* 平台确定采样链延迟，其余沿用控制默认值。 */
    cfg.current_sample_delay_s = 4.5f * cfg.ts / 8.0f; /* 模型 z^-1..z^-8 求平均的群延迟。 */

    run_request              = 0u;
    run_state                = (uint32_t)CHB_RUN_STATE_INIT;
    grid_frequency_rate_hz_s = 0.0f;
    grid_phase_error_rad     = 0.0f;
    harmonic_feedback_weight = 0.0f;
    grid_rms_v               = cfg.grid_rms_nominal_v;
    grid_hz                  = cfg.grid_hz;
    theta_rad                = 0.0f;
    i_beta_a                 = 0.0f;
    load_apply_pending       = 0u;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        grid_harmonic_peak_v[harmonic]    = 0.0f;
        current_harmonic_peak_a[harmonic] = 0.0f;
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        bus_v[cell]            = 0.0f;
        bus_i[cell]            = 0.0f;
        load_r_ohm[cell]       = CHB_APP_LOAD_OFF_OHM;
        load_pending_ohm[cell] = CHB_APP_LOAD_OFF_OHM;
        /* Valid positive resistance is required at t=0, before the first task tick. */
        plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_LOAD_R1 + cell), CHB_APP_LOAD_OFF_OHM);
    }
    (void)chb_cfg_set_run_request(0u);
    (void)chb_cfg_set_ctrl_cfg(&cfg);
    (void)chb_protect_configure(CHB_APP_CURRENT_TRIP_A, CHB_APP_BUS_MIN_V, CHB_APP_BUS_MAX_V);
    (void)chb_hal_bind(&binding);
    pwm_disable();
    main_relay_off();
}
REG_INIT(0, app_init)

/** @brief Acquire the eight physical inputs before this tick's HAL snapshot. */
static void app_sample(void)
{
    grid_v    = plecs_get_input(PLECS_INPUT_GRID_V);
    i_alpha_a = plecs_get_input(PLECS_INPUT_I_L);
    bus_v[0]  = plecs_get_input(PLECS_INPUT_BUS_1_V);
    bus_v[1]  = plecs_get_input(PLECS_INPUT_BUS_2_V);
    bus_v[2]  = plecs_get_input(PLECS_INPUT_BUS_3_V);
    bus_i[0]  = plecs_get_input(PLECS_INPUT_BUS_1_I);
    bus_i[1]  = plecs_get_input(PLECS_INPUT_BUS_2_I);
    bus_i[2]  = plecs_get_input(PLECS_INPUT_BUS_3_I);
}

REG_INTERRUPT(0, app_sample)

/** @brief Refresh Shell magnitudes from the last completed interrupt; PLECS tasks/ISR run serially. */
static void app_monitor_task(void)
{
    const chb_observer_sample_t *p_observed = chb_observer_get_sample();
    grid_rms_v                              = p_observed->grid_rms_v;
    grid_hz                                 = p_observed->grid_hz;
    theta_rad                               = p_observed->theta_rad;
    i_beta_a                                = p_observed->i_beta_a;
    grid_frequency_rate_hz_s                = p_observed->grid_frequency_rate_hz_s;
    grid_phase_error_rad                    = p_observed->grid_phase_error_rad;
    harmonic_feedback_weight                = p_observed->harmonic_feedback_weight;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        grid_harmonic_peak_v[harmonic]    = p_observed->grid_harmonic_peak_v[harmonic];
        current_harmonic_peak_a[harmonic] = p_observed->current_harmonic_peak_a[harmonic];
    }
}
REG_TASK_MS(CHB_APP_MONITOR_TASK_MS, app_monitor_task)

static void app_task(void)
{
    if (chb_protect_is_tripped() != 0u)
    {
        run_request = 0u;
    }
    (void)chb_cfg_set_run_request(run_request);
    run_state = (uint32_t)chb_fsm_get_run_state();
}
REG_TASK_MS(1u, app_task)

/** @brief Apply all three resistances in one callback, independently of PWM and the FSM. */
static void app_load_task(void)
{
    if (load_apply_pending != 0u)
    {
        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_LOAD_R1 + cell), load_pending_ohm[cell]);
        }
        load_apply_pending = 0u;
        PLECS_LOG("LOAD r=%g,%g,%g ohm\n",
                  (double)load_pending_ohm[0],
                  (double)load_pending_ohm[1],
                  (double)load_pending_ohm[2]);
    }
}
REG_TASK_MS(CHB_APP_LOAD_TASK_MS, app_load_task)
