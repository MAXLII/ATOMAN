// SPDX-License-Identifier: MIT
/**
 * @file    hc32_boot_handoff.h
 * @brief   HC32F334 retained bootloader handoff interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Share the retained upgrade-request contract between IAP and Bootloader
 *          - Provide complemented request set, get, and clear operations
 *          - Enter the reset path after an IAP ACK has drained
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Retained words are excluded from C runtime initialization
 *          - Reset is the only IAP-to-Bootloader transfer mechanism
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

#ifndef HC32_BOOT_HANDOFF_H
#define HC32_BOOT_HANDOFF_H

#include "bootloader_core.h"

bootloader_result_t hc32_boot_request_set(void);
bootloader_result_t hc32_boot_request_clear(void);
bootloader_boot_reason_t hc32_boot_request_get(void);
bootloader_result_t hc32_boot_handoff_enter(void);

#endif /* HC32_BOOT_HANDOFF_H */
