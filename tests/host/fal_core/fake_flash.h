// SPDX-License-Identifier: MIT
/**
 * @file    fake_flash.h
 * @brief   Host fake flash driver for FAL core verification.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Provide deterministic byte-addressable flash storage
 *          - Simulate asynchronous busy completion and driver errors
 *          - Record physical operations issued by the real FAL core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Test-only, single-threaded implementation
 *          - Hardware access is replaced by an in-memory model
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef FAKE_FLASH_H
#define FAKE_FLASH_H

#include "fal_core.h"

#include <stdint.h>

#define FAKE_FLASH_CAPACITY 8192u /**< Bytes provided by each fake device. */
#define FAKE_FLASH_MAX_CALLS 64u  /**< Maximum recorded platform operations. */

typedef enum
{
    FAKE_FLASH_CALL_READ = 0, /**< Recorded physical read. */
    FAKE_FLASH_CALL_PROGRAM,  /**< Recorded physical program. */
    FAKE_FLASH_CALL_ERASE,    /**< Recorded physical erase. */
    FAKE_FLASH_CALL_SYNC      /**< Recorded physical sync. */
} fake_flash_call_type_t;

typedef struct
{
    fake_flash_call_type_t type; /**< Operation type. */
    uint32_t address;            /**< Physical start address. */
    uint32_t length;             /**< Physical operation length. */
} fake_flash_call_t;

typedef struct
{
    uint8_t data[FAKE_FLASH_CAPACITY];                /**< Simulated nonvolatile contents. */
    fake_flash_call_t calls[FAKE_FLASH_MAX_CALLS];    /**< Ordered driver call history. */
    uint32_t call_count;                              /**< Valid entries in calls. */
    uint32_t busy_polls_remaining;                    /**< Busy state polls before completion. */
    uint32_t busy_polls_per_operation;                /**< Busy polls assigned to new work. */
    fal_result_t next_result;                         /**< One-shot result returned by the next operation. */
    uint8_t initialized;                              /**< Driver initialization state. */
} fake_flash_t;

void fake_flash_reset(fake_flash_t *p_flash);
fal_flash_ops_t fake_flash_ops_make(fake_flash_t *p_flash);

#endif /* FAKE_FLASH_H */
