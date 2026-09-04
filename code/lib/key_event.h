// SPDX-License-Identifier: MIT
/**
 * @file    key_event.h
 * @brief   Timed key-event detector public interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Classify bounded press durations into one-shot key events
 *          - Support press-qualified and release-qualified events
 *          - Suppress repeated events through a configurable cooldown window
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - One call to key_event_update represents one configured time step
 *          - GPIO sampling and task registration remain caller responsibilities
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

#ifndef KEY_EVENT_H
#define KEY_EVENT_H

#include <stdint.h>

typedef struct
{
    uint32_t min_press_ticks; /* Minimum qualified press duration. */
    uint32_t max_press_ticks; /* Maximum duration for a release-qualified event. */
    uint32_t cooldown_ticks;  /* Released ticks required before another event. */
    uint8_t release_required; /* 1 qualifies on release; 0 qualifies while pressed. */
} key_event_cfg_t;

typedef struct
{
    uint32_t press_ticks;              /* Saturating duration of the current press. */
    uint32_t cooldown_remaining_ticks; /* Released ticks remaining in cooldown. */
} key_event_inter_t;

typedef struct
{
    uint8_t event_pending; /* One-shot event retained until consumed. */
} key_event_output_t;

typedef struct
{
    key_event_cfg_t cfg;       /* Caller configuration copied during initialization. */
    key_event_inter_t inter;   /* Runtime timing state. */
    key_event_output_t output; /* Consumable event state. */
} key_event_t;

/**
 * @brief Initialize a key-event detector.
 * @param p_event Key-event detector owned by the caller.
 * @param p_cfg Timing and qualification configuration.
 * @return 1 when the configuration was accepted; otherwise 0.
 */
uint8_t key_event_init(key_event_t *p_event, const key_event_cfg_t *p_cfg);

/**
 * @brief Reset runtime state while retaining the current configuration.
 * @param p_event Key-event detector owned by the caller.
 */
void key_event_reset(key_event_t *p_event);

/**
 * @brief Advance key-event detection by one configured time step.
 * @param p_event Key-event detector owned by the caller.
 * @param is_pressed Normalized sampled state: 1 pressed, 0 released.
 */
void key_event_update(key_event_t *p_event, uint8_t is_pressed);

/**
 * @brief Consume a pending key event.
 * @param p_event Key-event detector owned by the caller.
 * @return 1 exactly once for each qualified event; otherwise 0.
 */
uint8_t key_event_take(key_event_t *p_event);

#endif /* KEY_EVENT_H */
