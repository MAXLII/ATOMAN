// SPDX-License-Identifier: MIT
/**
 * @file    bsp_qspi_flash.h
 * @brief   Zynq-7020 PS QSPI NOR driver mounted by FAL cfg.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize the PS QSPI controller and verify the attached JEDEC device
 *          - Submit page program and sector erase commands with polled completion state
 *          - Expose physical-address operations matching fal_flash_ops_t
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Calls are serialized by one FAL instance
 *          - XQspiPs and the board MIO assignment remain confined to this BSP
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

#ifndef BSP_QSPI_FLASH_H
#define BSP_QSPI_FLASH_H

#include "fal_core.h"

#include <stdint.h>

#define BSP_QSPI_FLASH_CAPACITY (16u * 1024u * 1024u)
#define BSP_QSPI_FLASH_PAGE_SIZE 256u
#define BSP_QSPI_FLASH_ERASE_SIZE (64u * 1024u)
#define BSP_QSPI_FLASH_MAX_READ 1024u

fal_result_t bsp_qspi_flash_init(void *p_context);
fal_device_state_t bsp_qspi_flash_state_get(void *p_context);
fal_result_t bsp_qspi_flash_read(void *p_context,
                                 uint32_t address,
                                 uint32_t length,
                                 uint8_t *p_data);
fal_result_t bsp_qspi_flash_program(void *p_context,
                                    uint32_t address,
                                    uint32_t length,
                                    const uint8_t *p_data);
fal_result_t bsp_qspi_flash_erase(void *p_context, uint32_t address, uint32_t length);
fal_result_t bsp_qspi_flash_sync(void *p_context);
void *bsp_qspi_flash_context_get(void);

#endif /* BSP_QSPI_FLASH_H */
