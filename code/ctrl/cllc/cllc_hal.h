// SPDX-License-Identifier: MIT
/**
 * @file    cllc_hal.h
 * @brief   Bidirectional CLLC HAL binding public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Bind battery/bus measurements to the hardware-independent controller
 *          - Bind direction-aware modulation and bridge-enable callbacks
 *          - Own binding locks and the hard-protection latch contract
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Bindings are changed only while the FSM is idle
 *          - Fast shutdown remains available from protection context
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
#ifndef __CLLC_HAL_H
#define __CLLC_HAL_H

#include "cllc_cfg.h"

#include <stdint.h>

/** Direction-aware normalized modulation update callback. */
typedef void (*cllc_modulation_setter_t)(CLLC_DIRECTION_E direction, float normalized_command);

/** Direction-aware PWM bridge-enable callback. */
typedef void (*cllc_pwm_enable_t)(CLLC_DIRECTION_E direction);

/** Fast-loop measurement and actuator bindings. */
typedef struct
{
    float *p_v_battery;                         /* Low-voltage battery-port sample in volts. */
    float *p_i_battery;                         /* Battery/load current magnitude in amperes. */
    float *p_v_bus;                             /* High-voltage bus sample in volts. */
    cllc_modulation_setter_t p_set_modulation;  /* Normalized hybrid-modulation update hook. */
    cllc_pwm_enable_t p_pwm_enable;             /* Direction-aware bridge startup hook. */
    void (*p_pwm_disable)(void);                /* Immediate all-bridge shutdown hook. */
} cllc_ctrl_hal_t;

/** FSM callbacks and shared protection latch. */
typedef struct
{
    void (*p_enter_run)(CLLC_DIRECTION_E direction); /* Prepare and enable the selected direction. */
    void (*p_exit_run)(void);                         /* Disable power transfer and clear run permission. */
    uint8_t *p_latched;                               /* Nonzero blocks start and requests fault state. */
} cllc_fsm_hal_t;

/** @brief Get fast-loop HAL bindings. @return Mutable internal HAL object. */
cllc_ctrl_hal_t *cllc_hal_get_ctrl(void);
/** @brief Get FSM HAL bindings. @return Mutable internal FSM HAL object. */
cllc_fsm_hal_t *cllc_hal_get_fsm(void);
/** @brief Validate all required bindings. @return 1 when ready, otherwise 0. */
uint8_t cllc_hal_is_ready(void);
/** @brief Reject subsequent setter calls until idle unlocks binding. */
void cllc_hal_lock_binding(void);
/** @brief Permit platform binding while the FSM is idle. */
void cllc_hal_unlock_binding(void);
/** @brief Immediately invoke the platform all-bridge disable hook. */
void cllc_hal_pwm_disable(void);
/** @brief Disable PWM, latch protection, and revoke run permission. */
void cllc_hal_hard_protect_trip(void);
/** @brief Clear the currently bound protection latch. */
void cllc_hal_hard_protect_clear(void);
/** @brief Bind battery voltage feedback. @param p_value Live voltage sample pointer. */
void cllc_hal_set_v_battery_ptr(float *p_value);
/** @brief Bind battery current feedback. @param p_value Live current sample pointer. */
void cllc_hal_set_i_battery_ptr(float *p_value);
/** @brief Bind bus voltage feedback. @param p_value Live voltage sample pointer. */
void cllc_hal_set_v_bus_ptr(float *p_value);
/** @brief Bind normalized modulation output. @param p_setter Direction-aware setter callback. */
void cllc_hal_set_modulation_setter(cllc_modulation_setter_t p_setter);
/** @brief Bind bridge startup. @param p_enable Direction-aware enable callback. */
void cllc_hal_set_pwm_enable(cllc_pwm_enable_t p_enable);
/** @brief Bind immediate bridge shutdown. @param p_disable All-bridge disable callback. */
void cllc_hal_set_pwm_disable(void (*p_disable)(void));
/** @brief Bind external protection latch. @param p_latched Live normalized latch pointer. */
void cllc_hal_set_latched_ptr(uint8_t *p_latched);

#endif /* __CLLC_HAL_H */
