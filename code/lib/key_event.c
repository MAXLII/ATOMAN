// SPDX-License-Identifier: MIT
/**
 * @file    key_event.c
 * @brief   Timed key-event detector implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Accumulate bounded evidence for the current key press
 *          - Publish one-shot events at the configured qualification point
 *          - Enforce a release-based cooldown before rearming detection
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Press duration saturates instead of wrapping
 *          - Invalid input state is normalized to released
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

#include "key_event.h"

#include <stddef.h>

uint8_t key_event_init(key_event_t *p_event, const key_event_cfg_t *p_cfg)
{
    if ((p_event == NULL) || /* A detector object is required. */
        (p_cfg == NULL))     /* Configuration must be supplied explicitly. */
    {
        return 0u;
    }

    p_event->cfg = *p_cfg;
    p_event->cfg.release_required = (p_cfg->release_required == 1u) ? 1u : 0u;
    key_event_reset(p_event);

    if ((p_event->cfg.release_required == 1u) && /* Release uses a bounded duration window. */
        (p_event->cfg.max_press_ticks < p_event->cfg.min_press_ticks)) /* Window must not invert. */
    {
        p_event->cfg.min_press_ticks = 0u;
        p_event->cfg.max_press_ticks = 0u;
        p_event->cfg.cooldown_ticks = 0u;
        p_event->cfg.release_required = 0u;
        return 0u;
    }

    return 1u;
}

void key_event_reset(key_event_t *p_event)
{
    if (p_event == NULL)
    {
        return;
    }

    p_event->inter.press_ticks = 0u;
    p_event->inter.cooldown_remaining_ticks = 0u;
    p_event->output.event_pending = 0u;
}

void key_event_update(key_event_t *p_event, uint8_t is_pressed)
{
    uint8_t pressed = 0u; /* Normalized sampled key state. */

    if (p_event == NULL)
    {
        return;
    }

    pressed = (is_pressed == 1u) ? 1u : 0u;
    if (p_event->inter.cooldown_remaining_ticks > 0u)
    {
        if (pressed == 0u)
        {
            p_event->inter.cooldown_remaining_ticks--;
            p_event->inter.press_ticks = 0u;
        }
        return;
    }

    if (pressed == 1u)
    {
        if (p_event->inter.press_ticks < UINT32_MAX)
        {
            p_event->inter.press_ticks++;
        }

        if ((p_event->cfg.release_required == 0u) && /* Press duration directly qualifies the event. */
            (p_event->inter.press_ticks >= p_event->cfg.min_press_ticks)) /* Minimum evidence is complete. */
        {
            p_event->output.event_pending = 1u;
            p_event->inter.cooldown_remaining_ticks = p_event->cfg.cooldown_ticks;
            p_event->inter.press_ticks = 0u;
        }
        return;
    }

    if ((p_event->cfg.release_required == 1u) && /* Release closes the measurement window. */
        (p_event->inter.press_ticks >= p_event->cfg.min_press_ticks) && /* Press was long enough. */
        (p_event->inter.press_ticks <= p_event->cfg.max_press_ticks)) /* Press did not exceed the window. */
    {
        p_event->output.event_pending = 1u;
        p_event->inter.cooldown_remaining_ticks = p_event->cfg.cooldown_ticks;
    }
    p_event->inter.press_ticks = 0u;
}

uint8_t key_event_take(key_event_t *p_event)
{
    uint8_t event_pending = 0u; /* Event state returned to the caller. */

    if (p_event == NULL)
    {
        return 0u;
    }

    event_pending = p_event->output.event_pending;
    p_event->output.event_pending = 0u;
    return event_pending;
}
