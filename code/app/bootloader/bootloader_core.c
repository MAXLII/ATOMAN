// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_core.c
 * @brief   Platform-independent boot and firmware installation state machine.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Remain safely resident when no structurally valid IAP image exists
 *          - Receive direct or staged firmware through a logical flash mount
 *          - Install verified staging data and launch only a validated IAP prefix
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; Section or another scheduler serializes calls
 *          - Hardware and storage access are abstracted through mounted operation tables
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

#include "bootloader_core.h"
#include "bootloader_metadata.h"

#include <stddef.h>
#include <string.h>

#define BOOTLOADER_FOOTER_FW_TYPE_OFFSET 4u
#define BOOTLOADER_FOOTER_VERSION_OFFSET 5u
#define BOOTLOADER_FOOTER_FILE_SIZE_OFFSET 9u
#define BOOTLOADER_FOOTER_MODULE_OFFSET 29u
#define BOOTLOADER_FOOTER_CRC_OFFSET 30u
#define BOOTLOADER_FOOTER_IAP_TYPE 1u

static uint32_t read_u32_le(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] |
           ((uint32_t)p_data[1] << 8u) |
           ((uint32_t)p_data[2] << 16u) |
           ((uint32_t)p_data[3] << 24u);
}

static uint32_t footer_crc32_calculate(const uint8_t *p_data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFu; /* FRAME footer CRC32 accumulator. */
    uint32_t byte_index = 0u;   /* Footer byte being folded into the CRC. */
    uint8_t bit_index = 0u;     /* Reflected CRC bit iteration. */

    for (byte_index = 0u; byte_index < length; byte_index++)
    {
        crc ^= p_data[byte_index];
        for (bit_index = 0u; bit_index < 8u; bit_index++)
        {
            crc = ((crc & 1u) != 0u) ? ((crc >> 1u) ^ 0xEDB88320u) : (crc >> 1u);
        }
    }
    return crc;
}

static void footer_tail_update(bootloader_t *p_bootloader,
                               const uint8_t *p_data,
                               uint32_t length)
{
    uint32_t keep_length = 0u; /* Existing tail bytes retained before appending. */

    if (length >= BOOTLOADER_FOOTER_SIZE)
    {
        (void)memcpy(p_bootloader->footer_buffer,
                     &p_data[length - BOOTLOADER_FOOTER_SIZE],
                     BOOTLOADER_FOOTER_SIZE);
        p_bootloader->footer_length = BOOTLOADER_FOOTER_SIZE;
        return;
    }
    keep_length = (uint32_t)p_bootloader->footer_length;
    if ((keep_length + length) > BOOTLOADER_FOOTER_SIZE)
    {
        keep_length = BOOTLOADER_FOOTER_SIZE - length;
        (void)memmove(p_bootloader->footer_buffer,
                      &p_bootloader->footer_buffer[(uint32_t)p_bootloader->footer_length - keep_length],
                      keep_length);
    }
    (void)memcpy(&p_bootloader->footer_buffer[keep_length], p_data, length);
    p_bootloader->footer_length = (uint8_t)(keep_length + length);
}

static uint8_t footer_is_valid(const bootloader_t *p_bootloader)
{
    const uint8_t *p_footer = p_bootloader->footer_buffer; /* Complete wire-format footer. */
    const uint32_t image_size = p_bootloader->upgrade_info.file_size; /* Received package bytes. */
    const uint32_t footer_file_size =
        read_u32_le(&p_footer[BOOTLOADER_FOOTER_FILE_SIZE_OFFSET]); /* Footer-declared length. */
    const uint32_t footer_crc =
        read_u32_le(&p_footer[BOOTLOADER_FOOTER_CRC_OFFSET]); /* Stored footer CRC32. */
    const uint8_t size_valid =
        ((footer_file_size == image_size) ||
         ((image_size >= BOOTLOADER_FOOTER_SIZE) &&
          (footer_file_size == (image_size - BOOTLOADER_FOOTER_SIZE))))
            ? 1u
            : 0u;

    return ((p_bootloader->footer_length == BOOTLOADER_FOOTER_SIZE) &&
            (p_footer[BOOTLOADER_FOOTER_FW_TYPE_OFFSET] == BOOTLOADER_FOOTER_IAP_TYPE) &&
            (read_u32_le(&p_footer[BOOTLOADER_FOOTER_VERSION_OFFSET]) ==
             p_bootloader->upgrade_info.version) &&
            (size_valid == 1u) &&
            (p_footer[BOOTLOADER_FOOTER_MODULE_OFFSET] ==
             p_bootloader->upgrade_info.module_id) &&
            (footer_crc32_calculate(p_footer, BOOTLOADER_FOOTER_CRC_OFFSET) == footer_crc))
               ? 1u
               : 0u;
}

static uint8_t flash_ops_valid(const bootloader_flash_ops_t *p_ops)
{
    return ((p_ops != NULL) &&
            (p_ops->p_init != NULL) &&
            (p_ops->p_process != NULL) &&
            (p_ops->p_zone_info_get != NULL) &&
            (p_ops->p_read != NULL) &&
            (p_ops->p_write != NULL) &&
            (p_ops->p_erase != NULL) &&
            (p_ops->p_is_busy != NULL) &&
            (p_ops->p_result_get != NULL))
               ? 1u
               : 0u;
}

