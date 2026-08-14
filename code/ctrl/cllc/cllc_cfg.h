// SPDX-License-Identifier: MIT
/**
 * @file    cllc_cfg.h
 * @brief   Bidirectional CLLC control configuration public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define forward and reverse operating setpoints
 *          - Derive all forward and reverse PI gains from named plant macros
 *          - Expose timing and active/building setpoint-buffer APIs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Building data is published by version and copied by the control side
 *          - Direction is selected before start and latched by the FSM
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
#ifndef __CLLC_CFG_H
#define __CLLC_CFG_H

#include "cllc_hw_param.h"
#include "my_math.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

/** CLLC energy-flow direction selected while the FSM is idle. */
typedef enum
{
    CLLC_DIRECTION_FORWARD = 0, /* High-voltage bus charges or supplies the battery port. */
    CLLC_DIRECTION_REVERSE = 1, /* Battery port discharges into the high-voltage bus. */
    CLLC_DIRECTION_MAX
} CLLC_DIRECTION_E;

/** Control and FSM scheduling periods. */
typedef struct
{
    float ctrl_ts;                /* Fast-loop sampling period in seconds. */
    float task_ts;                /* FSM task period in seconds. */
    uint32_t startup_delay_ticks; /* Optional bridge-settling delay in FSM ticks. */
} cllc_ctrl_timing_t;

/** Setpoints staged by the application and consumed by the control ISR. */
typedef struct
{
    uint8_t run_allowed;           /* Internal run gate controlled by the HAL/FSM. */
    CLLC_DIRECTION_E direction;    /* Requested direction, sampled only on start. */
    float battery_voltage_ref_v;   /* Forward low-voltage-port reference, 24...72 V. */
    float battery_current_limit_a; /* Forward load-current limit in amperes. */
    float bus_voltage_ref_v;       /* Reverse high-voltage-bus reference, 400...500 V. */
} cllc_ctrl_setpoint_t;

/** One side of the active/building setpoint manager. */
typedef struct
{
    cllc_ctrl_setpoint_t *p_data; /* Caller-visible payload buffer. */
    uint32_t version;             /* Publish counter used for coherent ISR updates. */
} cllc_ctrl_setpoint_buf_t;

/** Versioned double buffer shared by the application and control side. */
typedef struct
{
    cllc_ctrl_setpoint_buf_t active;   /* Stable snapshot consumed by control. */
    cllc_ctrl_setpoint_buf_t building; /* Staging buffer written by the application. */
} cllc_ctrl_setpoint_mgr_t;

/* Generic first-order-plant PI design formulas used by both directions. */
#define CLLC_CTRL_LOAD_RESISTANCE_OHM(voltage_v, power_w) \
    (((voltage_v) * (voltage_v)) / (power_w))
#define CLLC_CTRL_OUTPUT_TIME_CONSTANT_S(load_ohm, capacitance_f) \
    (0.5f * (load_ohm) * (capacitance_f))
#define CLLC_CTRL_OUTPUT_POLE_HZ(time_constant_s) \
    (1.0f / (M_2PI * (time_constant_s)))
#define CLLC_CTRL_PI_KP_FIRST_ORDER(control_gain, time_constant_s, crossover_hz, zero_hz) \
    (sqrtf(1.0f + ((M_2PI * (crossover_hz) * (time_constant_s)) *                         \
                   (M_2PI * (crossover_hz) * (time_constant_s)))) /                       \
     ((control_gain) * sqrtf(1.0f + (((zero_hz) / (crossover_hz)) *                       \
                                     ((zero_hz) / (crossover_hz))))))
#define CLLC_CTRL_PI_KI_FROM_ZERO(kp, zero_hz) \
    (M_2PI * (zero_hz) * (kp))
#define CLLC_CTRL_PI_KI_STEP(ki, sample_time_s) \
    ((ki) * (sample_time_s))

/* Forward design point: 48 V, 6.6 kW, 10 mF, voltage/current dual competition. */
#define CLLC_CTRL_FORWARD_DESIGN_VOLTAGE_V (48.0f)
#define CLLC_CTRL_FORWARD_DESIGN_POWER_W (6600.0f)
#define CLLC_CTRL_FORWARD_CONTROL_GAIN_V_PER_U (98.26f)
#define CLLC_CTRL_FORWARD_VOLTAGE_CROSSOVER_HZ (350.0f)
#define CLLC_CTRL_FORWARD_CURRENT_CROSSOVER_HZ (3500.0f)
#define CLLC_CTRL_FORWARD_LOAD_OHM                                    \
    CLLC_CTRL_LOAD_RESISTANCE_OHM(CLLC_CTRL_FORWARD_DESIGN_VOLTAGE_V, \
                                  CLLC_CTRL_FORWARD_DESIGN_POWER_W)
#define CLLC_CTRL_FORWARD_OUTPUT_TAU_S                           \
    CLLC_CTRL_OUTPUT_TIME_CONSTANT_S(CLLC_CTRL_FORWARD_LOAD_OHM, \
                                     CLLC_HW_BATTERY_OUTPUT_CAP_F)
