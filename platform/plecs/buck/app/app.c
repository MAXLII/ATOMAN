// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   PLECS buck application module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Bind simulated ADC and PWM interface objects to the buck HAL
 *          - Register Buck commands, setpoints, state, and analog values in Shell
 *          - Translate the Shell RUN request into buck FSM start and stop commands
 *          - Publish the buck FSM state to Shell and the PLECS output vector
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-24
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "app.h"
#include "bsp_adc.h"
#include "bsp_pwm.h"
#include "buck_cfg.h"
#include "buck_fsm.h"
#include "buck_hal.h"
#include "frame_tcp_server.h"
#include "plecs.h"
#include "section.h"
#include "shell.h"
#include "timing.h"

/* Inductor-current ADC center code used to convert offset-binary samples to signed control codes. */
#define APP_ADC_IND_CURR_CODE_CENTER ((int32_t)0x4000)

/* Maximum voltage feedback code exposed through Shell. */
#define APP_ADC_VOLT_CODE_MAX ((int32_t)BSP_ADC_VOLT_CODE_MAX)

/* Maximum signed inductor-current feedback code exposed through Shell. */
#define APP_ADC_IND_CURR_CODE_MAX \
    ((int32_t)(BSP_ADC_IND_CURR_CODE_MAX - (uint32_t)APP_ADC_IND_CURR_CODE_CENTER))

/* Minimum signed inductor-current feedback code exposed through Shell. */
#define APP_ADC_IND_CURR_CODE_MIN (-APP_ADC_IND_CURR_CODE_CENTER)

/* Maximum public Buck run state value exposed through Shell. */
#define APP_RUN_STATE_MAX ((uint32_t)buck_run_sta_run)

/* Minimum public Buck run state value exposed through Shell. */
#define APP_RUN_STATE_MIN ((uint32_t)buck_run_sta_init)

/* Tracks whether the PLECS application has already attempted HAL binding. */
static uint8_t app_buck_hal_bound = 0U;

/* Tracks whether Buck control timing has already been configured. */
static uint8_t app_buck_timing_bound = 0U;

/* Desired Buck run state toggled by the Shell RUN command. */
static uint8_t app_run_request = 0U;

/* Buck run state mirrored from the FSM for Shell and PLECS observation. */
static uint32_t app_run_state = (uint32_t)buck_run_sta_init;

/* Input power limit in watts, replacing the removed PLECS input port. */
static float app_pwr_lmt = BUCK_CTRL_IN_PWR_LMT_DEFAULT_W;

/* Input current limit in amperes, replacing the removed PLECS input port. */
static float app_in_curr_lmt = BUCK_CTRL_IN_CURR_LMT_DEFAULT_A;

/* Output current limit in amperes, replacing the removed PLECS input port. */
static float app_out_curr_lmt = BUCK_CTRL_OUT_CURR_LMT_DEFAULT_A;

/* Output voltage reference in volts, replacing the removed PLECS input port. */
static float app_out_volt_ref = BUCK_CTRL_OUT_VOLT_LOOP_REF_DEFAULT_V;

/* High-voltage physical input sampled from PLECS for Shell observation. */
static float app_hv = 0.0f;

/* Low-voltage physical input sampled from PLECS for Shell observation. */
static float app_lv = 0.0f;

/* A-channel inductor-current physical input sampled from PLECS for Shell observation. */
static float app_ila = 0.0f;

/* B-channel inductor-current physical input sampled from PLECS for Shell observation. */
static float app_ilb = 0.0f;

/* FRAME TCP server port exposed as a read-only Shell parameter. */
static uint32_t app_frame_tcp_port = FRAME_TCP_SERVER_PORT;

/* App-owned high-voltage feedback mirror supplied by the ADC interface. */
static int32_t app_adc_hv = 0;

/* App-owned low-voltage feedback mirror supplied by the ADC interface. */
static int32_t app_adc_lv = 0;

/* App-owned A-channel inductor-current feedback mirror supplied by the ADC interface. */
static int32_t app_adc_ila = 0;

/* App-owned B-channel inductor-current feedback mirror supplied by the ADC interface. */
static int32_t app_adc_ilb = 0;

/**
 * @brief Toggle the desired Buck run state from the Shell RUN command.
 * @param[in] my_printf Shell transport interface associated with the command request.
 */
