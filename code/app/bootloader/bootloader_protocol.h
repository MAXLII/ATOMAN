// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_protocol.h
 * @brief   Platform-independent FRAME firmware-upgrade command service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Parse the existing FRAME 0x08 through 0x0B upgrade payloads
 *          - Validate command lengths, module identity, ordering, and packet CRC
 *          - Submit bounded events to the bootloader core and encode direct ACK payloads
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; communication callbacks and bootloader processing are serialized
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

#ifndef BOOTLOADER_PROTOCOL_H
#define BOOTLOADER_PROTOCOL_H

#include "bootloader_core.h"

#include <stdint.h>

#define BOOTLOADER_PROTOCOL_CMD_SET 0x01u
#define BOOTLOADER_PROTOCOL_CMD_INFO 0x08u
#define BOOTLOADER_PROTOCOL_CMD_READY 0x09u
#define BOOTLOADER_PROTOCOL_CMD_DATA 0x0Au
#define BOOTLOADER_PROTOCOL_CMD_END 0x0Bu

#define BOOTLOADER_PROTOCOL_INFO_LENGTH 10u
#define BOOTLOADER_PROTOCOL_DATA_LENGTH 1033u
#define BOOTLOADER_PROTOCOL_END_LENGTH 2u
#define BOOTLOADER_PROTOCOL_MAX_ACK_LENGTH 3u

typedef uint16_t (*bootloader_protocol_crc16_t)(const uint8_t *p_data, uint32_t length);

typedef struct
{
    bootloader_t *p_bootloader;                 /**< Bootloader core receiving parsed events. */
    bootloader_protocol_crc16_t p_crc16;        /**< Existing FRAME packet CRC calculation. */
    bootloader_upgrade_mode_t upgrade_mode;     /**< Direct or staged mode selected for sessions. */
} bootloader_protocol_t;

bootloader_result_t bootloader_protocol_init(bootloader_protocol_t *p_protocol,
                                              bootloader_t *p_bootloader,
                                              bootloader_protocol_crc16_t p_crc16,
                                              bootloader_upgrade_mode_t upgrade_mode);
bootloader_result_t bootloader_protocol_handle(bootloader_protocol_t *p_protocol,
                                                uint8_t cmd_word,
                                                const uint8_t *p_payload,
                                                uint16_t payload_length,
                                                uint8_t *p_ack,
                                                uint16_t ack_capacity,
                                                uint16_t *p_ack_length);

#endif /* BOOTLOADER_PROTOCOL_H */
