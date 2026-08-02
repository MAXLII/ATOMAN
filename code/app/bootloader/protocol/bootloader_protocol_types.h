// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_protocol_types.h
 * @brief   FRAME firmware-upgrade payload definitions.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the published payload layout for commands 0x08 through 0x0B
 *          - Define upgrade, acknowledgement, and rejection values used on the wire
 *          - Verify every packed payload size at compile time
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Multi-byte wire fields use little-endian byte order
 *          - Payload structures are packed to 1-byte alignment
 *
 * @author  Max.Li
 * @date    2026-07-29
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BOOTLOADER_PROTOCOL_TYPES_H
#define BOOTLOADER_PROTOCOL_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define BOOTLOADER_PROTOCOL_CMD_SET 0x01
#define BOOTLOADER_PROTOCOL_CMD_INFO 0x08
#define BOOTLOADER_PROTOCOL_CMD_READY 0x09
#define BOOTLOADER_PROTOCOL_CMD_DATA 0x0A
#define BOOTLOADER_PROTOCOL_CMD_END 0x0B

#define BOOTLOADER_PROTOCOL_PACKET_DATA_SIZE 1024u

#define BOOTLOADER_PROTOCOL_REJECT_OVERSIZE_MASK 0x0001u
#define BOOTLOADER_PROTOCOL_REJECT_VERSION_MASK 0x0002u
#define BOOTLOADER_PROTOCOL_REJECT_MODULE_MASK 0x0004u

typedef enum
{
    BOOTLOADER_PROTOCOL_UPDATE_TYPE_NONE_E = 0u,   /**< The request does not start an upgrade. */
    BOOTLOADER_PROTOCOL_UPDATE_TYPE_NORMAL_E = 1u, /**< Apply normal version and target checks. */
    BOOTLOADER_PROTOCOL_UPDATE_TYPE_FORCE_E = 2u   /**< Force the upgrade after safety checks. */
} bootloader_protocol_update_type_t;

typedef enum
{
    BOOTLOADER_PROTOCOL_UPDATE_ACK_INVALID_E = 0u, /**< The acknowledgement value is invalid. */
    BOOTLOADER_PROTOCOL_UPDATE_ACK_ALLOW_E = 1u,   /**< The upgrade request is accepted. */
    BOOTLOADER_PROTOCOL_UPDATE_ACK_REJECT_E = 2u   /**< The upgrade request is rejected. */
} bootloader_protocol_update_ack_t;

typedef enum
{
    BOOTLOADER_PROTOCOL_DATA_INVALID_E = 0u, /**< Firmware packet validation failed. */
    BOOTLOADER_PROTOCOL_DATA_VALID_E = 1u    /**< Firmware packet validation succeeded. */
} bootloader_protocol_data_ack_t;

typedef enum
{
    BOOTLOADER_PROTOCOL_END_FAILED_E = 0u, /**< Complete firmware validation failed. */
    BOOTLOADER_PROTOCOL_END_SUCCESS_E = 1u /**< Complete firmware validation succeeded. */
} bootloader_protocol_end_ack_t;

#pragma pack(push, 1)

typedef union
{
    uint32_t raw; /**< Packed version value used by Bootloader Core and firmware footer. */
    struct
    {
        uint8_t debug_ver;     /**< Debug version byte. */
        uint8_t release_ver;   /**< Release version byte. */
        uint8_t device_vendor; /**< Device-vendor byte. */
        uint8_t hard_ver;      /**< Hardware-version byte. */
    } byte; /**< Published byte order from update.h. */
} bootloader_protocol_version_t;

typedef union
{
    uint16_t raw; /**< Complete rejection-reason bit mask. */
    struct
    {
        uint16_t oversize : 1;    /**< Firmware length exceeds the target region. */
        uint16_t version_err : 1; /**< Hardware or vendor version is incompatible. */
        uint16_t module_err : 1;  /**< Target module identifier is invalid. */
    } bit; /**< Published rejection bits from update.h. */
} bootloader_protocol_reject_reason_t;

typedef struct
{
    uint8_t module_id;                         /**< Target module identifier. */
    bootloader_protocol_version_t version;     /**< Candidate firmware version. */
    uint32_t file_size;                        /**< Firmware bytes including the footer. */
    uint8_t update_type;                       /**< bootloader_protocol_update_type_t value. */
} bootloader_protocol_info_request_t;

typedef struct
{
    uint8_t allow_update;                             /**< bootloader_protocol_update_ack_t value. */
    bootloader_protocol_reject_reason_t reject_reason; /**< Detailed rejection-reason bits. */
} bootloader_protocol_info_ack_t;

typedef struct
{
    uint8_t ready; /**< 1 when the Bootloader can accept command 0x0A; otherwise 0. */
} bootloader_protocol_ready_ack_t;

