// SPDX-License-Identifier: MIT
/**
 * @file    section_list_service.h
 * @brief   Runtime section-list debug protocol interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define directory and node-address query protocol identifiers
 *          - Define protocol version, name limits, and response status values
 *          - Keep protocol declarations independent from runtime-list ownership
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Protocol payloads are serialized explicitly in little-endian order
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-08-02
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __SECTION_LIST_SERVICE_H__
#define __SECTION_LIST_SERVICE_H__

#include <stdint.h>

#define CMD_SET_SECTION_LIST 0x01
#define CMD_WORD_SECTION_LIST_DIRECTORY 0x38
#define CMD_WORD_SECTION_LIST_NODE 0x39

#define SECTION_LIST_PROTOCOL_VERSION (1u)
#define SECTION_LIST_NAME_LEN_MAX (63u)

typedef enum
{
    SECTION_LIST_STATUS_OK = 0,
    SECTION_LIST_STATUS_INVALID_REQUEST = 1,
    SECTION_LIST_STATUS_INDEX_INVALID = 2,
    SECTION_LIST_STATUS_LIST_ID_INVALID = 3,
    SECTION_LIST_STATUS_NODE_INDEX_INVALID = 4,
    SECTION_LIST_STATUS_REGISTRATION_INVALID = 5,
    SECTION_LIST_STATUS_ADDRESS_UNAVAILABLE = 6
} section_list_status_t;

#endif /* __SECTION_LIST_SERVICE_H__ */
