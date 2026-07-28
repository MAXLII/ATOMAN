// SPDX-License-Identifier: MIT
/**
 * @file    zynq_iap_update_service.h
 * @brief   Independent Zynq IAP upgrade-trigger interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Describe the minimal FRAME 0x08 upgrade information
 *          - Expose the application preparation callback mount
 *          - Keep IAP declarations independent from Bootloader Core types
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Callback mounting completes before the first accepted request
 *          - The implementation registers no firmware data command
 *
 * @author  Max.Li
 * @date    2026-07-28
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

#include <stdint.h>

typedef enum
{
    ZYNQ_IAP_UPDATE_RESULT_SUCCESS = 0,
    ZYNQ_IAP_UPDATE_RESULT_INVALID_ARGUMENT,
    ZYNQ_IAP_UPDATE_RESULT_REJECTED
} zynq_iap_update_result_t;

typedef struct
{
    uint8_t module_id;
    uint32_t version;
    uint32_t file_size;
    uint8_t update_type;
} zynq_iap_update_info_t;

typedef zynq_iap_update_result_t (*zynq_iap_prepare_t)(
    void *p_context,
    const zynq_iap_update_info_t *p_info);

zynq_iap_update_result_t zynq_iap_update_prepare_mount(zynq_iap_prepare_t p_prepare,
                                                        void *p_context);

#endif /* ZYNQ_IAP_UPDATE_SERVICE_H */
