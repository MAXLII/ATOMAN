// SPDX-License-Identifier: MIT
/**
 * @file    zynq_iap_update_service.h
 * @brief   Zynq IAP callback mount for entering the independent bootloader.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose the user preparation callback reserved before bootloader entry
 *          - Keep IAP firmware-upgrade handling limited to FRAME command 0x08
 *          - Delegate retained request and control transfer to the Zynq handoff service
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Callback mounting completes before the first accepted upgrade request
 *          - Hardware access remains in Zynq BSP and handoff modules
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

#ifndef ZYNQ_IAP_UPDATE_SERVICE_H
#define ZYNQ_IAP_UPDATE_SERVICE_H

#include "bootloader_core.h"

typedef bootloader_result_t (*zynq_iap_prepare_t)(void *p_context,
                                                   const bootloader_upgrade_info_t *p_info);

bootloader_result_t zynq_iap_update_prepare_mount(zynq_iap_prepare_t p_prepare,
                                                   void *p_context);

#endif /* ZYNQ_IAP_UPDATE_SERVICE_H */
