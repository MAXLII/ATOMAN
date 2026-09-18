// SPDX-License-Identifier: MIT
/**
 * @file    codec_rle.h
 * @brief   COMM v1 RLE / PackBits codec public interface.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare the COMM v1 CODEC=010 RLE / PackBits encode and decode entry points
 *          - Document the shared codec parameter order and return convention
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Token layout: 0x00..0x7F literal run of token+1 bytes,
 *            0x80..0xFF repeat run of (token & 0x7F) + 3 copies of one byte
 *          - A run of three or more identical bytes becomes a repeat token
 *
 * @author  Max.Li
 * @date    2026-11-04
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __CODEC_RLE_H__
#define __CODEC_RLE_H__

#include <stdint.h>

/**
 * @brief Compress an input byte stream with RLE / PackBits.
 * @param input_len Number of input bytes; must be greater than zero.
 * @param p_input Input byte stream owned by the caller.
 * @param p_output_len Input/output: caller writes the output capacity first;
 *                     on success the actual output length is written back,
 *                     on failure it is set to zero.
 * @param p_output Output buffer owned by the caller; must not overlap p_input.
 * @param limit_len Exclusive upper bound for the encoded output length.
 *                 Encoding stops and fails as soon as the generated length
 *                 reaches this bound.
 * @return 1 on success; 0 on failure (invalid arguments, capacity exceeded,
 *         input not fully consumed, or the limit bound was reached).
 */
int8_t codec_rle_encode(uint16_t input_len, const uint8_t *p_input,
                        uint16_t *p_output_len, uint8_t *p_output,
                        uint16_t limit_len);

/**
 * @brief Decompress an RLE / PackBits encoded input byte stream.
 * @param input_len Number of encoded bytes; must be greater than zero.
 * @param p_input Encoded byte stream owned by the caller.
 * @param p_output_len Input/output: caller writes the output capacity first;
 *                     on success the actual output length is written back,
 *                     on failure it is set to zero.
 * @param p_output Output buffer owned by the caller; must not overlap p_input.
 * @param limit_len Exclusive upper bound for the decoded output length.
 * @return 1 on success; 0 on failure (invalid arguments, capacity exceeded,
 *         input not fully consumed, or the limit bound was reached).
 */
int8_t codec_rle_decode(uint16_t input_len, const uint8_t *p_input,
                        uint16_t *p_output_len, uint8_t *p_output,
                        uint16_t limit_len);

#endif /* __CODEC_RLE_H__ */
