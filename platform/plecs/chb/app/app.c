// SPDX-License-Identifier: MIT
/**
 * @file app.c
 * @brief Bind the CHB controller to eight PLECS samples and FRAME commands.
 * @details The model fixes the grid frequency at 50 Hz. A grid SOGI provides
 *          the grid angle and RMS amplitude. Cascaded current SOGIs reject DC
 *          before generating beta; alpha uses the model's eight-sample mean directly.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_cfg.h"
#include "chb_fsm.h"
#include "chb_hal.h"
#include "chb_protect.h"
#include "plecs.h"
#include "pwm.h"
#include "section.h"
#include "shell.h"
#include "sogi.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

#define CHB_APP_GRID_HZ         50.0f
#define CHB_APP_GRID_RMS_V      6000.0f
#define CHB_APP_SOGI_K          1.414213562f
#define CHB_APP_RMS_READY_TICKS 400u /* Two grid cycles before trusting amplitude. */
#define CHB_APP_CURRENT_TRIP_A  200.0f
#define CHB_APP_BUS_MIN_V       500.0f
#define CHB_APP_BUS_MAX_V       4200.0f
#define CHB_APP_LOAD_MIN_OHM    1.0f
#define CHB_APP_LOAD_OFF_OHM    1.0e6f /* Near-open load: 10.24 W at 3200 V; avoids ill-conditioned variable resistors. */
#define CHB_APP_LOAD_TASK_MS    1u

static float    grid_v;      /* Instantaneous grid voltage at the DLL input. */
static float    input_cap_v; /* This model has no input capacitor: always zero. */
static float    grid_rms_v;  /* SOGI amplitude divided by sqrt(2), V. */
static float    grid_hz;     /* Model frequency, Hz. */
static float    theta_rad;   /* Angle of the measured grid voltage, rad. */
static float    i_alpha_a;   /* Mean input inductor current supplied by the model, A. */
static float    i_beta_a;    /* SOGI quadrature current, A. */
static float    bus_v[CHB_CELL_COUNT];
static float    bus_i[CHB_CELL_COUNT]; /* 负载支路电流，母线流向负载为正，不含电容电流。 */
static sogi_t   grid_sogi;
static sogi_t   current_sogi;
static sogi_t   current_quadrature_sogi; /* 交流提取后再生成正交量，抑制 beta 通道的直流偏置。 */
static float    current_ac_a;            /* SOGI 带通提取的电流交流分量，A。 */
static uint32_t observer_ticks;
static uint8_t  run_request;          /* FRAME writable; applied by the 1 ms app task. */
static uint32_t run_state;            /* FRAME readable FSM state. */
static float    duty[CHB_CELL_COUNT]; /* Last modulation frame for FRAME inspection. */
static float    v_pwm_v[CHB_CELL_COUNT]; /* 各桥相位预测后的瞬时调制电压，V；死区补偿及 BSP 延迟前。 */
static float    cell_d_v[CHB_CELL_COUNT];         /* 各桥直接分配的 d 轴电压，V 峰值。 */
static float    cell_q_v[CHB_CELL_COUNT];         /* 各桥直接分配的 q 轴电压，V 峰值。 */
static float    load_r_ohm[CHB_CELL_COUNT];       /* FRAME staging values; LOAD_APPLY captures all three. */
static float    load_pending_ohm[CHB_CELL_COUNT]; /* Snapshot consumed by the dedicated load task. */
static uint8_t  load_apply_pending; /* Shell and tasks run in the same PLECS scheduler context. */

