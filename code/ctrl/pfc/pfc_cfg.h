// SPDX-License-Identifier: MIT
/**
 * @file    pfc_cfg.h
 * @brief   pfc_cfg control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define PFC setpoint structures and configuration manager types
 *          - Expose APIs for staging, publishing, and reading PFC control references
 *          - Provide the configuration contract shared by PFC control, FSM, and HAL glue
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
#ifndef __PFC_CFG_H
#define __PFC_CFG_H

#include <stdint.h>

#define PFC_CFG_DEFAULT_VBUS_REF_V (400.0f)    /* unit:V */
#define PFC_CFG_DEFAULT_VBUS_SLEW_VPS (200.0f) /* unit:V/s */

typedef struct
{
    uint8_t run_allowed; /* 0: stop, 1: allow run (not PWM enable) */
    float vbus_ref_v;    /* DC bus voltage reference [V] */
    float vbus_slew_vps; /* vbus_ref slew rate [V/s] */
} pfc_ctrl_setpoint_t;

typedef struct
{
    pfc_ctrl_setpoint_t *p_data;
    unsigned int version;
} pfc_ctrl_setpoint_buf_t;

typedef struct
{
    pfc_ctrl_setpoint_buf_t active;
    pfc_ctrl_setpoint_buf_t building;
} pfc_ctrl_setpoint_mgr_t;

void pfc_cfg_set_p_building(pfc_ctrl_setpoint_t *p_data);

pfc_ctrl_setpoint_t *pfc_cfg_get_p_active(void);
pfc_ctrl_setpoint_t *pfc_cfg_get_p_building(void);

void pfc_cfg_set_vbus_ref_v(float vbus_ref_v);
void pfc_cfg_set_vbus_slew_vps(float vbus_slew_vps);
void pfc_cfg_set_run_allowed(uint8_t run_allowed);

void pfc_cfg_publish_building(void);
void pfc_cfg_building_version_inc(void);

uint8_t pfc_cfg_is_ready(void);
void pfc_cfg_sync_building_to_active(void);

const pfc_ctrl_setpoint_mgr_t *pfc_cfg_get_mgr(void);

#endif
