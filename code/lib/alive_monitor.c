// SPDX-License-Identifier: MIT
/**
 * @file    alive_monitor.c
 * @brief   Activity timeout monitor implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize and reset caller-owned activity state
 *          - Reload bounded activity windows from validated evidence
 *          - Advance expiry using an explicit caller-provided time base
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - A zero reload interval always represents an expired monitor
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

#include "alive_monitor.h"

#include <stddef.h>

void alive_monitor_init(alive_monitor_t *p_monitor, uint32_t reload_ticks)
{
    if (p_monitor == NULL)
    {
        return;
    }

    p_monitor->reload_ticks = reload_ticks;
    p_monitor->remaining_ticks = 0u;
}

void alive_monitor_reset(alive_monitor_t *p_monitor)
{
    if (p_monitor == NULL)
    {
        return;
    }

    p_monitor->remaining_ticks = 0u;
}

void alive_monitor_feed(alive_monitor_t *p_monitor)
{
    if (p_monitor == NULL)
    {
        return;
    }

    p_monitor->remaining_ticks = p_monitor->reload_ticks;
}

void alive_monitor_tick(alive_monitor_t *p_monitor)
{
    if (p_monitor == NULL)
    {
        return;
    }

    if (p_monitor->remaining_ticks > 0u)
    {
        p_monitor->remaining_ticks--;
    }
}

uint8_t alive_monitor_is_alive(const alive_monitor_t *p_monitor)
{
    if (p_monitor == NULL)
    {
        return 0u;
    }

    return (p_monitor->remaining_ticks > 0u) ? 1u : 0u;
}
