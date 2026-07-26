// SPDX-License-Identifier: MIT
/**
 * @file    cllc_ctrl.c
 * @brief   Bidirectional CLLC controller module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Run forward voltage/current shared-integral PI competition
 *          - Add a 100 Hz output-voltage ripple PR contribution in forward operation
 *          - Run reverse single bus-voltage PI control
 *          - Slew references and send one direction-tagged normalized command
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The ISR uses only pre-bound pointers and allocation-free algorithms
 *          - Direction is immutable during one run and changes only through the FSM
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
#include "cllc_ctrl.h"

#include "cllc_hal.h"
#include "pi_dual_compete.h"
#include "pi_tustin.h"
#include "pr.h"
#include "section.h"

#include <stdbool.h>
#include <stddef.h>

static pi_dual_compete_t forward_compete = {0};                    /* Forward voltage/current competition state. */
static pr_t forward_voltage_pr = {0};                              /* Forward 100 Hz voltage-ripple PR state. */
static pi_tustin_t reverse_voltage_loop = {0};                     /* Reverse bus-voltage PI state. */
static cllc_ctrl_hal_t *p_ctrl_hal = NULL;                         /* HAL object sampled in the ISR. */
static cllc_ctrl_setpoint_t safe_setpoint = {0};                   /* Safe fallback when configuration is detached. */
static cllc_ctrl_setpoint_t *p_active_setpoint = &safe_setpoint;   /* Current active setpoint snapshot. */
static CLLC_DIRECTION_E active_direction = CLLC_DIRECTION_FORWARD; /* Direction latched by the FSM. */
static float battery_voltage_ref_v = 0.0f;                         /* Slewed forward voltage reference. */
static float battery_current_ref_a = 0.0f;                         /* Forward current-limit reference. */
static float bus_voltage_ref_v = 0.0f;                             /* Slewed reverse bus reference. */
static float battery_voltage_fbk_v = 0.0f;                         /* Latest low-voltage-port feedback. */
static float battery_current_fbk_a = 0.0f;                         /* Latest battery/load-current magnitude. */
static float bus_voltage_fbk_v = 0.0f;                             /* Latest high-voltage-bus feedback. */
static float forward_pr_ref_v = 0.0f;                              /* Zero reference for 100 Hz ripple suppression. */
static float forward_pr_fbk_v = 0.0f;                              /* Output-voltage deviation supplied to the PR. */
static float forward_pr_output = 0.0f;                             /* Bipolar PR correction in the -0.5...0.5 range. */
static float normalized_output = 0.0f;                             /* Command sent to the modulation HAL. */
static uint8_t controller_ready = 0u;                              /* Both PI instances initialized successfully. */
static uint8_t run_active = 0u;                                    /* ISR has observed run permission. */
static uint8_t direction_mismatch = 0u;                            /* Live setpoint differs from latched direction. */

/** Move a reference toward its target without overshoot. */
static float slew_to(float value, float target, float maximum_step)
{
    float difference = target - value; /* Remaining reference change. */

    if (difference > maximum_step)
    {
        return value + maximum_step;
    }
    if (difference < -maximum_step)
    {
        return value - maximum_step;
    }
    return target;
}

/** Clamp the summed forward modulation command to its physical normalized range. */
static float clamp_normalized(float value)
{
    if (value > CLLC_CTRL_OUTPUT_UP_LIMIT)
    {
        return CLLC_CTRL_OUTPUT_UP_LIMIT;
    }
    if (value < CLLC_CTRL_OUTPUT_DN_LIMIT)
    {
        return CLLC_CTRL_OUTPUT_DN_LIMIT;
    }
    return value;
}

/** Check all data and callback bindings required by the fast path. */
static uint8_t control_bindings_ready(void)
{
    if (p_ctrl_hal == NULL)
    {
        return 0u;
    }
    if (p_active_setpoint == NULL)
    {
        return 0u;
    }
    if (p_ctrl_hal->p_v_battery == NULL)
    {
        return 0u;
    }
    if (p_ctrl_hal->p_i_battery == NULL)
    {
        return 0u;
    }
    if (p_ctrl_hal->p_v_bus == NULL)
    {
        return 0u;
    }
    if (p_ctrl_hal->p_set_modulation == NULL)
    {
        return 0u;
    }
    return (cllc_cfg_is_ready() != 0u) ? 1u : 0u;
}

/** Snapshot all analog values before either control set executes. */
static void sample_feedback(void)
{
    battery_voltage_fbk_v = *p_ctrl_hal->p_v_battery;
    battery_current_fbk_a = *p_ctrl_hal->p_i_battery;
    bus_voltage_fbk_v = *p_ctrl_hal->p_v_bus;
}

