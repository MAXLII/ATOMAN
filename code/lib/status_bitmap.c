// SPDX-License-Identifier: MIT
/**
 * @file    status_bitmap.c
 * @brief   Active and historical status bitmap implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Validate caller-provided status indexes
 *          - Update active status without erasing historical evidence
 *          - Provide bounded status queries for business adapters
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Invalid objects and indexes leave state unchanged
 *          - No hardware, protocol, or framework dependencies
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

#include "status_bitmap.h"

#include <stddef.h>

static uint8_t status_bitmap_index_is_valid(const status_bitmap_t *p_bitmap, uint8_t bit_index)
{
    if (p_bitmap == NULL)
    {
        return 0u;
    }

    if (bit_index >= p_bitmap->bit_count)
    {
        return 0u;
    }
    if (bit_index >= STATUS_BITMAP_CAPACITY_BITS)
    {
        return 0u;
    }

    return 1u;
}

static uint32_t status_bitmap_mask(uint8_t bit_index)
{
    return UINT32_C(1) << bit_index;
}

void status_bitmap_init(status_bitmap_t *p_bitmap, uint8_t bit_count)
{
    if (p_bitmap == NULL)
    {
        return;
    }

    p_bitmap->active = 0u;
    p_bitmap->history = 0u;
    p_bitmap->bit_count = (bit_count <= STATUS_BITMAP_CAPACITY_BITS) ? bit_count : 0u;
}

uint8_t status_bitmap_set(status_bitmap_t *p_bitmap, uint8_t bit_index)
{
    uint32_t mask = 0u; /* Selected status bit after index validation. */

    if (status_bitmap_index_is_valid(p_bitmap, bit_index) == 0u)
    {
        return 0u;
    }

    mask = status_bitmap_mask(bit_index);
    p_bitmap->active |= mask;
    p_bitmap->history |= mask;
    return 1u;
}

uint8_t status_bitmap_clear(status_bitmap_t *p_bitmap, uint8_t bit_index)
{
    uint32_t mask = 0u; /* Selected status bit after index validation. */

    if (status_bitmap_index_is_valid(p_bitmap, bit_index) == 0u)
    {
        return 0u;
    }

    mask = status_bitmap_mask(bit_index);
    p_bitmap->active &= ~mask;
    return 1u;
}

uint8_t status_bitmap_get(const status_bitmap_t *p_bitmap, uint8_t bit_index)
{
    uint32_t mask = 0u; /* Selected status bit after index validation. */

    if (status_bitmap_index_is_valid(p_bitmap, bit_index) == 0u)
    {
        return 0u;
    }

    mask = status_bitmap_mask(bit_index);
    return ((p_bitmap->active & mask) != 0u) ? 1u : 0u;
}

uint32_t status_bitmap_active_get(const status_bitmap_t *p_bitmap)
{
    if (p_bitmap == NULL)
    {
        return 0u;
    }

    return p_bitmap->active;
}

uint32_t status_bitmap_history_get(const status_bitmap_t *p_bitmap)
{
    if (p_bitmap == NULL)
    {
        return 0u;
    }

    return p_bitmap->history;
}

void status_bitmap_clear_all(status_bitmap_t *p_bitmap)
{
    if (p_bitmap == NULL)
    {
        return;
    }

    p_bitmap->active = 0u;
}

void status_bitmap_history_clear_all(status_bitmap_t *p_bitmap)
{
    if (p_bitmap == NULL)
    {
        return;
    }

    p_bitmap->history = 0u;
}

uint8_t status_bitmap_any(const status_bitmap_t *p_bitmap)
{
    if (p_bitmap == NULL)
    {
        return 0u;
    }

    return (p_bitmap->active != 0u) ? 1u : 0u;
}
