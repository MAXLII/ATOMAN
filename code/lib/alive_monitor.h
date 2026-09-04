// SPDX-License-Identifier: MIT
/**
 * @file    alive_monitor.h
 * @brief   Activity timeout monitor public interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Represent peer activity with caller-owned countdown state
 *          - Reload the activity window when valid evidence arrives
 *          - Expire activity after a bounded number of periodic ticks
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - One call to alive_monitor_tick represents one configured time step
 *          - No hardware or framework dependencies
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef ALIVE_MONITOR_H
#define ALIVE_MONITOR_H

#include <stdint.h>

typedef struct
{
    uint32_t reload_ticks;    /* Activity window expressed in tick calls. */
    uint32_t remaining_ticks; /* Ticks remaining before activity expires. */
} alive_monitor_t;

/**
 * @brief Initialize an activity monitor in the expired state.
 * @param p_monitor Activity monitor owned by the caller.
 * @param reload_ticks Number of tick calls retained after each feed.
 */
void alive_monitor_init(alive_monitor_t *p_monitor, uint32_t reload_ticks);

/**
 * @brief Return an activity monitor to the expired state.
 * @param p_monitor Activity monitor owned by the caller.
 */
void alive_monitor_reset(alive_monitor_t *p_monitor);

/**
 * @brief Record valid activity and reload the configured timeout window.
 * @param p_monitor Activity monitor owned by the caller.
 */
void alive_monitor_feed(alive_monitor_t *p_monitor);

/**
 * @brief Advance the activity monitor by one configured time step.
 * @param p_monitor Activity monitor owned by the caller.
 */
void alive_monitor_tick(alive_monitor_t *p_monitor);

/**
 * @brief Read the current activity state.
 * @param p_monitor Activity monitor owned by the caller.
 * @return 1 when activity remains valid; otherwise 0.
 */
uint8_t alive_monitor_is_alive(const alive_monitor_t *p_monitor);

#endif /* ALIVE_MONITOR_H */
