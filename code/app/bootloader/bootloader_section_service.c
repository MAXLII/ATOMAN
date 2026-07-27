// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_section_service.c
 * @brief   Section integration for the platform-independent bootloader services.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Convert registered FRAME commands into bootloader protocol calls
 *          - Emit same-command direct ACK frames through the receiving link
 *          - Periodically advance Bootloader Core and its mounted FAL service
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Communication callbacks do not wait for flash completion
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

#include "bootloader_section_service.h"

#include "comm.h"

#include <stddef.h>

static bootloader_t *s_p_bootloader;
static bootloader_protocol_t *s_p_protocol;

static void command_handle(section_packform_t *p_request, DEC_MY_PRINTF)
{
    section_packform_t response = {0};
    uint8_t ack[BOOTLOADER_PROTOCOL_MAX_ACK_LENGTH] = {0};
    uint16_t ack_length = 0u;

    if ((s_p_protocol == NULL) || (p_request == NULL) || (my_printf == NULL))
    {
        return;
    }
    (void)bootloader_protocol_handle(s_p_protocol,
                                     p_request->cmd_word,
                                     p_request->p_data,
                                     p_request->len,
                                     ack,
                                     (uint16_t)sizeof(ack),
                                     &ack_length);
    if (ack_length == 0u)
    {
        return;
    }
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

static void bootloader_section_process(void)
{
    if (s_p_bootloader != NULL)
    {
        bootloader_process(s_p_bootloader);
    }
}

bootloader_result_t bootloader_section_service_mount(bootloader_t *p_bootloader,
                                                       bootloader_protocol_t *p_protocol)
{
    if ((p_bootloader == NULL) || (p_protocol == NULL) ||
        (p_protocol->p_bootloader != p_bootloader))
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }
    s_p_bootloader = p_bootloader;
    s_p_protocol = p_protocol;
    return BOOTLOADER_RESULT_SUCCESS;
}

REG_TASK_MS(1u, bootloader_section_process)
REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_INFO, command_handle)
REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_READY, command_handle)
REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_DATA, command_handle)
REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_END, command_handle)
