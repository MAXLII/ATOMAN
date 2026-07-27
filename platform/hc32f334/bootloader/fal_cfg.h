// SPDX-License-Identifier: MIT
/**
 * @file    fal_cfg.h
 * @brief   HC32F334 bootloader flash device and partition configuration.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define HC32 embedded-flash and W25Q64 FAL device identifiers
 *          - Describe the internal IAP and external staging/metadata partitions
 *          - Publish the Bootloader-to-FAL logical zone mapping
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Configuration is immutable after link
 *          - Hardware operations remain in HC32 BSP drivers
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

#ifndef HC32F334_BOOTLOADER_FAL_CFG_H
#define HC32F334_BOOTLOADER_FAL_CFG_H

#include "bootloader_fal_adapter.h"
#include "fal_core.h"

#ifndef HC32F334_IAP_BASE
#define HC32F334_IAP_BASE 0x00004000UL
#endif

#define HC32F334_FLASH_END             0x00020000UL
#define HC32F334_BOOT_SIZE             HC32F334_IAP_BASE
#define HC32F334_IAP_SIZE              (HC32F334_FLASH_END - HC32F334_IAP_BASE)
#define HC32F334_STAGING_OFFSET        0x00000000UL
#define HC32F334_STAGING_SIZE          0x00020000UL
#define HC32F334_META_A_OFFSET         0x00020000UL
#define HC32F334_META_B_OFFSET         0x00021000UL
#define HC32F334_LAYOUT_OFFSET         0x00022000UL
#define HC32F334_SMALL_ZONE_SIZE       0x00001000UL

typedef enum
{
    FAL_DEVICE_HC32_EFM = 1U,
    FAL_DEVICE_HC32_W25Q64 = 2U
} hc32f334_fal_device_id_t;

typedef enum
{
    FAL_ZONE_HC32_BOOT = 1U,
    FAL_ZONE_HC32_IAP,
    FAL_ZONE_HC32_IAP_STAGING,
    FAL_ZONE_HC32_UPDATE_META_A,
    FAL_ZONE_HC32_UPDATE_META_B,
    FAL_ZONE_HC32_LAYOUT
} hc32f334_fal_zone_id_t;

extern const fal_cfg_t g_hc32f334_fal_cfg;
extern const bootloader_fal_zone_map_t
    g_hc32f334_bootloader_zone_map[BOOTLOADER_FLASH_ZONE_COUNT];

#endif /* HC32F334_BOOTLOADER_FAL_CFG_H */
