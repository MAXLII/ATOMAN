// SPDX-License-Identifier: MIT
/**
 * @file    bsp_flash.c
 * @brief   Dispatch board NOR and NAND operations without cross-device state sharing.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Dispatch board NOR and NAND operations without cross-device state sharing.
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
#include "bsp_flash.h"
#include "bsp_flash_chip.h"
#include <stddef.h>
static const bsp_flash_chip_t *const chips[BSP_FLASH_COUNT] = { /* Indexed physical drivers. */
    &bsp_nor_chip, &bsp_nand_chip
};
void bsp_flash_geometry_get(uint8_t device, uint32_t *p_capacity, uint32_t *p_page,
                            uint32_t *p_block, uint32_t *p_unit)
{
    if ((p_capacity == NULL) || (p_page == NULL) || (p_block == NULL) || (p_unit == NULL)) { return; }
    if (device >= BSP_FLASH_COUNT)
    { *p_capacity = 0u; *p_page = 0u; *p_block = 0u; *p_unit = 0u; return; }
    if (device == 0u)
    {
        *p_capacity = 2u * 1024u * 1024u; *p_page = 256u; *p_block = 4096u; *p_unit = 1u;
    }
    else
    {
        *p_capacity = 128u * 1024u * 1024u; *p_page = 2048u; *p_block = 128u * 1024u; *p_unit = 2048u;
    }
}
void bsp_flash_health_get(uint8_t device, bsp_flash_health_t *p_health)
{
    if ((device < BSP_FLASH_COUNT) && (p_health != NULL)) { *p_health = *chips[device]->p_health; }
}
BSP_FLASH_RESULT_E bsp_flash_init(uint8_t device)
{
    return (device < BSP_FLASH_COUNT) ? chips[device]->p_init() : BSP_FLASH_INVALID_ARGUMENT;
}
BSP_FLASH_STATE_E bsp_flash_state(uint8_t device)
{
    return (device < BSP_FLASH_COUNT) ? chips[device]->p_state() : BSP_FLASH_ERROR;
}
BSP_FLASH_RESULT_E bsp_flash_read(uint8_t device, uint32_t address, uint32_t length, uint8_t *p_data)
{
    return (device < BSP_FLASH_COUNT) ? chips[device]->p_read(address, length, p_data) : BSP_FLASH_INVALID_ARGUMENT;
}
BSP_FLASH_RESULT_E bsp_flash_program(uint8_t device, uint32_t address, uint32_t length, const uint8_t *p_data)
{
    return (device < BSP_FLASH_COUNT) ? chips[device]->p_program(address, length, p_data) : BSP_FLASH_INVALID_ARGUMENT;
}
BSP_FLASH_RESULT_E bsp_flash_erase(uint8_t device, uint32_t address, uint32_t length)
{
    return (device < BSP_FLASH_COUNT) ? chips[device]->p_erase(address, length) : BSP_FLASH_INVALID_ARGUMENT;
}
