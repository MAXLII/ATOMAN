// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   PLECS bidirectional CLLC application module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Bind PLECS ADC/PWM interfaces to the shared CLLC HAL
 *          - Publish direction, references, run, trip, and reset commands
 *          - Export FSM and controller debug data to the PLECS output vector
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ADC sampling precedes the control ISR through interrupt priority 0
 *          - Debug export follows the control ISR through interrupt priority 8
 *
 * @author  Max.Li
 * @date    2026-07-26
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "app.h"

#include "adc.h"
#include "cllc_cfg.h"
#include "cllc_ctrl.h"
#include "cllc_fsm.h"
#include "cllc_hal.h"
#include "plecs.h"
#include "pwm.h"
#include "section.h"
#include "timing.h"

static uint8_t hal_bound = 0u;         /* Nonzero after all required CLLC HAL bindings validate. */
static uint8_t timing_bound = 0u;      /* Nonzero after valid control/FSM timing is installed. */
static uint8_t hard_fault_last = 0u;   /* Previous hard-fault command for rising-edge detection. */
static uint8_t fault_reset_last = 0u;  /* Previous reset command for rising-edge detection. */
static uint8_t fault_latched = 0u;     /* PLECS-visible hard-protection latch bound to the CLLC HAL. */
static uint8_t trace_header_written = 0u; /* Nonzero after the current run writes its CSV trace header. */
static float battery_voltage_v = 0.0f; /* Latest battery-voltage feedback. */
static float battery_current_a = 0.0f; /* Latest battery/load-current feedback. */
static float bus_voltage_v = 0.0f;     /* Latest high-voltage-bus feedback. */

/** Decode the external direction command into the public CLLC enum. */
static CLLC_DIRECTION_E read_direction(void)
{
    return (plecs_get_input(PLECS_INPUT_DIRECTION) > 0.5f)
               ? CLLC_DIRECTION_REVERSE
               : CLLC_DIRECTION_FORWARD;
}

/** Sample all physical feedback before the registered CLLC control interrupt. */
static void sample_feedback(void)
{
    battery_voltage_v = cllc_adc_get_battery_voltage();
    battery_current_a = cllc_adc_get_battery_current();
    bus_voltage_v = cllc_adc_get_bus_voltage();
}

/** Registered priority-0 analog sampling entry. */
static void feedback_isr(void)
{
    sample_feedback();
}

REG_INTERRUPT(0, feedback_isr)

/** Install the configured control timing and 1 ms common FSM timing. */
static void bind_timing(void)
{
    cllc_ctrl_timing_t timing = {
        /* Timing shared by controller and FSM. */
        .ctrl_ts = CTRL_TS,
        .task_ts = 1.0e-3f,
        .startup_delay_ticks = 1u,
    };

    if (timing_bound == 1u)
    {
        return;
    }
    cllc_cfg_set_timing(&timing);
    timing_bound = cllc_cfg_is_ready();
}

/** Bind shared control HAL pointers to the PLECS interface layer. */
static void bind_hal(void)
{
    if (hal_bound == 1u)
    {
        return;
    }

    cllc_hal_unlock_binding();
    cllc_hal_set_v_battery_ptr(&battery_voltage_v);
    cllc_hal_set_i_battery_ptr(&battery_current_a);
    cllc_hal_set_v_bus_ptr(&bus_voltage_v);
    cllc_hal_set_modulation_setter(cllc_pwm_set_normalized);
    cllc_hal_set_pwm_enable(cllc_pwm_enable_direction);
    cllc_hal_set_pwm_disable(cllc_pwm_disable);
    cllc_hal_set_latched_ptr(&fault_latched);
    hal_bound = cllc_hal_is_ready();
    if (hal_bound == 1u)
    {
        cllc_hal_lock_binding();
    }
}

/** Publish references and requested direction from the PLECS DLL input vector. */
static void publish_setpoint(void)
{
    cllc_cfg_set_direction(read_direction());
    cllc_cfg_set_battery_voltage_ref(plecs_get_input(PLECS_INPUT_V_BATTERY_REF));
    cllc_cfg_set_battery_current_limit(plecs_get_input(PLECS_INPUT_I_BATTERY_LIMIT));
    cllc_cfg_set_bus_voltage_ref(plecs_get_input(PLECS_INPUT_V_BUS_REF));
    cllc_cfg_publish_building();
}

/** Process external hard-fault and reset edges without continuously reposting commands. */
static void process_fault_commands(void)
{
    uint8_t hard_fault = (plecs_get_input(PLECS_INPUT_HARD_FAULT) > 0.5f) ? (uint8_t)1u : (uint8_t)0u;
    uint8_t fault_reset = (plecs_get_input(PLECS_INPUT_FAULT_RESET) > 0.5f) ? (uint8_t)1u : (uint8_t)0u;

    if ((hard_fault == 1u) &&    /* The external fault input is active. */
        (hard_fault_last == 0u)) /* Only its rising edge issues a new trip. */
    {
        cllc_hal_hard_protect_trip();
    }
    if ((fault_reset == 1u) &&      /* The external reset input has a rising edge. */
        (fault_reset_last == 0u) && /* Avoid reposting reset every task tick. */
        (hard_fault == 0u))         /* Never clear a fault while its source remains active. */
    {
        cllc_fsm_set_cmd(CLLC_FSM_CMD_RESET);
    }
    hard_fault_last = hard_fault;
    fault_reset_last = fault_reset;
}

