// SPDX-License-Identifier: MIT
/**
 * @file    bb_cfg.h
 * @brief   bb_cfg control public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare buck-boost setpoint data structures and manager handles
 *          - Expose APIs for staging, publishing, and reading buck-boost control references
 *          - Define the configuration contract used by the controller, FSM, and HAL glue
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
#ifndef __BB_CFG_H
#define __BB_CFG_H

#include <stdint.h>

typedef struct
{
    uint8_t run_allowed; /* run_allowed: 1 enables control execution, 0 forces stop */
    float pwr_lmt;       /* pwr_lmt: input power limit used to derive input current limit */
    float out_volt_ref;  /* out_volt_ref: output voltage reference for the outer loop */
    float in_volt_lmt;   /* in_volt_lmt: input-voltage protection threshold */
    float in_curr_lmt;   /* in_curr_lmt: configured input-current ceiling */
    float out_curr_lmt;  /* out_curr_lmt: configured output-current ceiling */
} bb_ctrl_setpoint_t;

typedef struct
{
    bb_ctrl_setpoint_t *p_data; /* p_data: payload buffer pointer */
    unsigned int version;       /* version: monotonically increasing publish counter */
} bb_ctrl_setpoint_buf_t;

typedef struct
{
    bb_ctrl_setpoint_buf_t active;   /* active: control-side snapshot consumed by ISR/task */
    bb_ctrl_setpoint_buf_t building; /* building: update-side scratch buffer */
} bb_ctrl_setpoint_mgr_t;

/**
 * @brief Replace the default building-buffer pointer used for staged updates.
 * @param p_data Pointer to an externally supplied building buffer.
 * @return None.
 */
void bb_cfg_set_p_building(bb_ctrl_setpoint_t *p_data);

/**
 * @brief Get the active setpoint buffer consumed by control logic.
 * @param None.
 * @return Pointer to the active setpoint buffer, or NULL if detached.
 */
bb_ctrl_setpoint_t *bb_cfg_get_p_active(void);

/**
 * @brief Get the building setpoint buffer updated by upper layers.
 * @param None.
 * @return Pointer to the building setpoint buffer, or NULL if detached.
 */
bb_ctrl_setpoint_t *bb_cfg_get_p_building(void);

/**
 * @brief Update the staged run-allow flag in the building buffer.
 * @param run_allowed 1 enables run, 0 disables run.
 * @return None.
 */
void bb_cfg_set_run_allowed(uint8_t run_allowed);

/**
 * @brief Update the staged input-power limit in the building buffer.
 * @param pwr_lmt Input power limit in watts.
 * @return None.
 */
void bb_cfg_set_pwr_lmt(float pwr_lmt);

/**
 * @brief Update the staged output-voltage reference in the building buffer.
 * @param out_volt_ref Output-voltage reference.
 * @return None.
 */
void bb_cfg_set_out_volt_ref(float out_volt_ref);

/**
 * @brief Update the staged input-voltage limit in the building buffer.
 * @param in_volt_lmt Input-voltage limit.
 * @return None.
 */
void bb_cfg_set_in_volt_lmt(float in_volt_lmt);

/**
 * @brief Update the staged input-current limit in the building buffer.
 * @param in_curr_lmt Input-current limit.
 * @return None.
 */
void bb_cfg_set_in_curr_lmt(float in_curr_lmt);

/**
 * @brief Update the staged output-current limit in the building buffer.
 * @param out_curr_lmt Output-current limit.
 * @return None.
 */
void bb_cfg_set_out_curr_lmt(float out_curr_lmt);

/**
 * @brief Publish the building buffer into the active buffer immediately.
 * @param None.
 * @return None. Active version is synchronized to the new building version.
 */
void bb_cfg_publish_building(void);

/**
 * @brief Increment the building-buffer version without copying data to active.
 * @param None.
 * @return None.
 */
void bb_cfg_building_version_inc(void);

/**
 * @brief Check whether both active and building buffers are valid.
 * @param None.
 * @return 1 when both buffers are available, otherwise 0.
 */
uint8_t bb_cfg_is_ready(void);

/**
 * @brief Synchronize the active buffer to the building buffer when versions differ.
 * @param None.
 * @return None.
 */
void bb_cfg_sync_building_to_active(void);

/**
 * @brief Get the setpoint manager that owns active/building buffers.
 * @param None.
 * @return Pointer to the setpoint manager object.
 */
const bb_ctrl_setpoint_mgr_t *bb_cfg_get_mgr(void);

#endif
