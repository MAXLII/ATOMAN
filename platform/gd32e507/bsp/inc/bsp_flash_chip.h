// SPDX-License-Identifier: MIT
/**
 * @file    bsp_flash_chip.h
 * @brief   Define bounded board Flash operations and diagnostics.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Define bounded board Flash operations and diagnostics.
 *          - Keep operations bounded and report invalid requests.
 *          Design notes:
 *          - C11 compatible; no dynamic memory allocation.
 *          - Task-context API; not ISR-safe.
 *          - Hardware access is confined to the platform BSP.
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef BSP_FLASH_CHIP_H
#define BSP_FLASH_CHIP_H
#include "bsp_flash.h"
/* Private board-driver dispatch contract. All calls run in one storage task. */
typedef struct
{
    BSP_FLASH_RESULT_E (*p_init)(void); /* Probe without erase/program operations. */
    BSP_FLASH_STATE_E (*p_state)(void); /* Progress one bounded step. */
    BSP_FLASH_RESULT_E (*p_read)(uint32_t address, uint32_t length, uint8_t *p_data); /* Begin read. */
    BSP_FLASH_RESULT_E (*p_program)(uint32_t address, uint32_t length, const uint8_t *p_data); /* Begin program. */
    BSP_FLASH_RESULT_E (*p_erase)(uint32_t address, uint32_t length); /* Begin erase. */
    bsp_flash_health_t *p_health; /* Driver-owned diagnostic state. */
} bsp_flash_chip_t;
extern const bsp_flash_chip_t bsp_nor_chip; /* GD25Q16 SQPI implementation. */
extern const bsp_flash_chip_t bsp_nand_chip; /* GD9FU1G8F2A EXMC implementation. */
#endif
