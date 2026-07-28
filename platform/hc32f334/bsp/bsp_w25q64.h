// SPDX-License-Identifier: MIT
/**
 * @file    bsp_w25q64.h
 * @brief   HC32F334 board support interface for the W25Q64 SPI flash.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define geometry and JEDEC identity for the board-mounted W25Q64
 *          - Expose physical-address read, page-program, and sector-erase operations
 *          - Report device WIP state without blocking the upper flash state machine
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; calls are serialized by the FAL owner
 *          - Hardware access is isolated in the HC32F334 BSP
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

#ifndef BSP_W25Q64_H
#define BSP_W25Q64_H

#include <stdint.h>

#define BSP_W25Q64_CAPACITY_BYTES (8UL * 1024UL * 1024UL)
#define BSP_W25Q64_PAGE_SIZE 256UL
#define BSP_W25Q64_SECTOR_SIZE 4096UL
#define BSP_W25Q64_EXPECTED_JEDEC_ID 0x00EF4017UL

typedef enum
{
    BSP_W25Q64_RESULT_SUCCESS = 0,
    BSP_W25Q64_RESULT_INVALID_ARGUMENT = -1,
    BSP_W25Q64_RESULT_OUT_OF_RANGE = -2,
    BSP_W25Q64_RESULT_BUSY = -3,
    BSP_W25Q64_RESULT_IO_ERROR = -4,
    BSP_W25Q64_RESULT_ID_MISMATCH = -5,
    BSP_W25Q64_RESULT_TIMEOUT = -7
} bsp_w25q64_result_t;

typedef enum
{
    BSP_W25Q64_STATE_READY = 0,
    BSP_W25Q64_STATE_BUSY,
    BSP_W25Q64_STATE_ERROR
} bsp_w25q64_state_t;

bsp_w25q64_result_t bsp_w25q64_init(void);
bsp_w25q64_result_t bsp_w25q64_read_jedec_id(uint32_t *p_jedec_id);
bsp_w25q64_state_t bsp_w25q64_state_get(void);
bsp_w25q64_result_t bsp_w25q64_read(uint32_t address, uint32_t length, uint8_t *p_data);
bsp_w25q64_result_t bsp_w25q64_page_program(uint32_t address,
                                            uint32_t length,
                                            const uint8_t *p_data);
bsp_w25q64_result_t bsp_w25q64_sector_erase(uint32_t address);

#endif /* BSP_W25Q64_H */
