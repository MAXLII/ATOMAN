// SPDX-License-Identifier: MIT
/**
 * @file    hc32_boot_handoff.c
 * @brief   HC32F334 retained bootloader handoff implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Store an upgrade request and its inverse in retained SRAM
 *          - Reject random or partially written retained values
 *          - Reset the MCU to re-enter the bootloader at address zero
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Retained storage is not initialized by startup code
 *          - Request publication is completed with a data synchronization barrier
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

#include "hc32_boot_handoff.h"

#include "hc32f3xx.h"

#define HC32_BOOT_REQUEST_MAGIC 0x42544C44UL

typedef struct
{
    uint32_t magic;
    uint32_t inverse;
} hc32_boot_request_t;

static hc32_boot_request_t s_boot_request
    __attribute__((used, section(".noinit.boot_request")));

bootloader_result_t hc32_boot_request_set(void)
{
    s_boot_request.magic = HC32_BOOT_REQUEST_MAGIC;
    s_boot_request.inverse = ~HC32_BOOT_REQUEST_MAGIC;
    __DSB();
    return BOOTLOADER_RESULT_SUCCESS;
}

bootloader_result_t hc32_boot_request_clear(void)
{
    s_boot_request.magic = 0UL;
    s_boot_request.inverse = ~0UL;
    __DSB();
    return BOOTLOADER_RESULT_SUCCESS;
}

bootloader_boot_reason_t hc32_boot_request_get(void)
{
    return ((HC32_BOOT_REQUEST_MAGIC == s_boot_request.magic) &&
            ((~HC32_BOOT_REQUEST_MAGIC) == s_boot_request.inverse))
               ? BOOTLOADER_BOOT_REASON_IAP_REQUEST
               : BOOTLOADER_BOOT_REASON_POWER_ON;
}

bootloader_result_t hc32_boot_handoff_enter(void)
{
    NVIC_SystemReset();
    return BOOTLOADER_RESULT_STORAGE_ERROR;
}
