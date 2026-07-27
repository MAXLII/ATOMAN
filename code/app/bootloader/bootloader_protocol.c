// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_protocol.c
 * @brief   FRAME firmware-upgrade payload parsing and ACK generation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Decode fixed little-endian upgrade command fields without packed casts
 *          - Reject malformed, out-of-order, and corrupted firmware packets
 *          - Return ACK payloads for the same command while the core performs flash work
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; calls are serialized with bootloader_process
 *          - Transport framing and hardware access remain outside this module
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

#include "bootloader_protocol.h"

#include <stddef.h>

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

static bootloader_result_t info_handle(bootloader_protocol_t *p_protocol,
                                       const uint8_t *p_payload,
                                       uint16_t payload_length,
                                       uint8_t *p_ack,
                                       uint16_t ack_capacity,
                                       uint16_t *p_ack_length)
{
    bootloader_upgrade_info_t info = {0};
    bootloader_result_t result = BOOTLOADER_RESULT_INVALID_ARGUMENT;

    if ((payload_length != BOOTLOADER_PROTOCOL_INFO_LENGTH) ||
        (p_payload == NULL) || (ack_capacity < 3u))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR;
    }
    info.module_id = p_payload[0];
    info.version = read_u32_le(&p_payload[1]);
    info.file_size = read_u32_le(&p_payload[5]);
    info.update_type = p_payload[9];
    result = bootloader_upgrade_begin(p_protocol->p_bootloader, &info, p_protocol->upgrade_mode);
    p_ack[0] = (result == BOOTLOADER_RESULT_SUCCESS) ? 1u : 2u;
    p_ack[1] = 0u;
    p_ack[2] = 0u;
    if (result == BOOTLOADER_RESULT_OUT_OF_RANGE)
    {
        p_ack[1] = 1u;
    }
    else if (result == BOOTLOADER_RESULT_INVALID_ARGUMENT)
    {
        p_ack[1] = 4u;
    }
    *p_ack_length = 3u;
    return result;
}

static bootloader_result_t ready_handle(bootloader_protocol_t *p_protocol,
                                        uint16_t payload_length,
                                        uint8_t *p_ack,
                                        uint16_t ack_capacity,
                                        uint16_t *p_ack_length)
{
    if ((payload_length != 0u) || (ack_capacity < 1u))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR;
    }
    p_ack[0] = bootloader_is_download_ready(p_protocol->p_bootloader);
    *p_ack_length = 1u;
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t data_handle(bootloader_protocol_t *p_protocol,
                                       const uint8_t *p_payload,
                                       uint16_t payload_length,
                                       uint8_t *p_ack,
                                       uint16_t ack_capacity,
                                       uint16_t *p_ack_length)
{
    bootloader_result_t result = BOOTLOADER_RESULT_PROTOCOL_ERROR;
    uint32_t offset = 0u;
    uint16_t data_length = 0u;
    uint16_t packet_crc = 0u;

    if ((payload_length != BOOTLOADER_PROTOCOL_DATA_LENGTH) ||
        (p_payload == NULL) || (ack_capacity < 1u))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR;
    }
    offset = read_u32_le(p_payload);
    data_length = read_u16_le(&p_payload[5]);
    packet_crc = read_u16_le(&p_payload[1031]);
    if ((p_payload[4] != p_protocol->p_bootloader->config.expected_module_id) ||
        (data_length == 0u) || (data_length > BOOTLOADER_PACKET_DATA_SIZE) ||
        (packet_crc != p_protocol->p_crc16(p_payload, 1031u)))
    {
        p_ack[0] = 0u;
        *p_ack_length = 1u;
        return BOOTLOADER_RESULT_PROTOCOL_ERROR;
    }
    result = bootloader_packet_submit(p_protocol->p_bootloader,
                                      offset,
                                      &p_payload[7],
                                      data_length);
    p_ack[0] = (result == BOOTLOADER_RESULT_SUCCESS)
                   ? 1u
                   : ((result == BOOTLOADER_RESULT_BUSY) ? 2u : 0u);
    *p_ack_length = 1u;
    return result;
}

static bootloader_result_t end_handle(bootloader_protocol_t *p_protocol,
                                      const uint8_t *p_payload,
                                      uint16_t payload_length,
                                      uint8_t *p_ack,
                                      uint16_t ack_capacity,
                                      uint16_t *p_ack_length)
{
    bootloader_result_t result = BOOTLOADER_RESULT_PROTOCOL_ERROR;

    if ((payload_length != BOOTLOADER_PROTOCOL_END_LENGTH) ||
        (p_payload == NULL) || (ack_capacity < 1u))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR;
    }
    result = bootloader_upgrade_end(p_protocol->p_bootloader, read_u16_le(p_payload));
    p_ack[0] = (result == BOOTLOADER_RESULT_SUCCESS) ? 1u : 0u;
    *p_ack_length = 1u;
    return result;
}

bootloader_result_t bootloader_protocol_init(bootloader_protocol_t *p_protocol,
                                              bootloader_t *p_bootloader,
                                              bootloader_protocol_crc16_t p_crc16,
                                              bootloader_upgrade_mode_t upgrade_mode)
{
    if ((p_protocol == NULL) || (p_bootloader == NULL) || (p_crc16 == NULL) ||
        (upgrade_mode > BOOTLOADER_UPGRADE_MODE_STAGED))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    p_protocol->p_bootloader = p_bootloader;
    p_protocol->p_crc16 = p_crc16;
    p_protocol->upgrade_mode = upgrade_mode;
    return BOOTLOADER_RESULT_SUCCESS;
}

bootloader_result_t bootloader_protocol_handle(bootloader_protocol_t *p_protocol,
                                                uint8_t cmd_word,
                                                const uint8_t *p_payload,
                                                uint16_t payload_length,
                                                uint8_t *p_ack,
                                                uint16_t ack_capacity,
                                                uint16_t *p_ack_length)
{
    if ((p_protocol == NULL) || (p_protocol->p_bootloader == NULL) ||
        (p_protocol->p_crc16 == NULL) || (p_ack == NULL) || (p_ack_length == NULL))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    *p_ack_length = 0u;
    switch (cmd_word)
    {
    case BOOTLOADER_PROTOCOL_CMD_INFO:
        return info_handle(p_protocol, p_payload, payload_length, p_ack, ack_capacity, p_ack_length);
    case BOOTLOADER_PROTOCOL_CMD_READY:
        return ready_handle(p_protocol, payload_length, p_ack, ack_capacity, p_ack_length);
    case BOOTLOADER_PROTOCOL_CMD_DATA:
        return data_handle(p_protocol, p_payload, payload_length, p_ack, ack_capacity, p_ack_length);
    case BOOTLOADER_PROTOCOL_CMD_END:
        return end_handle(p_protocol, p_payload, payload_length, p_ack, ack_capacity, p_ack_length);
    default:
        return BOOTLOADER_RESULT_PROTOCOL_ERROR;
    }
}
