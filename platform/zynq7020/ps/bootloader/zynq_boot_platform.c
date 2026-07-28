// SPDX-License-Identifier: MIT
/**
 * @file    zynq_boot_platform.c
 * @brief   Zynq-7020 platform boot decision and image transfer implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Implement retained boot-request callbacks used by Bootloader and IAP services
 *          - Validate the ARM reset vector and the complete image DDR range
 *          - Copy the QSPI IAP partition into DDR before a cache-safe control transfer
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Transfer functions execute with interrupts disabled
 *          - Zynq cache and address details remain confined to this platform module
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

#include "zynq_boot_platform.h"

#include "bsp_qspi_flash.h"
#include "fal_cfg.h"
#include "zynq_boot_handoff.h"
#include "xil_cache.h"
#include "xil_exception.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct
{
    bootloader_t *p_bootloader;
} zynq_boot_platform_t;

static zynq_boot_platform_t s_platform;

static bootloader_boot_reason_t boot_reason_get(void *p_context)
{
    (void)p_context;
    return zynq_boot_request_get();
}

static bootloader_result_t boot_reason_clear(void *p_context)
{
    (void)p_context;
    return zynq_boot_request_clear();
}

static bootloader_result_t image_header_is_valid(void *p_context,
                                                  const uint8_t *p_header,
                                                  uint32_t header_length,
                                                  uint32_t image_size,
                                                  uint8_t *p_valid)
{
    uint32_t reset_vector = 0u;

    (void)p_context;
    if ((p_header == NULL) || (p_valid == NULL) || (header_length < sizeof(reset_vector)))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }
    (void)memcpy(&reset_vector, p_header, sizeof(reset_vector));
    *p_valid = (((reset_vector & 0xFF000000u) == 0xEA000000u) &&
                (image_size != 0u) &&
                (image_size <= ZYNQ7020_QSPI_IAP_SIZE) &&
                (image_size <= (ZYNQ7020_DMA_RESERVED_ADDRESS - ZYNQ7020_IAP_DDR_ADDRESS)))
                   ? 1u
                   : 0u;
    return BOOTLOADER_RESULT_SUCCESS_E;
}

static bootloader_result_t jump_to_iap(void *p_context)
{
    zynq_boot_platform_t *p_platform = (zynq_boot_platform_t *)p_context;
    uint8_t *p_destination = (uint8_t *)(uintptr_t)ZYNQ7020_IAP_DDR_ADDRESS;
    uint32_t offset = 0u;
    uint32_t remaining = 0u;

    if ((p_platform == NULL) || (p_platform->p_bootloader == NULL))
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR_E;
    }
    remaining = p_platform->p_bootloader->upgrade_info.file_size;
    if ((remaining == 0u) || (remaining > ZYNQ7020_QSPI_IAP_SIZE))
    {
        return BOOTLOADER_RESULT_IMAGE_INVALID_E;
    }
    while (remaining != 0u)
    {
        const uint32_t chunk = (remaining > BSP_QSPI_FLASH_MAX_READ)
                                   ? BSP_QSPI_FLASH_MAX_READ
                                   : remaining;
        if (bsp_qspi_flash_read(NULL,
                                ZYNQ7020_QSPI_BOOT_SIZE + offset,
                                chunk,
                                &p_destination[offset]) != FAL_RESULT_SUCCESS)
        {
            return BOOTLOADER_RESULT_STORAGE_ERROR_E;
        }
        offset += chunk;
        remaining -= chunk;
    }
    Xil_DCacheFlushRange((INTPTR)ZYNQ7020_IAP_DDR_ADDRESS,
                         p_platform->p_bootloader->upgrade_info.file_size);
    Xil_ExceptionDisable();
    Xil_DCacheDisable();
    Xil_ICacheDisable();
    ((void (*)(void))(uintptr_t)ZYNQ7020_IAP_DDR_ADDRESS)();
    return BOOTLOADER_RESULT_STORAGE_ERROR_E;
}

bootloader_platform_ops_t zynq_boot_platform_ops_make(bootloader_t *p_bootloader)
{
    s_platform.p_bootloader = p_bootloader;
    return (bootloader_platform_ops_t){
        .p_context = &s_platform,
        .p_boot_reason_get = boot_reason_get,
        .p_boot_reason_clear = boot_reason_clear,
        .p_image_header_is_valid = image_header_is_valid,
        .p_jump_to_iap = jump_to_iap,
        .p_watchdog_kick = NULL,
    };
}
