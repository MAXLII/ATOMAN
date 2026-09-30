// SPDX-License-Identifier: MIT
/**
 * @file    fake_fal_cfg.h
 * @brief   Host FAL configuration fixture.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define stable fake device and zone identifiers
 *          - Build valid and deliberately invalid FAL configurations
 *          - Expose fake device instances for test observation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Test-only configuration
 *          - Hardware access is replaced by fake_flash
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

#ifndef FAKE_FAL_CFG_H
#define FAKE_FAL_CFG_H

#include "fake_flash.h"

enum
{
    FAKE_FAL_ZONE_BOOT = 1,   /**< Read-only protected boot image. */
    FAKE_FAL_ZONE_IAP = 2,    /**< Writable application image. */
    FAKE_FAL_ZONE_STAGING = 3,/**< Writable staged application image. */
    FAKE_FAL_ZONE_SECOND = 4  /**< Zone located on the second fake device. */
};

typedef struct
{
    fake_flash_t first_flash;      /**< Primary fake device. */
    fake_flash_t second_flash;     /**< Secondary fake device. */
    fal_device_cfg_t devices[2];   /**< Mutable device table. */
    fal_zone_cfg_t zones[4];       /**< Mutable logical partition table. */
    fal_cfg_t cfg;                 /**< Configuration mounted by FAL. */
} fake_fal_fixture_t;

void fake_fal_fixture_reset(fake_fal_fixture_t *p_fixture);

#endif /* FAKE_FAL_CFG_H */
