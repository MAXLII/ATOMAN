// SPDX-License-Identifier: MIT
/**
 * @file codec_dict.h
 * @brief COMM v1 static dictionary codec public interface.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Declare the COMM v1 CODEC=001 dictionary encode and decode entry points
 *          - Expose the built-in codebook identity for codebook negotiation
 *          - Document the shared codec parameter order and return convention
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Both peers preset the same codebook; up to 64 entries of 1..256 bytes
 *          - Token layout: 0x00..0x7F literal run of token+1 bytes,
 *            0x80..0xBF dictionary reference index = token & 0x3F,
 *            0xC0..0xFF invalid
 *          - The encoder prefers the longest match and then the smallest index
 *
 * @author Max.Li
 * @date 2026-11-04
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __CODEC_DICT_H__
#define __CODEC_DICT_H__

#include <stdint.h>

/**
 * @brief Compress an input byte stream with the preset static dictionary.
 * @param input_len Number of input bytes; must be greater than zero.
 * @param p_input Input byte stream owned by the caller.
 * @param p_output_len Input/output: caller writes the output capacity first;
 *        on success the actual output length is written back,
 *        on failure it is set to zero.
 * @param p_output Output buffer owned by the caller; must not overlap p_input.
 * @param limit_len Exclusive upper bound for the encoded output length.
 *        Encoding stops and fails as soon as the generated length
 *        reaches this bound.
 * @return 1 on success; 0 on failure (invalid arguments, capacity exceeded,
 *         input not fully consumed, or the limit bound was reached).
 */
int8_t codec_dict_encode(uint16_t input_len,
                         const uint8_t *p_input,
                         uint16_t *p_output_len,
                         uint8_t *p_output,
                         uint16_t limit_len);

/**
 * @brief Decompress a dictionary encoded input byte stream.
 * @param input_len Number of encoded bytes; must be greater than zero.
 * @param p_input Encoded byte stream owned by the caller.
 * @param p_output_len Input/output: caller writes the output capacity first;
 *        on success the actual output length is written back,
 *        on failure it is set to zero.
 * @param p_output Output buffer owned by the caller; must not overlap p_input.
 * @param limit_len Exclusive upper bound for the decoded output length.
 * @return 1 on success; 0 on failure (invalid arguments, capacity exceeded,
 *         invalid token, input not fully consumed, or the limit bound reached).
 */
int8_t codec_dict_decode(uint16_t input_len,
                         const uint8_t *p_input,
                         uint16_t *p_output_len,
                         uint8_t *p_output,
                         uint16_t limit_len);

/**
 * @brief Return the built-in codebook identifier used in CODEC_SELECT.
 * @return Codebook id, currently fixed to 0.
 */
uint8_t codec_dict_codebook_id(void);

/**
 * @brief Return the built-in codebook version used in CODEC_SELECT.
 * @return Codebook version, currently fixed to 1.
 */
uint8_t codec_dict_codebook_version(void);

/**
 * @brief Return the CRC32 of the built-in codebook content.
 * @return IEEE CRC32 over the concatenated codebook entries, in payload order.
 */
uint32_t codec_dict_codebook_crc32(void);

#endif /* __CODEC_DICT_H__ */
