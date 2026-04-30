// SPDX-License-Identifier: MIT
/**
 * @file    pfc_hal.h
 * @brief   pfc_hal control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare PFC HAL binding accessors and protection-control APIs
 *          - Expose binding lock, unlock, and readiness checks for platform integration
 *          - Provide the hardware abstraction bridge for PFC control and FSM modules
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __PFC_HAL_H
#define __PFC_HAL_H

#include <stdint.h>

typedef struct
{
    float *p_v_g;
    float *p_v_cap;
    float *p_i_l;
    float *p_v_bus;
    float *p_v_rms;
    uint8_t *p_main_rly_is_closed;
    void (*p_set_pwm_func)(float v_pwm, float v_bus);
    void (*p_pwm_enable)(void);
    void (*p_pwm_disable)(void);
} pfc_ctrl_hal_t;

/**
 * @brief Bus voltage status classification
 */
typedef enum
{
    PFC_VBUS_STA_BELOW_INPUT_PEAK,
    PFC_VBUS_STA_AT_INPUT_PEAK,
    PFC_VBUS_STA_IN_REGULATION,
} pfc_vbus_sta_e;

typedef struct
{
    pfc_vbus_sta_e *p_vbus_sta;
    uint8_t *p_main_rly_is_closed;
    void (*p_enter_run_func)(void);
    void (*p_exit_run_func)(void);
    void (*p_main_rly_on_func)(void);
    void (*p_main_rly_off_func)(void);
    uint8_t *p_latched;
} pfc_fsm_hal_t;

pfc_ctrl_hal_t *pfc_hal_get_ctrl(void);
pfc_fsm_hal_t *pfc_hal_get_fsm(void);
void pfc_hal_hard_protect_trip(void);
void pfc_hal_hard_protect_clear(void);
uint8_t pfc_hal_is_ready(void);
void pfc_hal_lock_binding(void);
void pfc_hal_unlock_binding(void);
void pfc_hal_set_v_g_ptr(float *p);
void pfc_hal_set_v_cap_ptr(float *p);
void pfc_hal_set_i_l_ptr(float *p);
void pfc_hal_set_v_bus_ptr(float *p);
void pfc_hal_set_v_rms_ptr(float *p);
void pfc_hal_set_main_rly_is_closed_ptr(uint8_t *p);
void pfc_hal_set_pwm_setter(void (*p)(float v_pwm, float v_bus));
void pfc_hal_set_pwm_enable(void (*p)(void));
void pfc_hal_set_pwm_disable(void (*p)(void));
void pfc_hal_set_vbus_sta_ptr(pfc_vbus_sta_e *p);
void pfc_hal_set_enter_run_func(void (*p)(void));
void pfc_hal_set_exit_run_func(void (*p)(void));
void pfc_hal_set_main_rly_on_func(void (*p)(void));
void pfc_hal_set_main_rly_off_func(void (*p)(void));
void pfc_hal_set_latched_ptr(uint8_t *p);

#endif