static uint8_t platform_ops_valid(const bootloader_platform_ops_t *p_ops)
{
    return ((p_ops != NULL) &&
            (p_ops->p_boot_reason_get != NULL) &&
            (p_ops->p_boot_reason_clear != NULL) &&
            (p_ops->p_image_header_is_valid != NULL) &&
            (p_ops->p_jump_to_iap != NULL))
               ? 1u
               : 0u;
}

static void resident_failure(bootloader_t *p_bootloader, bootloader_result_t result)
{
    p_bootloader->session_active = 0u;
    p_bootloader->pending_packet_offset = 0u;
    p_bootloader->pending_packet_length = 0u;
    p_bootloader->copy_chunk_length = 0u;
    p_bootloader->state = BOOTLOADER_STATE_WAIT_UPGRADE;
    p_bootloader->result = result;
}

static bootloader_result_t metadata_encode_current(bootloader_t *p_bootloader)
{
    const bootloader_metadata_t metadata = {
        .state = (bootloader_metadata_state_t)p_bootloader->metadata_state,
        .mode = p_bootloader->mode,
        .module_id = p_bootloader->upgrade_info.module_id,
        .retry_count = p_bootloader->metadata_retry_count,
        .sequence = p_bootloader->metadata_sequence + 1u,
        .version = p_bootloader->upgrade_info.version,
        .file_size = p_bootloader->upgrade_info.file_size,
        .expected_crc = p_bootloader->expected_crc,
        .received_length = p_bootloader->received_length,
        .running_crc = p_bootloader->running_crc,
        .copy_offset = p_bootloader->copy_offset,
        .error_code = 0u,
    };
    return bootloader_metadata_encode(&metadata,
                                      p_bootloader->config.p_copy_buffer,
                                      p_bootloader->config.copy_buffer_size);
}

static void metadata_commit_start(bootloader_t *p_bootloader,
                                  bootloader_metadata_state_t metadata_state,
                                  bootloader_state_t next_state)
{
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS;

    p_bootloader->metadata_state = (uint8_t)metadata_state;
    p_bootloader->metadata_next_state = next_state;
    p_bootloader->metadata_target_zone =
        (p_bootloader->metadata_source_zone == BOOTLOADER_FLASH_ZONE_META_A)
            ? BOOTLOADER_FLASH_ZONE_META_B
            : BOOTLOADER_FLASH_ZONE_META_A;
    result = metadata_encode_current(p_bootloader);
    if (result != BOOTLOADER_RESULT_SUCCESS)
    {
        resident_failure(p_bootloader, result);
        return;
    }
    p_bootloader->state = BOOTLOADER_STATE_METADATA_ERASE;
    p_bootloader->result = BOOTLOADER_RESULT_IN_PROGRESS;
}

static void metadata_read_submit(bootloader_t *p_bootloader,
                                 bootloader_flash_zone_t zone,
                                 uint32_t buffer_offset,
                                 bootloader_state_t wait_state)
{
    bootloader_result_t result = p_bootloader->flash_ops.p_read(
        p_bootloader->flash_ops.p_context,
        zone,
        0u,
        BOOTLOADER_METADATA_ENCODED_SIZE,
        &p_bootloader->config.p_copy_buffer[buffer_offset]);

    if ((result == BOOTLOADER_RESULT_SUCCESS) || (result == BOOTLOADER_RESULT_IN_PROGRESS))
    {
        p_bootloader->state = wait_state;
    }
    else
    {
        resident_failure(p_bootloader, result);
    }
}

