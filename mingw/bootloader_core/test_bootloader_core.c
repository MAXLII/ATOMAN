// SPDX-License-Identifier: MIT
/**
 * @file    test_bootloader_core.c
 * @brief   Host verification for the bootloader core and FAL adapter.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Verify safe startup decisions without target hardware
 *          - Exercise direct and staged firmware installation state transitions
 *          - Verify Bootloader-to-FAL mapping and result translation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Test-only, single-threaded implementation
 *          - Hardware and storage are replaced by deterministic in-memory fakes
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
#include "bootloader_fal_adapter.h"
#include "bootloader_metadata.h"
#include "bootloader_protocol.h"
#include "iap_boot_service.h"

#include <stdio.h>
#include <string.h>

#define TEST_ZONE_SIZE 4096u
#define TEST_HEADER_SIZE 8u
#define TEST_PROCESS_LIMIT 2000u

typedef struct
{
    uint8_t zones[BOOTLOADER_FLASH_ZONE_COUNT][TEST_ZONE_SIZE];
    bootloader_result_t result;
    uint32_t write_count;
    uint32_t iap_read_count;
    uint8_t fail_next_write;
    uint8_t corrupt_next_iap_read;
} fake_storage_t;

typedef struct
{
    bootloader_boot_reason_t reason;
    uint32_t jump_count;
    uint32_t clear_count;
    uint32_t watchdog_count;
} fake_platform_t;

static FILE *g_log_file;
static uint32_t g_pass_count;
static uint32_t g_fail_count;
static fal_zone_id_t g_fake_fal_last_zone;

static void check_result(uint8_t condition, const char *p_name)
{
    const char *p_status = (condition != 0u) ? "PASS" : "FAIL";

    (void)printf("[%s] %s\n", p_status, p_name);
    if (g_log_file != NULL)
    {
        (void)fprintf(g_log_file, "[%s] %s\n", p_status, p_name);
    }
    if (condition != 0u)
    {
        g_pass_count++;
    }
    else
    {
        g_fail_count++;
    }
}

static bootloader_result_t fake_storage_init(void *p_context)
{
    fake_storage_t *p_storage = (fake_storage_t *)p_context;

    p_storage->result = BOOTLOADER_RESULT_SUCCESS;
    return BOOTLOADER_RESULT_SUCCESS;
}

static void fake_storage_process(void *p_context)
{
    (void)p_context;
}

static bootloader_result_t fake_zone_info(void *p_context,
                                          bootloader_flash_zone_t zone,
                                          bootloader_flash_zone_info_t *p_info)
{
    (void)p_context;
    if ((zone >= BOOTLOADER_FLASH_ZONE_COUNT) || (p_info == NULL))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    *p_info = (bootloader_flash_zone_info_t){
        .size = TEST_ZONE_SIZE,
        .program_page_size = 256u,
        .erase_block_size = 256u,
        .readable = 1u,
        .writable = (uint8_t)((zone == BOOTLOADER_FLASH_ZONE_LAYOUT) ? 0u : 1u),
        .erasable = (uint8_t)((zone == BOOTLOADER_FLASH_ZONE_LAYOUT) ? 0u : 1u),
    };
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t fake_read(void *p_context,
                                     bootloader_flash_zone_t zone,
                                     uint32_t offset,
                                     uint32_t length,
                                     uint8_t *p_data)
{
    fake_storage_t *p_storage = (fake_storage_t *)p_context;

    if ((zone >= BOOTLOADER_FLASH_ZONE_COUNT) || (p_data == NULL) ||
        (offset > TEST_ZONE_SIZE) || (length > (TEST_ZONE_SIZE - offset)))
    {
        return BOOTLOADER_RESULT_OUT_OF_RANGE;
    }
    (void)memcpy(p_data, &p_storage->zones[zone][offset], length);
    if (zone == BOOTLOADER_FLASH_ZONE_IAP)
    {
        p_storage->iap_read_count++;
        if ((p_storage->corrupt_next_iap_read != 0u) && (length != 0u))
        {
            p_data[0] ^= 1u;
            p_storage->corrupt_next_iap_read = 0u;
        }
    }
    p_storage->result = BOOTLOADER_RESULT_SUCCESS;
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t fake_write(void *p_context,
                                      bootloader_flash_zone_t zone,
                                      uint32_t offset,
                                      uint32_t length,
                                      const uint8_t *p_data)
{
    fake_storage_t *p_storage = (fake_storage_t *)p_context;

    if (p_storage->fail_next_write != 0u)
    {
        p_storage->fail_next_write = 0u;
        p_storage->result = BOOTLOADER_RESULT_STORAGE_ERROR;
        return BOOTLOADER_RESULT_STORAGE_ERROR;
    }
    if ((zone >= BOOTLOADER_FLASH_ZONE_COUNT) || (p_data == NULL) ||
        (offset > TEST_ZONE_SIZE) || (length > (TEST_ZONE_SIZE - offset)))
    {
        return BOOTLOADER_RESULT_OUT_OF_RANGE;
    }
    (void)memcpy(&p_storage->zones[zone][offset], p_data, length);
    p_storage->write_count++;
    p_storage->result = BOOTLOADER_RESULT_SUCCESS;
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t fake_erase(void *p_context,
                                      bootloader_flash_zone_t zone,
                                      uint32_t offset,
                                      uint32_t length)
{
    fake_storage_t *p_storage = (fake_storage_t *)p_context;

    if ((zone >= BOOTLOADER_FLASH_ZONE_COUNT) ||
        (offset > TEST_ZONE_SIZE) || (length > (TEST_ZONE_SIZE - offset)))
    {
        return BOOTLOADER_RESULT_OUT_OF_RANGE;
    }
    (void)memset(&p_storage->zones[zone][offset], 0xFF, length);
    p_storage->result = BOOTLOADER_RESULT_SUCCESS;
    return BOOTLOADER_RESULT_SUCCESS;
}

static uint8_t fake_is_busy(void *p_context)
{
    (void)p_context;
    return 0u;
}

static bootloader_result_t fake_result_get(void *p_context)
{
    return ((fake_storage_t *)p_context)->result;
}

static bootloader_boot_reason_t fake_reason_get(void *p_context)
{
    return ((fake_platform_t *)p_context)->reason;
}

static bootloader_result_t fake_reason_clear(void *p_context)
{
    ((fake_platform_t *)p_context)->clear_count++;
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t fake_header_valid(void *p_context,
                                              const uint8_t *p_header,
                                              uint32_t header_length,
                                              uint32_t image_size,
                                              uint8_t *p_valid)
{
    (void)p_context;
    (void)image_size;
    if ((p_header == NULL) || (p_valid == NULL) || (header_length < 2u))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    *p_valid = ((p_header[0] == 0x5Au) && (p_header[1] == 0xA5u)) ? 1u : 0u;
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t fake_jump(void *p_context)
{
    ((fake_platform_t *)p_context)->jump_count++;
    return BOOTLOADER_RESULT_SUCCESS;
}

static void fake_watchdog(void *p_context)
{
    ((fake_platform_t *)p_context)->watchdog_count++;
}

static uint16_t test_crc_init(void)
{
    return 0xFFFFu;
}

static uint16_t test_crc_update(const uint8_t *p_data, uint32_t length, uint16_t crc)
{
    uint32_t index = 0u;

    for (index = 0u; index < length; index++)
    {
        crc = (uint16_t)(crc ^ (uint16_t)p_data[index]);
        crc = (uint16_t)((crc << 1u) | (crc >> 15u));
    }
    return crc;
}

static uint16_t test_crc_calculate(const uint8_t *p_data, uint32_t length)
{
    return test_crc_update(p_data, length, test_crc_init());
}

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

static uint32_t test_footer_crc32(const uint8_t *p_data, uint32_t length)
{
    uint32_t crc = 0xFFFFFFFFu; /* Footer CRC32 accumulator. */
    uint32_t byte_index = 0u;   /* Input byte index. */
    uint8_t bit_index = 0u;     /* Reflected CRC bit index. */

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

