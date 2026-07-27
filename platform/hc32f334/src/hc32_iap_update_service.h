// SPDX-License-Identifier: MIT
/**
 * @file    hc32_iap_update_service.h
 * @brief   HC32F334 IAP-to-Bootloader upgrade trigger interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Mount the minimal IAP 0x08 service during Section initialization
 *          - Expose a weak user preparation callback before handoff
 *          - Defer software reset until the USART2 ACK has completed
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The IAP does not register firmware data or end commands
 *          - User preparation runs in the communication callback context
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

#ifndef HC32_IAP_UPDATE_SERVICE_H
#define HC32_IAP_UPDATE_SERVICE_H

#include "bootloader_core.h"

bootloader_result_t hc32_iap_update_prepare(const bootloader_upgrade_info_t *p_info);

#endif /* HC32_IAP_UPDATE_SERVICE_H */