static void metadata_startup_evaluate(bootloader_t *p_bootloader)
{
    bootloader_metadata_t metadata = {0};
    bootloader_flash_zone_t source = BOOTLOADER_FLASH_ZONE_META_A;
    const bootloader_result_t result = bootloader_metadata_select(
        p_bootloader->config.p_copy_buffer,
        &p_bootloader->config.p_copy_buffer[BOOTLOADER_METADATA_ENCODED_SIZE],
        &metadata,
        &source);

    if (result == BOOTLOADER_RESULT_SUCCESS)
    {
        p_bootloader->metadata_valid = 1u;
        p_bootloader->metadata_source_zone = source;
        p_bootloader->metadata_sequence = metadata.sequence;
        p_bootloader->metadata_state = (uint8_t)metadata.state;
        p_bootloader->metadata_retry_count = metadata.retry_count;
        p_bootloader->mode = metadata.mode;
        p_bootloader->upgrade_info.module_id = metadata.module_id;
        p_bootloader->upgrade_info.version = metadata.version;
        p_bootloader->upgrade_info.file_size = metadata.file_size;
        p_bootloader->received_length = metadata.received_length;
        p_bootloader->running_crc = metadata.running_crc;
        p_bootloader->expected_crc = metadata.expected_crc;
        p_bootloader->copy_offset = metadata.copy_offset;

        if (((metadata.state == BOOTLOADER_METADATA_STATE_INSTALL_PENDING) ||
             (metadata.state == BOOTLOADER_METADATA_STATE_COPYING)) &&
            (metadata.mode == BOOTLOADER_UPGRADE_MODE_STAGED) &&
            (metadata.file_size != 0u))
        {
            if (metadata.retry_count >= 3u)
            {
                resident_failure(p_bootloader, BOOTLOADER_RESULT_RECOVERY_REQUIRED);
                return;
            }
            p_bootloader->metadata_retry_count++;
            p_bootloader->copy_offset = 0u;
            metadata_commit_start(p_bootloader,
                                  BOOTLOADER_METADATA_STATE_COPYING,
                                  BOOTLOADER_STATE_COPY_ERASE);
            return;
        }
        if ((metadata.state == BOOTLOADER_METADATA_STATE_DOWNLOAD_DIRECT) ||
            (metadata.state == BOOTLOADER_METADATA_STATE_DOWNLOAD_STAGED) ||
            (metadata.state == BOOTLOADER_METADATA_STATE_FAILED))
        {
            resident_failure(p_bootloader, BOOTLOADER_RESULT_RECOVERY_REQUIRED);
            return;
        }
    }
    else
    {
        p_bootloader->metadata_valid = 0u;
        p_bootloader->metadata_source_zone = BOOTLOADER_FLASH_ZONE_META_B;
        p_bootloader->metadata_sequence = 0u;
    }

    if ((p_bootloader->boot_reason == BOOTLOADER_BOOT_REASON_IAP_REQUEST) ||
        (p_bootloader->boot_reason == BOOTLOADER_BOOT_REASON_RECOVERY))
    {
        p_bootloader->state = BOOTLOADER_STATE_WAIT_UPGRADE;
    }
    else
    {
        p_bootloader->state = BOOTLOADER_STATE_STARTUP_READ;
    }
    p_bootloader->result = BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t zone_requirements_check(bootloader_t *p_bootloader,
                                                    bootloader_flash_zone_t zone,
                                                    uint32_t required_size)
{
    bootloader_flash_zone_info_t info = {0}; /* Mounted logical zone geometry. */
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Geometry query result. */

    result = p_bootloader->flash_ops.p_zone_info_get(p_bootloader->flash_ops.p_context, zone, &info);
    if (result != BOOTLOADER_RESULT_SUCCESS)
    {
        return result;
    }
    if ((required_size > info.size) ||
        (info.readable == 0u) ||
        (info.writable == 0u) ||
        (info.erasable == 0u))
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }
    return BOOTLOADER_RESULT_SUCCESS;
}

static void startup_read_submit(bootloader_t *p_bootloader)
{
    bootloader_flash_zone_info_t info = {0}; /* IAP zone geometry used to bound the prefix read. */
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Flash submission result. */

    result = p_bootloader->flash_ops.p_zone_info_get(p_bootloader->flash_ops.p_context,
                                                     BOOTLOADER_FLASH_ZONE_IAP,
                                                     &info);
    if ((result != BOOTLOADER_RESULT_SUCCESS) ||
        (info.readable == 0u) ||
        (p_bootloader->config.image_header_length > info.size))
    {
        resident_failure(p_bootloader, BOOTLOADER_RESULT_CONFIG_ERROR);
        return;
    }
    result = p_bootloader->flash_ops.p_read(p_bootloader->flash_ops.p_context,
                                            BOOTLOADER_FLASH_ZONE_IAP,
                                            0u,
                                            p_bootloader->config.image_header_length,
                                            p_bootloader->config.p_packet_buffer);
    if (result == BOOTLOADER_RESULT_IN_PROGRESS)
    {
        p_bootloader->state = BOOTLOADER_STATE_STARTUP_WAIT;
    }
    else if (result == BOOTLOADER_RESULT_SUCCESS)
    {
        p_bootloader->state = BOOTLOADER_STATE_STARTUP_WAIT;
    }
    else
    {
        resident_failure(p_bootloader, result);
    }
}

static void installed_header_evaluate(bootloader_t *p_bootloader, uint8_t final_image)
{
    bootloader_flash_zone_info_t info = {0}; /* IAP zone geometry supplied to platform validation. */
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Platform validation result. */
    uint8_t valid = 0u; /* Normalized platform image validity. */
    uint32_t image_size = p_bootloader->upgrade_info.file_size; /* Installed or provisioned image size. */

    if (image_size == 0u)
    {
        result = p_bootloader->flash_ops.p_zone_info_get(p_bootloader->flash_ops.p_context,
                                                         BOOTLOADER_FLASH_ZONE_IAP,
                                                         &info);
        if (result != BOOTLOADER_RESULT_SUCCESS)
        {
            resident_failure(p_bootloader, result);
            return;
        }
        image_size = info.size;
        p_bootloader->upgrade_info.file_size = image_size;
    }

    result = p_bootloader->platform_ops.p_image_header_is_valid(
        p_bootloader->platform_ops.p_context,
        p_bootloader->config.p_packet_buffer,
        p_bootloader->config.image_header_length,
        image_size,
        &valid);
    if ((result != BOOTLOADER_RESULT_SUCCESS) || (valid == 0u))
    {
        resident_failure(p_bootloader, BOOTLOADER_RESULT_IMAGE_INVALID);
        return;
    }
    if (final_image != 0u)
    {
        p_bootloader->metadata_retry_count = 0u;
        metadata_commit_start(p_bootloader,
                              BOOTLOADER_METADATA_STATE_VALID,
                              BOOTLOADER_STATE_JUMP_PENDING);
    }
    else
    {
        p_bootloader->state = BOOTLOADER_STATE_JUMP_PENDING;
        p_bootloader->result = BOOTLOADER_RESULT_SUCCESS;
    }
}

