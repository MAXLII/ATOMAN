// SPDX-License-Identifier: MIT
/**
 * @file    hc32_iap_update_service.c
 * @brief   HC32F334 IAP-to-Bootloader upgrade trigger implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Mount user preparation, retained request, TX-idle, and reset callbacks
 *          - Register the shared minimal IAP Section service
 *          - Provide a weak application hook for shutdown and state preservation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Only FRAME command 0x08 is registered by the shared service
 *          - Reset occurs after USART2 DMA and shift-register completion
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

#include "hc32_iap_update_service.h"

#include "bsp_usart.h"
#include "hc32_boot_handoff.h"
#include "iap_boot_service.h"
#include "iap_section_service.h"
#include "section.h"

#include <stddef.h>

static iap_boot_service_t s_iap_boot_service;

__attribute__((weak)) bootloader_result_t
hc32_iap_update_prepare(const bootloader_upgrade_info_t *p_info)
{
    (void)p_info;
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t prepare(void *p_context, const bootloader_upgrade_info_t *p_info)
{
    (void)p_context;
    return hc32_iap_update_prepare(p_info);
}

static bootloader_result_t boot_reason_set(void *p_context)
{
    (void)p_context;
    return hc32_boot_request_set();
}

static uint8_t tx_is_idle(void *p_context)
{
    (void)p_context;
    return bsp_usart_iso_tx_is_idle();
}

static bootloader_result_t enter_bootloader(void *p_context)
{
    (void)p_context;
    return hc32_boot_handoff_enter();
}

static void hc32_iap_update_service_init(void)
{
    const iap_boot_service_ops_t ops = {
        NULL,
        prepare,
        boot_reason_set,
        tx_is_idle,
        enter_bootloader,
    };

    if ((BOOTLOADER_RESULT_SUCCESS != iap_boot_service_init(&s_iap_boot_service, &ops)) ||
        (BOOTLOADER_RESULT_SUCCESS != iap_section_service_mount(&s_iap_boot_service)))
    {
        for (;;)
        {
        }
    }
}

REG_INIT(20, hc32_iap_update_service_init)
