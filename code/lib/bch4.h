// SPDX-License-Identifier: MIT
/**
 * @file    bch4.h
 * @brief   Encode and correct shortened BCH(8191,8139) NAND sectors.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Encode and correct shortened BCH(8191,8139) NAND sectors.
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
#ifndef BCH4_H
#define BCH4_H
#include <stdint.h>
#define BCH4_DATA_SIZE 512u /* Payload octets per ECC sector. */
#define BCH4_ECC_SIZE 7u /* 52 parity bits, little-endian polynomial order. */
/** @param p_data 512 data bytes. @param p_ecc Output parity, 7 bytes. */
void bch4_encode(const uint8_t *p_data, uint8_t *p_ecc);
/** @param p_data In/out sector. @param p_ecc In/out parity.
 * @return Corrected bit count (0..4), or -1 when decoding fails.
 * @note Buffers may be modified on failure. More than four errors require an
 * independent integrity check (such as CRC); BCH alone may miscorrect. */
int32_t bch4_correct(uint8_t *p_data, uint8_t *p_ecc);
#endif