static void final_read_submit(bootloader_t *p_bootloader)
{
    bootloader_result_t result = p_bootloader->flash_ops.p_read(
        p_bootloader->flash_ops.p_context,
        BOOTLOADER_FLASH_ZONE_IAP,
        0u,
        p_bootloader->config.image_header_length,
        p_bootloader->config.p_packet_buffer); /* Installed header read submission result. */

    if ((result == BOOTLOADER_RESULT_IN_PROGRESS) ||
        (result == BOOTLOADER_RESULT_SUCCESS))
    {
        p_bootloader->state = BOOTLOADER_STATE_FINAL_READ_WAIT;
    }
    else
    {
        resident_failure(p_bootloader, result);
    }
}

static void copy_read_submit(bootloader_t *p_bootloader)
{
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Staging read submission result. */
    uint32_t remaining = p_bootloader->upgrade_info.file_size - p_bootloader->copy_offset; /* Bytes left. */

    p_bootloader->copy_chunk_length = (remaining < p_bootloader->config.copy_buffer_size)
                                              ? remaining
                                              : p_bootloader->config.copy_buffer_size;
    result = p_bootloader->flash_ops.p_read(p_bootloader->flash_ops.p_context,
                                            BOOTLOADER_FLASH_ZONE_STAGING,
                                            p_bootloader->copy_offset,
                                            p_bootloader->copy_chunk_length,
                                            p_bootloader->config.p_copy_buffer);
    if ((result == BOOTLOADER_RESULT_IN_PROGRESS) ||
        (result == BOOTLOADER_RESULT_SUCCESS))
    {
        p_bootloader->state = BOOTLOADER_STATE_COPY_READ_WAIT;
    }
    else
    {
        resident_failure(p_bootloader, result);
    }
}

static void copy_verify_read_submit(bootloader_t *p_bootloader)
{
    const bootloader_result_t result = p_bootloader->flash_ops.p_read(
        p_bootloader->flash_ops.p_context,
        BOOTLOADER_FLASH_ZONE_IAP,
        p_bootloader->copy_offset,
        p_bootloader->copy_chunk_length,
        p_bootloader->config.p_copy_buffer); /* Copied-chunk readback submission result. */

    if ((result == BOOTLOADER_RESULT_IN_PROGRESS) ||
        (result == BOOTLOADER_RESULT_SUCCESS))
    {
        p_bootloader->state = BOOTLOADER_STATE_COPY_VERIFY_READ_WAIT;
    }
    else
    {
        resident_failure(p_bootloader, result);
    }
}

static void verify_read_submit(bootloader_t *p_bootloader)
{
    const uint32_t remaining =
        p_bootloader->upgrade_info.file_size - p_bootloader->verify_offset; /* Target bytes left. */
    const uint32_t chunk = (remaining < p_bootloader->config.copy_buffer_size)
                               ? remaining
                               : p_bootloader->config.copy_buffer_size; /* Next target read length. */
    const bootloader_result_t result = p_bootloader->flash_ops.p_read(
        p_bootloader->flash_ops.p_context,
        BOOTLOADER_FLASH_ZONE_IAP,
        p_bootloader->verify_offset,
        chunk,
        p_bootloader->config.p_copy_buffer); /* Complete-target read submission result. */

    if ((result == BOOTLOADER_RESULT_IN_PROGRESS) ||
        (result == BOOTLOADER_RESULT_SUCCESS))
    {
        p_bootloader->copy_chunk_length = chunk;
        p_bootloader->state = BOOTLOADER_STATE_VERIFY_READ_WAIT;
    }
    else
    {
        resident_failure(p_bootloader, result);
    }
}