REG_SHELL_VAR(RUN_REQUEST, run_request, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(RUN_STATE, run_state, SHELL_UINT32, 5u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_V, grid_v, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(GRID_RMS_V, grid_rms_v, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_HZ, grid_hz, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
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
REG_SHELL_VAR(CELL_VD_1, cell_d_v[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CELL_VD_2, cell_d_v[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CELL_VD_3, cell_d_v[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CELL_VQ_1, cell_q_v[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CELL_VQ_2, cell_q_v[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CELL_VQ_3, cell_q_v[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
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

static void pwm_write(const chb_pwm_command_t *p_command)
{
    chb_pwm_update(p_command, v_pwm_v);
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        cell_d_v[cell] = p_command->cell_d_v[cell];
        cell_q_v[cell] = p_command->cell_q_v[cell];
        duty[cell] = 0.5f * (v_pwm_v[cell] / p_command->bus_v[cell] + 1.0f);
    }

    if ((plecs_time_100us % 5u) == 0u)
    {
        PLECS_LOG("PWM vg=%g il=%g ib=%g th=%g bus=%g,%g,%g vcmd=%g duty=%g,%g,%g idref=%g iqref=%g vpwm=%g,%g,%g\n",
                  (double)grid_v,
                  (double)i_alpha_a,
                  (double)i_beta_a,
                  (double)theta_rad,
                  (double)bus_v[0],
                  (double)bus_v[1],
                  (double)bus_v[2],
                  (double)(v_pwm_v[0] + v_pwm_v[1] + v_pwm_v[2]),
                  (double)duty[0],
                  (double)duty[1],
                  (double)duty[2],
                  (double)p_command->id_ref_a,
                  (double)(-p_command->i_comp_ref_a * sinf(theta_rad)
                           + p_command->i_comp_beta_ref_a * cosf(theta_rad)),
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
        duty[cell] = 0.0f;
        v_pwm_v[cell] = 0.0f;
    }
}

static void soft_start_relay_close(void)
{
    plecs_set_output(PLECS_OUTPUT_SOFT_START_RELAY, 1.0f);
}

static void soft_start_relay_open(void)
{
    plecs_set_output(PLECS_OUTPUT_SOFT_START_RELAY, 0.0f);
}

static void main_relay_close(void)
{
    plecs_set_output(PLECS_OUTPUT_MAIN_RELAY, 1.0f);
}

static void main_relay_open(void)
{
    plecs_set_output(PLECS_OUTPUT_MAIN_RELAY, 0.0f);
}

static void app_init(void)
{
    const chb_ctrl_hal_t binding = {
        .p_grid_v      = &grid_v,
        .p_input_cap_v = &input_cap_v,
        .p_grid_rms_v  = &grid_rms_v,
        .p_grid_hz     = &grid_hz,
        .p_theta_rad   = &theta_rad,
        .p_i_alpha_a   = &i_alpha_a,
        .p_i_beta_a    = &i_beta_a,
        .p_bus_v = {&bus_v[0], &bus_v[1], &bus_v[2]},
        .p_load_i_a = {&bus_i[0], &bus_i[1], &bus_i[2]},
        .p_set_pwm_func           = pwm_write,
        .p_pwm_disable            = pwm_disable,
        .p_soft_start_relay_close = soft_start_relay_close,
        .p_soft_start_relay_open  = soft_start_relay_open,
        .p_main_relay_close       = main_relay_close,
        .p_main_relay_open        = main_relay_open,
    };
    chb_ctrl_cfg_t cfg = chb_cfg_default(); /* 平台确定采样链延迟，其余沿用控制默认值。 */
    cfg.current_sample_delay_s = 4.5f * cfg.ts / 8.0f; /* 模型 z^-1..z^-8 求平均的群延迟。 */

    observer_ticks     = 0u;
    run_request        = 0u;
    run_state          = (uint32_t)CHB_RUN_STATE_INIT;
    input_cap_v        = 0.0f;
    grid_rms_v         = CHB_APP_GRID_RMS_V;
    grid_hz            = CHB_APP_GRID_HZ;
    theta_rad          = 0.0f;
    i_beta_a           = 0.0f;
    current_ac_a       = 0.0f;
    load_apply_pending = 0u;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        bus_v[cell]            = 0.0f;
        bus_i[cell]            = 0.0f;
        cell_d_v[cell]         = 0.0f;
        cell_q_v[cell]         = 0.0f;
        load_r_ohm[cell]       = CHB_APP_LOAD_OFF_OHM;
        load_pending_ohm[cell] = CHB_APP_LOAD_OFF_OHM;
        /* Valid positive resistance is required at t=0, before the first task tick. */
        plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_LOAD_R1 + cell), CHB_APP_LOAD_OFF_OHM);
    }
    sogi_init(&grid_sogi, cfg.ts, 2.0f * 3.141592654f * grid_hz, CHB_APP_SOGI_K, &grid_v);
    sogi_init(&current_sogi, cfg.ts, 2.0f * 3.141592654f * grid_hz, CHB_APP_SOGI_K, &i_alpha_a);
    sogi_init(&current_quadrature_sogi,
              cfg.ts,
              2.0f * 3.141592654f * grid_hz,
              CHB_APP_SOGI_K,
              &current_ac_a);
    (void)chb_cfg_set_run_request(0u);
    (void)chb_cfg_set_ctrl_cfg(&cfg);
    (void)chb_protect_configure(CHB_APP_CURRENT_TRIP_A, CHB_APP_BUS_MIN_V, CHB_APP_BUS_MAX_V);
    (void)chb_hal_bind(&binding);
    pwm_disable();
    main_relay_open();
}
REG_INIT(0, app_init)

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
    sogi_cal(&grid_sogi);
    sogi_cal(&current_sogi);
    current_ac_a = current_sogi.osg_u[0];
    sogi_cal(&current_quadrature_sogi);
    i_beta_a  = current_quadrature_sogi.osg_qu[0];
    theta_rad = atan2f(grid_sogi.osg_qu[0], grid_sogi.osg_u[0]);

    if (observer_ticks < CHB_APP_RMS_READY_TICKS)
    {
        ++observer_ticks;
    }
    else
    {
        float measured_rms_v = hypotf(grid_sogi.osg_u[0], grid_sogi.osg_qu[0]) * 0.707106781f;
        grid_rms_v += 0.01f * (measured_rms_v - grid_rms_v);
    }
}
REG_INTERRUPT(0, app_sample)

static void app_task(void)
{
    if (chb_hal_is_tripped() != 0u)
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
