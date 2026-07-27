// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_metadata.c
 * @brief   Redundant bootloader metadata serialization and selection.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Serialize persistent fields in a fixed little-endian representation
 *          - Reject torn writes through a CRC and final commit marker
 *          - Select the newest valid metadata sequence with wrap-aware ordering
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

#include "bootloader_metadata.h"

#include <stddef.h>
#include <string.h>

#define METADATA_MAGIC 0x424C4D44u
#define METADATA_FORMAT_VERSION 1u
#define METADATA_COMMIT_MARKER 0x434F4D54u
#define METADATA_CRC_OFFSET 36u
#define METADATA_COMMIT_OFFSET 38u

static void write_u16_le(uint8_t *p_data, uint16_t value)
{
    p_data[0] = (uint8_t)(value & 0xFFu);
    p_data[1] = (uint8_t)(value >> 8u);
}

static void write_u32_le(uint8_t *p_data, uint32_t value)
{
    p_data[0] = (uint8_t)(value & 0xFFu);
    p_data[1] = (uint8_t)((value >> 8u) & 0xFFu);
    p_data[2] = (uint8_t)((value >> 16u) & 0xFFu);
    p_data[3] = (uint8_t)(value >> 24u);
}

static uint16_t read_u16_le(const uint8_t *p_data)
{
    return (uint16_t)((uint16_t)p_data[0] | ((uint16_t)p_data[1] << 8u));
}

static uint32_t read_u32_le(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] |
           ((uint32_t)p_data[1] << 8u) |
           ((uint32_t)p_data[2] << 16u) |
           ((uint32_t)p_data[3] << 24u);
}

static uint8_t state_valid(bootloader_metadata_state_t state)
{
    return (state <= BOOTLOADER_METADATA_STATE_FAILED) ? 1u : 0u;
}

static uint8_t sequence_is_newer(uint32_t candidate, uint32_t reference)
{
    const uint32_t difference = candidate - reference;

    return ((difference != 0u) && (difference < 0x80000000u)) ? 1u : 0u;
}

static uint16_t metadata_crc16(const uint8_t *p_data, uint32_t length)
{
    uint16_t crc = 0xFFFFu;
    uint32_t index = 0u;
    uint8_t bit = 0u;

    for (index = 0u; index < length; index++)
    {
        crc ^= (uint16_t)((uint16_t)p_data[index] << 8u);
        for (bit = 0u; bit < 8u; bit++)
        {
            crc = ((crc & 0x8000u) != 0u)
                      ? (uint16_t)(((uint32_t)crc << 1u) ^ 0x1021u)
                      : (uint16_t)((uint32_t)crc << 1u);
        }
    }
    return crc;
}

bootloader_result_t bootloader_metadata_encode(const bootloader_metadata_t *p_metadata,
                                                uint8_t *p_encoded,
                                                uint32_t encoded_capacity)
{
    if ((p_metadata == NULL) || (p_encoded == NULL) ||
        (encoded_capacity < BOOTLOADER_METADATA_ENCODED_SIZE) ||
        (state_valid(p_metadata->state) == 0u) ||
        (p_metadata->mode > BOOTLOADER_UPGRADE_MODE_STAGED))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    (void)memset(p_encoded, 0xFF, BOOTLOADER_METADATA_ENCODED_SIZE);
    write_u32_le(&p_encoded[0], METADATA_MAGIC);
    write_u16_le(&p_encoded[4], METADATA_FORMAT_VERSION);
    p_encoded[6] = (uint8_t)p_metadata->state;
    p_encoded[7] = (uint8_t)p_metadata->mode;
    p_encoded[8] = p_metadata->module_id;
    p_encoded[9] = p_metadata->retry_count;
    write_u32_le(&p_encoded[10], p_metadata->sequence);
    write_u32_le(&p_encoded[14], p_metadata->version);
    write_u32_le(&p_encoded[18], p_metadata->file_size);
    write_u16_le(&p_encoded[22], p_metadata->expected_crc);
    write_u32_le(&p_encoded[24], p_metadata->received_length);
    write_u16_le(&p_encoded[28], p_metadata->running_crc);
    write_u32_le(&p_encoded[30], p_metadata->copy_offset);
    write_u16_le(&p_encoded[34], p_metadata->error_code);
    write_u16_le(&p_encoded[METADATA_CRC_OFFSET], metadata_crc16(p_encoded, METADATA_CRC_OFFSET));
    write_u32_le(&p_encoded[METADATA_COMMIT_OFFSET], METADATA_COMMIT_MARKER);
    return BOOTLOADER_RESULT_SUCCESS;
}