#define CLLC_CTRL_FORWARD_ZERO_HZ \
    CLLC_CTRL_OUTPUT_POLE_HZ(CLLC_CTRL_FORWARD_OUTPUT_TAU_S)
#define CLLC_CTRL_FORWARD_CURRENT_GAIN_A_PER_U \
    (CLLC_CTRL_FORWARD_CONTROL_GAIN_V_PER_U / CLLC_CTRL_FORWARD_LOAD_OHM)
#define CLLC_CTRL_FORWARD_VOLTAGE_KP                                    \
    CLLC_CTRL_PI_KP_FIRST_ORDER(CLLC_CTRL_FORWARD_CONTROL_GAIN_V_PER_U, \
                                CLLC_CTRL_FORWARD_OUTPUT_TAU_S,         \
                                CLLC_CTRL_FORWARD_VOLTAGE_CROSSOVER_HZ, \
                                CLLC_CTRL_FORWARD_ZERO_HZ)
#define CLLC_CTRL_FORWARD_VOLTAGE_KI                        \
    CLLC_CTRL_PI_KI_FROM_ZERO(CLLC_CTRL_FORWARD_VOLTAGE_KP, \
                              CLLC_CTRL_FORWARD_ZERO_HZ)
#define CLLC_CTRL_FORWARD_CURRENT_KP                                    \
    CLLC_CTRL_PI_KP_FIRST_ORDER(CLLC_CTRL_FORWARD_CURRENT_GAIN_A_PER_U, \
                                CLLC_CTRL_FORWARD_OUTPUT_TAU_S,         \
                                CLLC_CTRL_FORWARD_CURRENT_CROSSOVER_HZ, \
                                CLLC_CTRL_FORWARD_ZERO_HZ)
#define CLLC_CTRL_FORWARD_CURRENT_KI                        \
    CLLC_CTRL_PI_KI_FROM_ZERO(CLLC_CTRL_FORWARD_CURRENT_KP, \
                              CLLC_CTRL_FORWARD_ZERO_HZ)

/* Forward 100 Hz output-voltage ripple PR design. */
#define CLLC_CTRL_FORWARD_PR_FREQ_HZ (100.0f)
#define CLLC_CTRL_FORWARD_PR_BANDWIDTH_HZ (5.0f)
#define CLLC_CTRL_FORWARD_PR_TARGET_LOOP_GAIN (10.5f)
#define CLLC_CTRL_FORWARD_PR_W0_RAD_PER_S \
    (M_2PI * CLLC_CTRL_FORWARD_PR_FREQ_HZ)
#define CLLC_CTRL_FORWARD_PR_WC_RAD_PER_S \
    (M_2PI * CLLC_CTRL_FORWARD_PR_BANDWIDTH_HZ)
#define CLLC_CTRL_FORWARD_PR_PLANT_MAG_V_PER_U                                     \
    (CLLC_CTRL_FORWARD_CONTROL_GAIN_V_PER_U /                                      \
     sqrtf(1.0f +                                                                  \
           ((CLLC_CTRL_FORWARD_PR_W0_RAD_PER_S * CLLC_CTRL_FORWARD_OUTPUT_TAU_S) * \
            (CLLC_CTRL_FORWARD_PR_W0_RAD_PER_S * CLLC_CTRL_FORWARD_OUTPUT_TAU_S))))
#define CLLC_CTRL_FORWARD_PR_KP (0.0f)
#define CLLC_CTRL_FORWARD_PR_KR \
    (CLLC_CTRL_FORWARD_PR_TARGET_LOOP_GAIN / CLLC_CTRL_FORWARD_PR_PLANT_MAG_V_PER_U)
#define CLLC_CTRL_FORWARD_PR_UP_LIMIT (0.5f)
#define CLLC_CTRL_FORWARD_PR_DN_LIMIT (-0.5f)

/* Reverse design point: 48 V to 450 V at 3 kW, 1360 uF, one bus-voltage PI. */
#define CLLC_CTRL_REVERSE_DESIGN_VOLTAGE_V (450.0f)
#define CLLC_CTRL_REVERSE_DESIGN_POWER_W (3000.0f)
#define CLLC_CTRL_REVERSE_CONTROL_GAIN_V_PER_U (2372.89f)
#define CLLC_CTRL_REVERSE_VOLTAGE_CROSSOVER_HZ (200.0f)
#define CLLC_CTRL_REVERSE_LOAD_OHM                                    \
    CLLC_CTRL_LOAD_RESISTANCE_OHM(CLLC_CTRL_REVERSE_DESIGN_VOLTAGE_V, \
                                  CLLC_CTRL_REVERSE_DESIGN_POWER_W)
