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
 *          - Keep the Bootloader self region read-only and outside online mappings
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Configuration objects are immutable
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

#include <stddef.h>

static fal_result_t efm_init(void *p_context)
{
    (void)p_context;
    return (BSP_EFM_FLASH_RESULT_SUCCESS == bsp_efm_flash_init())
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

static const fal_device_cfg_t s_devices[] = {
    {
        .device_id = FAL_DEVICE_HC32_EFM,
        .capacity = BSP_EFM_FLASH_CAPACITY_BYTES,
        .program_page_size = BSP_EFM_FLASH_PROGRAM_SIZE,
        .erase_block_size = BSP_EFM_FLASH_ERASE_SIZE,
        .max_read_size = 1024UL,
        .ops = {NULL, efm_init, efm_state_get, efm_read, efm_program, efm_erase, NULL},
    },
    {
        .device_id = FAL_DEVICE_HC32_W25Q64,
        .capacity = BSP_W25Q64_CAPACITY_BYTES,
        .program_page_size = BSP_W25Q64_PAGE_SIZE,
        .erase_block_size = BSP_W25Q64_SECTOR_SIZE,
        .max_read_size = 1024UL,
        .ops = {NULL, w25q_init, w25q_state_get, w25q_read, w25q_program, w25q_erase, NULL},
    },
};

static const fal_zone_cfg_t s_zones[] = {
    {FAL_ZONE_HC32_BOOT, FAL_DEVICE_HC32_EFM, 0UL, HC32F334_BOOT_SIZE, FAL_ZONE_PERMISSION_READ},
    {FAL_ZONE_HC32_IAP, FAL_DEVICE_HC32_EFM, HC32F334_IAP_BASE, HC32F334_IAP_SIZE, FAL_ZONE_PERMISSION_ALL},
    {FAL_ZONE_HC32_IAP_STAGING, FAL_DEVICE_HC32_W25Q64, HC32F334_STAGING_OFFSET, HC32F334_STAGING_SIZE, FAL_ZONE_PERMISSION_ALL},
    {FAL_ZONE_HC32_UPDATE_META_A, FAL_DEVICE_HC32_W25Q64, HC32F334_META_A_OFFSET, HC32F334_SMALL_ZONE_SIZE, FAL_ZONE_PERMISSION_ALL},
    {FAL_ZONE_HC32_UPDATE_META_B, FAL_DEVICE_HC32_W25Q64, HC32F334_META_B_OFFSET, HC32F334_SMALL_ZONE_SIZE, FAL_ZONE_PERMISSION_ALL},
    {FAL_ZONE_HC32_LAYOUT, FAL_DEVICE_HC32_W25Q64, HC32F334_LAYOUT_OFFSET, HC32F334_SMALL_ZONE_SIZE, FAL_ZONE_PERMISSION_READ},
};

const fal_cfg_t g_hc32f334_fal_cfg = {
    s_devices,
    (uint16_t)(sizeof(s_devices) / sizeof(s_devices[0])),
    s_zones,
    (uint16_t)(sizeof(s_zones) / sizeof(s_zones[0])),
};

const bootloader_fal_zone_map_t
    g_hc32f334_bootloader_zone_map[BOOTLOADER_FLASH_ZONE_COUNT] = {
        {BOOTLOADER_FLASH_ZONE_IAP, FAL_ZONE_HC32_IAP},
        {BOOTLOADER_FLASH_ZONE_STAGING, FAL_ZONE_HC32_IAP_STAGING},
        {BOOTLOADER_FLASH_ZONE_META_A, FAL_ZONE_HC32_UPDATE_META_A},
        {BOOTLOADER_FLASH_ZONE_META_B, FAL_ZONE_HC32_UPDATE_META_B},
        {BOOTLOADER_FLASH_ZONE_LAYOUT, FAL_ZONE_HC32_LAYOUT},
    };
