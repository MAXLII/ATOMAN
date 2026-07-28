// SPDX-License-Identifier: MIT
/**
 * @file    fal_cfg.h
 * @brief   Zynq-7020 QSPI device and bootloader partition configuration.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define platform FAL zone identifiers and continuous QSPI allocation
 *          - Bind the Zynq PS QSPI BSP operation table to FAL Core
 *          - Keep platform Flash allocation independent from Bootloader interfaces
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

#ifndef ZYNQ7020_BOOTLOADER_FAL_CFG_H
#define ZYNQ7020_BOOTLOADER_FAL_CFG_H

#include "fal_core.h"

#include <stdint.h>

#define ZYNQ7020_QSPI_BOOT_SIZE 0x00500000u
#define ZYNQ7020_QSPI_IAP_SIZE 0x00300000u
#define ZYNQ7020_QSPI_STAGING_SIZE 0x00300000u
#define ZYNQ7020_QSPI_SMALL_ZONE_SIZE 0x00010000u

typedef enum
{
    FAL_DEVICE_ZYNQ_QSPI = 1u
} zynq7020_fal_device_id_t;

typedef enum
{
    FAL_ZONE_ZYNQ_BOOT = 1u,
    FAL_ZONE_ZYNQ_IAP,
    FAL_ZONE_ZYNQ_IAP_STAGING,
    FAL_ZONE_ZYNQ_UPDATE_META_A,
    FAL_ZONE_ZYNQ_UPDATE_META_B,
    FAL_ZONE_ZYNQ_LAYOUT
} zynq7020_fal_zone_id_t;

extern const fal_cfg_t g_zynq7020_fal_cfg;
extern fal_t g_zynq7020_fal; /* Shared platform FAL state-machine instance. */

#endif /* ZYNQ7020_BOOTLOADER_FAL_CFG_H */