static void valid_footer_set(uint8_t *p_image,
                             uint32_t image_size,
                             uint32_t version,
                             uint8_t module_id)
{
    uint8_t *p_footer = &p_image[image_size - BOOTLOADER_FOOTER_SIZE];

    (void)memset(p_footer, 0, BOOTLOADER_FOOTER_SIZE);
    p_footer[4] = 1u;
    write_u32_le(&p_footer[5], version);
    write_u32_le(&p_footer[9], image_size);
    (void)memcpy(&p_footer[13], "test-commit", 11u);
    p_footer[29] = module_id;
    write_u32_le(&p_footer[30], test_footer_crc32(p_footer, 30u));
}

static bootloader_flash_ops_t storage_ops_make(fake_storage_t *p_storage)
{
    return (bootloader_flash_ops_t){
        .p_context = p_storage,
        .p_init = fake_storage_init,
        .p_process = fake_storage_process,
        .p_zone_info_get = fake_zone_info,
        .p_read = fake_read,
        .p_write = fake_write,
        .p_erase = fake_erase,
        .p_is_busy = fake_is_busy,
        .p_result_get = fake_result_get,
    };
}

static bootloader_platform_ops_t platform_ops_make(fake_platform_t *p_platform)
{
    return (bootloader_platform_ops_t){
        .p_context = p_platform,
        .p_boot_reason_get = fake_reason_get,
        .p_boot_reason_clear = fake_reason_clear,
        .p_image_header_is_valid = fake_header_valid,
        .p_jump_to_iap = fake_jump,
        .p_watchdog_kick = fake_watchdog,
    };
}