/** Reset algorithm memory and initialize coefficient sets from configuration macros. */
static void initialize_control_sets(void)
{
    float ctrl_ts = cllc_cfg_get_ctrl_ts(); /* Fast-loop period used by both discretizations. */
    bool forward_ok = false;                /* Forward shared-integral PI initialization result. */
    bool forward_pr_ok = false;             /* Forward 100 Hz PR initialization result. */
    bool reverse_ok = false;                /* Reverse Tustin PI initialization result. */

    forward_ok = pi_dual_compete_init(
        &forward_compete,
        CLLC_CTRL_FORWARD_VOLTAGE_KP,
        CLLC_CTRL_PI_KI_STEP(CLLC_CTRL_FORWARD_VOLTAGE_KI, ctrl_ts),
        CLLC_CTRL_FORWARD_CURRENT_KP,
        CLLC_CTRL_PI_KI_STEP(CLLC_CTRL_FORWARD_CURRENT_KI, ctrl_ts),
        CLLC_CTRL_OUTPUT_UP_LIMIT,
        CLLC_CTRL_OUTPUT_DN_LIMIT,
        PI_DUAL_COMPETE_MIN,
        &battery_voltage_ref_v,
        &battery_voltage_fbk_v,
        &battery_current_ref_a,
        &battery_current_fbk_a);

    forward_pr_ok = pr_init(
        &forward_voltage_pr,
        CLLC_CTRL_FORWARD_PR_KP,
        CLLC_CTRL_FORWARD_PR_KR,
        CLLC_CTRL_FORWARD_PR_W0_RAD_PER_S,
        CLLC_CTRL_FORWARD_PR_WC_RAD_PER_S,
        ctrl_ts,
        CLLC_CTRL_FORWARD_PR_UP_LIMIT,
        CLLC_CTRL_FORWARD_PR_DN_LIMIT,
        &forward_pr_ref_v,
        &forward_pr_fbk_v);

    reverse_ok = pi_tustin_init(
        &reverse_voltage_loop,
        CLLC_CTRL_REVERSE_VOLTAGE_KP,
        CLLC_CTRL_REVERSE_VOLTAGE_KI,
        ctrl_ts,
        CLLC_CTRL_OUTPUT_UP_LIMIT,
        CLLC_CTRL_OUTPUT_DN_LIMIT,
        &bus_voltage_ref_v,
        &bus_voltage_fbk_v);

    controller_ready = ((forward_ok == true) &&    /* Forward PI competition is valid. */
                        (forward_pr_ok == true) && /* Forward 100 Hz PR is valid. */
                        (reverse_ok == true))      /* Reverse PI is valid. */
                           ? 1u
                           : 0u;
}

void cllc_ctrl_prepare_run(CLLC_DIRECTION_E direction)
{
    cllc_ctrl_setpoint_t *p_configured_setpoint = NULL; /* Active setpoint after publish synchronization. */

    p_ctrl_hal = cllc_hal_get_ctrl();
    cllc_cfg_sync_building_to_active();
    p_configured_setpoint = cllc_cfg_get_p_active();
    p_active_setpoint = (p_configured_setpoint != NULL) ? p_configured_setpoint : &safe_setpoint;
    active_direction = ((direction >= CLLC_DIRECTION_FORWARD) && /* Reject negative enum values. */
                        (direction < CLLC_DIRECTION_MAX))        /* Accept only defined directions. */
                           ? direction
                           : CLLC_DIRECTION_FORWARD;
    normalized_output = 0.0f;
    forward_pr_ref_v = 0.0f;
    forward_pr_fbk_v = 0.0f;
    forward_pr_output = 0.0f;
    controller_ready = 0u;
    run_active = 0u;
    direction_mismatch = 0u;

    if (control_bindings_ready() == 0u)
    {
        return;
    }

    sample_feedback();
    battery_voltage_ref_v = battery_voltage_fbk_v;
    battery_current_ref_a = p_active_setpoint->battery_current_limit_a;
    bus_voltage_ref_v = bus_voltage_fbk_v;
    initialize_control_sets();
}

/** Force algorithm memory and the modulation output to the safe lower limit. */
static void force_safe_output(void)
{
    run_active = 0u;
    normalized_output = CLLC_CTRL_OUTPUT_DN_LIMIT;
    pi_dual_compete_reset(&forward_compete);
    pr_reset(&forward_voltage_pr);
    pi_tustin_reset(&reverse_voltage_loop);
    cllc_hal_pwm_disable();
}

