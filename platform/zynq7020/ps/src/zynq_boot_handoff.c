// SPDX-License-Identifier: MIT
/**
 * @file    zynq_boot_handoff.c
 * @brief   Retained Zynq IAP-to-Bootloader handoff implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Read the retained request committed by the independent IAP service
 *          - Validate the record integrity fields after Bootloader startup
 *          - Clear the consumed request before launching the application
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The record occupies a fixed on-chip-memory location
 *          - The transfer target is the Bootloader link address
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

#include "zynq_boot_handoff.h"

#include <stdint.h>

#define ZYNQ_BOOT_REASON_ADDRESS 0x0003FFF0u
#define ZYNQ_BOOT_REASON_MAGIC 0x42544C44u

typedef struct
{
    uint32_t magic;
    uint32_t reason;
    uint32_t inverted_magic;
} zynq_boot_reason_record_t;

static volatile zynq_boot_reason_record_t *record_get(void)
{
    return (volatile zynq_boot_reason_record_t *)(uintptr_t)ZYNQ_BOOT_REASON_ADDRESS;
}

bootloader_boot_reason_t zynq_boot_request_get(void)
{
    volatile const zynq_boot_reason_record_t *p_record = record_get();

    if ((p_record->magic == ZYNQ_BOOT_REASON_MAGIC) &&
        (p_record->inverted_magic == (uint32_t)(~ZYNQ_BOOT_REASON_MAGIC)) &&
        (p_record->reason == (uint32_t)BOOTLOADER_BOOT_REASON_IAP_REQUEST_E))
    {
        return BOOTLOADER_BOOT_REASON_IAP_REQUEST_E;
    }
    return BOOTLOADER_BOOT_REASON_POWER_ON_E;
}

bootloader_result_t zynq_boot_request_clear(void)
{
    volatile zynq_boot_reason_record_t *p_record = record_get();

    p_record->magic = 0u;
    p_record->reason = 0u;
    p_record->inverted_magic = 0u;
    __asm__ volatile("dmb sy" ::: "memory");
    return BOOTLOADER_RESULT_SUCCESS_E;
}
