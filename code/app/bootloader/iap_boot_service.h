// SPDX-License-Identifier: MIT
/**
 * @file    iap_boot_service.h
 * @brief   Minimal IAP-side handoff service for entering the bootloader.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Accept only the existing 0x08 firmware information command in IAP
 *          - Invoke a caller-mounted preparation hook before requesting bootloader entry
 *          - Defer the platform transfer until the direct ACK has left the transport
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; callbacks and process calls must be serialized
 *          - Reset retention and platform transfer are mounted through function pointers
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

#ifndef IAP_BOOT_SERVICE_H
#define IAP_BOOT_SERVICE_H

#include "bootloader_core.h"

#include <stdint.h>

typedef struct
{
    void *p_context;
    bootloader_result_t (*p_prepare)(void *p_context,
                                     const bootloader_upgrade_info_t *p_info);
    bootloader_result_t (*p_boot_reason_set)(void *p_context);
    uint8_t (*p_tx_is_idle)(void *p_context);
    bootloader_result_t (*p_enter_bootloader)(void *p_context);
} iap_boot_service_ops_t;

typedef struct
{
    iap_boot_service_ops_t ops;
    bootloader_upgrade_info_t upgrade_info;
    bootloader_result_t result;
    uint8_t transfer_pending;
    uint8_t transfer_called;
} iap_boot_service_t;

bootloader_result_t iap_boot_service_init(iap_boot_service_t *p_service,
                                          const iap_boot_service_ops_t *p_ops);
bootloader_result_t iap_boot_service_info_handle(iap_boot_service_t *p_service,
                                                 const uint8_t *p_payload,
                                                 uint16_t payload_length,
                                                 uint8_t *p_ack,
                                                 uint16_t ack_capacity,
                                                 uint16_t *p_ack_length);
void iap_boot_service_process(iap_boot_service_t *p_service);

#endif /* IAP_BOOT_SERVICE_H */