static void storage_wait_handle(bootloader_t *p_bootloader)
{
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Completed storage operation result. */

    if (p_bootloader->flash_ops.p_is_busy(p_bootloader->flash_ops.p_context) == 1u)
    {
        return;
    }
    result = p_bootloader->flash_ops.p_result_get(p_bootloader->flash_ops.p_context);
    if (result != BOOTLOADER_RESULT_SUCCESS)
    {
        resident_failure(p_bootloader, result);
        return;
    }

    switch (p_bootloader->state)
    {
    case BOOTLOADER_STATE_META_A_WAIT:
        p_bootloader->state = BOOTLOADER_STATE_META_B_READ;
        break;
    case BOOTLOADER_STATE_META_B_WAIT:
        metadata_startup_evaluate(p_bootloader);
        break;
    case BOOTLOADER_STATE_STARTUP_WAIT:
        installed_header_evaluate(p_bootloader, 0u);
        break;
    case BOOTLOADER_STATE_DOWNLOAD_ERASE_WAIT:
        metadata_commit_start(
            p_bootloader,
            (p_bootloader->mode == BOOTLOADER_UPGRADE_MODE_STAGED)
                ? BOOTLOADER_METADATA_STATE_DOWNLOAD_STAGED
                : BOOTLOADER_METADATA_STATE_DOWNLOAD_DIRECT,
            BOOTLOADER_STATE_DOWNLOAD_READY);
        break;
    case BOOTLOADER_STATE_PACKET_WRITE_WAIT:
        footer_tail_update(p_bootloader,
                           p_bootloader->config.p_packet_buffer,
                           p_bootloader->pending_packet_length);
        p_bootloader->running_crc = p_bootloader->config.p_crc16_update(
            p_bootloader->config.p_packet_buffer,
            p_bootloader->pending_packet_length,
            p_bootloader->running_crc);
        p_bootloader->last_packet_offset = p_bootloader->pending_packet_offset;
        p_bootloader->last_packet_length = p_bootloader->pending_packet_length;
        p_bootloader->received_length += p_bootloader->pending_packet_length;
        p_bootloader->pending_packet_offset = 0u;
        p_bootloader->pending_packet_length = 0u;
        p_bootloader->state = BOOTLOADER_STATE_DOWNLOAD_READY;
        p_bootloader->result = BOOTLOADER_RESULT_SUCCESS;
        break;
    case BOOTLOADER_STATE_COPY_ERASE_WAIT:
        p_bootloader->copy_offset = 0u;
        p_bootloader->state = BOOTLOADER_STATE_COPY_READ;
        break;
    case BOOTLOADER_STATE_COPY_READ_WAIT:
        p_bootloader->copy_chunk_crc = p_bootloader->config.p_crc16_update(
            p_bootloader->config.p_copy_buffer,
            p_bootloader->copy_chunk_length,
            p_bootloader->config.p_crc16_init());
        p_bootloader->state = BOOTLOADER_STATE_COPY_WRITE;
        break;
    case BOOTLOADER_STATE_COPY_WRITE_WAIT:
        p_bootloader->state = BOOTLOADER_STATE_COPY_VERIFY_READ;
        break;
    case BOOTLOADER_STATE_COPY_VERIFY_READ_WAIT:
        if (p_bootloader->config.p_crc16_update(
                p_bootloader->config.p_copy_buffer,
                p_bootloader->copy_chunk_length,
                p_bootloader->config.p_crc16_init()) != p_bootloader->copy_chunk_crc)
        {
            resident_failure(p_bootloader, BOOTLOADER_RESULT_IMAGE_INVALID);
            break;
        }
        p_bootloader->copy_offset += p_bootloader->copy_chunk_length;
        p_bootloader->copy_chunk_length = 0u;
        if (p_bootloader->copy_offset >= p_bootloader->upgrade_info.file_size)
        {
            p_bootloader->verify_offset = 0u;
            p_bootloader->verify_crc = p_bootloader->config.p_crc16_init();
            metadata_commit_start(p_bootloader,
                                  BOOTLOADER_METADATA_STATE_COPYING,
                                  BOOTLOADER_STATE_VERIFY_READ);
        }
        else
        {
            metadata_commit_start(p_bootloader,
                                  BOOTLOADER_METADATA_STATE_COPYING,
                                  BOOTLOADER_STATE_COPY_READ);
        }
        break;
    case BOOTLOADER_STATE_VERIFY_READ_WAIT:
        p_bootloader->verify_crc = p_bootloader->config.p_crc16_update(
            p_bootloader->config.p_copy_buffer,
            p_bootloader->copy_chunk_length,
            p_bootloader->verify_crc);
        p_bootloader->verify_offset += p_bootloader->copy_chunk_length;
        p_bootloader->copy_chunk_length = 0u;
        if (p_bootloader->verify_offset >= p_bootloader->upgrade_info.file_size)
        {
            if (p_bootloader->verify_crc != p_bootloader->expected_crc)
            {
                resident_failure(p_bootloader, BOOTLOADER_RESULT_IMAGE_INVALID);
            }
            else
            {
                p_bootloader->state = BOOTLOADER_STATE_FINAL_READ;
            }
        }
        else
        {
            p_bootloader->state = BOOTLOADER_STATE_VERIFY_READ;
        }
        break;
    case BOOTLOADER_STATE_FINAL_READ_WAIT:
        installed_header_evaluate(p_bootloader, 1u);
        break;
    case BOOTLOADER_STATE_METADATA_ERASE_WAIT:
        p_bootloader->state = BOOTLOADER_STATE_METADATA_WRITE;
        break;
    case BOOTLOADER_STATE_METADATA_WRITE_WAIT:
        p_bootloader->metadata_sequence++;
        p_bootloader->metadata_source_zone = p_bootloader->metadata_target_zone;
        p_bootloader->metadata_valid = 1u;
        p_bootloader->state = p_bootloader->metadata_next_state;
        p_bootloader->result = (p_bootloader->state == BOOTLOADER_STATE_DOWNLOAD_READY)
                                   ? BOOTLOADER_RESULT_SUCCESS
                                   : BOOTLOADER_RESULT_IN_PROGRESS;
        break;
    default:
        resident_failure(p_bootloader, BOOTLOADER_RESULT_CONFIG_ERROR);
        break;
    }
}

