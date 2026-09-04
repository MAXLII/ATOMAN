// SPDX-License-Identifier: MIT
/**
 * @file    flash_port.c
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
#include "flash_port.h"
#include "bsp_flash.h"
#include <stddef.h>
/** @param result Physical submission result. @return Platform-independent interface result. */
static FLASH_PORT_RESULT_E result_map(BSP_FLASH_RESULT_E result)
{
    switch (result)
    {
    case BSP_FLASH_SUCCESS: return FLASH_PORT_SUCCESS;
    case BSP_FLASH_INVALID_ARGUMENT: return FLASH_PORT_INVALID_ARGUMENT;
    default: return FLASH_PORT_IO_ERROR;
    }
}
FLASH_PORT_RESULT_E flash_port_init(uint8_t device)
{
    return result_map(bsp_flash_init(device));
}
FLASH_PORT_STATE_E flash_port_state_get(uint8_t device)
{
    switch (bsp_flash_state(device))
    {
    case BSP_FLASH_READY: return FLASH_PORT_READY;
    case BSP_FLASH_BUSY: return FLASH_PORT_BUSY;
    default: return FLASH_PORT_ERROR;
    }
}
FLASH_PORT_RESULT_E flash_port_read(uint8_t device, uint32_t address, uint32_t length, uint8_t *p_data)
{
    return result_map(bsp_flash_read(device, address, length, p_data));
}
FLASH_PORT_RESULT_E flash_port_program(uint8_t device, uint32_t address, uint32_t length, const uint8_t *p_data)
{
    return result_map(bsp_flash_program(device, address, length, p_data));
}
FLASH_PORT_RESULT_E flash_port_erase(uint8_t device, uint32_t address, uint32_t length)
{
    return result_map(bsp_flash_erase(device, address, length));
}

uint8_t flash_port_geometry_get(uint8_t device, flash_port_geometry_t *p_geometry)
{
    if ((device >= FLASH_PORT_COUNT) || /* Reject unknown physical selector. */
        (p_geometry == NULL)) /* Require caller-owned destination. */
    { return 0u; }
    bsp_flash_geometry_get(device, &p_geometry->capacity, &p_geometry->page_size,
                           &p_geometry->block_size, &p_geometry->program_unit);
    return 1u;
}
void flash_port_health_get(uint8_t device, flash_port_health_t *p_health)
{
    bsp_flash_health_t health = {0}; /* BSP-owned values copied across the interface. */
    if (p_health == NULL) { return; }
    bsp_flash_health_get(device, &health);
    p_health->chip_id = health.id;
    p_health->corrected_bits = health.corrected_bits;
    p_health->bad_blocks = health.bad_blocks;
    p_health->error = health.error;
}