typedef struct
{
    uint32_t offset; /**< Byte offset in the complete firmware image. */
    uint8_t module_id; /**< Target module identifier. */
    uint16_t data_length; /**< Valid bytes in packet_data. */
    uint8_t packet_data[BOOTLOADER_PROTOCOL_PACKET_DATA_SIZE]; /**< Data padded with 0xFF. */
    uint16_t packet_crc; /**< FRAME CRC16 over all preceding fields in this payload. */
} bootloader_protocol_data_request_t;

typedef struct
{
    uint8_t data_is_ok; /**< bootloader_protocol_data_ack_t value. */
} bootloader_protocol_data_ack_payload_t;

typedef struct
{
    uint16_t fw_crc; /**< FRAME CRC16 over the complete firmware including its footer. */
} bootloader_protocol_end_request_t;

typedef struct
{
    uint8_t success_flg; /**< bootloader_protocol_end_ack_t value. */
} bootloader_protocol_end_ack_payload_t;

#pragma pack(pop)

#define BOOTLOADER_PROTOCOL_INFO_LENGTH ((uint16_t)sizeof(bootloader_protocol_info_request_t))
#define BOOTLOADER_PROTOCOL_INFO_ACK_LENGTH ((uint16_t)sizeof(bootloader_protocol_info_ack_t))
#define BOOTLOADER_PROTOCOL_READY_ACK_LENGTH ((uint16_t)sizeof(bootloader_protocol_ready_ack_t))
#define BOOTLOADER_PROTOCOL_DATA_LENGTH ((uint16_t)sizeof(bootloader_protocol_data_request_t))
#define BOOTLOADER_PROTOCOL_DATA_ACK_LENGTH ((uint16_t)sizeof(bootloader_protocol_data_ack_payload_t))
#define BOOTLOADER_PROTOCOL_END_LENGTH ((uint16_t)sizeof(bootloader_protocol_end_request_t))
#define BOOTLOADER_PROTOCOL_END_ACK_LENGTH ((uint16_t)sizeof(bootloader_protocol_end_ack_payload_t))
#define BOOTLOADER_PROTOCOL_MAX_ACK_LENGTH BOOTLOADER_PROTOCOL_INFO_ACK_LENGTH

_Static_assert(sizeof(bootloader_protocol_version_t) == 4u,
               "FRAME upgrade version payload must be 4 bytes");
_Static_assert(sizeof(bootloader_protocol_reject_reason_t) == 2u,
               "FRAME upgrade rejection reason must be 2 bytes");
_Static_assert(sizeof(bootloader_protocol_info_request_t) == 10u,
               "FRAME command 0x08 request payload must be 10 bytes");
_Static_assert(offsetof(bootloader_protocol_info_request_t, version) == 1u,
               "FRAME command 0x08 version must start at byte 1");
_Static_assert(offsetof(bootloader_protocol_info_request_t, file_size) == 5u,
               "FRAME command 0x08 file size must start at byte 5");
_Static_assert(offsetof(bootloader_protocol_info_request_t, update_type) == 9u,
               "FRAME command 0x08 update type must start at byte 9");
_Static_assert(sizeof(bootloader_protocol_info_ack_t) == 3u,
               "FRAME command 0x08 ACK payload must be 3 bytes");
_Static_assert(offsetof(bootloader_protocol_info_ack_t, reject_reason) == 1u,
               "FRAME command 0x08 rejection reason must start at byte 1");
_Static_assert(sizeof(bootloader_protocol_ready_ack_t) == 1u,
               "FRAME command 0x09 ACK payload must be 1 byte");
_Static_assert(sizeof(bootloader_protocol_data_request_t) == 1033u,
               "FRAME command 0x0A request payload must be 1033 bytes");
_Static_assert(offsetof(bootloader_protocol_data_request_t, module_id) == 4u,
               "FRAME command 0x0A module ID must start at byte 4");
_Static_assert(offsetof(bootloader_protocol_data_request_t, data_length) == 5u,
               "FRAME command 0x0A data length must start at byte 5");
_Static_assert(offsetof(bootloader_protocol_data_request_t, packet_data) == 7u,
               "FRAME command 0x0A data must start at byte 7");
_Static_assert(offsetof(bootloader_protocol_data_request_t, packet_crc) == 1031u,
               "FRAME command 0x0A packet CRC must start at byte 1031");
_Static_assert(sizeof(bootloader_protocol_data_ack_payload_t) == 1u,
               "FRAME command 0x0A ACK payload must be 1 byte");
_Static_assert(sizeof(bootloader_protocol_end_request_t) == 2u,
               "FRAME command 0x0B request payload must be 2 bytes");
_Static_assert(sizeof(bootloader_protocol_end_ack_payload_t) == 1u,
               "FRAME command 0x0B ACK payload must be 1 byte");

#endif /* BOOTLOADER_PROTOCOL_TYPES_H */
