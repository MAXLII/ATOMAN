// SPDX-License-Identifier: MIT
/**
 * @file    fake_fal_cfg.c
 * @brief   Host FAL configuration fixture implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Reset fake devices before each independent test case
 *          - Construct devices with different page, block, and read geometry
 *          - Construct non-overlapping zones with explicit access permissions
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

#include "fake_fal_cfg.h"

#include <stddef.h>
#include <string.h>

void fake_fal_fixture_reset(fake_fal_fixture_t *p_fixture)
{
    if (p_fixture == NULL)
    {
        return;
    }
    (void)memset(p_fixture, 0, sizeof(*p_fixture));
    fake_flash_reset(&p_fixture->first_flash);
    fake_flash_reset(&p_fixture->second_flash);

    p_fixture->devices[0].device_id = 10u;
    p_fixture->devices[0].capacity = FAKE_FLASH_CAPACITY;
    p_fixture->devices[0].program_page_size = 16u;
    p_fixture->devices[0].erase_block_size = 64u;
    p_fixture->devices[0].max_read_size = 24u;
    p_fixture->devices[0].p_zones = &p_fixture->zones[0];
    p_fixture->devices[0].zone_count = 3u;
    p_fixture->devices[0].ops = fake_flash_ops_make(&p_fixture->first_flash);

    p_fixture->devices[1].device_id = 20u;
    p_fixture->devices[1].capacity = FAKE_FLASH_CAPACITY;
    p_fixture->devices[1].program_page_size = 32u;
    p_fixture->devices[1].erase_block_size = 128u;
    p_fixture->devices[1].max_read_size = 0u;
    p_fixture->devices[1].p_zones = &p_fixture->zones[3];
    p_fixture->devices[1].zone_count = 1u;
    p_fixture->devices[1].ops = fake_flash_ops_make(&p_fixture->second_flash);

    p_fixture->zones[0] = (fal_zone_cfg_t){
        .zone_id = FAKE_FAL_ZONE_BOOT,
        .size = 512u,
        .permissions = FAL_ZONE_PERMISSION_READ,
    };
    p_fixture->zones[1] = (fal_zone_cfg_t){
        .zone_id = FAKE_FAL_ZONE_IAP,
        .size = 2048u,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    };
    p_fixture->zones[2] = (fal_zone_cfg_t){
        .zone_id = FAKE_FAL_ZONE_STAGING,
        .size = 2048u,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    };
    p_fixture->zones[3] = (fal_zone_cfg_t){
        .zone_id = FAKE_FAL_ZONE_SECOND,
        .size = 1024u,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    };

    p_fixture->cfg.p_devices = p_fixture->devices;
    p_fixture->cfg.device_count = 2u;
}
