// SPDX-License-Identifier: MIT
/**
 * @file    test_lib_utility.c
 * @brief   Host tests for reusable business-support utilities.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Verify bounded activity timeout behavior
 *          - Verify active and historical status bitmap behavior
 *          - Verify press, release, cooldown, and one-shot key events
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Tests use explicit tick calls instead of wall-clock time
 *          - All production modules are compiled directly into the host test
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
#include "key_event.h"
#include "status_bitmap.h"

#include <stdint.h>
#include <stdio.h>

static uint32_t check_count = 0u; /* Total assertions evaluated. */
static uint32_t fail_count = 0u;  /* Assertions that failed. */

static void check_u32(const char *p_name, uint32_t expected, uint32_t actual)
{
    uint8_t passed = (expected == actual) ? 1u : 0u; /* Current assertion result. */

    check_count++;
    if (passed == 0u)
    {
        fail_count++;
    }
    (void)printf("CHECK %03lu %-34s expected=%lu actual=%lu %s\n",
                 (unsigned long)check_count,
                 p_name,
                 (unsigned long)expected,
                 (unsigned long)actual,
                 (passed == 1u) ? "PASS" : "FAIL");
}

static void case_alive_monitor(void)
{
    alive_monitor_t monitor = {0}; /* Activity monitor under test. */

    (void)printf("\nCASE alive_monitor\n");
    alive_monitor_init(&monitor, 3u);
    check_u32("initially expired", 0u, alive_monitor_is_alive(&monitor));
    alive_monitor_feed(&monitor);
    check_u32("feed marks alive", 1u, alive_monitor_is_alive(&monitor));
    alive_monitor_tick(&monitor);
    alive_monitor_tick(&monitor);
    check_u32("alive before final tick", 1u, alive_monitor_is_alive(&monitor));
    alive_monitor_tick(&monitor);
    check_u32("expires on final tick", 0u, alive_monitor_is_alive(&monitor));
    alive_monitor_feed(&monitor);
    alive_monitor_reset(&monitor);
    check_u32("reset expires", 0u, alive_monitor_is_alive(&monitor));
    alive_monitor_init(&monitor, 0u);
    alive_monitor_feed(&monitor);
    check_u32("zero window stays expired", 0u, alive_monitor_is_alive(&monitor));
    check_u32("null monitor is expired", 0u, alive_monitor_is_alive(NULL));
}

static void case_status_bitmap(void)
{
    status_bitmap_t bitmap = {0}; /* Status bitmap under test. */

    (void)printf("\nCASE status_bitmap\n");
    status_bitmap_init(&bitmap, 3u);
    check_u32("set valid bit", 1u, status_bitmap_set(&bitmap, 1u));
    check_u32("active bit value", UINT32_C(2), status_bitmap_active_get(&bitmap));
    check_u32("history latched", UINT32_C(2), status_bitmap_history_get(&bitmap));
    check_u32("any active", 1u, status_bitmap_any(&bitmap));
    check_u32("clear valid bit", 1u, status_bitmap_clear(&bitmap, 1u));
    check_u32("active cleared", 0u, status_bitmap_active_get(&bitmap));
    check_u32("history retained", UINT32_C(2), status_bitmap_history_get(&bitmap));
    check_u32("reject out-of-range bit", 0u, status_bitmap_set(&bitmap, 3u));
    check_u32("invalid set changes nothing", 0u, status_bitmap_active_get(&bitmap));
    status_bitmap_history_clear_all(&bitmap);
    check_u32("history clear", 0u, status_bitmap_history_get(&bitmap));
    status_bitmap_init(&bitmap, STATUS_BITMAP_CAPACITY_BITS);
    check_u32("set highest bit", 1u, status_bitmap_set(&bitmap, 31u));
    check_u32("highest bit value", UINT32_C(0x80000000), status_bitmap_active_get(&bitmap));
    status_bitmap_init(&bitmap, 33u);
    check_u32("reject oversized capacity", 0u, bitmap.bit_count);
    check_u32("null bitmap is empty", 0u, status_bitmap_any(NULL));
}

static void case_release_qualified_key(void)
{
    const key_event_cfg_t cfg = {
        .min_press_ticks = 2u,
        .max_press_ticks = 4u,
        .cooldown_ticks = 2u,
        .release_required = 1u,
    }; /* Short-press configuration under test. */
    key_event_t event = {0}; /* Key-event detector under test. */

    (void)printf("\nCASE release_qualified_key\n");
    check_u32("release config accepted", 1u, key_event_init(&event, &cfg));
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    check_u32("waits for release", 0u, key_event_take(&event));
    key_event_update(&event, 0u);
    check_u32("event on qualified release", 1u, key_event_take(&event));
    check_u32("event consumed once", 0u, key_event_take(&event));
    key_event_update(&event, 0u);
    key_event_update(&event, 0u);
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    key_event_update(&event, 0u);
    check_u32("rearms after cooldown", 1u, key_event_take(&event));

    key_event_reset(&event);
    key_event_update(&event, 1u);
    key_event_update(&event, 0u);
    check_u32("reject short press", 0u, key_event_take(&event));
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    key_event_update(&event, 0u);
    check_u32("reject overlong release", 0u, key_event_take(&event));
}

static void case_press_qualified_key(void)
{
    const key_event_cfg_t cfg = {
        .min_press_ticks = 3u,
        .max_press_ticks = 0u,
        .cooldown_ticks = 2u,
        .release_required = 0u,
    }; /* Long-press configuration under test. */
    const key_event_cfg_t invalid_cfg = {
        .min_press_ticks = 4u,
        .max_press_ticks = 3u,
        .cooldown_ticks = 0u,
        .release_required = 1u,
    }; /* Inverted release window used to verify rejection. */
    key_event_t event = {0}; /* Key-event detector under test. */

    (void)printf("\nCASE press_qualified_key\n");
    check_u32("press config accepted", 1u, key_event_init(&event, &cfg));
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    check_u32("not yet qualified", 0u, key_event_take(&event));
    key_event_update(&event, 1u);
    check_u32("event at threshold", 1u, key_event_take(&event));
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    check_u32("held key does not retrigger", 0u, key_event_take(&event));
    key_event_update(&event, 0u);
    key_event_update(&event, 0u);
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    key_event_update(&event, 1u);
    check_u32("press event rearms", 1u, key_event_take(&event));
    check_u32("reject inverted window", 0u, key_event_init(&event, &invalid_cfg));
    check_u32("null detector rejected", 0u, key_event_init(NULL, &cfg));
}

int main(void)
{
    case_alive_monitor();
    case_status_bitmap();
    case_release_qualified_key();
    case_press_qualified_key();

    (void)printf("\nSUMMARY checks=%lu failures=%lu\n",
                 (unsigned long)check_count,
                 (unsigned long)fail_count);
    return (fail_count == 0u) ? 0 : 1;
}
