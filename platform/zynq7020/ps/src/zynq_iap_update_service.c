// SPDX-License-Identifier: MIT
/**
 * @file    zynq_iap_update_service.c
 * @brief   Minimal Section-registered Zynq IAP bootloader handoff.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Mount the user preparation hook into the generic IAP boot service
 *          - Set the retained request only after preparation succeeds
 *          - Wait for the PS UART ACK to drain before entering Bootloader DDR code
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - IAP does not register 0x09, 0x0A, or 0x0B handlers
 *          - Hardware access remains in Zynq BSP and handoff modules
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

#include "zynq_iap_update_service.h"

#include "bsp_usart.h"
#include "iap_boot_service.h"
#include "iap_section_service.h"
#include "section.h"
#include "zynq_boot_handoff.h"

#include <stddef.h>

static iap_boot_service_t s_service;
static zynq_iap_prepare_t s_p_prepare;
static void *s_p_prepare_context;

static bootloader_result_t default_prepare(void *p_context,
                                           const bootloader_upgrade_info_t *p_info)
{
    (void)p_context;
    (void)p_info;
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t prepare_call(void *p_context,
                                        const bootloader_upgrade_info_t *p_info)
{
    (void)p_context;
    return s_p_prepare(s_p_prepare_context, p_info);
}

static bootloader_result_t request_set(void *p_context)
{
    (void)p_context;
    return zynq_boot_request_set();
}

static uint8_t tx_is_idle(void *p_context)
{
    (void)p_context;
    return bsp_usart_dbg_tx_is_idle();
}

static bootloader_result_t enter_bootloader(void *p_context)
{
    (void)p_context;
    return zynq_enter_bootloader();
}

static void zynq_iap_update_init(void)
{
    const iap_boot_service_ops_t ops = {
        .p_context = NULL,
        .p_prepare = prepare_call,
        .p_boot_reason_set = request_set,
        .p_tx_is_idle = tx_is_idle,
        .p_enter_bootloader = enter_bootloader,
    };

    if (s_p_prepare == NULL)
    {
        s_p_prepare = default_prepare;
    }
    if (iap_boot_service_init(&s_service, &ops) == BOOTLOADER_RESULT_SUCCESS)
    {
        (void)iap_section_service_mount(&s_service);
    }
}

bootloader_result_t zynq_iap_update_prepare_mount(zynq_iap_prepare_t p_prepare,
                                                   void *p_context)
{
    if (p_prepare == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    s_p_prepare = p_prepare;
    s_p_prepare_context = p_context;
    return BOOTLOADER_RESULT_SUCCESS;
}

REG_INIT(1, zynq_iap_update_init)
