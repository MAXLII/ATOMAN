// SPDX-License-Identifier: MIT
/**
 * @file    iap_section_service.c
 * @brief   Section integration for the minimal IAP boot handoff.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Parse and directly acknowledge only FRAME command 0x08 in IAP
 *          - Leave firmware data commands unregistered in the IAP image
 *          - Periodically complete the deferred transfer after transport drain
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The communication callback does not reset or jump directly
 *          - Section is intentionally confined to this service boundary
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

#include "iap_section_service.h"

#include "bootloader_protocol.h"
#include "comm.h"

#include <stddef.h>

static iap_boot_service_t *s_p_iap_service;

static void iap_info_handle(section_packform_t *p_request, DEC_MY_PRINTF)
{
    section_packform_t response = {0};
    uint8_t ack[BOOTLOADER_PROTOCOL_MAX_ACK_LENGTH] = {0};
    uint16_t ack_length = 0u;

    if ((s_p_iap_service == NULL) || (p_request == NULL) || (my_printf == NULL))
    {
        return;
    }
    (void)iap_boot_service_info_handle(s_p_iap_service,
                                       p_request->p_data,
                                       p_request->len,
                                       ack,
                                       (uint16_t)sizeof(ack),
                                       &ack_length);
    response.cmd_set = p_request->cmd_set;
    response.cmd_word = p_request->cmd_word;
    response.dst = p_request->src;
    response.d_dst = p_request->d_src;
    response.src = p_request->dst;
    response.d_src = p_request->d_dst;
    response.is_ack = 1u;
    response.len = ack_length;
    response.p_data = ack;
    comm_send_data(&response, my_printf);
}

static void iap_section_process(void)
{
    if (s_p_iap_service != NULL)
    {
        iap_boot_service_process(s_p_iap_service);
    }
}

bootloader_result_t iap_section_service_mount(iap_boot_service_t *p_service)
{
    if (p_service == NULL)
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }
    s_p_iap_service = p_service;
    return BOOTLOADER_RESULT_SUCCESS;
}

REG_TASK_MS(1u, iap_section_process)
REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_INFO, iap_info_handle)
