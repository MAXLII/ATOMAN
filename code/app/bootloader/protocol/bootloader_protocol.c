// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_protocol.c
 * @brief   FRAME firmware-upgrade protocol service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Decode fixed little-endian upgrade command fields without packed casts
 *          - Reject malformed, out-of-order, and corrupted firmware packets
 *          - Register FRAME commands, send direct ACK frames, and periodically advance the core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; calls are serialized with bootloader_process
 *          - Communication callbacks do not wait for flash completion
 *
 * @author  Max.Li
 * @date    2026-07-28
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bootloader_protocol.h"

#if defined(BOOTLOADER_PROTOCOL_FRAME_SERVICE_ENABLE)
#include "comm.h"
#endif

#include <stddef.h>

static bootloader_protocol_t *p_protocol_active;

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

static void write_u16_le(uint8_t *p_data, uint16_t value)
{
    p_data[0] = (uint8_t)(value & 0x00FFu);
    p_data[1] = (uint8_t)(value >> 8u);
}

static bootloader_result_t info_handle(bootloader_protocol_t *p_protocol,
                                       const uint8_t *p_payload,
                                       uint16_t payload_length,
                                       uint8_t *p_ack,
                                       uint16_t ack_capacity,
                                       uint16_t *p_ack_length)
{
    bootloader_upgrade_info_t info = {0};
    bootloader_result_t result = BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    uint16_t reject_reason = 0u;

    if ((payload_length != BOOTLOADER_PROTOCOL_INFO_LENGTH) ||
        (p_payload == NULL) ||
        (ack_capacity < BOOTLOADER_PROTOCOL_INFO_ACK_LENGTH))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR_E;
    }
    info.module_id = p_payload[offsetof(bootloader_protocol_info_request_t, module_id)];
    info.version = read_u32_le(&p_payload[offsetof(bootloader_protocol_info_request_t, version)]);
    info.file_size = read_u32_le(&p_payload[offsetof(bootloader_protocol_info_request_t, file_size)]);
    info.update_type = p_payload[offsetof(bootloader_protocol_info_request_t, update_type)];
    result = bootloader_upgrade_begin(p_protocol->p_bootloader, &info, p_protocol->upgrade_mode);
    p_ack[offsetof(bootloader_protocol_info_ack_t, allow_update)] =
        (result == BOOTLOADER_RESULT_SUCCESS_E)
            ? (uint8_t)BOOTLOADER_PROTOCOL_UPDATE_ACK_ALLOW_E
            : (uint8_t)BOOTLOADER_PROTOCOL_UPDATE_ACK_REJECT_E;
    if (result == BOOTLOADER_RESULT_OUT_OF_RANGE_E)
    {
        reject_reason = BOOTLOADER_PROTOCOL_REJECT_OVERSIZE_MASK;
    }
    else if (result == BOOTLOADER_RESULT_INVALID_ARGUMENT_E)
    {
        reject_reason = BOOTLOADER_PROTOCOL_REJECT_MODULE_MASK;
    }
    write_u16_le(&p_ack[offsetof(bootloader_protocol_info_ack_t, reject_reason)], reject_reason);
    *p_ack_length = BOOTLOADER_PROTOCOL_INFO_ACK_LENGTH;
    return result;
}

static bootloader_result_t ready_handle(bootloader_protocol_t *p_protocol,
                                        uint16_t payload_length,
                                        uint8_t *p_ack,
                                        uint16_t ack_capacity,
                                        uint16_t *p_ack_length)
{
    if ((payload_length != 0u) ||
        (ack_capacity < BOOTLOADER_PROTOCOL_READY_ACK_LENGTH))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR_E;
    }
    p_ack[offsetof(bootloader_protocol_ready_ack_t, ready)] =
        bootloader_is_download_ready(p_protocol->p_bootloader);
    *p_ack_length = BOOTLOADER_PROTOCOL_READY_ACK_LENGTH;
    return BOOTLOADER_RESULT_SUCCESS_E;
}

