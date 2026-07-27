// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_metadata.h
 * @brief   Stable codec for redundant bootloader upgrade metadata.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define persistent upgrade and installation recovery states
 *          - Encode metadata without compiler padding or native-endian dependencies
 *          - Validate and select the newest committed copy from META_A and META_B
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Pure functions are reentrant when the CRC callback is reentrant
 *          - Flash access remains behind the bootloader logical flash service
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

#ifndef BOOTLOADER_METADATA_H
#define BOOTLOADER_METADATA_H

#include "bootloader_core.h"

#include <stdint.h>

#define BOOTLOADER_METADATA_ENCODED_SIZE 42u

typedef enum
{
    BOOTLOADER_METADATA_STATE_EMPTY = 0,
    BOOTLOADER_METADATA_STATE_DOWNLOAD_DIRECT,
    BOOTLOADER_METADATA_STATE_DOWNLOAD_STAGED,
    BOOTLOADER_METADATA_STATE_INSTALL_PENDING,
    BOOTLOADER_METADATA_STATE_COPYING,
    BOOTLOADER_METADATA_STATE_VALID,
    BOOTLOADER_METADATA_STATE_FAILED
} bootloader_metadata_state_t;

typedef struct
{
    bootloader_metadata_state_t state;
    bootloader_upgrade_mode_t mode;
    uint8_t module_id;
    uint8_t retry_count;
    uint32_t sequence;
    uint32_t version;
    uint32_t file_size;
    uint16_t expected_crc;
    uint32_t received_length;
    uint16_t running_crc;
    uint32_t copy_offset;
    uint16_t error_code;
} bootloader_metadata_t;

bootloader_result_t bootloader_metadata_encode(const bootloader_metadata_t *p_metadata,
                                                uint8_t *p_encoded,
                                                uint32_t encoded_capacity);
bootloader_result_t bootloader_metadata_decode(const uint8_t *p_encoded,
                                                uint32_t encoded_length,
                                                bootloader_metadata_t *p_metadata);
bootloader_result_t bootloader_metadata_select(const uint8_t *p_meta_a,
                                                const uint8_t *p_meta_b,
                                                bootloader_metadata_t *p_metadata,
                                                bootloader_flash_zone_t *p_source_zone);

#endif /* BOOTLOADER_METADATA_H */
