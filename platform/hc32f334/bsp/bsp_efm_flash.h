// SPDX-License-Identifier: MIT
/**
 * @file    bsp_efm_flash.h
 * @brief   HC32F334 embedded flash physical driver interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose the 128 KiB EFM as a physical-address flash device
 *          - Enforce native four-byte programming and 4 KiB erase geometry
 *          - Provide driver state and result types independent from FAL
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; operations execute from the bootloader foreground
 *          - Flash controller access is confined to this BSP module
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

#ifndef BSP_EFM_FLASH_H
#define BSP_EFM_FLASH_H

#include <stdint.h>

#define BSP_EFM_FLASH_CAPACITY_BYTES 0x00020000UL
#define BSP_EFM_FLASH_PROGRAM_SIZE   4UL
#define BSP_EFM_FLASH_ERASE_SIZE     0x00001000UL

typedef enum
{
    BSP_EFM_FLASH_RESULT_SUCCESS = 0,
    BSP_EFM_FLASH_RESULT_INVALID_ARGUMENT = -1,
    BSP_EFM_FLASH_RESULT_OUT_OF_RANGE = -2,
    BSP_EFM_FLASH_RESULT_IO_ERROR = -3
} bsp_efm_flash_result_t;

typedef enum
{
    BSP_EFM_FLASH_STATE_READY = 0,
    BSP_EFM_FLASH_STATE_ERROR
} bsp_efm_flash_state_t;

bsp_efm_flash_result_t bsp_efm_flash_init(void);
bsp_efm_flash_state_t bsp_efm_flash_state_get(void);
bsp_efm_flash_result_t bsp_efm_flash_read(uint32_t address, uint32_t length, uint8_t *p_data);
bsp_efm_flash_result_t bsp_efm_flash_program(uint32_t address,
                                            uint32_t length,
                                            const uint8_t *p_data);
bsp_efm_flash_result_t bsp_efm_flash_erase(uint32_t address, uint32_t length);

#endif /* BSP_EFM_FLASH_H */
