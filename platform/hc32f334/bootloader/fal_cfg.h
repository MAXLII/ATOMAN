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
 *          - Publish the HC32 FAL configuration and shared instance
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

#include "fal_core.h"

#define HC32F334_EFM_BLOCK_SIZE (4UL * 1024UL)
#define HC32F334_W25Q64_BLOCK_SIZE (4UL * 1024UL)

#define HC32F334_BOOT_SIZE (4UL * HC32F334_EFM_BLOCK_SIZE)
#define HC32F334_IAP_SIZE (28UL * HC32F334_EFM_BLOCK_SIZE)
#define HC32F334_FLASH_END (32UL * HC32F334_EFM_BLOCK_SIZE)

#define HC32F334_STAGING_SIZE (32UL * HC32F334_W25Q64_BLOCK_SIZE)
#define HC32F334_META_A_SIZE (1UL * HC32F334_W25Q64_BLOCK_SIZE)
#define HC32F334_META_B_SIZE (1UL * HC32F334_W25Q64_BLOCK_SIZE)
#define HC32F334_LAYOUT_SIZE (1UL * HC32F334_W25Q64_BLOCK_SIZE)

#ifndef HC32F334_IAP_BASE
#define HC32F334_IAP_BASE HC32F334_BOOT_SIZE
#endif

typedef enum
{
    FAL_DEVICE_HC32_EFM_E = 1U,
    FAL_DEVICE_HC32_W25Q64_E = 2U
} hc32f334_fal_device_id_t;

typedef enum
{
    FAL_ZONE_HC32_BOOT_E = 1U,
    FAL_ZONE_HC32_IAP_E,
    FAL_ZONE_HC32_IAP_STAGING_E,
    FAL_ZONE_HC32_UPDATE_META_A_E,
    FAL_ZONE_HC32_UPDATE_META_B_E,
    FAL_ZONE_HC32_LAYOUT_E
} hc32f334_fal_zone_id_t;

extern const fal_cfg_t g_hc32f334_fal_cfg;
extern fal_t g_hc32f334_fal;

#endif /* HC32F334_BOOTLOADER_FAL_CFG_H */