bootloader_result_t bootloader_metadata_decode(const uint8_t *p_encoded,
                                                uint32_t encoded_length,
                                                bootloader_metadata_t *p_metadata)
{
    bootloader_metadata_state_t state = BOOTLOADER_METADATA_STATE_EMPTY;
    bootloader_upgrade_mode_t mode = BOOTLOADER_UPGRADE_MODE_DIRECT;

    if ((p_encoded == NULL) || (p_metadata == NULL) ||
        (encoded_length < BOOTLOADER_METADATA_ENCODED_SIZE))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    state = (bootloader_metadata_state_t)p_encoded[6];
    mode = (bootloader_upgrade_mode_t)p_encoded[7];
    if ((read_u32_le(&p_encoded[0]) != METADATA_MAGIC) ||
        (read_u16_le(&p_encoded[4]) != METADATA_FORMAT_VERSION) ||
        (state_valid(state) == 0u) ||
        (mode > BOOTLOADER_UPGRADE_MODE_STAGED) ||
        (read_u16_le(&p_encoded[METADATA_CRC_OFFSET]) != metadata_crc16(p_encoded, METADATA_CRC_OFFSET)) ||
        (read_u32_le(&p_encoded[METADATA_COMMIT_OFFSET]) != METADATA_COMMIT_MARKER))
    {
        return BOOTLOADER_RESULT_IMAGE_INVALID;
    }
    p_metadata->state = state;
    p_metadata->mode = mode;
    p_metadata->module_id = p_encoded[8];
    p_metadata->retry_count = p_encoded[9];
    p_metadata->sequence = read_u32_le(&p_encoded[10]);
    p_metadata->version = read_u32_le(&p_encoded[14]);
    p_metadata->file_size = read_u32_le(&p_encoded[18]);
    p_metadata->expected_crc = read_u16_le(&p_encoded[22]);
    p_metadata->received_length = read_u32_le(&p_encoded[24]);
    p_metadata->running_crc = read_u16_le(&p_encoded[28]);
    p_metadata->copy_offset = read_u32_le(&p_encoded[30]);
    p_metadata->error_code = read_u16_le(&p_encoded[34]);
    return BOOTLOADER_RESULT_SUCCESS;
}

bootloader_result_t bootloader_metadata_select(const uint8_t *p_meta_a,
                                                const uint8_t *p_meta_b,
                                                bootloader_metadata_t *p_metadata,
                                                bootloader_flash_zone_t *p_source_zone)
{
    bootloader_metadata_t meta_a = {0};
    bootloader_metadata_t meta_b = {0};
    const uint8_t valid_a = (bootloader_metadata_decode(p_meta_a,
                                                         BOOTLOADER_METADATA_ENCODED_SIZE,
                                                         &meta_a) == BOOTLOADER_RESULT_SUCCESS)
                                ? 1u
                                : 0u;
    const uint8_t valid_b = (bootloader_metadata_decode(p_meta_b,
                                                         BOOTLOADER_METADATA_ENCODED_SIZE,
                                                         &meta_b) == BOOTLOADER_RESULT_SUCCESS)
                                ? 1u
                                : 0u;

    if ((p_metadata == NULL) || (p_source_zone == NULL))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    if ((valid_a == 0u) && (valid_b == 0u))
    {
        return BOOTLOADER_RESULT_IMAGE_INVALID;
    }
    if ((valid_b != 0u) && ((valid_a == 0u) || (sequence_is_newer(meta_b.sequence, meta_a.sequence) != 0u)))
    {
        *p_metadata = meta_b;
        *p_source_zone = BOOTLOADER_FLASH_ZONE_META_B;
    }
    else
    {
        *p_metadata = meta_a;
        *p_source_zone = BOOTLOADER_FLASH_ZONE_META_A;
    }
    return BOOTLOADER_RESULT_SUCCESS;
}
