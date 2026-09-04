// SPDX-License-Identifier: MIT
/**
 * @file    bsp_flash.h
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
#ifndef BSP_FLASH_H
#define BSP_FLASH_H
#include <stdint.h>
#define BSP_FLASH_COUNT 2u /* NOR and NAND devices. */

typedef enum
{
    BSP_FLASH_SUCCESS = 0, /* Operation accepted by the physical driver. */
    BSP_FLASH_INVALID_ARGUMENT = -1, /* Invalid physical range or transfer shape. */
    BSP_FLASH_IO_ERROR = -2 /* Chip operation failed; inspect board diagnostics. */
} BSP_FLASH_RESULT_E;

typedef enum
{
    BSP_FLASH_READY = 0, /* Last physical operation completed. */
    BSP_FLASH_BUSY, /* Physical operation still in progress. */
    BSP_FLASH_ERROR /* Driver requires reinitialization after failure. */
} BSP_FLASH_STATE_E;
typedef struct
{
    uint32_t id; /* Last observed chip ID. */
    uint32_t corrected_bits; /* Corrected NAND bit count. */
    uint32_t bad_blocks; /* Distinct rejected blocks this boot. */
    int32_t error; /* 0 ready, -1 ID, -2 timeout, -3 protected, -4 bad block, -5 ECC, -6 IO, -7 write rule. */
} bsp_flash_health_t;
/** @param device 0 NOR, 1 NAND. @param p_health Diagnostic snapshot destination. */
void bsp_flash_health_get(uint8_t device, bsp_flash_health_t *p_health);
/** @param device Index. @param p_capacity Main bytes. @param p_page Program boundary.
 * @param p_block Erase unit. @param p_unit Required program alignment. */
void bsp_flash_geometry_get(uint8_t device, uint32_t *p_capacity, uint32_t *p_page,
                            uint32_t *p_block, uint32_t *p_unit);
/** @param device Physical device index. @return Device initialization outcome. */
BSP_FLASH_RESULT_E bsp_flash_init(uint8_t device);
/** @param device Physical device index. @return Current asynchronous operation state. */
BSP_FLASH_STATE_E bsp_flash_state(uint8_t device);
/** @param device Index. @param address Physical address. @param length Byte count. @param p_data Destination.
 * @return Submission outcome; caller retains buffers until READY or ERROR. */
BSP_FLASH_RESULT_E bsp_flash_read(uint8_t device, uint32_t address, uint32_t length, uint8_t *p_data);
/** @param device Index. @param address Physical address. @param length Byte count. @param p_data Source.
 * @return Submission outcome. */
BSP_FLASH_RESULT_E bsp_flash_program(uint8_t device, uint32_t address, uint32_t length, const uint8_t *p_data);
/** @param device Index. @param address Physical address. @param length Byte count. @return Submission outcome. */
BSP_FLASH_RESULT_E bsp_flash_erase(uint8_t device, uint32_t address, uint32_t length);
#endif