bootloader_result_t bootloader_init(bootloader_t *p_bootloader,
                                    const bootloader_config_t *p_config,
                                    const bootloader_flash_ops_t *p_flash_ops,
                                    const bootloader_platform_ops_t *p_platform_ops)
{
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Mounted storage initialization result. */
    bootloader_boot_reason_t reason = BOOTLOADER_BOOT_REASON_POWER_ON; /* Startup reason. */

    if (p_bootloader == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    (void)memset(p_bootloader, 0, sizeof(*p_bootloader));
    p_bootloader->state = BOOTLOADER_STATE_UNINITIALIZED;
    p_bootloader->result = BOOTLOADER_RESULT_CONFIG_ERROR;

    if ((p_config == NULL) ||
        (flash_ops_valid(p_flash_ops) == 0u) ||
        (platform_ops_valid(p_platform_ops) == 0u) ||
        (p_config->p_packet_buffer == NULL) ||
        (p_config->packet_buffer_size < BOOTLOADER_PACKET_DATA_SIZE) ||
        (p_config->p_copy_buffer == NULL) ||
        (p_config->copy_buffer_size < (2u * BOOTLOADER_METADATA_ENCODED_SIZE)) ||
        (p_config->image_header_length == 0u) ||
        (p_config->image_header_length > p_config->packet_buffer_size) ||
        (p_config->p_crc16_init == NULL) ||
        (p_config->p_crc16_update == NULL) ||
        (p_config->default_mode > BOOTLOADER_UPGRADE_MODE_STAGED))
    {
        p_bootloader->state = BOOTLOADER_STATE_CONFIG_ERROR;
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }

    p_bootloader->config = *p_config;
    p_bootloader->flash_ops = *p_flash_ops;
    p_bootloader->platform_ops = *p_platform_ops;
    p_bootloader->mode = p_config->default_mode;
    result = p_bootloader->flash_ops.p_init(p_bootloader->flash_ops.p_context);
    if (result != BOOTLOADER_RESULT_SUCCESS)
    {
        p_bootloader->state = BOOTLOADER_STATE_CONFIG_ERROR;
        p_bootloader->result = result;
        return result;
    }

    reason = p_bootloader->platform_ops.p_boot_reason_get(p_bootloader->platform_ops.p_context);
    p_bootloader->boot_reason = reason;
    p_bootloader->metadata_source_zone = BOOTLOADER_FLASH_ZONE_META_B;
    p_bootloader->state = BOOTLOADER_STATE_META_A_READ;
    p_bootloader->result = BOOTLOADER_RESULT_SUCCESS;
    return BOOTLOADER_RESULT_SUCCESS;
}

void bootloader_process(bootloader_t *p_bootloader)
{
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Storage or jump operation result. */

    if (p_bootloader == NULL)
    {
        return;
    }
    if (p_bootloader->platform_ops.p_watchdog_kick != NULL)
    {
        p_bootloader->platform_ops.p_watchdog_kick(p_bootloader->platform_ops.p_context);
    }
    p_bootloader->flash_ops.p_process(p_bootloader->flash_ops.p_context);

    switch (p_bootloader->state)
    {
    case BOOTLOADER_STATE_META_A_READ:
        metadata_read_submit(p_bootloader,
                             BOOTLOADER_FLASH_ZONE_META_A,
                             0u,
                             BOOTLOADER_STATE_META_A_WAIT);
        break;
    case BOOTLOADER_STATE_META_B_READ:
        metadata_read_submit(p_bootloader,
                             BOOTLOADER_FLASH_ZONE_META_B,
                             BOOTLOADER_METADATA_ENCODED_SIZE,
                             BOOTLOADER_STATE_META_B_WAIT);
        break;
    case BOOTLOADER_STATE_STARTUP_READ:
        startup_read_submit(p_bootloader);
        break;
    case BOOTLOADER_STATE_META_A_WAIT:
    case BOOTLOADER_STATE_META_B_WAIT:
    case BOOTLOADER_STATE_STARTUP_WAIT:
    case BOOTLOADER_STATE_DOWNLOAD_ERASE_WAIT:
    case BOOTLOADER_STATE_PACKET_WRITE_WAIT:
    case BOOTLOADER_STATE_COPY_ERASE_WAIT:
    case BOOTLOADER_STATE_COPY_READ_WAIT:
    case BOOTLOADER_STATE_COPY_WRITE_WAIT:
    case BOOTLOADER_STATE_COPY_VERIFY_READ_WAIT:
    case BOOTLOADER_STATE_VERIFY_READ_WAIT:
    case BOOTLOADER_STATE_FINAL_READ_WAIT:
    case BOOTLOADER_STATE_METADATA_ERASE_WAIT:
    case BOOTLOADER_STATE_METADATA_WRITE_WAIT:
        storage_wait_handle(p_bootloader);
        break;
    case BOOTLOADER_STATE_DOWNLOAD_ERASE:
        result = p_bootloader->flash_ops.p_erase(p_bootloader->flash_ops.p_context,
                                                 p_bootloader->download_zone,
                                                 0u,
                                                 p_bootloader->upgrade_info.file_size);
        if ((result == BOOTLOADER_RESULT_IN_PROGRESS) ||
            (result == BOOTLOADER_RESULT_SUCCESS))
        {
            p_bootloader->state = BOOTLOADER_STATE_DOWNLOAD_ERASE_WAIT;
        }
        else
        {
            resident_failure(p_bootloader, result);
        }
        break;
    case BOOTLOADER_STATE_COPY_ERASE:
        result = p_bootloader->flash_ops.p_erase(p_bootloader->flash_ops.p_context,
                                                 BOOTLOADER_FLASH_ZONE_IAP,
                                                 0u,
                                                 p_bootloader->upgrade_info.file_size);
        if ((result == BOOTLOADER_RESULT_IN_PROGRESS) ||
            (result == BOOTLOADER_RESULT_SUCCESS))
        {
            p_bootloader->state = BOOTLOADER_STATE_COPY_ERASE_WAIT;
        }
        else
        {
            resident_failure(p_bootloader, result);
        }
        break;
    case BOOTLOADER_STATE_COPY_READ:
        copy_read_submit(p_bootloader);
        break;
    case BOOTLOADER_STATE_COPY_WRITE:
        result = p_bootloader->flash_ops.p_write(p_bootloader->flash_ops.p_context,
                                                 BOOTLOADER_FLASH_ZONE_IAP,
                                                 p_bootloader->copy_offset,
                                                 p_bootloader->copy_chunk_length,
                                                 p_bootloader->config.p_copy_buffer);
        if ((result == BOOTLOADER_RESULT_IN_PROGRESS) ||
            (result == BOOTLOADER_RESULT_SUCCESS))
        {
            p_bootloader->state = BOOTLOADER_STATE_COPY_WRITE_WAIT;
        }
        else
        {
            resident_failure(p_bootloader, result);
        }
        break;
    case BOOTLOADER_STATE_COPY_VERIFY_READ:
        copy_verify_read_submit(p_bootloader);
        break;
    case BOOTLOADER_STATE_VERIFY_READ:
        verify_read_submit(p_bootloader);
        break;
    case BOOTLOADER_STATE_FINAL_READ:
        final_read_submit(p_bootloader);
        break;
    case BOOTLOADER_STATE_METADATA_ERASE:
        result = p_bootloader->flash_ops.p_erase(p_bootloader->flash_ops.p_context,
                                                 p_bootloader->metadata_target_zone,
                                                 0u,
                                                 BOOTLOADER_METADATA_ENCODED_SIZE);
        if ((result == BOOTLOADER_RESULT_SUCCESS) || (result == BOOTLOADER_RESULT_IN_PROGRESS))
        {
            p_bootloader->state = BOOTLOADER_STATE_METADATA_ERASE_WAIT;
        }
        else
        {
            resident_failure(p_bootloader, result);
        }
        break;
    case BOOTLOADER_STATE_METADATA_WRITE:
        result = p_bootloader->flash_ops.p_write(p_bootloader->flash_ops.p_context,
                                                 p_bootloader->metadata_target_zone,
                                                 0u,
                                                 BOOTLOADER_METADATA_ENCODED_SIZE,
                                                 p_bootloader->config.p_copy_buffer);
        if ((result == BOOTLOADER_RESULT_SUCCESS) || (result == BOOTLOADER_RESULT_IN_PROGRESS))
        {
            p_bootloader->state = BOOTLOADER_STATE_METADATA_WRITE_WAIT;
        }
        else
        {
            resident_failure(p_bootloader, result);
        }
        break;
    case BOOTLOADER_STATE_JUMP_PENDING:
        if (p_bootloader->jump_called == 0u)
        {
            result = p_bootloader->platform_ops.p_boot_reason_clear(p_bootloader->platform_ops.p_context);
            if (result == BOOTLOADER_RESULT_SUCCESS)
            {
                p_bootloader->jump_called = 1u;
                result = p_bootloader->platform_ops.p_jump_to_iap(p_bootloader->platform_ops.p_context);
            }
            if (result != BOOTLOADER_RESULT_SUCCESS)
            {
                resident_failure(p_bootloader, result);
            }
        }
        break;
    case BOOTLOADER_STATE_UNINITIALIZED:
    case BOOTLOADER_STATE_WAIT_UPGRADE:
    case BOOTLOADER_STATE_DOWNLOAD_READY:
    case BOOTLOADER_STATE_CONFIG_ERROR:
    default:
        break;
    }
}

bootloader_result_t bootloader_upgrade_begin(bootloader_t *p_bootloader,
                                             const bootloader_upgrade_info_t *p_info,
                                             bootloader_upgrade_mode_t mode)
{
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Zone capacity validation result. */

    if ((p_bootloader == NULL) || (p_info == NULL))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    if ((p_bootloader->state != BOOTLOADER_STATE_WAIT_UPGRADE) ||
        (p_bootloader->flash_ops.p_is_busy(p_bootloader->flash_ops.p_context) == 1u))
    {
        return BOOTLOADER_RESULT_BUSY;
    }
    if ((p_info->module_id != p_bootloader->config.expected_module_id) ||
        (p_info->file_size == 0u) ||
        (mode > BOOTLOADER_UPGRADE_MODE_STAGED))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }

    result = zone_requirements_check(p_bootloader, BOOTLOADER_FLASH_ZONE_IAP, p_info->file_size);
    if (result != BOOTLOADER_RESULT_SUCCESS)
    {
        return result;
    }
    if (mode == BOOTLOADER_UPGRADE_MODE_STAGED)
    {
        result = zone_requirements_check(p_bootloader,
                                         BOOTLOADER_FLASH_ZONE_STAGING,
                                         p_info->file_size);
        if (result != BOOTLOADER_RESULT_SUCCESS)
        {
            return result;
        }
    }

    p_bootloader->upgrade_info = *p_info;
    p_bootloader->mode = mode;
    p_bootloader->download_zone = (mode == BOOTLOADER_UPGRADE_MODE_STAGED)
                                              ? BOOTLOADER_FLASH_ZONE_STAGING
                                              : BOOTLOADER_FLASH_ZONE_IAP;
    p_bootloader->received_length = 0u;
    p_bootloader->pending_packet_offset = 0u;
    p_bootloader->pending_packet_length = 0u;
    p_bootloader->last_packet_offset = 0u;
    p_bootloader->last_packet_length = 0u;
    p_bootloader->copy_offset = 0u;
    p_bootloader->copy_chunk_length = 0u;
    p_bootloader->verify_offset = 0u;
    p_bootloader->copy_chunk_crc = 0u;
    p_bootloader->verify_crc = p_bootloader->config.p_crc16_init();
    p_bootloader->running_crc = p_bootloader->config.p_crc16_init();
    p_bootloader->expected_crc = 0u;
    p_bootloader->footer_length = 0u;
    (void)memset(p_bootloader->footer_buffer, 0, sizeof(p_bootloader->footer_buffer));
    p_bootloader->session_active = 1u;
    p_bootloader->jump_called = 0u;
    p_bootloader->metadata_retry_count = 0u;
    metadata_commit_start(
        p_bootloader,
        (mode == BOOTLOADER_UPGRADE_MODE_STAGED)
            ? BOOTLOADER_METADATA_STATE_DOWNLOAD_STAGED
            : BOOTLOADER_METADATA_STATE_DOWNLOAD_DIRECT,
        BOOTLOADER_STATE_DOWNLOAD_ERASE);
    return BOOTLOADER_RESULT_SUCCESS;
}

