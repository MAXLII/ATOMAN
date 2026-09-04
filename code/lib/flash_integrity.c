// SPDX-License-Identifier: MIT
/**
 * @file    flash_integrity.c
 * @brief   Provide portable Flash record CRC and octet encoding.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Provide portable Flash record CRC and octet encoding.
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
#include "flash_integrity.h"
#include <stddef.h>
uint32_t flash_integrity_crc32(const uint8_t *p_data, uint32_t length)
{
    uint32_t crc = UINT32_MAX; /* Reflected CRC register. */
    if (p_data == NULL) { return 0u; }
    for (uint32_t i = 0u; i < length; i++)
    {
        crc ^= p_data[i];
        for (uint32_t bit = 0u; bit < 8u; bit++)
        {
            uint32_t mask = 0u - (crc & 1u); /* Conditional polynomial mask. */
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}
uint32_t flash_integrity_u32_get(const uint8_t *p_data)
{
    if (p_data == NULL) { return 0u; }
    return (uint32_t)p_data[0] | ((uint32_t)p_data[1] << 8u) |
           ((uint32_t)p_data[2] << 16u) | ((uint32_t)p_data[3] << 24u);
}
void flash_integrity_u32_put(uint8_t *p_data, uint32_t value)
{
    if (p_data == NULL) { return; }
    for (uint32_t i = 0u; i < 4u; i++) { p_data[i] = (uint8_t)(value >> (8u * i)); }
}
