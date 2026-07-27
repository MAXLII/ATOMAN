// SPDX-License-Identifier: MIT
/**
 * @file    fal_cfg.c
 * @brief   Zynq-7020 bootloader FAL device and zone tables.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Describe the board's single 16 MiB PS QSPI NOR device
 *          - Allocate aligned Bootloader, IAP, staging, metadata, and layout zones
 *          - Keep the Bootloader self zone outside the online-upgrade logical mapping
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Configuration objects are immutable after construction
 *          - QSPI register access remains in the BSP driver
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

#include "bsp_qspi_flash.h"

#include <stddef.h>

static const fal_device_cfg_t s_devices[] = {
    {
        .device_id = FAL_DEVICE_ZYNQ_QSPI,
        .capacity = BSP_QSPI_FLASH_CAPACITY,
        .program_page_size = BSP_QSPI_FLASH_PAGE_SIZE,
        .erase_block_size = BSP_QSPI_FLASH_ERASE_SIZE,
        .max_read_size = BSP_QSPI_FLASH_MAX_READ,
        .ops = {
            .p_context = NULL,
            .p_init = bsp_qspi_flash_init,
            .p_get_state = bsp_qspi_flash_state_get,
            .p_read = bsp_qspi_flash_read,
            .p_program = bsp_qspi_flash_program,
            .p_erase = bsp_qspi_flash_erase,
            .p_sync = bsp_qspi_flash_sync,
        },
    },
};

static const fal_zone_cfg_t s_zones[] = {
    {FAL_ZONE_ZYNQ_BOOT, FAL_DEVICE_ZYNQ_QSPI, ZYNQ7020_QSPI_BOOT_OFFSET,
     ZYNQ7020_QSPI_BOOT_SIZE, FAL_ZONE_PERMISSION_READ},
    {FAL_ZONE_ZYNQ_IAP, FAL_DEVICE_ZYNQ_QSPI, ZYNQ7020_QSPI_IAP_OFFSET,
     ZYNQ7020_QSPI_IAP_SIZE, FAL_ZONE_PERMISSION_ALL},
    {FAL_ZONE_ZYNQ_IAP_STAGING, FAL_DEVICE_ZYNQ_QSPI, ZYNQ7020_QSPI_STAGING_OFFSET,
     ZYNQ7020_QSPI_STAGING_SIZE, FAL_ZONE_PERMISSION_ALL},
    {FAL_ZONE_ZYNQ_UPDATE_META_A, FAL_DEVICE_ZYNQ_QSPI, ZYNQ7020_QSPI_META_A_OFFSET,
     ZYNQ7020_QSPI_SMALL_ZONE_SIZE, FAL_ZONE_PERMISSION_ALL},
    {FAL_ZONE_ZYNQ_UPDATE_META_B, FAL_DEVICE_ZYNQ_QSPI, ZYNQ7020_QSPI_META_B_OFFSET,
     ZYNQ7020_QSPI_SMALL_ZONE_SIZE, FAL_ZONE_PERMISSION_ALL},
    {FAL_ZONE_ZYNQ_LAYOUT, FAL_DEVICE_ZYNQ_QSPI, ZYNQ7020_QSPI_LAYOUT_OFFSET,
     ZYNQ7020_QSPI_SMALL_ZONE_SIZE, FAL_ZONE_PERMISSION_READ},
};

const fal_cfg_t g_zynq7020_fal_cfg = {
    .p_devices = s_devices,
    .device_count = (uint16_t)(sizeof(s_devices) / sizeof(s_devices[0])),
    .p_zones = s_zones,
    .zone_count = (uint16_t)(sizeof(s_zones) / sizeof(s_zones[0])),
};

const bootloader_fal_zone_map_t
    g_zynq7020_bootloader_zone_map[BOOTLOADER_FLASH_ZONE_COUNT] = {
        {BOOTLOADER_FLASH_ZONE_IAP, FAL_ZONE_ZYNQ_IAP},
        {BOOTLOADER_FLASH_ZONE_STAGING, FAL_ZONE_ZYNQ_IAP_STAGING},
        {BOOTLOADER_FLASH_ZONE_META_A, FAL_ZONE_ZYNQ_UPDATE_META_A},
        {BOOTLOADER_FLASH_ZONE_META_B, FAL_ZONE_ZYNQ_UPDATE_META_B},
        {BOOTLOADER_FLASH_ZONE_LAYOUT, FAL_ZONE_ZYNQ_LAYOUT},
    };