#define CLLC_CTRL_REVERSE_OUTPUT_TAU_S                           \
    CLLC_CTRL_OUTPUT_TIME_CONSTANT_S(CLLC_CTRL_REVERSE_LOAD_OHM, \
                                     CLLC_HW_BUS_OUTPUT_CAP_F)
#define CLLC_CTRL_REVERSE_ZERO_HZ \
    CLLC_CTRL_OUTPUT_POLE_HZ(CLLC_CTRL_REVERSE_OUTPUT_TAU_S)
#define CLLC_CTRL_REVERSE_VOLTAGE_KP                                    \
    CLLC_CTRL_PI_KP_FIRST_ORDER(CLLC_CTRL_REVERSE_CONTROL_GAIN_V_PER_U, \
                                CLLC_CTRL_REVERSE_OUTPUT_TAU_S,         \
                                CLLC_CTRL_REVERSE_VOLTAGE_CROSSOVER_HZ, \
                                CLLC_CTRL_REVERSE_ZERO_HZ)
#define CLLC_CTRL_REVERSE_VOLTAGE_KI                        \
    CLLC_CTRL_PI_KI_FROM_ZERO(CLLC_CTRL_REVERSE_VOLTAGE_KP, \
                              CLLC_CTRL_REVERSE_ZERO_HZ)

/* Common output and soft-reference limits. */
#define CLLC_CTRL_OUTPUT_UP_LIMIT (1.0f)
#define CLLC_CTRL_OUTPUT_DN_LIMIT (0.0f)
#define CLLC_CTRL_FORWARD_REF_SLEW_V_PER_S (1000.0f)
#define CLLC_CTRL_REVERSE_REF_SLEW_V_PER_S (2000.0f)
#define CLLC_CTRL_STARTUP_DELAY_S (1.0e-3f)

extern cllc_ctrl_setpoint_mgr_t g_cllc_cfg_setpoint_mgr;

/** @brief Set control/FSM timing. @param p_timing Valid caller-owned timing values. */
void cllc_cfg_set_timing(const cllc_ctrl_timing_t *p_timing);
/** @brief Get configured timing. @return Read-only timing object. */
const cllc_ctrl_timing_t *cllc_cfg_get_timing(void);
/** @brief Get fast-loop period. @return Control period in seconds. */
float cllc_cfg_get_ctrl_ts(void);
/** @brief Get FSM task period. @return Task period in seconds. */
float cllc_cfg_get_task_ts(void);
/** @brief Get startup delay. @return Delay in FSM ticks. */
uint32_t cllc_cfg_get_startup_delay_ticks(void);
/** @brief Replace the staging buffer. @param p_data Caller-owned building buffer. */
void cllc_cfg_set_p_building(cllc_ctrl_setpoint_t *p_data);
/** @brief Get the active setpoint. @return Active buffer pointer or NULL. */
cllc_ctrl_setpoint_t *cllc_cfg_get_p_active(void);
/** @brief Get the staging setpoint. @return Building buffer pointer or NULL. */
cllc_ctrl_setpoint_t *cllc_cfg_get_p_building(void);
/** @brief Stage the next run direction while the FSM is idle. @param direction Forward or reverse. */
void cllc_cfg_set_direction(CLLC_DIRECTION_E direction);
/** @brief Reject subsequent direction updates until the FSM returns to idle. */
void cllc_cfg_lock_direction(void);
/** @brief Permit direction updates while the FSM remains idle. */
void cllc_cfg_unlock_direction(void);
/** @brief Stage forward voltage reference. @param voltage_v Battery-port reference in volts. */
void cllc_cfg_set_battery_voltage_ref(float voltage_v);
/** @brief Stage forward current limit. @param current_a Battery/load-current limit in amperes. */
void cllc_cfg_set_battery_current_limit(float current_a);
/** @brief Stage reverse bus reference. @param voltage_v High-voltage-bus reference in volts. */
void cllc_cfg_set_bus_voltage_ref(float voltage_v);
/** @brief Check timing and buffer bindings. @return 1 when ready, otherwise 0. */
uint8_t cllc_cfg_is_ready(void);
/** @brief Get the setpoint manager. @return Read-only active/building manager. */
const cllc_ctrl_setpoint_mgr_t *cllc_cfg_get_mgr(void);

/** Copy a newly published building setpoint into the active ISR snapshot. */
static inline void cllc_cfg_sync_building_to_active(void)
{
    if (g_cllc_cfg_setpoint_mgr.building.p_data == NULL)
    {
        return;
    }
    if (g_cllc_cfg_setpoint_mgr.active.p_data == NULL)
    {
        return;
    }
    if (g_cllc_cfg_setpoint_mgr.active.version != g_cllc_cfg_setpoint_mgr.building.version)
    {
        *g_cllc_cfg_setpoint_mgr.active.p_data = *g_cllc_cfg_setpoint_mgr.building.p_data;
        g_cllc_cfg_setpoint_mgr.active.version = g_cllc_cfg_setpoint_mgr.building.version;
    }
}

#endif /* __CLLC_CFG_H */
