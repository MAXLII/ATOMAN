// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_fal_adapter.h
 * @brief   Mount FAL as the bootloader logical flash service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Map bootloader logical zones to platform-defined FAL zones
 *          - Translate FAL results and zone geometry into bootloader types
 *          - Publish bootloader_flash_ops_t without leaking FAL into the core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; the owning bootloader task serializes calls
 *          - Hardware access remains behind the mounted FAL configuration
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

#ifndef BOOTLOADER_FAL_ADAPTER_H
#define BOOTLOADER_FAL_ADAPTER_H

#include "bootloader_flash.h"
#include "fal_core.h"

#include <stdint.h>

typedef struct
{
    bootloader_flash_zone_t bootloader_zone; /**< Bootloader-visible logical zone. */
    fal_zone_id_t fal_zone;                  /**< Platform-defined FAL zone. */
} bootloader_fal_zone_map_t;

typedef struct
{
    fal_t *p_fal;                                  /**< Mounted FAL state-machine instance. */
    const fal_api_t *p_fal_api;                    /**< Mounted FAL implementation table. */
    const fal_cfg_t *p_fal_cfg;                    /**< Platform cfg mounted during initialization. */
    const bootloader_fal_zone_map_t *p_zone_map;   /**< Logical-to-FAL zone mapping array. */
    uint16_t zone_map_count;                       /**< Entries in p_zone_map. */
} bootloader_fal_adapter_t;

bootloader_result_t bootloader_fal_adapter_mount(
    bootloader_fal_adapter_t *p_adapter,
    fal_t *p_fal,
    const fal_api_t *p_fal_api,
    const fal_cfg_t *p_fal_cfg,
    const bootloader_fal_zone_map_t *p_zone_map,
    uint16_t zone_map_count,
    bootloader_flash_ops_t *p_flash_ops);

#endif /* BOOTLOADER_FAL_ADAPTER_H */
