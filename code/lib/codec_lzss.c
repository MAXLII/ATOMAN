// SPDX-License-Identifier: MIT
/**
 * @file codec_lzss.c
 * @brief COMM v1 LZSS-256 codec implementation.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Encode a byte stream into LZSS-256 literal and back-reference tokens
 *          - Decode LZSS-256 tokens, including overlapping back references
 *          - Enforce output capacity, the exclusive limit bound, and full input consumption
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Input and output buffers are caller-owned and must not overlap
 *          - Only the already-decoded bytes of the current DATA block form the
 *            history window; no state is kept across frames
 *          - Token layout: 0x00..0x7F literal run of token+1 bytes,
 *            0x80..0xFF back reference of (token & 0x7F) + 3 bytes copied from
 *            offset_minus_1 + 1 bytes before the current output position;
 *            overlapping copies are allowed
 *          - offset must be at least 1 and at most the current output length
 *          - Literal runs hold at most 128 bytes (token 0x7F)
 *          - Back references hold 3..130 bytes (token 0x80..0xFF)
 *          - Matches shorter than 3 bytes stay inside literal runs
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
#include "codec_lzss.h"

#include <string.h>

/* Maximum literal run length expressed by one token. */
#define CODEC_LZSS_LITERAL_MAX 128u
/* Maximum back-reference length expressed by one token. */
#define CODEC_LZSS_REF_LEN_MAX 130u
/* Minimum match length that is encoded as a back reference. */
#define CODEC_LZSS_REF_LEN_MIN 3u
/* Maximum back-reference offset; offsets are expressed as offset_minus_1. */
#define CODEC_LZSS_WINDOW 256u

static int8_t codec_lzss_fail(uint16_t *p_output_len)
{
    *p_output_len = 0u;
    return 0;
}

int8_t codec_lzss_encode(uint16_t input_len,
                         const uint8_t *p_input,
                         uint16_t *p_output_len,
                         uint8_t *p_output,
                         uint16_t limit_len)
{
    uint16_t out_len       = 0u;
    uint16_t capacity      = 0u;
    uint16_t index         = 0u;
    uint16_t literal_start = 0u;
    uint16_t literal_len   = 0u;

    if (p_output_len == NULL)
    {
        return 0;
    }

    if (    (p_input == NULL)
         || (p_output == NULL)
         || (input_len == 0u)
         || (limit_len == 0u))
    {
        return codec_lzss_fail(p_output_len);
    }

    capacity      = *p_output_len;
    *p_output_len = 0u;

    if (capacity == 0u)
    {
        return 0;
    }

    while (index < input_len)
    {
        /* Greedy match search inside the 256-byte history window. */
        uint16_t best_len    = 0u;
        uint16_t best_offset = 0u;
        const uint16_t max_offset = (index < CODEC_LZSS_WINDOW) ? index : CODEC_LZSS_WINDOW;
        uint16_t offset = 0u;

        for (offset = 1u; offset <= max_offset; ++offset)
        {
            uint16_t match_len = 0u;

            while (    (match_len < CODEC_LZSS_REF_LEN_MAX)
                    && ((index + match_len) < input_len)
                    && (p_input[index + match_len] == p_input[index - offset + match_len]))
            {
                match_len++;
            }

            if (match_len > best_len)
            {
                best_len    = match_len;
                best_offset = offset;

                if (match_len == CODEC_LZSS_REF_LEN_MAX)
                {
                    /* No longer match exists for this token length. */
                    break;
                }
            }
        }

        if (best_len >= CODEC_LZSS_REF_LEN_MIN)
        {
            /* Flush the pending literal run before emitting the reference. */

            if (literal_len != 0u)
            {
                if ((out_len + 1u + literal_len) > capacity)
                {
                    return 0;
                }

                if ((out_len + 1u + literal_len) >= limit_len)
                {
                    return codec_lzss_fail(p_output_len);
                }

                p_output[out_len] = (uint8_t)(literal_len - 1u);
                out_len++;
                (void)memcpy(&p_output[out_len], &p_input[literal_start], literal_len);
                out_len += literal_len;
                literal_len = 0u;
            }

            if ((out_len + 2u) > capacity)
            {
                return 0;
            }

            if ((out_len + 2u) >= limit_len)
            {
                return codec_lzss_fail(p_output_len);
            }

            p_output[out_len] = (uint8_t)(0x80u | (uint8_t)(best_len - CODEC_LZSS_REF_LEN_MIN));
            out_len++;
            p_output[out_len] = (uint8_t)(best_offset - 1u);
            out_len++;
            index += best_len;
            literal_start = index;
        }
        else
        {
            /* No usable match here; collect this byte into the literal run. */
            literal_len++;
            index++;

            if (literal_len == CODEC_LZSS_LITERAL_MAX)
            {
                if ((out_len + 1u + literal_len) > capacity)
                {
                    return 0;
                }

                if ((out_len + 1u + literal_len) >= limit_len)
                {
                    return codec_lzss_fail(p_output_len);
                }

                p_output[out_len] = (uint8_t)(literal_len - 1u);
                out_len++;
                (void)memcpy(&p_output[out_len], &p_input[literal_start], literal_len);
                out_len += literal_len;
                literal_len   = 0u;
                literal_start = index;
            }
        }
    }

    /* Flush the trailing literal run. */

    if (literal_len != 0u)
    {
        if ((out_len + 1u + literal_len) > capacity)
        {
            return 0;
        }

        if ((out_len + 1u + literal_len) >= limit_len)
        {
            return codec_lzss_fail(p_output_len);
        }

        p_output[out_len] = (uint8_t)(literal_len - 1u);
        out_len++;
        (void)memcpy(&p_output[out_len], &p_input[literal_start], literal_len);
        out_len += literal_len;
    }

    /* A successful result must be non-empty and strictly shorter than the limit. */

    if (    (out_len == 0u)
         || (out_len >= limit_len))
    {
        return codec_lzss_fail(p_output_len);
    }

    *p_output_len = out_len;
    return 1;
}

