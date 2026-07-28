// SPDX-License-Identifier: MIT
/**
 * @file    fal_cfg.c
 * @brief   HC32F334 bootloader FAL configuration implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Adapt the HC32 EFM and W25Q64 BSP results to FAL callbacks
 *          - Define aligned device and logical partition tables
 *          - Own and schedule the shared HC32 FAL instance
 *          - Keep the Bootloader self region read-only and outside online mappings
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Configuration objects are immutable after construction
 *          - Physical address calculations remain in FAL Core
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

#include "fal_cfg.h"

#include "bsp_efm_flash.h"
#include "bsp_w25q64.h"
#include "section.h"

#include <stddef.h>

_Static_assert(HC32F334_EFM_BLOCK_SIZE == BSP_EFM_FLASH_ERASE_SIZE,
               "HC32 EFM partition unit must match the erase block");
_Static_assert(HC32F334_W25Q64_BLOCK_SIZE == BSP_W25Q64_SECTOR_SIZE,
               "W25Q64 partition unit must match the erase sector");
_Static_assert(HC32F334_IAP_BASE == HC32F334_BOOT_SIZE,
               "IAP base must immediately follow the Bootloader partition");
_Static_assert(HC32F334_FLASH_END == BSP_EFM_FLASH_CAPACITY_BYTES,
               "EFM partition sizes must cover the complete device");

static fal_result_t efm_init(void *p_context)
{
    bsp_efm_flash_result_t result;

    (void)p_context;
    result = bsp_efm_flash_init();
    if (BSP_EFM_FLASH_RESULT_SUCCESS == result)
    {
        result = bsp_efm_flash_write_range_enable(HC32F334_IAP_BASE, HC32F334_IAP_SIZE);
    }
    return (BSP_EFM_FLASH_RESULT_SUCCESS == result)
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static fal_device_state_t efm_state_get(void *p_context)
{
    (void)p_context;
    return (BSP_EFM_FLASH_STATE_READY == bsp_efm_flash_state_get())
               ? FAL_DEVICE_STATE_READY
               : FAL_DEVICE_STATE_ERROR;
}

static fal_result_t efm_read(void *p_context, uint32_t address, uint32_t length, uint8_t *p_data)
{
    (void)p_context;
    return (BSP_EFM_FLASH_RESULT_SUCCESS == bsp_efm_flash_read(address, length, p_data))
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static fal_result_t efm_program(void *p_context,
                                uint32_t address,
                                uint32_t length,
                                const uint8_t *p_data)
{
    (void)p_context;
    return (BSP_EFM_FLASH_RESULT_SUCCESS == bsp_efm_flash_program(address, length, p_data))
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static fal_result_t efm_erase(void *p_context, uint32_t address, uint32_t length)
{
    (void)p_context;
    return (BSP_EFM_FLASH_RESULT_SUCCESS == bsp_efm_flash_erase(address, length))
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static fal_result_t w25q_init(void *p_context)
{
    (void)p_context;
    return (BSP_W25Q64_RESULT_SUCCESS == bsp_w25q64_init())
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static fal_device_state_t w25q_state_get(void *p_context)
{
    const bsp_w25q64_state_t state = bsp_w25q64_state_get();
    (void)p_context;
    if (BSP_W25Q64_STATE_READY == state)
    {
        return FAL_DEVICE_STATE_READY;
    }
    return (BSP_W25Q64_STATE_BUSY == state) ? FAL_DEVICE_STATE_BUSY : FAL_DEVICE_STATE_ERROR;
}

static fal_result_t w25q_read(void *p_context, uint32_t address, uint32_t length, uint8_t *p_data)
{
    (void)p_context;
    return (BSP_W25Q64_RESULT_SUCCESS == bsp_w25q64_read(address, length, p_data))
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static fal_result_t w25q_program(void *p_context,
                                 uint32_t address,
                                 uint32_t length,
                                 const uint8_t *p_data)
{
    (void)p_context;
    return (BSP_W25Q64_RESULT_SUCCESS == bsp_w25q64_page_program(address, length, p_data))
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static fal_result_t w25q_erase(void *p_context, uint32_t address, uint32_t length)
{
    (void)p_context;
    if (BSP_W25Q64_SECTOR_SIZE != length)
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    return (BSP_W25Q64_RESULT_SUCCESS == bsp_w25q64_sector_erase(address))
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}

static const fal_zone_cfg_t s_efm_zones[] = {
    {
        .zone_id = FAL_ZONE_HC32_BOOT_E,
        .size = HC32F334_BOOT_SIZE,
        .permissions = FAL_ZONE_PERMISSION_READ,
    },
    {
        .zone_id = FAL_ZONE_HC32_IAP_E,
        .size = HC32F334_IAP_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
};

static const fal_zone_cfg_t s_w25q64_zones[] = {
    {
        .zone_id = FAL_ZONE_HC32_IAP_STAGING_E,
        .size = HC32F334_STAGING_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
    {
        .zone_id = FAL_ZONE_HC32_UPDATE_META_A_E,
        .size = HC32F334_META_A_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
    {
        .zone_id = FAL_ZONE_HC32_UPDATE_META_B_E,
        .size = HC32F334_META_B_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
    {
        .zone_id = FAL_ZONE_HC32_LAYOUT_E,
        .size = HC32F334_LAYOUT_SIZE,
        .permissions = FAL_ZONE_PERMISSION_READ,
    },
};

static const fal_device_cfg_t s_devices[] = {
    {
        .device_id = FAL_DEVICE_HC32_EFM_E,
        .capacity = BSP_EFM_FLASH_CAPACITY_BYTES,
        .program_page_size = BSP_EFM_FLASH_PROGRAM_SIZE,
        .erase_block_size = BSP_EFM_FLASH_ERASE_SIZE,
        .max_read_size = 1024UL,
        .p_zones = s_efm_zones,
        .zone_count = (uint16_t)(sizeof(s_efm_zones) / sizeof(s_efm_zones[0])),
        .ops = {
            .p_context = NULL,
            .p_init = efm_init,
            .p_get_state = efm_state_get,
            .p_read = efm_read,
            .p_program = efm_program,
            .p_erase = efm_erase,
            .p_sync = NULL,
        },
    },
    {
        .device_id = FAL_DEVICE_HC32_W25Q64_E,
        .capacity = BSP_W25Q64_CAPACITY_BYTES,
        .program_page_size = BSP_W25Q64_PAGE_SIZE,
        .erase_block_size = BSP_W25Q64_SECTOR_SIZE,
        .max_read_size = 1024UL,
        .p_zones = s_w25q64_zones,
        .zone_count = (uint16_t)(sizeof(s_w25q64_zones) / sizeof(s_w25q64_zones[0])),
        .ops = {
            .p_context = NULL,
            .p_init = w25q_init,
            .p_get_state = w25q_state_get,
            .p_read = w25q_read,
            .p_program = w25q_program,
            .p_erase = w25q_erase,
            .p_sync = NULL,
        },
    },
};

const fal_cfg_t g_hc32f334_fal_cfg = {
    .p_devices = s_devices,
    .device_count = (uint16_t)(sizeof(s_devices) / sizeof(s_devices[0])),
};

/*
 * The immutable FAL configuration and the Section-managed runtime service
 * currently remain in this file. Although the responsibilities are not an
 * ideal match, they are intentionally kept together until the platform FAL
 * service boundary is reorganized.
 */
fal_t g_hc32f334_fal = {0}; /* Shared platform FAL state-machine instance. */

static void fal_service_init(void)
{
    (void)fal_init(&g_hc32f334_fal, &g_hc32f334_fal_cfg);
}

static void fal_service_process(void)
{
    fal_process(&g_hc32f334_fal);
}

REG_INIT(0, fal_service_init)
REG_TASK_MS(1u, fal_service_process)