bootloader_result_t bootloader_packet_submit(bootloader_t *p_bootloader,
                                             uint32_t offset,
                                             const uint8_t *p_data,
                                             uint32_t length)
{
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS; /* Packet write submission result. */

    if ((p_bootloader == NULL) ||
        (p_data == NULL) ||
        (length == 0u) ||
        (length > BOOTLOADER_PACKET_DATA_SIZE))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    if ((p_bootloader->state != BOOTLOADER_STATE_DOWNLOAD_READY) ||
        (p_bootloader->session_active == 0u))
    {
        return BOOTLOADER_RESULT_BUSY;
    }
    if ((p_bootloader->last_packet_length != 0u) &&
        (offset == p_bootloader->last_packet_offset) &&
        (length == p_bootloader->last_packet_length) &&
        (memcmp(p_bootloader->config.p_packet_buffer, p_data, length) == 0))
    {
        return BOOTLOADER_RESULT_SUCCESS;
    }
    if ((offset != p_bootloader->received_length) ||
        (length > (p_bootloader->upgrade_info.file_size - p_bootloader->received_length)))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR;
    }

    (void)memcpy(p_bootloader->config.p_packet_buffer, p_data, length);
    result = p_bootloader->flash_ops.p_write(p_bootloader->flash_ops.p_context,
                                             p_bootloader->download_zone,
                                             offset,
                                             length,
                                             p_bootloader->config.p_packet_buffer);
    if ((result != BOOTLOADER_RESULT_IN_PROGRESS) &&
        (result != BOOTLOADER_RESULT_SUCCESS))
    {
        resident_failure(p_bootloader, result);
        return result;
    }
    p_bootloader->pending_packet_offset = offset;
    p_bootloader->pending_packet_length = length;
    p_bootloader->state = BOOTLOADER_STATE_PACKET_WRITE_WAIT;
    p_bootloader->result = BOOTLOADER_RESULT_IN_PROGRESS;
    return BOOTLOADER_RESULT_SUCCESS;
}