static bootloader_result_t data_handle(bootloader_protocol_t *p_protocol,
                                       const uint8_t *p_payload,
                                       uint16_t payload_length,
                                       uint8_t *p_ack,
                                       uint16_t ack_capacity,
                                       uint16_t *p_ack_length)
{
    bootloader_result_t result = BOOTLOADER_RESULT_PROTOCOL_ERROR_E;
    uint32_t offset = 0u;
    uint16_t data_length = 0u;
    uint16_t packet_crc = 0u;

    if ((payload_length != BOOTLOADER_PROTOCOL_DATA_LENGTH) ||
        (p_payload == NULL) ||
        (ack_capacity < BOOTLOADER_PROTOCOL_DATA_ACK_LENGTH))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR_E;
    }
    offset = read_u32_le(&p_payload[offsetof(bootloader_protocol_data_request_t, offset)]);
    data_length = read_u16_le(&p_payload[offsetof(bootloader_protocol_data_request_t, data_length)]);
    packet_crc = read_u16_le(&p_payload[offsetof(bootloader_protocol_data_request_t, packet_crc)]);
    if ((p_payload[offsetof(bootloader_protocol_data_request_t, module_id)] !=
         p_protocol->p_bootloader->config.expected_module_id) ||
        (data_length == 0u) || (data_length > BOOTLOADER_PACKET_DATA_SIZE) ||
        (packet_crc != p_protocol->p_crc16(
                           p_payload,
                           (uint32_t)offsetof(bootloader_protocol_data_request_t, packet_crc))))
    {
        p_ack[offsetof(bootloader_protocol_data_ack_payload_t, data_is_ok)] =
            (uint8_t)BOOTLOADER_PROTOCOL_DATA_INVALID_E;
        *p_ack_length = BOOTLOADER_PROTOCOL_DATA_ACK_LENGTH;
        return BOOTLOADER_RESULT_PROTOCOL_ERROR_E;
    }
    result = bootloader_packet_submit(p_protocol->p_bootloader,
                                      offset,
                                      &p_payload[offsetof(bootloader_protocol_data_request_t, packet_data)],
                                      data_length);
    p_ack[offsetof(bootloader_protocol_data_ack_payload_t, data_is_ok)] =
        (result == BOOTLOADER_RESULT_SUCCESS_E)
            ? (uint8_t)BOOTLOADER_PROTOCOL_DATA_VALID_E
            : (uint8_t)BOOTLOADER_PROTOCOL_DATA_INVALID_E;
    *p_ack_length = BOOTLOADER_PROTOCOL_DATA_ACK_LENGTH;
    return result;
}

static bootloader_result_t end_handle(bootloader_protocol_t *p_protocol,
                                      const uint8_t *p_payload,
                                      uint16_t payload_length,
                                      uint8_t *p_ack,
                                      uint16_t ack_capacity,
                                      uint16_t *p_ack_length)
{
    bootloader_result_t result = BOOTLOADER_RESULT_PROTOCOL_ERROR_E;

    if ((payload_length != BOOTLOADER_PROTOCOL_END_LENGTH) ||
        (p_payload == NULL) ||
        (ack_capacity < BOOTLOADER_PROTOCOL_END_ACK_LENGTH))
    {
        return BOOTLOADER_RESULT_PROTOCOL_ERROR_E;
    }
    result = bootloader_upgrade_end(
        p_protocol->p_bootloader,
        read_u16_le(&p_payload[offsetof(bootloader_protocol_end_request_t, fw_crc)]));
    p_ack[offsetof(bootloader_protocol_end_ack_payload_t, success_flg)] =
        (result == BOOTLOADER_RESULT_SUCCESS_E)
            ? (uint8_t)BOOTLOADER_PROTOCOL_END_SUCCESS_E
            : (uint8_t)BOOTLOADER_PROTOCOL_END_FAILED_E;
    *p_ack_length = BOOTLOADER_PROTOCOL_END_ACK_LENGTH;
    return result;
}

bootloader_result_t bootloader_protocol_init(bootloader_protocol_t *p_protocol,
                                             bootloader_t *p_bootloader,
                                             bootloader_protocol_crc16_t p_crc16,
                                             bootloader_upgrade_mode_t upgrade_mode)
{
    if ((p_protocol == NULL) || (p_bootloader == NULL) || (p_crc16 == NULL) ||
        (upgrade_mode > BOOTLOADER_UPGRADE_MODE_STAGED_E))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }
    p_protocol->p_bootloader = p_bootloader;
    p_protocol->p_crc16 = p_crc16;
    p_protocol->upgrade_mode = upgrade_mode;
    return BOOTLOADER_RESULT_SUCCESS_E;
}

bootloader_result_t bootloader_protocol_mount(bootloader_protocol_t *p_protocol)
{
    if ((p_protocol == NULL) || (p_protocol->p_bootloader == NULL) ||
        (p_protocol->p_crc16 == NULL))
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR_E;
    }
    p_protocol_active = p_protocol;
    return BOOTLOADER_RESULT_SUCCESS_E;
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
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
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
        return BOOTLOADER_RESULT_PROTOCOL_ERROR_E;
    }
}

#if defined(BOOTLOADER_PROTOCOL_FRAME_SERVICE_ENABLE)
static void update_info_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    bootloader_protocol_info_ack_t update_info_ack = {0}; /* Command 0x08 direct ACK payload. */
    uint16_t ack_length = 0u; /* Encoded command 0x08 ACK payload length. */

    if ((p_protocol_active == NULL) ||
        (p_pack == NULL) ||
        (my_printf == NULL) ||
        (p_pack->is_ack == 1u))
    {
        return;
    }
    (void)bootloader_protocol_handle(p_protocol_active,
                                     BOOTLOADER_PROTOCOL_CMD_INFO,
                                     p_pack->p_data,
                                     p_pack->len,
                                     (uint8_t *)&update_info_ack,
                                     (uint16_t)sizeof(update_info_ack),
                                     &ack_length);
    if (ack_length == 0u)
    {
        return;
    }
    section_packform_t packform = {
        .src = p_pack->dst,
        .d_src = p_pack->d_dst,
        .dst = p_pack->src,
        .d_dst = p_pack->d_src,
        .cmd_set = BOOTLOADER_PROTOCOL_CMD_SET,
        .cmd_word = BOOTLOADER_PROTOCOL_CMD_INFO,
        .is_ack = 1u,
        .len = ack_length,
        .p_data = (uint8_t *)&update_info_ack,
    }; /* FRAME response routed back to the command 0x08 sender. */

    comm_send_data(&packform, my_printf);
}

REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_INFO, update_info_act)

static void update_ready_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    bootloader_protocol_ready_ack_t update_ready_ack = {0}; /* Command 0x09 direct ACK payload. */
    uint16_t ack_length = 0u; /* Encoded command 0x09 ACK payload length. */

    if ((p_protocol_active == NULL) ||
        (p_pack == NULL) ||
        (my_printf == NULL) ||
        (p_pack->is_ack == 1u))
    {
        return;
    }
    (void)bootloader_protocol_handle(p_protocol_active,
                                     BOOTLOADER_PROTOCOL_CMD_READY,
                                     p_pack->p_data,
                                     p_pack->len,
                                     (uint8_t *)&update_ready_ack,
                                     (uint16_t)sizeof(update_ready_ack),
                                     &ack_length);
    if (ack_length == 0u)
    {
        return;
    }
    section_packform_t packform = {
        .src = p_pack->dst,
        .d_src = p_pack->d_dst,
        .dst = p_pack->src,
        .d_dst = p_pack->d_src,
        .cmd_set = BOOTLOADER_PROTOCOL_CMD_SET,
        .cmd_word = BOOTLOADER_PROTOCOL_CMD_READY,
        .is_ack = 1u,
        .len = ack_length,
        .p_data = (uint8_t *)&update_ready_ack,
    }; /* FRAME response routed back to the command 0x09 sender. */

    comm_send_data(&packform, my_printf);
}

REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_READY, update_ready_act)

static void update_fw_pack_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    bootloader_protocol_data_ack_payload_t update_fw_ack = {0}; /* Command 0x0A direct ACK payload. */
    uint16_t ack_length = 0u; /* Encoded command 0x0A ACK payload length. */

    if ((p_protocol_active == NULL) ||
        (p_pack == NULL) ||
        (my_printf == NULL) ||
        (p_pack->is_ack == 1u))
    {
        return;
    }
    (void)bootloader_protocol_handle(p_protocol_active,
                                     BOOTLOADER_PROTOCOL_CMD_DATA,
                                     p_pack->p_data,
                                     p_pack->len,
                                     (uint8_t *)&update_fw_ack,
                                     (uint16_t)sizeof(update_fw_ack),
                                     &ack_length);
    if (ack_length == 0u)
    {
        return;
    }
    section_packform_t packform = {
        .src = p_pack->dst,
        .d_src = p_pack->d_dst,
        .dst = p_pack->src,
        .d_dst = p_pack->d_src,
        .cmd_set = BOOTLOADER_PROTOCOL_CMD_SET,
        .cmd_word = BOOTLOADER_PROTOCOL_CMD_DATA,
        .is_ack = 1u,
        .len = ack_length,
        .p_data = (uint8_t *)&update_fw_ack,
    }; /* FRAME response routed back to the command 0x0A sender. */

    comm_send_data(&packform, my_printf);
}

REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_DATA, update_fw_pack_act)

static void update_end_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    bootloader_protocol_end_ack_payload_t update_end_ack = {0}; /* Command 0x0B direct ACK payload. */
    uint16_t ack_length = 0u; /* Encoded command 0x0B ACK payload length. */

    if ((p_protocol_active == NULL) ||
        (p_pack == NULL) ||
        (my_printf == NULL) ||
        (p_pack->is_ack == 1u))
    {
        return;
    }
    (void)bootloader_protocol_handle(p_protocol_active,
                                     BOOTLOADER_PROTOCOL_CMD_END,
                                     p_pack->p_data,
                                     p_pack->len,
                                     (uint8_t *)&update_end_ack,
                                     (uint16_t)sizeof(update_end_ack),
                                     &ack_length);
    if (ack_length == 0u)
    {
        return;
    }
    section_packform_t packform = {
        .src = p_pack->dst,
        .d_src = p_pack->d_dst,
        .dst = p_pack->src,
        .d_dst = p_pack->d_src,
        .cmd_set = BOOTLOADER_PROTOCOL_CMD_SET,
        .cmd_word = BOOTLOADER_PROTOCOL_CMD_END,
        .is_ack = 1u,
        .len = ack_length,
        .p_data = (uint8_t *)&update_end_ack,
    }; /* FRAME response routed back to the command 0x0B sender. */

    comm_send_data(&packform, my_printf);
}

REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_END, update_end_act)

static void bootloader_protocol_process(void)
{
    if ((p_protocol_active != NULL) && (p_protocol_active->p_bootloader != NULL))
    {
        bootloader_process(p_protocol_active->p_bootloader);
    }
}

REG_TASK_MS(1u, bootloader_protocol_process)
#endif /* BOOTLOADER_PROTOCOL_FRAME_SERVICE_ENABLE */
