// SPDX-License-Identifier: MIT
/**
 * @file test_realtime_scope.c
 * @brief Host verification of the production scope core.
 * @details Part of base. Checks sparse banks, wire bytes, wrap and overflow.
 *          No hardware addresses or network timing are simulated by these tests.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "realtime_scope.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static realtime_scope_t scope;
static uint8_t storage[65538];
int main(int argc, char **p_argv)
{
    uint32_t first = UINT32_C(0x1234EBEA);
    int16_t last   = -123;
    realtime_scope_frame_t frame;
    uint32_t queue_bytes = argc == 3 ? (uint32_t)strtoul(p_argv[2], NULL, 10) : 65536u;
    assert(queue_bytes <= sizeof(storage) - 2u);
    assert(!realtime_scope_init(NULL, storage, queue_bytes));
    assert(!realtime_scope_init(&scope, NULL, queue_bytes));
    assert(!realtime_scope_init(&scope, storage, REALTIME_SCOPE_FRAME_MAX - 1u));
    storage[0]                = 0xA5u;
    storage[queue_bytes + 1u] = 0x5Au;
    assert(realtime_scope_init(&scope, &storage[1], queue_bytes));
    realtime_scope_bank_t *p_a = scope.p_active, *p_b = scope.p_config;
    assert(!realtime_scope_set(&scope, 0u, &first, RS_U32));
    assert(!realtime_scope_set(&scope, 33u, &first, RS_U32));
    assert(!realtime_scope_set(&scope, 1u, &first, (realtime_scope_type_e)99));
    assert(!realtime_scope_swap(&scope));
    assert(realtime_scope_set(&scope, 32u, &last, RS_I16));
    assert(realtime_scope_set(&scope, 1u, &first, RS_U32));
    assert(p_b->p_head == &p_b->nodes[0]);
    assert(p_b->p_head->p_next == &p_b->nodes[31]);
    assert(p_b->nodes[31].p_next == NULL);
    assert(realtime_scope_swap(&scope));
    assert(    scope.p_active == p_b
            && scope.p_config == p_a);
    assert(realtime_scope_start(&scope));
    /* Editing the other bank while running must not change active sampling. */
    assert(realtime_scope_set(&scope, 8u, &first, RS_U32));
    assert(!realtime_scope_swap(&scope));

    for (uint32_t i = 0u; i < 20000u; ++i)
    {
        realtime_scope_sample(&scope);
        assert(realtime_scope_pop(&scope, &frame));
        assert(    frame.size == 11u
                && frame.data[0] == 0xEAu
                && frame.data[1] == (uint8_t)i);
        assert(    frame.data[2] == 0xEAu
                && frame.data[3] == 0xEBu);
        assert(    frame.data[6] == 0x85u
                && frame.data[7] == 0xFFu);
        assert(    frame.data[8] == 6u
                && frame.data[9] == 0u);
        uint8_t sum = 0u;

        for (uint32_t j = 0u; j < 10u; ++j)
            sum = (uint8_t)(sum + frame.data[j]);
        assert(sum == frame.data[10]);

        if (    argc == 2
             && i == 0u)
        {
            FILE *p_file = fopen(p_argv[1], "wb");
            assert(p_file != NULL);
            assert(fwrite(frame.data, 1u, frame.size, p_file) == frame.size);
            assert(fclose(p_file) == 0);
        }
    }

    const uint32_t capacity = queue_bytes / 11u;

    for (uint32_t i = 0u; i <= capacity; ++i)
        realtime_scope_sample(&scope);
    assert(    !scope.running
            && scope.overflows == 1u
            && scope.count == capacity);
    assert(    storage[0] == 0xA5u
            && storage[queue_bytes + 1u] == 0x5Au);
    assert(!realtime_scope_swap(&scope));

    while (realtime_scope_pop(&scope, &frame))
    {
    }
    assert(realtime_scope_swap(&scope));
    assert(    scope.p_active == p_a
            && scope.p_config == p_b);
    assert(scope.p_config->payload_size == 6u); /* No bank copy or erasure. */
    realtime_scope_clear(&scope);

    for (uint8_t i = 1u; i <= REALTIME_SCOPE_CHANNELS; ++i)
        assert(realtime_scope_set(&scope, i, &first, RS_U32));
    assert(scope.p_config->payload_size == 128u);

    for (uint8_t i = 1u; i <= REALTIME_SCOPE_CHANNELS; ++i)
        assert(realtime_scope_set(&scope, i, NULL, RS_U32));
    assert(    scope.p_config->p_head == NULL
            && scope.p_config->enabled == 0u);
    puts("PASS: sparse 32-channel banks, pointer swap, EA/EB in payload, exact wire bytes, modulo wrap, overflow stop");
    return 0;
}
