// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_update_request.h
 * @brief   Retained IAP-to-Bootloader upgrade request contract.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the reset-retained request shared by IAP and Bootloader images
 *          - Carry the complete accepted FRAME command 0x08 payload across reset
 *          - Provide a platform-independent integrity checksum for retained SRAM
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The commit marker is written last and cleared before updating the payload
 *          - Both images must place the request at the same no-init SRAM address
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

#ifndef BOOTLOADER_UPDATE_REQUEST_H
#define BOOTLOADER_UPDATE_REQUEST_H

#include "bootloader_protocol_types.h"

#include <stdint.h>

#define BOOTLOADER_UPDATE_REQUEST_MAGIC 0x42544C44u

#pragma pack(push, 1)

typedef struct
{
    uint32_t magic;                                /**< Commit marker written after payload and checksum. */
    bootloader_protocol_info_request_t info;       /**< Complete accepted FRAME command 0x08 payload. */
    uint16_t checksum;                             /**< Integrity checksum over info. */
} bootloader_update_request_t;

#pragma pack(pop)

/**
 * @brief Calculate the retained request checksum.
 * @param p_info Upgrade information whose byte representation is protected.
 * @return 16-bit checksum over the complete command 0x08 payload.
 */
static inline uint16_t bootloader_update_request_checksum_calculate(
    const bootloader_protocol_info_request_t *p_info)
{
    const uint8_t *p_data = (const uint8_t *)p_info; /* Packed payload bytes protected across reset. */
    uint16_t checksum = 0xA55Au;                    /* Non-zero checksum seed. */
    uint32_t index = 0u;                            /* Current payload byte index. */

    if (p_info == NULL)
    {
        return 0u;
    }
    for (index = 0u; index < (uint32_t)sizeof(*p_info); index++)
    {
        checksum = (uint16_t)((checksum << 5u) | (checksum >> 11u));
        checksum = (uint16_t)(checksum ^ p_data[index]);
    }
    return checksum;
}

_Static_assert(sizeof(bootloader_update_request_t) == 16u,
               "Retained IAP-to-Bootloader request must be 16 bytes");

#endif /* BOOTLOADER_UPDATE_REQUEST_H */
