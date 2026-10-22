// SPDX-License-Identifier: MIT
/**
 * @file test_frame_slots.c
 * @brief Verify sample-time byte snapshots through full-frame queue wrap.
 * @details Uses an independent byte encoder for every wire type, unaligned
 *          sources/storage, mixed queue occupancy and maximum-size frames.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "realtime_scope.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t expected[64][REALTIME_SCOPE_FRAME_MAX];

static void check_pop(realtime_scope_t *p_scope, uint32_t sequence, uint32_t frame_size)
{
    realtime_scope_frame_t frame;
    assert(realtime_scope_pop(p_scope, &frame));
    assert(frame.size == frame_size);
    assert(memcmp(frame.data, expected[sequence % 64u], frame_size) == 0);
}

static void exercise(uint32_t capacity, bool maximum)
{
    static const uint8_t widths[] = {1u, 1u, 2u, 2u, 4u, 4u, 8u, 8u, 4u, 8u, 1u};
    realtime_scope_t scope;
    uint8_t storage[2502];
    volatile uint8_t source[32][9];
    uint32_t payload = 0u;
    uint32_t written = 0u, read = 0u;
    memset(storage, 0xA5, sizeof(storage));
    assert(realtime_scope_init(&scope, &storage[1], capacity));

    for (uint8_t channel = 32u; channel != 0u; --channel)
    {
        uint8_t type = maximum ? (uint8_t)RS_U64 : (uint8_t)((channel - 1u) % 11u + 1u);
        payload += widths[type - 1u];
        assert(realtime_scope_set(&scope, channel, &source[channel - 1u][1], (realtime_scope_type_e)type));
    }
    assert(realtime_scope_swap(&scope));
    assert(realtime_scope_start(&scope));
    const uint32_t frame_size = payload + 5u;
    const uint32_t slots      = capacity / frame_size;
    assert(    slots > 0u
            && slots <= 64u);

    for (uint32_t step = 0u; step < 4096u; ++step)
    {
        if (written - read < slots)
        {
            uint8_t *p_expected = expected[written % 64u];
            uint32_t position   = 2u;
            p_expected[0]       = 0xEAu;
            p_expected[1]       = (uint8_t)written;

            for (uint32_t channel = 0u; channel < 32u; ++channel)
            {
                uint32_t width = maximum ? 8u : widths[channel % 11u];

                for (uint32_t byte = 0u; byte < width; ++byte)
                {
                    uint8_t value = (uint8_t)(step * 37u + channel * 13u + byte * 71u);
                    source[channel][byte + 1u] = value;
                    p_expected[position++]     = value;
                }
            }
            p_expected[position++] = (uint8_t)payload;
            p_expected[position++] = (uint8_t)(payload >> 8u);
            uint8_t sum = 0u;

            for (uint32_t byte = 0u; byte < position; ++byte)
                sum = (uint8_t)(sum + p_expected[byte]);
            p_expected[position] = sum;
            realtime_scope_sample(&scope);
            assert(    scope.running
                    && scope.overflows == 0u);
            ++written;
        }

        if (    step % 3u != 0u
             && read != written)
            check_pop(&scope, read++, frame_size);
        assert(scope.count == written - read);
        assert(    storage[0] == 0xA5u
                && storage[capacity + 1u] == 0xA5u);
    }

    while (read != written)
        check_pop(&scope, read++, frame_size);
    /* Fill from unchanged source RAM, then verify overflow does not publish a frame. */

    for (uint32_t i = 0u; i < slots; ++i)
        realtime_scope_sample(&scope);
    uint8_t index = scope.index;
    realtime_scope_sample(&scope);
    assert(    !scope.running
            && scope.overflows == 1u
            && scope.count == slots
            && scope.index == index);
    assert(    storage[0] == 0xA5u
            && storage[capacity + 1u] == 0xA5u);
}

int main(void)
{
    exercise(261u, false);
    exercise(2500u, false);
    exercise(261u, true);
    exercise(2500u, true);
    puts("PASS: all wire types, byte snapshots, unaligned storage, mixed occupancy, maximum frames and overflow");
    return 0;
}