static bootloader_result_t bootloader_make(bootloader_t *p_bootloader,
                                            fake_storage_t *p_storage,
                                            fake_platform_t *p_platform,
                                            uint8_t *p_packet,
                                            uint8_t *p_copy)
{
    const bootloader_config_t config = {
        .expected_module_id = 2u,
        .default_mode = BOOTLOADER_UPGRADE_MODE_STAGED,
        .image_header_length = TEST_HEADER_SIZE,
        .p_packet_buffer = p_packet,
        .packet_buffer_size = BOOTLOADER_PACKET_DATA_SIZE,
        .p_copy_buffer = p_copy,
        .copy_buffer_size = 300u,
        .p_crc16_init = test_crc_init,
        .p_crc16_update = test_crc_update,
    };
    const bootloader_flash_ops_t flash_ops = storage_ops_make(p_storage);
    const bootloader_platform_ops_t platform_ops = platform_ops_make(p_platform);

    return bootloader_init(p_bootloader, &config, &flash_ops, &platform_ops);
}

static void process_until(bootloader_t *p_bootloader, bootloader_state_t state)
{
    uint32_t count = 0u;

    while ((bootloader_state_get(p_bootloader) != state) && (count < TEST_PROCESS_LIMIT))
    {
        bootloader_process(p_bootloader);
        count++;
    }
}

static void test_startup_paths(void)
{
    bootloader_t bootloader;
    fake_storage_t storage;
    fake_platform_t platform;
    uint8_t packet[BOOTLOADER_PACKET_DATA_SIZE];
    uint8_t copy[300];

    (void)memset(&storage, 0, sizeof(storage));
    (void)memset(storage.zones, 0xFF, sizeof(storage.zones));
    (void)memset(&platform, 0, sizeof(platform));
    storage.result = BOOTLOADER_RESULT_SUCCESS;
    check_result(bootloader_make(&bootloader, &storage, &platform, packet, copy) == BOOTLOADER_RESULT_SUCCESS,
                 "blank image initialization succeeds");
    process_until(&bootloader, BOOTLOADER_STATE_WAIT_UPGRADE);
    check_result((bootloader_state_get(&bootloader) == BOOTLOADER_STATE_WAIT_UPGRADE) &&
                     (platform.jump_count == 0u),
                 "blank image remains in bootloader");

    storage.zones[BOOTLOADER_FLASH_ZONE_IAP][0] = 0x5Au;
    storage.zones[BOOTLOADER_FLASH_ZONE_IAP][1] = 0xA5u;
    (void)memset(&platform, 0, sizeof(platform));
    check_result(bootloader_make(&bootloader, &storage, &platform, packet, copy) == BOOTLOADER_RESULT_SUCCESS,
                 "valid image initialization succeeds");
    process_until(&bootloader, BOOTLOADER_STATE_JUMP_PENDING);
    bootloader_process(&bootloader);
    check_result((platform.jump_count == 1u) && (platform.clear_count == 1u),
                 "valid image launches exactly once");

    platform.reason = BOOTLOADER_BOOT_REASON_IAP_REQUEST;
    platform.jump_count = 0u;
    check_result(bootloader_make(&bootloader, &storage, &platform, packet, copy) == BOOTLOADER_RESULT_SUCCESS,
                 "IAP request initialization succeeds");
    process_until(&bootloader, BOOTLOADER_STATE_WAIT_UPGRADE);
    check_result((bootloader_state_get(&bootloader) == BOOTLOADER_STATE_WAIT_UPGRADE) &&
                     (platform.jump_count == 0u),
                 "IAP request suppresses automatic launch");
}

