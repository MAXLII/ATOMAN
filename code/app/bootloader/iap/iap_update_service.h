// SPDX-License-Identifier: MIT
/**
 * @file    iap_update_service.h
 * @brief   Platform-independent IAP upgrade-trigger service interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Describe the minimal FRAME 0x08 upgrade information
 *          - Expose the polled application preparation callback
 *          - Define the preparation states used before the reset transfer
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The service depends only on FRAME communication and Section runtime contracts
 *          - The implementation publishes the accepted request before reset
 *
 * @author  Max.Li
 * @date    2026-07-29
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef IAP_UPDATE_SERVICE_H
#define IAP_UPDATE_SERVICE_H

#include "bootloader_protocol_types.h"

#include <stdint.h>

typedef enum
{
    IAP_UPDATE_PREPARE_READY_E = 0,    /**< Preparation completed; reset is now permitted. */
    IAP_UPDATE_PREPARE_IN_PROGRESS_E,  /**< Preparation is active and must be polled again. */
    IAP_UPDATE_PREPARE_FAILED_E        /**< Preparation failed; remain in the current IAP. */
} iap_update_prepare_result_t;

typedef bootloader_protocol_info_request_t iap_update_info_t;

/**
 * @brief Advance application shutdown and persistence before entering Bootloader.
 * @param p_info Accepted upgrade information retained by the IAP service.
 * @return IAP_UPDATE_PREPARE_READY_E when preparation has completed.
 * @return IAP_UPDATE_PREPARE_IN_PROGRESS_E while preparation must continue.
 * @return IAP_UPDATE_PREPARE_FAILED_E when this transfer must be cancelled.
 */
iap_update_prepare_result_t iap_update_prepare(const iap_update_info_t *p_info);

#endif /* IAP_UPDATE_SERVICE_H */
