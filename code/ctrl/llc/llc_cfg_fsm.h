// SPDX-License-Identifier: MIT
/**
 * @file    llc_cfg_fsm.h
 * @brief   llc configuration publishing interface for the FSM.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Expose the staged-configuration publish operation only to FSM implementation files
 *          - Keep application code limited to configuration setters and FSM commands
 *          - Establish the FSM as the single owner of configuration commit timing
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Publishing is performed only at FSM-controlled lifecycle boundaries
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-08-10
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef LLC_CFG_FSM_H
#define LLC_CFG_FSM_H

#include <stdint.h>

void llc_cfg_set_run_allowed(uint8_t run_allowed);
void llc_cfg_publish_building(void);

#endif /* LLC_CFG_FSM_H */