static void test_staged_upgrade(void)
{
    bootloader_t bootloader;
    fake_storage_t storage;
    fake_platform_t platform = {.reason = BOOTLOADER_BOOT_REASON_IAP_REQUEST};
    bootloader_upgrade_info_t info = {.module_id = 2u, .file_size = 1040u, .version = 0x010203u, .update_type = 0u};
    uint8_t packet_buffer[BOOTLOADER_PACKET_DATA_SIZE];
    uint8_t copy_buffer[300];
    uint8_t image[1040];
    uint32_t offset = 0u;
    uint32_t length = 0u;
    uint16_t crc = test_crc_init();
    uint32_t writes_before_duplicate = 0u;

    (void)memset(&storage, 0, sizeof(storage));
    (void)memset(storage.zones, 0xFF, sizeof(storage.zones));
    storage.result = BOOTLOADER_RESULT_SUCCESS;
    for (offset = 0u; offset < (uint32_t)sizeof(image); offset++)
    {
        image[offset] = (uint8_t)(offset & 0xFFu);
    }
    image[0] = 0x5Au;
    image[1] = 0xA5u;
    valid_footer_set(image, (uint32_t)sizeof(image), info.version, info.module_id);
    crc = test_crc_update(image, (uint32_t)sizeof(image), crc);
    check_result(bootloader_make(&bootloader, &storage, &platform, packet_buffer, copy_buffer) == BOOTLOADER_RESULT_SUCCESS,
                 "staged upgrade initialization succeeds");
    process_until(&bootloader, BOOTLOADER_STATE_WAIT_UPGRADE);
    check_result(bootloader_upgrade_begin(&bootloader, &info, BOOTLOADER_UPGRADE_MODE_STAGED) == BOOTLOADER_RESULT_SUCCESS,
                 "staged upgrade session starts");
    process_until(&bootloader, BOOTLOADER_STATE_DOWNLOAD_READY);

    offset = 0u;
    while (offset < (uint32_t)sizeof(image))
    {
        length = (((uint32_t)sizeof(image) - offset) > BOOTLOADER_PACKET_DATA_SIZE)
                     ? BOOTLOADER_PACKET_DATA_SIZE
                     : ((uint32_t)sizeof(image) - offset);
        check_result(bootloader_packet_submit(&bootloader, offset, &image[offset], length) == BOOTLOADER_RESULT_SUCCESS,
                     "firmware packet accepted");
        process_until(&bootloader, BOOTLOADER_STATE_DOWNLOAD_READY);
        if (offset == 0u)
        {
            writes_before_duplicate = storage.write_count;
            check_result(bootloader_packet_submit(&bootloader, offset, &image[offset], length) == BOOTLOADER_RESULT_SUCCESS,
                         "last packet retransmission is idempotent");
            check_result(storage.write_count == writes_before_duplicate,
                         "idempotent packet does not program flash twice");
        }
        offset += length;
    }
    check_result(bootloader_upgrade_end(&bootloader, crc) == BOOTLOADER_RESULT_SUCCESS,
                 "verified staged image starts installation");
    process_until(&bootloader, BOOTLOADER_STATE_JUMP_PENDING);
    check_result(memcmp(storage.zones[BOOTLOADER_FLASH_ZONE_IAP], image, sizeof(image)) == 0,
                 "staged image copied into IAP exactly");
    check_result(storage.iap_read_count >= 9u,
                 "copied blocks and complete target are read back before launch");
    bootloader_process(&bootloader);
    check_result(platform.jump_count == 1u, "staged image launches after validation");
}

static void test_direct_failure(void)
{
    bootloader_t bootloader;
    fake_storage_t storage;
    fake_platform_t platform = {.reason = BOOTLOADER_BOOT_REASON_IAP_REQUEST};
    bootloader_upgrade_info_t info = {.module_id = 2u, .file_size = 16u, .version = 1u, .update_type = 0u};
    uint8_t packet_buffer[BOOTLOADER_PACKET_DATA_SIZE];
    uint8_t copy_buffer[300];
    uint8_t data[16] = {0x5Au, 0xA5u};

    (void)memset(&storage, 0, sizeof(storage));
    (void)memset(storage.zones, 0xFF, sizeof(storage.zones));
    storage.result = BOOTLOADER_RESULT_SUCCESS;
    (void)bootloader_make(&bootloader, &storage, &platform, packet_buffer, copy_buffer);
    process_until(&bootloader, BOOTLOADER_STATE_WAIT_UPGRADE);
    check_result(bootloader_upgrade_begin(&bootloader, &info, BOOTLOADER_UPGRADE_MODE_DIRECT) == BOOTLOADER_RESULT_SUCCESS,
                 "direct upgrade session starts");
    process_until(&bootloader, BOOTLOADER_STATE_DOWNLOAD_READY);
    storage.fail_next_write = 1u;
    check_result(bootloader_packet_submit(&bootloader, 0u, data, (uint32_t)sizeof(data)) == BOOTLOADER_RESULT_STORAGE_ERROR,
                 "direct write failure is reported");
    check_result((bootloader_state_get(&bootloader) == BOOTLOADER_STATE_WAIT_UPGRADE) &&
                     (platform.jump_count == 0u),
                 "direct write failure remains recoverable in bootloader");
}