/** Translate the external run command into start/stop commands for the common FSM. */
static void process_run_command(void)
{
    uint8_t run_request = (plecs_get_input(PLECS_INPUT_RUN) > 0.5f) ? (uint8_t)1u : (uint8_t)0u;
    CLLC_RUN_STATE_E run_state = cllc_fsm_get_run_state(); /* Current common-FSM lifecycle state. */

    if (run_request == 1u)
    {
        if (run_state == CLLC_RUN_STATE_IDLE)
        {
            bind_hal();
            if (hal_bound == 1u)
            {
                cllc_fsm_set_cmd(CLLC_FSM_CMD_START);
            }
        }
        return;
    }

    if ((run_state == CLLC_RUN_STATE_STARTUP) || /* Cancel an in-progress bridge startup. */
        (run_state == CLLC_RUN_STATE_RUN))       /* Stop either active power-flow direction. */
    {
        cllc_fsm_set_cmd(CLLC_FSM_CMD_STOP);
    }
    if (run_state == CLLC_RUN_STATE_IDLE)
    {
        hal_bound = 0u;
    }
}

/** Publish controller, modulation, FSM, and protection observations to PLECS. */
static void publish_debug_isr(void)
{
    cllc_ctrl_debug_t debug = {0}; /* Coherent controller debug snapshot. */

    cllc_ctrl_get_debug(&debug);
    plecs_set_output(PLECS_OUTPUT_FSM_STATE, (float)cllc_fsm_get_run_state());
    plecs_set_output(PLECS_OUTPUT_ACTIVE_DIRECTION, (float)debug.direction);
    plecs_set_output(PLECS_OUTPUT_PI_REFERENCE, debug.ref);
    plecs_set_output(PLECS_OUTPUT_PI_FEEDBACK, debug.fbk);
    plecs_set_output(PLECS_OUTPUT_PI_OUTPUT, debug.output);
    plecs_set_output(PLECS_OUTPUT_CURRENT_REFERENCE, debug.current_ref_a);
    plecs_set_output(PLECS_OUTPUT_CURRENT_FEEDBACK, debug.current_fbk_a);
    plecs_set_output(PLECS_OUTPUT_VOLTAGE_CANDIDATE, debug.voltage_candidate);
    plecs_set_output(PLECS_OUTPUT_CURRENT_CANDIDATE, debug.current_candidate);
    plecs_set_output(PLECS_OUTPUT_CURRENT_LIMIT_ACTIVE, (float)debug.current_limit_active);
    plecs_set_output(PLECS_OUTPUT_DIRECTION_MISMATCH, (float)debug.direction_mismatch);
    plecs_set_output(PLECS_OUTPUT_FAULT_LATCH, (float)fault_latched);
    plecs_set_output(PLECS_OUTPUT_DBG, debug.pr_output);
}

REG_INTERRUPT(8, publish_debug_isr)

/** Initialize PLECS-side outputs before the first scheduler and interrupt pass. */
static void app_init(void)
{
    sample_feedback();
    cllc_pwm_disable();
    publish_debug_isr();
}

REG_INIT(0, app_init)

/** Write one 1 kHz forward-control sample for 100 Hz ripple analysis. */
static void log_forward_control_trace(void)
{
    cllc_ctrl_debug_t debug = {0}; /* Coherent controller values sampled by the 1 ms application task. */

    if ((cllc_fsm_get_run_state() != CLLC_RUN_STATE_RUN) || /* Log only settled control operation. */
        (cllc_ctrl_get_direction() != CLLC_DIRECTION_FORWARD)) /* Exclude the reverse control law. */
    {
        trace_header_written = 0u;
        return;
    }

    cllc_ctrl_get_debug(&debug);
    if (trace_header_written == 0u)
    {
        PLECS_LOG("CLLC_TRACE_HEADER,v_bus_v,v_out_v,v_ref_v,i_out_a,i_ref_a,"
                  "u_final,u_pr,u_voltage,u_current,current_limit_active\n");
        trace_header_written = 1u;
    }
    PLECS_LOG("CLLC_TRACE,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%u\n",
              (double)bus_voltage_v,
              (double)battery_voltage_v,
              (double)debug.ref,
              (double)battery_current_a,
              (double)debug.current_ref_a,
              (double)debug.output,
              (double)debug.pr_output,
              (double)debug.voltage_candidate,
              (double)debug.current_candidate,
              (unsigned int)debug.current_limit_active);
}

/** Execute slow configuration, protection, and lifecycle orchestration. */
static void app_task(void)
{
    sample_feedback();
    bind_timing();
    publish_setpoint();
    process_fault_commands();
    process_run_command();
    log_forward_control_trace();
}

REG_TASK_MS(1, app_task)
