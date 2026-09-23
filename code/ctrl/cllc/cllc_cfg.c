// SPDX-License-Identifier: MIT
/**
 * @file    cllc_cfg.c
 * @brief   Bidirectional CLLC control configuration module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Own the default active/building setpoint buffers
 *          - Validate and stage direction, voltage, and current references
 *          - Publish coherent configuration snapshots to the control side
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Setters are update-side APIs and are not called by the fast ISR
 *          - References are clamped to the rated hardware range
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
#include "cllc_cfg.h"

#include <stddef.h>

static cllc_ctrl_setpoint_t setpoint_active = {0}; /* Stable control-side setpoint snapshot. */
static cllc_ctrl_setpoint_t setpoint_building = {  /* Application-side staged setpoint. */
    .run_allowed = 0u,
    .direction = CLLC_DIRECTION_FORWARD,
    .battery_voltage_ref_v = CLLC_HW_BATTERY_VOLTAGE_NOMINAL_V,
    .battery_current_limit_a = CLLC_HW_FORWARD_CURRENT_LIMIT_A,
    .bus_voltage_ref_v = CLLC_HW_BUS_VOLTAGE_NOMINAL_V,
};
static cllc_ctrl_timing_t ctrl_timing = {0}; /* Runtime control and FSM periods. */
static uint8_t direction_locked = 1u;         /* Nonzero rejects direction updates outside the idle state. */

cllc_ctrl_setpoint_mgr_t g_cllc_cfg_setpoint_mgr = { /* Public manager used by fast inline synchronization. */
    .active = {
        .p_data = &setpoint_active,
        .version = 0u,
    },
    .building = {
        .p_data = &setpoint_building,
        .version = 0u,
    },
};

/** Clamp one floating-point value to an inclusive range. */
static float clamp_float(float value, float lower, float upper)
{
    if (value > upper)
    {
        return upper;
    }
    if (value < lower)
    {
        return lower;
    }
    return value;
}

/** Validate timing before enabling the controller or FSM. */
static uint8_t timing_is_valid(const cllc_ctrl_timing_t *p_timing)
{
    if (p_timing == NULL)
    {
        return 0u;
    }
    if (p_timing->ctrl_ts <= 0.0f)
    {
        return 0u;
    }
    if (p_timing->task_ts <= 0.0f)
    {
        return 0u;
    }
    return 1u;
}

void cllc_cfg_set_timing(const cllc_ctrl_timing_t *p_timing)
{
    if (timing_is_valid(p_timing) == 0u)
    {
        ctrl_timing = (cllc_ctrl_timing_t){0};
        return;
    }
    ctrl_timing = *p_timing;
}

const cllc_ctrl_timing_t *cllc_cfg_get_timing(void)
{
    return &ctrl_timing;
}

float cllc_cfg_get_ctrl_ts(void)
{
    return ctrl_timing.ctrl_ts;
}

float cllc_cfg_get_task_ts(void)
{
    return ctrl_timing.task_ts;
}

uint32_t cllc_cfg_get_startup_delay_ticks(void)
{
    if (ctrl_timing.startup_delay_ticks != 0u)
    {
        return ctrl_timing.startup_delay_ticks;
    }
    if (ctrl_timing.task_ts <= 0.0f)
    {
        return 0u;
    }
    return (uint32_t)((CLLC_CTRL_STARTUP_DELAY_S / ctrl_timing.task_ts) + 0.5f);
}

void cllc_cfg_set_p_building(cllc_ctrl_setpoint_t *p_data)
{
    if (p_data != NULL)
    {
        g_cllc_cfg_setpoint_mgr.building.p_data = p_data;
    }
}

cllc_ctrl_setpoint_t *cllc_cfg_get_p_active(void)
{
    return g_cllc_cfg_setpoint_mgr.active.p_data;
}

cllc_ctrl_setpoint_t *cllc_cfg_get_p_building(void)
{
    return g_cllc_cfg_setpoint_mgr.building.p_data;
}

void cllc_cfg_set_run_allowed(uint8_t run_allowed)
{
    cllc_ctrl_setpoint_t *p_setpoint = g_cllc_cfg_setpoint_mgr.building.p_data; /* Staged setpoint target. */

    if (p_setpoint != NULL)
    {
        p_setpoint->run_allowed = (run_allowed == 0u) ? (uint8_t)0u : (uint8_t)1u;
    }
}

void cllc_cfg_set_direction(CLLC_DIRECTION_E direction)
{
    cllc_ctrl_setpoint_t *p_setpoint = g_cllc_cfg_setpoint_mgr.building.p_data; /* Staged setpoint target. */

    if ((direction_locked == 0u) && /* Direction changes are accepted only while the FSM is idle. */
        (p_setpoint != NULL) && /* A valid staging buffer is bound. */
        (direction >= CLLC_DIRECTION_FORWARD) && /* Reject negative enum values from external casts. */
        (direction < CLLC_DIRECTION_MAX)) /* Only defined power-flow directions may be staged. */
    {
        p_setpoint->direction = direction;
    }
}

void cllc_cfg_lock_direction(void)
{
    direction_locked = 1u;
}

void cllc_cfg_unlock_direction(void)
{
    direction_locked = 0u;
}

void cllc_cfg_set_battery_voltage_ref(float voltage_v)
{
    cllc_ctrl_setpoint_t *p_setpoint = g_cllc_cfg_setpoint_mgr.building.p_data; /* Staged setpoint target. */

    if (p_setpoint != NULL)
    {
        p_setpoint->battery_voltage_ref_v = clamp_float(
            voltage_v,
            CLLC_HW_BATTERY_VOLTAGE_MIN_V,
            CLLC_HW_BATTERY_VOLTAGE_MAX_V);
    }
}

void cllc_cfg_set_battery_current_limit(float current_a)
{
    cllc_ctrl_setpoint_t *p_setpoint = g_cllc_cfg_setpoint_mgr.building.p_data; /* Staged setpoint target. */

    if (p_setpoint != NULL)
    {
        p_setpoint->battery_current_limit_a = clamp_float(
            current_a,
            0.0f,
            CLLC_HW_FORWARD_CURRENT_LIMIT_A);
    }
}

void cllc_cfg_set_bus_voltage_ref(float voltage_v)
{
    cllc_ctrl_setpoint_t *p_setpoint = g_cllc_cfg_setpoint_mgr.building.p_data; /* Staged setpoint target. */

    if (p_setpoint != NULL)
    {
        p_setpoint->bus_voltage_ref_v = clamp_float(
            voltage_v,
            CLLC_HW_BUS_VOLTAGE_MIN_V,
            CLLC_HW_BUS_VOLTAGE_MAX_V);
    }
}

void cllc_cfg_publish_building(void)
{
    if (g_cllc_cfg_setpoint_mgr.building.p_data != NULL)
    {
        g_cllc_cfg_setpoint_mgr.building.version++;
    }
}

uint8_t cllc_cfg_is_ready(void)
{
    if (g_cllc_cfg_setpoint_mgr.active.p_data == NULL)
    {
        return 0u;
    }
    if (g_cllc_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return 0u;
    }
    return timing_is_valid(&ctrl_timing);
}

const cllc_ctrl_setpoint_mgr_t *cllc_cfg_get_mgr(void)
{
    return &g_cllc_cfg_setpoint_mgr;
}