static void test_copy_readback_failure(void)
{
    bootloader_t bootloader;
    fake_storage_t storage;
    fake_platform_t platform = {.reason = BOOTLOADER_BOOT_REASON_IAP_REQUEST};
    bootloader_upgrade_info_t info = {
        .module_id = 2u,
        .file_size = 64u,
        .version = 9u,
        .update_type = 0u,
    };
    uint8_t packet_buffer[BOOTLOADER_PACKET_DATA_SIZE];
    uint8_t copy_buffer[300];
    uint8_t image[64] = {0x5Au, 0xA5u};
    uint16_t crc = 0u;

    (void)memset(&storage, 0, sizeof(storage));
    (void)memset(storage.zones, 0xFF, sizeof(storage.zones));
    storage.result = BOOTLOADER_RESULT_SUCCESS;
    valid_footer_set(image, (uint32_t)sizeof(image), info.version, info.module_id);
    crc = test_crc_calculate(image, (uint32_t)sizeof(image));
    (void)bootloader_make(&bootloader, &storage, &platform, packet_buffer, copy_buffer);
    process_until(&bootloader, BOOTLOADER_STATE_WAIT_UPGRADE);
    (void)bootloader_upgrade_begin(&bootloader, &info, BOOTLOADER_UPGRADE_MODE_STAGED);
    process_until(&bootloader, BOOTLOADER_STATE_DOWNLOAD_READY);
    (void)bootloader_packet_submit(&bootloader, 0u, image, (uint32_t)sizeof(image));
    process_until(&bootloader, BOOTLOADER_STATE_DOWNLOAD_READY);
    storage.corrupt_next_iap_read = 1u;
    (void)bootloader_upgrade_end(&bootloader, crc);
    process_until(&bootloader, BOOTLOADER_STATE_WAIT_UPGRADE);
    check_result((bootloader_result_get(&bootloader) == BOOTLOADER_RESULT_IMAGE_INVALID) &&
                     (platform.jump_count == 0u),
                 "copied chunk readback corruption prevents launch");
}

static void test_staged_power_loss_recovery(void)
{
    bootloader_t first_boot;
    bootloader_t recovered_boot;
    fake_storage_t storage;
    fake_platform_t platform = {.reason = BOOTLOADER_BOOT_REASON_IAP_REQUEST};
    bootloader_upgrade_info_t info = {.module_id = 2u, .file_size = 64u, .version = 7u, .update_type = 1u};
    uint8_t first_packet[BOOTLOADER_PACKET_DATA_SIZE];
    uint8_t first_copy[300];
    uint8_t recovered_packet[BOOTLOADER_PACKET_DATA_SIZE];
    uint8_t recovered_copy[300];
    uint8_t image[64] = {0x5Au, 0xA5u};
    uint16_t crc = 0u;

    (void)memset(&storage, 0, sizeof(storage));
    (void)memset(storage.zones, 0xFF, sizeof(storage.zones));
    storage.result = BOOTLOADER_RESULT_SUCCESS;
    valid_footer_set(image, (uint32_t)sizeof(image), info.version, info.module_id);
    crc = test_crc_calculate(image, (uint32_t)sizeof(image));
    (void)bootloader_make(&first_boot, &storage, &platform, first_packet, first_copy);
    process_until(&first_boot, BOOTLOADER_STATE_WAIT_UPGRADE);
    (void)bootloader_upgrade_begin(&first_boot, &info, BOOTLOADER_UPGRADE_MODE_STAGED);
    process_until(&first_boot, BOOTLOADER_STATE_DOWNLOAD_READY);
    (void)bootloader_packet_submit(&first_boot, 0u, image, (uint32_t)sizeof(image));
    process_until(&first_boot, BOOTLOADER_STATE_DOWNLOAD_READY);
    (void)bootloader_upgrade_end(&first_boot, crc);
    process_until(&first_boot, BOOTLOADER_STATE_COPY_ERASE);

    platform.reason = BOOTLOADER_BOOT_REASON_POWER_ON;
    platform.jump_count = 0u;
    check_result(bootloader_make(&recovered_boot, &storage, &platform,
                                  recovered_packet, recovered_copy) == BOOTLOADER_RESULT_SUCCESS,
                 "power-loss restart initializes from redundant metadata");
    process_until(&recovered_boot, BOOTLOADER_STATE_JUMP_PENDING);
    check_result(memcmp(storage.zones[BOOTLOADER_FLASH_ZONE_IAP], image, sizeof(image)) == 0,
                 "staged install restarts safely after power loss");
    bootloader_process(&recovered_boot);
    check_result(platform.jump_count == 1u, "recovered staged image launches only after validation");
}

