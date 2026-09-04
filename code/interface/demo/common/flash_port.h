// SPDX-License-Identifier: MIT
/**
 * @file    flash_port.h
 * @brief   Adapt demo Flash operations exclusively through the selected BSP.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Adapt demo Flash operations exclusively through the selected BSP.
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
#ifndef FLASH_PORT_H
#define FLASH_PORT_H
#include <stdint.h>
#define FLASH_PORT_COUNT 2u /* Device selectors, 0 NOR and 1 NAND. */

typedef enum
{
    FLASH_PORT_SUCCESS = 0, /* Physical request accepted; query state for completion. */
    FLASH_PORT_INVALID_ARGUMENT = -1, /* Invalid selector, address or transfer shape. */
    FLASH_PORT_IO_ERROR = -2 /* Physical device operation failed. */
} FLASH_PORT_RESULT_E;
typedef enum
{
    FLASH_PORT_READY = 0, /* Transfer completed and buffers released. */
    FLASH_PORT_BUSY, /* Transfer in progress; caller retains buffers. */
    FLASH_PORT_ERROR /* Device requires recovery or reinitialization. */
} FLASH_PORT_STATE_E;
typedef struct
{
    uint32_t chip_id; /* Observed physical identity. */
    uint32_t corrected_bits; /* Cumulative corrected bits this boot. */
    uint32_t bad_blocks; /* Boot-local bad/quarantined block count. */
    int32_t error; /* Last board-driver diagnostic. */
} flash_port_health_t;
typedef struct
{
    uint32_t capacity; /* Main-area capacity in bytes. */
    uint32_t page_size; /* Program-page boundary in bytes. */
    uint32_t block_size; /* Physical erase unit in bytes. */
    uint32_t program_unit; /* Required program address/length alignment. */
} flash_port_geometry_t;

/** @param device Physical selector. @return Probe result; initialization does not erase or program. */
FLASH_PORT_RESULT_E flash_port_init(uint8_t device);
/** @param device Physical selector. @return State after one bounded progress step. */
FLASH_PORT_STATE_E flash_port_state_get(uint8_t device);
/** @param device Selector. @param address Physical byte address. @param length Byte count.
 * @param p_data Destination retained until READY/ERROR. @return Submission result. */
FLASH_PORT_RESULT_E flash_port_read(uint8_t device, uint32_t address, uint32_t length, uint8_t *p_data);
/** @param device Selector. @param address Physical byte address. @param length Byte count.
 * @param p_data Source retained until READY/ERROR. @return Submission result. */
FLASH_PORT_RESULT_E flash_port_program(uint8_t device, uint32_t address, uint32_t length, const uint8_t *p_data);
/** @param device Selector. @param address Physical byte address. @param length Erase bytes.
 * @return Submission result. All operations require one serialized task context. */
FLASH_PORT_RESULT_E flash_port_erase(uint8_t device, uint32_t address, uint32_t length);
/** @param device Physical index. @param p_geometry Output physical geometry.
 * @return 1 for a valid device/output, otherwise 0. */
uint8_t flash_port_geometry_get(uint8_t device, flash_port_geometry_t *p_geometry);
/** @param device Index. @param p_health Output diagnostics. */
void flash_port_health_get(uint8_t device, flash_port_health_t *p_health);
#endif