static void app_run_cmd(DEC_MY_PRINTF)
{
    app_run_request = (app_run_request == 0U) ? 1U : 0U;
    PLECS_LOG("Shell RUN toggled request to %u\n", (unsigned)app_run_request);

    if ((my_printf != NULL) &&
        (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("RUN request=%u\r\n", (unsigned)app_run_request);
    }
}

REG_SHELL_CMD(RUN, app_run_cmd)
REG_SHELL_VAR(PWR_LMT,
              app_pwr_lmt,
              SHELL_FP32,
              BUCK_CTRL_IN_PWR_LMT_MAX_W,
              0.0f,
              NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(IN_CURR_LMT,
              app_in_curr_lmt,
              SHELL_FP32,
              BUCK_CTRL_IN_CURR_LMT_MAX_A,
              0.0f,
              NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(OUT_CURR_LMT,
              app_out_curr_lmt,
              SHELL_FP32,
              BUCK_CTRL_OUT_CURR_LMT_MAX_A,
              0.0f,
              NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(OUT_VOLT_REF,
              app_out_volt_ref,
              SHELL_FP32,
              BUCK_CTRL_OUT_VOLT_LOOP_REF_MAX_V,
              0.0f,
              NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(RUN_REQUEST, app_run_request, SHELL_UINT8, 1U, 0U, NULL, SHELL_STA_READ_ONLY)
REG_SHELL_VAR(RUN_STATE,
              app_run_state,
              SHELL_UINT32,
              APP_RUN_STATE_MAX,
              APP_RUN_STATE_MIN,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(HV,
              app_hv,
              SHELL_FP32,
              BSP_ADC_VOLT_MAX_V,
              BSP_ADC_VOLT_MIN_V,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(LV,
              app_lv,
              SHELL_FP32,
              BSP_ADC_VOLT_MAX_V,
              BSP_ADC_VOLT_MIN_V,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(ILA,
              app_ila,
              SHELL_FP32,
              BSP_ADC_IND_CURR_MAX_A,
              BSP_ADC_IND_CURR_MIN_A,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(ILB,
              app_ilb,
              SHELL_FP32,
              BSP_ADC_IND_CURR_MAX_A,
              BSP_ADC_IND_CURR_MIN_A,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(HV_CODE,
              app_adc_hv,
              SHELL_INT32,
              APP_ADC_VOLT_CODE_MAX,
              0,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(LV_CODE,
              app_adc_lv,
              SHELL_INT32,
              APP_ADC_VOLT_CODE_MAX,
              0,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(ILA_CODE,
              app_adc_ila,
              SHELL_INT32,
              APP_ADC_IND_CURR_CODE_MAX,
              APP_ADC_IND_CURR_CODE_MIN,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(ILB_CODE,
              app_adc_ilb,
              SHELL_INT32,
              APP_ADC_IND_CURR_CODE_MAX,
              APP_ADC_IND_CURR_CODE_MIN,
              NULL,
              SHELL_STA_READ_ONLY)
REG_SHELL_VAR(FRAME_TCP_PORT,
              app_frame_tcp_port,
              SHELL_UINT32,
              FRAME_TCP_SERVER_PORT,
              FRAME_TCP_SERVER_PORT,
              NULL,
              SHELL_STA_READ_ONLY)

static inline void app_update_adc_feedback(void)
{
    app_hv = plecs_get_input(PLECS_INPUT_HV);
    app_lv = plecs_get_input(PLECS_INPUT_LV);
    app_ila = plecs_get_input(PLECS_INPUT_ILA);
    app_ilb = plecs_get_input(PLECS_INPUT_ILB);

    app_adc_hv = (int32_t)BSP_ADC_HV;
    app_adc_lv = (int32_t)BSP_ADC_LV;
    app_adc_ila = (int32_t)BSP_ADC_ILA - APP_ADC_IND_CURR_CODE_CENTER;
    app_adc_ilb = (int32_t)BSP_ADC_ILB - APP_ADC_IND_CURR_CODE_CENTER;

    plecs_set_output(PLECS_OUTPUT_HV_CODE, (float)app_adc_hv);
    plecs_set_output(PLECS_OUTPUT_LV_CODE, (float)app_adc_lv);
    plecs_set_output(PLECS_OUTPUT_ILA_CODE, (float)app_adc_ila);
    plecs_set_output(PLECS_OUTPUT_ILB_CODE, (float)app_adc_ilb);
}

static void app_adc_feedback_isr(void)
{
    app_update_adc_feedback();
}

REG_INTERRUPT(0, app_adc_feedback_isr)

static void app_bind_buck_hal(void)
{
    if (app_buck_hal_bound != 0U)
    {
        return;
    }

    /* ADC feedback is supplied by the interface layer through app-owned mirrors. */
    buck_hal_set_v_in_ptr(&app_adc_hv);
    buck_hal_set_v_out_ptr(&app_adc_lv);
    buck_hal_set_i_l_ptr(0U, &app_adc_ila);
    buck_hal_set_i_l_ptr(1U, &app_adc_ilb);
    buck_hal_set_pwm_setter(0U, bsp_pwm_set_a_cmp);
    buck_hal_set_pwm_setter(1U, bsp_pwm_set_b_cmp);
    buck_hal_set_pwm_disable(bsp_pwm_disable);

    app_buck_hal_bound = buck_hal_is_ready();
}

static void app_bind_buck_timing(void)
{
    buck_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
        .task_ts = 100.0e-6f,
        .pwm_cmp_max = CTRL_PWM_CMP_MAX,
    };

    if (app_buck_timing_bound != 0U)
    {
        return;
    }

    buck_cfg_set_timing(&timing);
    app_buck_timing_bound = buck_cfg_is_ready();
}

static void app_update_buck_setpoint(void)
{
    buck_cfg_set_pwr_lmt(app_pwr_lmt);
    buck_cfg_set_in_curr_lmt(app_in_curr_lmt);
    buck_cfg_set_out_curr_lmt(app_out_curr_lmt);
    buck_cfg_set_out_volt_ref(app_out_volt_ref);
}

static void app_task(void)
{
    /* Buck FSM state mirrored to the PLECS output vector. */
    buck_run_sta_e run_sta = buck_run_sta_init;

    app_update_adc_feedback();
    app_bind_buck_timing();
    app_update_buck_setpoint();

    run_sta = buck_fsm_get_run_sta();
    app_run_state = (uint32_t)run_sta;
    plecs_set_output(PLECS_OUTPUT_RUN_STATE, (float)run_sta);

    if (app_run_request != 0U)
    {
        if (run_sta == buck_run_sta_idle)
        {
            app_bind_buck_hal();
            buck_fsm_set_cmd(buck_fsm_cmd_start);
        }
    }
    else
    {
        app_buck_hal_bound = 0U;
        if (run_sta != buck_run_sta_idle)
        {
            buck_fsm_set_cmd(buck_fsm_cmd_stop);
        }
    }
}

REG_TASK_MS(1, app_task)
