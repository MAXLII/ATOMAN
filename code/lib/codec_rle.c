// SPDX-License-Identifier: MIT
/**
 * @file codec_rle.c
 * @brief COMM v1 RLE / PackBits codec implementation.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Encode a byte stream into RLE / PackBits tokens
 *          - Decode RLE / PackBits tokens back into the original byte stream
 *          - Enforce output capacity, the exclusive limit bound, and full input consumption
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Input and output buffers are caller-owned and must not overlap
 *          - Token layout: 0x00..0x7F literal run of token+1 bytes,
 *            0x80..0xFF repeat run of (token & 0x7F) + 3 copies of one byte
 *          - A run of three or more identical bytes becomes a repeat token;
 *            runs of one or two identical bytes stay inside a literal run
 *          - Literal runs hold at most 128 bytes (token 0x7F)
 *          - Repeat runs hold at most 130 bytes (token 0xFF)
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
#include "codec_rle.h"

#include <string.h>

/* Maximum literal run length expressed by one token. */
#define CODEC_RLE_LITERAL_MAX 128u
/* Maximum repeat run length expressed by one token. */
#define CODEC_RLE_REPEAT_MAX 130u
/* Minimum repeat run length that is encoded as a repeat token. */
#define CODEC_RLE_REPEAT_MIN 3u

static int8_t codec_rle_decode_fail(uint16_t *p_output_len)
{
    *p_output_len = 0u;
    return 0;
}

int8_t codec_rle_encode(uint16_t input_len,
                        const uint8_t *p_input,
                        uint16_t *p_output_len,
                        uint8_t *p_output,
                        uint16_t limit_len)
{
    uint16_t out_len  = 0u;
    uint16_t capacity = 0u;
    uint16_t index    = 0u;

    if (p_output_len == NULL)
    {
        return 0;
    }

    if (    (p_input == NULL)
         || (p_output == NULL)
         || (input_len == 0u)
         || (limit_len == 0u))
    {
        return codec_rle_decode_fail(p_output_len);
    }

    capacity      = *p_output_len;
    *p_output_len = 0u;

    if (capacity == 0u)
    {
        return 0;
    }

    while (index < input_len)
    {
        uint16_t run = 1u;

        while (    ((index + run) < input_len) /* The next byte still belongs to the same run. */
                && (p_input[index + run] == p_input[index])
                && (run < CODEC_RLE_REPEAT_MAX)) /* One token cannot express a longer run. */
        {
            run++;
        }

        if (run >= CODEC_RLE_REPEAT_MIN)
        {
            /* The repeat token plus its value byte must fit and stay below the limit. */

            if ((out_len + 2u) > capacity)
            {
                return 0;
            }

            if ((out_len + 2u) >= limit_len)
            {
                return codec_rle_decode_fail(p_output_len);
            }

            p_output[out_len] = (uint8_t)(0x80u | (uint8_t)(run - CODEC_RLE_REPEAT_MIN));
            out_len++;
            p_output[out_len] = p_input[index];
            out_len++;
            index += run;
        }
        else
        {
            /* Collect a literal run; short identical pairs stay literal. */
            uint16_t literal_start = index;
            uint16_t literal_len   = 0u;

            while (    (index < input_len)
                    && (literal_len < CODEC_RLE_LITERAL_MAX))
            {
                uint16_t ahead = 1u;

                while (    ((index + ahead) < input_len)
                        && (p_input[index + ahead] == p_input[index])
                        && (ahead < CODEC_RLE_REPEAT_MAX))
                {
                    ahead++;
                }

                if (ahead >= CODEC_RLE_REPEAT_MIN)
                {
                    /* The next run belongs to a repeat token instead. */
                    break;
                }

                if ((literal_len + ahead) > CODEC_RLE_LITERAL_MAX)
                {
                    /* The remaining identical bytes do not fit this literal run. */
                    break;
                }

                literal_len += ahead;
                index += ahead;
            }

            if ((out_len + 1u + literal_len) > capacity)
            {
                return 0;
            }

            if ((out_len + 1u + literal_len) >= limit_len)
            {
                return codec_rle_decode_fail(p_output_len);
            }

            p_output[out_len] = (uint8_t)(literal_len - 1u);
            out_len++;
            (void)memcpy(&p_output[out_len], &p_input[literal_start], literal_len);
            out_len += literal_len;
        }
    }

    /* A successful result must be non-empty and strictly shorter than the limit. */

    if (    (out_len == 0u)
         || (out_len >= limit_len))
    {
        return codec_rle_decode_fail(p_output_len);
    }

    *p_output_len = out_len;
    return 1;
}

int8_t codec_rle_decode(uint16_t input_len,
                        const uint8_t *p_input,
                        uint16_t *p_output_len,
                        uint8_t *p_output,
                        uint16_t limit_len)
{
    uint16_t out_len  = 0u;
    uint16_t capacity = 0u;
    uint16_t index    = 0u;

    if (p_output_len == NULL)
    {
        return 0;
    }

    if (    (p_input == NULL)
         || (p_output == NULL)
         || (input_len == 0u)
         || (limit_len == 0u))
    {
        return codec_rle_decode_fail(p_output_len);
    }

    capacity      = *p_output_len;
    *p_output_len = 0u;

    if (capacity == 0u)
    {
        return 0;
    }

    while (index < input_len)
    {
        const uint8_t token = p_input[index];
        index++;

        if (token <= 0x7Fu)
        {
            uint16_t literal_len = (uint16_t)token + 1u;

            if (literal_len > (input_len - index))
            {
                /* The literal payload is truncated. */
                return codec_rle_decode_fail(p_output_len);
            }

            if ((out_len + literal_len) > capacity)
            {
                return codec_rle_decode_fail(p_output_len);
            }

            if ((out_len + literal_len) >= limit_len)
            {
                return codec_rle_decode_fail(p_output_len);
            }

            (void)memcpy(&p_output[out_len], &p_input[index], literal_len);
            out_len += literal_len;
            index += literal_len;
        }
        else
        {
            uint16_t repeat_len = (uint16_t)(token & 0x7Fu) + CODEC_RLE_REPEAT_MIN;

            if (index >= input_len)
            {
                /* The repeat value byte is missing. */
                return codec_rle_decode_fail(p_output_len);
            }

            if ((out_len + repeat_len) > capacity)
            {
                return codec_rle_decode_fail(p_output_len);
            }

            if ((out_len + repeat_len) >= limit_len)
            {
                return codec_rle_decode_fail(p_output_len);
            }

            (void)memset(&p_output[out_len], p_input[index], repeat_len);
            out_len += repeat_len;
            index++;
        }
    }

    /* A successful result must be non-empty and strictly shorter than the limit. */

    if (    (out_len == 0u)
         || (out_len >= limit_len))
    {
        return codec_rle_decode_fail(p_output_len);
    }

    *p_output_len = out_len;
    return 1;
}
