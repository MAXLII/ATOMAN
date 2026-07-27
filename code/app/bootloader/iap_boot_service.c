// SPDX-License-Identifier: MIT
/**
 * @file    iap_boot_service.c
 * @brief   Minimal IAP-to-bootloader handoff implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Parse the stable prefix of the FRAME 0x08 payload
 *          - Execute user preparation and persistent boot-request callbacks in order
 *          - Enter the bootloader only after the mounted transport becomes idle
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

#include "iap_boot_service.h"

#include <stddef.h>
#include <string.h>

static uint32_t read_u32_le(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] |
           ((uint32_t)p_data[1] << 8u) |
           ((uint32_t)p_data[2] << 16u) |
           ((uint32_t)p_data[3] << 24u);
}

bootloader_result_t iap_boot_service_init(iap_boot_service_t *p_service,
                                          const iap_boot_service_ops_t *p_ops)
{
    if ((p_service == NULL) || (p_ops == NULL) ||
        (p_ops->p_prepare == NULL) ||
        (p_ops->p_boot_reason_set == NULL) ||
        (p_ops->p_tx_is_idle == NULL) ||
        (p_ops->p_enter_bootloader == NULL))
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }
    (void)memset(p_service, 0, sizeof(*p_service));
    p_service->ops = *p_ops;
    p_service->result = BOOTLOADER_RESULT_SUCCESS;
    return BOOTLOADER_RESULT_SUCCESS;
}

bootloader_result_t iap_boot_service_info_handle(iap_boot_service_t *p_service,
                                                 const uint8_t *p_payload,
                                                 uint16_t payload_length,
                                                 uint8_t *p_ack,
                                                 uint16_t ack_capacity,
                                                 uint16_t *p_ack_length)
{
    bootloader_result_t result = BOOTLOADER_RESULT_SUCCESS;

    if ((p_service == NULL) || (p_payload == NULL) || (p_ack == NULL) ||
        (p_ack_length == NULL) || (ack_capacity < 3u))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    *p_ack_length = 3u;
    p_ack[0] = 2u;
    p_ack[1] = 0u;
    p_ack[2] = 0u;
    if (payload_length < 10u)
    {
        p_service->result = BOOTLOADER_RESULT_PROTOCOL_ERROR;
        return p_service->result;
    }
    if (p_service->transfer_pending != 0u)
    {
        return BOOTLOADER_RESULT_BUSY;
    }

    p_service->upgrade_info.module_id = p_payload[0];
    p_service->upgrade_info.version = read_u32_le(&p_payload[1]);
    p_service->upgrade_info.file_size = read_u32_le(&p_payload[5]);
    p_service->upgrade_info.update_type = p_payload[9];
    result = p_service->ops.p_prepare(p_service->ops.p_context, &p_service->upgrade_info);
    if (result == BOOTLOADER_RESULT_SUCCESS)
    {
        result = p_service->ops.p_boot_reason_set(p_service->ops.p_context);
    }
    if (result == BOOTLOADER_RESULT_SUCCESS)
    {
        p_service->transfer_pending = 1u;
        p_service->transfer_called = 0u;
        p_ack[0] = 1u;
    }
    p_service->result = result;
    return result;
}

void iap_boot_service_process(iap_boot_service_t *p_service)
{
    if ((p_service == NULL) || (p_service->transfer_pending == 0u) ||
        (p_service->transfer_called != 0u) ||
        (p_service->ops.p_tx_is_idle(p_service->ops.p_context) == 0u))
    {
        return;
    }
    p_service->transfer_called = 1u;
    p_service->result = p_service->ops.p_enter_bootloader(p_service->ops.p_context);
    if (p_service->result != BOOTLOADER_RESULT_SUCCESS)
    {
        p_service->transfer_pending = 0u;
    }
}