static void test_protocol(void)
{
    bootloader_t bootloader;
    bootloader_protocol_t protocol;
    fake_storage_t storage;
    fake_platform_t platform = {.reason = BOOTLOADER_BOOT_REASON_IAP_REQUEST};
    uint8_t packet_buffer[BOOTLOADER_PACKET_DATA_SIZE];
    uint8_t copy_buffer[300];
    uint8_t payload[BOOTLOADER_PROTOCOL_DATA_LENGTH] = {0};
    uint8_t ack[BOOTLOADER_PROTOCOL_MAX_ACK_LENGTH] = {0};
    uint16_t ack_length = 0u;
    uint16_t crc = 0u;

    (void)memset(&storage, 0, sizeof(storage));
    (void)memset(storage.zones, 0xFF, sizeof(storage.zones));
    storage.result = BOOTLOADER_RESULT_SUCCESS;
    (void)bootloader_make(&bootloader, &storage, &platform, packet_buffer, copy_buffer);
    process_until(&bootloader, BOOTLOADER_STATE_WAIT_UPGRADE);
    check_result(bootloader_protocol_init(&protocol, &bootloader, test_crc_calculate,
                                           BOOTLOADER_UPGRADE_MODE_DIRECT) == BOOTLOADER_RESULT_SUCCESS,
                 "FRAME protocol initializes");

    payload[0] = 2u;
    write_u32_le(&payload[1], 0x01020304u);
    write_u32_le(&payload[5], 16u);
    payload[9] = 1u;
    check_result(bootloader_protocol_handle(&protocol, BOOTLOADER_PROTOCOL_CMD_INFO,
                                             payload, BOOTLOADER_PROTOCOL_INFO_LENGTH,
                                             ack, (uint16_t)sizeof(ack), &ack_length) == BOOTLOADER_RESULT_SUCCESS,
                 "0x08 starts direct upgrade");
    check_result((ack_length == 3u) && (ack[0] == 1u), "0x08 direct ACK allows upgrade");
    process_until(&bootloader, BOOTLOADER_STATE_DOWNLOAD_READY);
    check_result(bootloader_protocol_handle(&protocol, BOOTLOADER_PROTOCOL_CMD_READY,
                                             NULL, 0u, ack, (uint16_t)sizeof(ack), &ack_length) ==
                     BOOTLOADER_RESULT_SUCCESS && ack[0] == 1u,
                 "0x09 reports download readiness");

    (void)memset(payload, 0xFF, sizeof(payload));
    write_u32_le(payload, 0u);
    payload[4] = 2u;
    write_u16_le(&payload[5], 16u);
    payload[7] = 0x5Au;
    payload[8] = 0xA5u;
    crc = test_crc_calculate(payload, 1031u);
    write_u16_le(&payload[1031], crc);
    check_result(bootloader_protocol_handle(&protocol, BOOTLOADER_PROTOCOL_CMD_DATA,
                                             payload, BOOTLOADER_PROTOCOL_DATA_LENGTH,
                                             ack, (uint16_t)sizeof(ack), &ack_length) == BOOTLOADER_RESULT_SUCCESS,
                 "0x0A accepts valid 1024-byte protocol envelope");
    process_until(&bootloader, BOOTLOADER_STATE_DOWNLOAD_READY);
    payload[1031] ^= 1u;
    check_result(bootloader_protocol_handle(&protocol, BOOTLOADER_PROTOCOL_CMD_DATA,
                                             payload, BOOTLOADER_PROTOCOL_DATA_LENGTH,
                                             ack, (uint16_t)sizeof(ack), &ack_length) ==
                      BOOTLOADER_RESULT_PROTOCOL_ERROR && ack[0] == 0u,
                 "0x0A rejects packet CRC corruption");
    check_result(bootloader_upgrade_end(&bootloader,
                                        test_crc_calculate(&payload[7], 16u)) ==
                     BOOTLOADER_RESULT_IMAGE_INVALID,
                 "firmware without a valid footer is rejected");
}