/** Execute the forward dual-competition loop. */
static void run_forward(float ctrl_ts)
{
    float reference_step_v = CLLC_CTRL_FORWARD_REF_SLEW_V_PER_S * ctrl_ts; /* Per-ISR soft-start step. */

    battery_voltage_ref_v = slew_to(
        battery_voltage_ref_v,
        p_active_setpoint->battery_voltage_ref_v,
        reference_step_v);
    battery_current_ref_a = p_active_setpoint->battery_current_limit_a;

    if (pi_dual_compete_cal(&forward_compete) == false)
    {
        force_safe_output();
        return;
    }

    /* Remove the commanded DC level so the zero-reference PR sees only voltage deviation/ripple. */
    forward_pr_fbk_v = battery_voltage_fbk_v - battery_voltage_ref_v;
    if (pr_cal(&forward_voltage_pr) == false)
    {
        force_safe_output();
        return;
    }
    forward_pr_output = forward_voltage_pr.output.val;
    /* Superimpose the 100 Hz ripple compensation on the shared-integral PI command. */
    normalized_output = clamp_normalized(forward_compete.output.val + forward_pr_output);
}

/** Execute the reverse single bus-voltage loop. */
static void run_reverse(float ctrl_ts)
{
    float reference_step_v = CLLC_CTRL_REVERSE_REF_SLEW_V_PER_S * ctrl_ts; /* Per-ISR soft-start step. */

    forward_pr_output = 0.0f;
    bus_voltage_ref_v = slew_to(
        bus_voltage_ref_v,
        p_active_setpoint->bus_voltage_ref_v,
        reference_step_v);
    if (pi_tustin_cal(&reverse_voltage_loop) == false)
    {
        force_safe_output();
        return;
    }
    normalized_output = reverse_voltage_loop.output.val;
}

/** Fast control entry registered in the common interrupt dispatcher. */
static void cllc_ctrl_isr(void)
{
    float ctrl_ts = 0.0f; /* Configured fast-loop period. */

    if ((controller_ready == 0u) ||
        (control_bindings_ready() == 0u)) /* Run only after both PI sets and all HAL bindings are valid. */
    {
        return;
    }

    cllc_cfg_sync_building_to_active();
    sample_feedback();
    if (p_active_setpoint->run_allowed == 0u)
    {
        if (run_active != 0u)
        {
            force_safe_output();
        }
        return;
    }

    run_active = 1u;
    direction_mismatch = (p_active_setpoint->direction == active_direction) ? (uint8_t)0u : (uint8_t)1u;
    ctrl_ts = cllc_cfg_get_ctrl_ts();
    if (active_direction == CLLC_DIRECTION_FORWARD)
    {
        run_forward(ctrl_ts);
    }
    else
    {
        run_reverse(ctrl_ts);
    }

    if (run_active != 0u)
    {
        p_ctrl_hal->p_set_modulation(active_direction, normalized_output);
    }
}

REG_INTERRUPT(2, cllc_ctrl_isr)

uint8_t cllc_ctrl_get_enable(void)
{
    return run_active;
}

CLLC_DIRECTION_E cllc_ctrl_get_direction(void)
{
    return active_direction;
}

void cllc_ctrl_get_debug(cllc_ctrl_debug_t *p_debug)
{
    if (p_debug == NULL)
    {
        return;
    }

    p_debug->direction = active_direction;
    p_debug->current_ref_a = battery_current_ref_a;
    p_debug->current_fbk_a = battery_current_fbk_a;
    p_debug->output = normalized_output;
    p_debug->pr_output = forward_pr_output;
    p_debug->voltage_candidate = forward_compete.output.val_a;
    p_debug->current_candidate = forward_compete.output.val_b;
    p_debug->current_limit_active =
        ((active_direction == CLLC_DIRECTION_FORWARD) &&    /* Only forward mode owns the competing current loop. */
         (forward_compete.inter.active_ch == PI_DUAL_CH_B)) /* Channel B is the configured current-limit loop. */
            ? 1u
            : 0u;
    p_debug->direction_mismatch = direction_mismatch;

    if (active_direction == CLLC_DIRECTION_FORWARD)
    {
        p_debug->ref = battery_voltage_ref_v;
        p_debug->fbk = battery_voltage_fbk_v;
    }
    else
    {
        p_debug->ref = bus_voltage_ref_v;
        p_debug->fbk = bus_voltage_fbk_v;
    }
}
