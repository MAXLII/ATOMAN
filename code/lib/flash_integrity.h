// SPDX-License-Identifier: MIT
/**
 * @file    flash_integrity.h
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
#ifndef FLASH_INTEGRITY_H
#define FLASH_INTEGRITY_H
#include <stdint.h>
/** @param p_data Bytes to checksum. @param length Octet count. @return CRC-32/ISO-HDLC. */
uint32_t flash_integrity_crc32(const uint8_t *p_data, uint32_t length);
/** @param p_data Four little-endian bytes. @return Decoded value. */
uint32_t flash_integrity_u32_get(const uint8_t *p_data);
/** @param p_data Four writable bytes. @param value Value to encode. */
void flash_integrity_u32_put(uint8_t *p_data, uint32_t value);
#endif
