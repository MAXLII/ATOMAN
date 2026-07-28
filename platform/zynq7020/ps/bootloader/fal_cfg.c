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
#include "section.h"

#include <stddef.h>

static const fal_zone_cfg_t s_qspi_zones[] = {
    {
        .zone_id = FAL_ZONE_ZYNQ_BOOT,
        .size = ZYNQ7020_QSPI_BOOT_SIZE,
        .permissions = FAL_ZONE_PERMISSION_READ,
    },
    {
        .zone_id = FAL_ZONE_ZYNQ_IAP,
        .size = ZYNQ7020_QSPI_IAP_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
    {
        .zone_id = FAL_ZONE_ZYNQ_IAP_STAGING,
        .size = ZYNQ7020_QSPI_STAGING_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
    {
        .zone_id = FAL_ZONE_ZYNQ_UPDATE_META_A,
        .size = ZYNQ7020_QSPI_SMALL_ZONE_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
    {
        .zone_id = FAL_ZONE_ZYNQ_UPDATE_META_B,
        .size = ZYNQ7020_QSPI_SMALL_ZONE_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
    {
        .zone_id = FAL_ZONE_ZYNQ_LAYOUT,
        .size = ZYNQ7020_QSPI_SMALL_ZONE_SIZE,
        .permissions = FAL_ZONE_PERMISSION_READ,
    },
};

static const fal_device_cfg_t s_devices[] = {
    {
        .device_id = FAL_DEVICE_ZYNQ_QSPI,
        .capacity = BSP_QSPI_FLASH_CAPACITY,
        .program_page_size = BSP_QSPI_FLASH_PAGE_SIZE,
        .erase_block_size = BSP_QSPI_FLASH_ERASE_SIZE,
        .max_read_size = BSP_QSPI_FLASH_MAX_READ,
        .p_zones = s_qspi_zones,
        .zone_count = (uint16_t)(sizeof(s_qspi_zones) / sizeof(s_qspi_zones[0])),
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

const fal_cfg_t g_zynq7020_fal_cfg = {
    .p_devices = s_devices,
    .device_count = (uint16_t)(sizeof(s_devices) / sizeof(s_devices[0])),
};

/*
 * The immutable FAL configuration and the Section-managed runtime service
 * currently remain in this file. Although the responsibilities are not an
 * ideal match, they are intentionally kept together until the platform FAL
 * service boundary is reorganized.
 */
fal_t g_zynq7020_fal = {0}; /* Shared platform FAL state-machine instance. */

static void fal_service_init(void)
{
    (void)fal_init(&g_zynq7020_fal, &g_zynq7020_fal_cfg);
}

static void fal_service_process(void)
{
    fal_process(&g_zynq7020_fal);
}

REG_INIT(0, fal_service_init)
REG_TASK_MS(1u, fal_service_process)