static void test_metadata(void)
{
    bootloader_metadata_t input = {
        .state = BOOTLOADER_METADATA_STATE_COPYING,
        .mode = BOOTLOADER_UPGRADE_MODE_STAGED,
        .module_id = 2u,
        .retry_count = 1u,
        .sequence = 5u,
        .version = 0x01020304u,
        .file_size = 1500u,
        .expected_crc = 0x1234u,
        .received_length = 1500u,
        .running_crc = 0x1234u,
        .copy_offset = 600u,
        .error_code = 0u,
    };
    bootloader_metadata_t output = {0};
    uint8_t meta_a[BOOTLOADER_METADATA_ENCODED_SIZE];
    uint8_t meta_b[BOOTLOADER_METADATA_ENCODED_SIZE];
    bootloader_flash_zone_t source = BOOTLOADER_FLASH_ZONE_META_A;

    check_result(bootloader_metadata_encode(&input, meta_a,
                                             (uint32_t)sizeof(meta_a)) == BOOTLOADER_RESULT_SUCCESS,
                 "metadata encodes without native struct layout");
    check_result(bootloader_metadata_decode(meta_a, (uint32_t)sizeof(meta_a),
                                             &output) == BOOTLOADER_RESULT_SUCCESS &&
                     output.copy_offset == 600u && output.sequence == 5u,
                 "metadata committed copy decodes");
    input.sequence = 6u;
    input.copy_offset = 900u;
    (void)bootloader_metadata_encode(&input, meta_b, (uint32_t)sizeof(meta_b));
    check_result(bootloader_metadata_select(meta_a, meta_b, &output, &source) ==
                     BOOTLOADER_RESULT_SUCCESS && source == BOOTLOADER_FLASH_ZONE_META_B &&
                     output.copy_offset == 900u,
                 "newest redundant metadata copy is selected");
    meta_b[20] ^= 1u;
    check_result(bootloader_metadata_select(meta_a, meta_b, &output, &source) ==
                     BOOTLOADER_RESULT_SUCCESS && source == BOOTLOADER_FLASH_ZONE_META_A,
                 "torn newest metadata falls back to older copy");
}

typedef struct
{
    uint8_t tx_idle;
    uint32_t prepare_count;
    uint32_t reason_count;
    uint32_t enter_count;
} fake_iap_t;

static bootloader_result_t fake_iap_prepare(void *p_context, const bootloader_upgrade_info_t *p_info)
{
    fake_iap_t *p_iap = (fake_iap_t *)p_context;

    p_iap->prepare_count++;
    return (p_info->file_size == 16u) ? BOOTLOADER_RESULT_SUCCESS : BOOTLOADER_RESULT_INVALID_ARGUMENT;
}

static bootloader_result_t fake_iap_reason_set(void *p_context)
{
    ((fake_iap_t *)p_context)->reason_count++;
    return BOOTLOADER_RESULT_SUCCESS;
}

static uint8_t fake_iap_tx_idle(void *p_context)
{
    return ((fake_iap_t *)p_context)->tx_idle;
}

static bootloader_result_t fake_iap_enter(void *p_context)
{
    ((fake_iap_t *)p_context)->enter_count++;
    return BOOTLOADER_RESULT_SUCCESS;
}

static void test_iap_service(void)
{
    fake_iap_t fake = {0};
    iap_boot_service_t service;
    const iap_boot_service_ops_t ops = {
        .p_context = &fake,
        .p_prepare = fake_iap_prepare,
        .p_boot_reason_set = fake_iap_reason_set,
        .p_tx_is_idle = fake_iap_tx_idle,
        .p_enter_bootloader = fake_iap_enter,
    };
    uint8_t payload[12] = {2u};
    uint8_t ack[3] = {0};
    uint16_t ack_length = 0u;

    write_u32_le(&payload[1], 1u);
    write_u32_le(&payload[5], 16u);
    payload[9] = 1u;
    check_result(iap_boot_service_init(&service, &ops) == BOOTLOADER_RESULT_SUCCESS,
                 "minimal IAP service initializes");
    check_result(iap_boot_service_info_handle(&service, payload, (uint16_t)sizeof(payload), ack,
                                               (uint16_t)sizeof(ack), &ack_length) == BOOTLOADER_RESULT_SUCCESS &&
                     ack[0] == 1u,
                 "IAP 0x08 invokes preparation and accepts tail extension");
    iap_boot_service_process(&service);
    check_result(fake.enter_count == 0u, "IAP waits for ACK transport completion");
    fake.tx_idle = 1u;
    iap_boot_service_process(&service);
    check_result((fake.prepare_count == 1u) && (fake.reason_count == 1u) && (fake.enter_count == 1u),
                 "IAP enters bootloader after prepare, reason, and TX idle");
}

static fal_result_t fake_fal_init(fal_t *p_fal, const fal_cfg_t *p_cfg)
{
    (void)p_fal;
    (void)p_cfg;
    return FAL_RESULT_SUCCESS;
}

static void fake_fal_process(fal_t *p_fal)
{
    (void)p_fal;
}

static fal_result_t fake_fal_info(const fal_t *p_fal, fal_zone_id_t zone, fal_zone_info_t *p_info)
{
    (void)p_fal;
    g_fake_fal_last_zone = zone;
    p_info->size = 100u;
    p_info->program_page_size = 10u;
    p_info->erase_block_size = 20u;
    p_info->permissions = FAL_ZONE_PERMISSION_ALL;
    return FAL_RESULT_SUCCESS;
}

