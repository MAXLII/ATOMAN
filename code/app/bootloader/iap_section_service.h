// SPDX-License-Identifier: MIT
/**
 * @file    iap_section_service.h
 * @brief   Section binding for the minimal IAP boot handoff service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Mount the minimal IAP handoff state into Section
 *          - Register only the existing FRAME 0x08 firmware information command
 *          - Advance deferred bootloader entry after the direct ACK completes
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The IAP does not allocate the 0x0A firmware payload context
 *          - Platform reset retention and transfer remain mounted callbacks
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

#ifndef IAP_SECTION_SERVICE_H
#define IAP_SECTION_SERVICE_H

#include "iap_boot_service.h"

bootloader_result_t iap_section_service_mount(iap_boot_service_t *p_service);

#endif /* IAP_SECTION_SERVICE_H */