bootloader_result_t bootloader_upgrade_end(bootloader_t *p_bootloader, uint16_t expected_crc)
{
    if (p_bootloader == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    if ((p_bootloader->state != BOOTLOADER_STATE_DOWNLOAD_READY) ||
        (p_bootloader->session_active == 0u))
    {
        return BOOTLOADER_RESULT_BUSY;
    }
    if ((p_bootloader->received_length != p_bootloader->upgrade_info.file_size) ||
        (p_bootloader->running_crc != expected_crc) ||
        (footer_is_valid(p_bootloader) == 0u))
    {
        resident_failure(p_bootloader, BOOTLOADER_RESULT_IMAGE_INVALID);
        return BOOTLOADER_RESULT_IMAGE_INVALID;
    }

    p_bootloader->expected_crc = expected_crc;
    p_bootloader->session_active = 0u;
    if (p_bootloader->mode == BOOTLOADER_UPGRADE_MODE_STAGED)
    {
        metadata_commit_start(p_bootloader,
                              BOOTLOADER_METADATA_STATE_INSTALL_PENDING,
                              BOOTLOADER_STATE_COPY_ERASE);
    }
    else
    {
        p_bootloader->verify_offset = 0u;
        p_bootloader->verify_crc = p_bootloader->config.p_crc16_init();
        p_bootloader->state = BOOTLOADER_STATE_VERIFY_READ;
    }
    p_bootloader->result = BOOTLOADER_RESULT_IN_PROGRESS;
    return BOOTLOADER_RESULT_SUCCESS;
}

uint8_t bootloader_is_download_ready(const bootloader_t *p_bootloader)
{
    return ((p_bootloader != NULL) &&
            (p_bootloader->state == BOOTLOADER_STATE_DOWNLOAD_READY))
               ? 1u
               : 0u;
}

bootloader_state_t bootloader_state_get(const bootloader_t *p_bootloader)
{
    return (p_bootloader == NULL) ? BOOTLOADER_STATE_CONFIG_ERROR : p_bootloader->state;
}

bootloader_result_t bootloader_result_get(const bootloader_t *p_bootloader)
{
    return (p_bootloader == NULL) ? BOOTLOADER_RESULT_INVALID_ARGUMENT : p_bootloader->result;
}