static fal_result_t fake_fal_read(fal_t *p_fal,
                                  fal_zone_id_t zone,
                                  uint32_t offset,
                                  uint32_t length,
                                  uint8_t *p_data)
{
    (void)p_fal;
    (void)offset;
    (void)length;
    (void)p_data;
    g_fake_fal_last_zone = zone;
    return FAL_RESULT_SUCCESS;
}

static fal_result_t fake_fal_write(fal_t *p_fal,
                                   fal_zone_id_t zone,
                                   uint32_t offset,
                                   uint32_t length,
                                   const uint8_t *p_data)
{
    (void)p_fal;
    (void)offset;
    (void)length;
    (void)p_data;
    g_fake_fal_last_zone = zone;
    return FAL_RESULT_PERMISSION_DENIED;
}

static fal_result_t fake_fal_erase(fal_t *p_fal,
                                   fal_zone_id_t zone,
                                   uint32_t offset,
                                   uint32_t length)
{
    (void)p_fal;
    (void)offset;
    (void)length;
    g_fake_fal_last_zone = zone;
    return FAL_RESULT_SUCCESS;
}

static uint8_t fake_fal_busy(const fal_t *p_fal)
{
    (void)p_fal;
    return 0u;
}

static fal_result_t fake_fal_result(const fal_t *p_fal)
{
    (void)p_fal;
    return FAL_RESULT_SUCCESS;
}

static void test_fal_adapter(void)
{
    static const fal_api_t api = {
        .p_init = fake_fal_init,
        .p_process = fake_fal_process,
        .p_zone_info_get = fake_fal_info,
        .p_read = fake_fal_read,
        .p_write = fake_fal_write,
        .p_erase = fake_fal_erase,
        .p_is_busy = fake_fal_busy,
        .p_result_get = fake_fal_result,
    };
    static const bootloader_fal_zone_map_t map[BOOTLOADER_FLASH_ZONE_COUNT] = {
        {BOOTLOADER_FLASH_ZONE_IAP, 10u},
        {BOOTLOADER_FLASH_ZONE_STAGING, 11u},
        {BOOTLOADER_FLASH_ZONE_META_A, 12u},
        {BOOTLOADER_FLASH_ZONE_META_B, 13u},
        {BOOTLOADER_FLASH_ZONE_LAYOUT, 14u},
    };
    static const fal_cfg_t cfg = {0};
    bootloader_fal_adapter_t adapter;
    bootloader_flash_ops_t ops;
    bootloader_flash_zone_info_t info;
    fal_t fal;
    uint8_t byte = 0u;

    check_result(bootloader_fal_adapter_mount(&adapter, &fal, &api, &cfg, map,
                                               (uint16_t)BOOTLOADER_FLASH_ZONE_COUNT, &ops) == BOOTLOADER_RESULT_SUCCESS,
                 "complete FAL mapping mounts");
    check_result(ops.p_zone_info_get(ops.p_context, BOOTLOADER_FLASH_ZONE_STAGING, &info) == BOOTLOADER_RESULT_SUCCESS,
                 "adapter forwards zone info");
    check_result((g_fake_fal_last_zone == 11u) && (info.size == 100u),
                 "logical staging maps to configured FAL zone");
    check_result(ops.p_write(ops.p_context, BOOTLOADER_FLASH_ZONE_IAP, 0u, 1u, &byte) ==
                     BOOTLOADER_RESULT_PERMISSION_DENIED,
                 "adapter translates FAL permission result");
    check_result(g_fake_fal_last_zone == 10u, "logical IAP maps independently");
    check_result(bootloader_fal_adapter_mount(&adapter, &fal, &api, &cfg, map,
                                               (uint16_t)(BOOTLOADER_FLASH_ZONE_COUNT - 1), &ops) ==
                     BOOTLOADER_RESULT_CONFIG_ERROR,
                 "incomplete FAL mapping is rejected");
}

int main(void)
{
    g_log_file = fopen("test_bootloader_core.log", "w");
    test_startup_paths();
    test_staged_upgrade();
    test_direct_failure();
    test_copy_readback_failure();
    test_staged_power_loss_recovery();
    test_protocol();
    test_metadata();
    test_iap_service();
    test_fal_adapter();
    (void)printf("SUMMARY: %lu passed, %lu failed\n",
                 (unsigned long)g_pass_count,
                 (unsigned long)g_fail_count);
    if (g_log_file != NULL)
    {
        (void)fprintf(g_log_file, "SUMMARY: %lu passed, %lu failed\n",
                      (unsigned long)g_pass_count,
                      (unsigned long)g_fail_count);
        (void)fclose(g_log_file);
    }
    return (g_fail_count == 0u) ? 0 : 1;
}
