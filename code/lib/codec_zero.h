// SPDX-License-Identifier: MIT
/**
 * @file    codec_zero.h
 * @brief   COMM v1 ZERO (nibble zero-count) codec public interface.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare the COMM v1 CODEC=100 nibble zero-count encode and decode entry points
 *          - Document the shared codec parameter order and return convention
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Each nibble n encodes as 1^k + 0^(m+1) + 1 with k = n >> 2,
 *            m = n & 3; bits fill output bytes MSB-first and trailing bits
 *            of the last byte are padded with ones
 *          - Favors data dense in small values; uniform nibbles expand to
 *            5 bits each on average
 *
 * @author  Max.Li
 * @date    2026-09-19
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __CODEC_ZERO_H__
#define __CODEC_ZERO_H__

#include <stdint.h>

/**
 * @brief Compress an input byte stream with the nibble zero-count code.
 * @param input_len Number of input bytes; must be greater than zero.
 * @param p_input Input byte stream owned by the caller.
 * @param p_output_len Input/output: caller writes the output capacity first;
 *                     on success the actual output length is written back,
 *                     on failure it is set to zero.
 * @param p_output Output buffer owned by the caller; must not overlap p_input.
 * @param limit_len Exclusive upper bound for the encoded output length.
 * @return 1 on success; 0 on failure (invalid arguments, capacity exceeded,
 *         or the limit bound was reached).
 */
int8_t codec_zero_encode(uint16_t input_len, const uint8_t *p_input,
                         uint16_t *p_output_len, uint8_t *p_output,
                         uint16_t limit_len);

/**
 * @brief Decompress a nibble zero-count encoded input byte stream.
 * @param input_len Number of encoded bytes; must be greater than zero.
 * @param p_input Encoded byte stream owned by the caller.
 * @param p_output_len Input/output: caller writes the output capacity first;
 *                     on success the actual output length is written back,
 *                     on failure it is set to zero.
 * @param p_output Output buffer owned by the caller; must not overlap p_input.
 * @param limit_len Exclusive upper bound for the decoded output length.
 * @return 1 on success; 0 on failure (invalid arguments, capacity exceeded,
 *         truncated nibble, odd nibble count, or the limit bound reached).
 */
int8_t codec_zero_decode(uint16_t input_len, const uint8_t *p_input,
                         uint16_t *p_output_len, uint8_t *p_output,
                         uint16_t limit_len);

#endif /* __CODEC_ZERO_H__ */
