// SPDX-License-Identifier: MIT
/**
 * @file    iap_update_service.c
 * @brief   Platform-independent IAP upgrade-trigger service implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register and directly acknowledge FRAME command 0x08 in IAP images
 *          - Poll application preparation without blocking the communication callback
 *          - Publish a retained request and reset through the Section platform contract
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The IAP service is compiled only when IS_IAP is defined
 *          - Reset is delayed in software so queued ACK data can leave the communication link
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

#include "iap_update_service.h"

#if defined(IS_IAP)

#include "bootloader_update_request.h"
#include "comm.h"
#include "section.h"

#include <stddef.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define IAP_UPDATE_RETAINED __attribute__((used, section(".noinit.boot_request")))
#else
#define IAP_UPDATE_RETAINED
#endif

static volatile bootloader_update_request_t boot_request
    IAP_UPDATE_RETAINED; /* Shared no-init request at the same address in both images. */

#ifndef IAP_UPDATE_RESET_DELAY_MS
#define IAP_UPDATE_RESET_DELAY_MS 100u
#endif

static void boot_request_set(const iap_update_info_t *p_info)
{
    boot_request.magic = 0u;
    boot_request.info = *p_info;
    boot_request.checksum = bootloader_update_request_checksum_calculate(p_info);
    boot_request.magic = BOOTLOADER_UPDATE_REQUEST_MAGIC;
}

static iap_update_info_t upgrade_info = {0}; /* Accepted request retained while preparation runs. */
static uint32_t reset_delay_ms = 0u; /* Software drain time elapsed after preparation completes. */
static uint8_t transfer_pending = 0u; /* A valid 0x08 request is waiting for preparation and reset. */
static uint8_t prepare_ready = 0u; /* Application preparation completed successfully. */
static uint8_t reset_called = 0u; /* SYSTEM_RESET was already requested for the active transfer. */

__attribute__((weak)) iap_update_prepare_result_t
iap_update_prepare(const iap_update_info_t *p_info)
{
    (void)p_info;
    return IAP_UPDATE_PREPARE_READY_E;
}

static void update_info_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    iap_update_info_t update_info = {0}; /* Command 0x08 request retained while preparation runs. */
    bootloader_protocol_info_ack_t update_info_ack = {
        .allow_update = (uint8_t)BOOTLOADER_PROTOCOL_UPDATE_ACK_REJECT_E,
        .reject_reason = {
            .raw = 0u,
        },
    }; /* Command 0x08 direct ACK payload. */

    if ((p_pack == NULL) ||
        (my_printf == NULL) ||
        (p_pack->is_ack == 1u))
    {
        return;
    }

    if ((p_pack->p_data != NULL) &&
        (p_pack->len == BOOTLOADER_PROTOCOL_INFO_LENGTH) &&
        (transfer_pending == 0u))
    {
        (void)memcpy(&update_info, p_pack->p_data, sizeof(update_info));
        upgrade_info = update_info;
        reset_delay_ms = 0u;
        transfer_pending = 1u;
        prepare_ready = 0u;
        reset_called = 0u;
        update_info_ack.allow_update =
            (uint8_t)BOOTLOADER_PROTOCOL_UPDATE_ACK_ALLOW_E;
    }

    section_packform_t packform = {
        .src = p_pack->dst,
        .d_src = p_pack->d_dst,
        .dst = p_pack->src,
        .d_dst = p_pack->d_src,
        .cmd_set = BOOTLOADER_PROTOCOL_CMD_SET,
        .cmd_word = BOOTLOADER_PROTOCOL_CMD_INFO,
        .is_ack = 1u,
        .len = (uint16_t)sizeof(update_info_ack),
        .p_data = (uint8_t *)&update_info_ack,
    }; /* FRAME response routed back to the command 0x08 sender. */

    comm_send_data(&packform, my_printf);
}

static void iap_update_process(void)
{
    iap_update_prepare_result_t prepare_result =
        IAP_UPDATE_PREPARE_FAILED_E; /* Result of the current preparation step. */

    if ((transfer_pending == 0u) ||
        (reset_called == 1u))
    {
        return;
    }

    if (prepare_ready == 0u)
    {
        prepare_result = iap_update_prepare(&upgrade_info);
        if (prepare_result == IAP_UPDATE_PREPARE_IN_PROGRESS_E)
        {
            return;
        }
        if (prepare_result != IAP_UPDATE_PREPARE_READY_E)
        {
            transfer_pending = 0u;
            return;
        }
        prepare_ready = 1u;
    }

    if (reset_delay_ms < IAP_UPDATE_RESET_DELAY_MS)
    {
        reset_delay_ms++;
        return;
    }

    boot_request_set(&upgrade_info);
    reset_called = 1u;
    SYSTEM_RESET;
}

REG_TASK_MS(1u, iap_update_process)
REG_COMM(BOOTLOADER_PROTOCOL_CMD_SET, BOOTLOADER_PROTOCOL_CMD_INFO, update_info_act)

#endif /* IS_IAP */
