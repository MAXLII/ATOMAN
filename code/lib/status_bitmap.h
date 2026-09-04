// SPDX-License-Identifier: MIT
/**
 * @file    status_bitmap.h
 * @brief   Active and historical status bitmap public interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Maintain caller-owned active status bits
 *          - Latch historical evidence independently from active state
 *          - Reject status indexes outside the configured bitmap range
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Supports from 0 to 32 configured status bits
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

#ifndef STATUS_BITMAP_H
#define STATUS_BITMAP_H

#include <stdint.h>

#define STATUS_BITMAP_CAPACITY_BITS 32u /* Number of bits in the fixed-width storage word. */

typedef struct
{
    uint32_t active;   /* Status bits that are currently asserted. */
    uint32_t history;  /* Status bits latched since the last history clear. */
    uint8_t bit_count; /* Number of valid status indexes. */
} status_bitmap_t;

/**
 * @brief Initialize a status bitmap with all bits cleared.
 * @param p_bitmap Status bitmap owned by the caller.
 * @param bit_count Number of valid status indexes from 0 through 32.
 */
void status_bitmap_init(status_bitmap_t *p_bitmap, uint8_t bit_count);

/**
 * @brief Set one active bit and latch its historical bit.
 * @param p_bitmap Status bitmap owned by the caller.
 * @param bit_index Zero-based status index.
 * @return 1 when the index was accepted; otherwise 0.
 */
uint8_t status_bitmap_set(status_bitmap_t *p_bitmap, uint8_t bit_index);

/**
 * @brief Clear one active bit without clearing its history.
 * @param p_bitmap Status bitmap owned by the caller.
 * @param bit_index Zero-based status index.
 * @return 1 when the index was accepted; otherwise 0.
 */
uint8_t status_bitmap_clear(status_bitmap_t *p_bitmap, uint8_t bit_index);

/**
 * @brief Read one active status bit.
 * @param p_bitmap Status bitmap owned by the caller.
 * @param bit_index Zero-based status index.
 * @return 1 when the selected status is active; otherwise 0.
 */
uint8_t status_bitmap_get(const status_bitmap_t *p_bitmap, uint8_t bit_index);

/**
 * @brief Read all active status bits.
 * @param p_bitmap Status bitmap owned by the caller.
 * @return Current active bitmap, or 0 for a null object.
 */
uint32_t status_bitmap_active_get(const status_bitmap_t *p_bitmap);

/**
 * @brief Read all historical status bits.
 * @param p_bitmap Status bitmap owned by the caller.
 * @return Historical bitmap, or 0 for a null object.
 */
uint32_t status_bitmap_history_get(const status_bitmap_t *p_bitmap);

/**
 * @brief Clear all active status bits without clearing history.
 * @param p_bitmap Status bitmap owned by the caller.
 */
void status_bitmap_clear_all(status_bitmap_t *p_bitmap);

/**
 * @brief Clear all historical status bits.
 * @param p_bitmap Status bitmap owned by the caller.
 */
void status_bitmap_history_clear_all(status_bitmap_t *p_bitmap);

/**
 * @brief Read whether any active status bit is asserted.
 * @param p_bitmap Status bitmap owned by the caller.
 * @return 1 when at least one active bit is set; otherwise 0.
 */
uint8_t status_bitmap_any(const status_bitmap_t *p_bitmap);

#endif /* STATUS_BITMAP_H */