int8_t codec_lzss_decode(uint16_t input_len,
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
        return codec_lzss_fail(p_output_len);
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
                return codec_lzss_fail(p_output_len);
            }

            if ((out_len + literal_len) > capacity)
            {
                return codec_lzss_fail(p_output_len);
            }

            if ((out_len + literal_len) >= limit_len)
            {
                return codec_lzss_fail(p_output_len);
            }

            (void)memcpy(&p_output[out_len], &p_input[index], literal_len);
            out_len += literal_len;
            index += literal_len;
        }
        else
        {
            const uint16_t ref_len = (uint16_t)(token & 0x7Fu) + CODEC_LZSS_REF_LEN_MIN;
            uint16_t offset     = 0u;
            uint16_t copy_index = 0u;

            if (index >= input_len)
            {
                /* The offset byte is missing. */
                return codec_lzss_fail(p_output_len);
            }

            offset = (uint16_t)p_input[index] + 1u;
            index++;

            if (offset > out_len)
            {
                /* The offset must stay inside the decoded history. */
                return codec_lzss_fail(p_output_len);
            }

            if ((out_len + ref_len) > capacity)
            {
                return codec_lzss_fail(p_output_len);
            }

            if ((out_len + ref_len) >= limit_len)
            {
                return codec_lzss_fail(p_output_len);
            }

            /* Copy one byte at a time so overlapping references expand correctly. */

            for (copy_index = 0u; copy_index < ref_len; ++copy_index)
            {
                p_output[out_len] = p_output[out_len - offset];
                out_len++;
            }
        }
    }

    /* A successful result must be non-empty and strictly shorter than the limit. */

    if (    (out_len == 0u)
         || (out_len >= limit_len))
    {
        return codec_lzss_fail(p_output_len);
    }

    *p_output_len = out_len;
    return 1;
}
