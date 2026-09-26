// SPDX-License-Identifier: MIT
/**
 * @file codec_zero.c
 * @brief COMM v1 ZERO (nibble zero-count) codec implementation.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Encode a byte stream into nibble zero-count variable-length bits
 *          - Decode the bit stream back into the original byte stream
 *          - Enforce output capacity, the exclusive limit bound, and full input consumption
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Each nibble n encodes as 1^k + 0^(m+1) + 1 with k = n >> 2
 *            (segment overflow prefix) and m = n & 3 (in-segment offset)
 *          - Bits fill output bytes MSB-first; trailing bits of the last byte
 *            are padded with ones
 *          - Decoding stops when the overflow prefix runs to the end of the
 *            input, treating those ones as padding
 *
 * @author Max.Li
 * @date 2026-09-19
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "codec_zero.h"

#include <string.h>

/* Nibble segment width and maximum in-segment offset. */
#define CODEC_ZERO_SEGMENT_BITS 2u
#define CODEC_ZERO_SEGMENT_MAX  3u
/* Maximum value representable by one nibble. */
#define CODEC_ZERO_NIBBLE_MAX 0xFu

static int8_t codec_zero_fail(uint16_t *p_output_len)
{
    *p_output_len = 0u;
    return 0;
}

static uint8_t codec_zero_get_bit(const uint8_t *p_input, uint32_t bit_index)
{
    return (uint8_t)((p_input[bit_index >> 3u] >> (7u - (bit_index & 7u))) & 1u);
}

int8_t codec_zero_encode(uint16_t input_len,
                         const uint8_t *p_input,
                         uint16_t *p_output_len,
                         uint8_t *p_output,
                         uint16_t limit_len)
{
    uint16_t capacity  = 0u;
    uint32_t bit_count = 0u;
    uint16_t index     = 0u;

    if (p_output_len == NULL)
    {
        return 0;
    }

    if (    (p_input == NULL)
         || (p_output == NULL)
         || (input_len == 0u)
         || (limit_len == 0u))
    {
        return codec_zero_fail(p_output_len);
    }

    capacity      = *p_output_len;
    *p_output_len = 0u;

    if (capacity == 0u)
    {
        return 0;
    }

    /* Zero the whole output; the encoder only sets bits and pads with ones. */
    (void)memset(p_output, 0, capacity);

    for (index = 0u; index < input_len; ++index)
    {
        uint8_t nibble_index = 0u;

        for (nibble_index = 0u; nibble_index < 2u; ++nibble_index)
        {
            const uint8_t nibble = (nibble_index == 0u) ? (uint8_t)(p_input[index] >> 4u)
                                                        : (uint8_t)(p_input[index] & 0x0Fu);
            const uint8_t segment = (uint8_t)(nibble >> CODEC_ZERO_SEGMENT_BITS);
            const uint8_t offset = (uint8_t)(nibble & CODEC_ZERO_SEGMENT_MAX);
            uint8_t bit_index    = 0u;

            /* Segment overflow prefix: segment times '1'. */

            for (bit_index = 0u; bit_index < segment; ++bit_index)
            {
                p_output[bit_count >> 3u] |= (uint8_t)(1u << (7u - (bit_count & 7u)));
                bit_count++;
            }
            /* In-segment offset: offset+1 times '0'. */

            for (bit_index = 0u; bit_index <= offset; ++bit_index)
            {
                bit_count++;
            }
            /* Terminating separator: one '1'. */
            p_output[bit_count >> 3u] |= (uint8_t)(1u << (7u - (bit_count & 7u)));
            bit_count++;

            /* Abort as soon as the encoded size reaches the exclusive limit. */

            if (((bit_count + 7u) / 8u) >= limit_len)
            {
                return codec_zero_fail(p_output_len);
            }
        }
    }

    /* Pad the remaining bits of the last byte with ones. */

    while ((bit_count & 7u) != 0u)
    {
        p_output[bit_count >> 3u] |= (uint8_t)(1u << (7u - (bit_count & 7u)));
        bit_count++;
    }

    {
        const uint16_t out_len = (uint16_t)(bit_count / 8u);

        if (    (out_len == 0u)
             || (out_len > capacity)
             || (out_len >= limit_len))
        {
            return codec_zero_fail(p_output_len);
        }
        *p_output_len = out_len;
        return 1;
    }
}

int8_t codec_zero_decode(uint16_t input_len,
                         const uint8_t *p_input,
                         uint16_t *p_output_len,
                         uint8_t *p_output,
                         uint16_t limit_len)
{
    uint16_t capacity    = 0u;
    uint32_t bit_total   = 0u;
    uint32_t bit_index   = 0u;
    uint16_t out_len     = 0u;
    uint8_t pending_high = 0u;
    uint8_t has_pending  = 0u;

    if (p_output_len == NULL)
    {
        return 0;
    }

    if (    (p_input == NULL)
         || (p_output == NULL)
         || (input_len == 0u)
         || (limit_len == 0u))
    {
        return codec_zero_fail(p_output_len);
    }

    capacity      = *p_output_len;
    *p_output_len = 0u;

    if (capacity == 0u)
    {
        return 0;
    }

    bit_total = (uint32_t)input_len * 8u;

    while (bit_index < bit_total)
    {
        uint8_t segment      = 0u;
        uint8_t offset_count = 0u;
        uint8_t offset       = 0u;
        uint8_t nibble       = 0u;

        /* Overflow prefix: count '1' until a '0' or the end of the input. */

        while (    (bit_index < bit_total)
                && (codec_zero_get_bit(p_input, bit_index) == 1u))
        {
            segment++;
            bit_index++;
        }

        if (bit_index >= bit_total)
        {
            /* The trailing ones are byte padding; stop without a nibble. */
            break;
        }

        if (segment > CODEC_ZERO_SEGMENT_MAX)
        {
            return codec_zero_fail(p_output_len);
        }

        /* In-segment offset: count '0' until the terminating '1'. */

        while (    (bit_index < bit_total)
                && (codec_zero_get_bit(p_input, bit_index) == 0u))
        {
            offset_count++;
            bit_index++;
        }

        if (bit_index >= bit_total)
        {
            /* The offset run must end with a separator; a truncated run is corruption. */
            return codec_zero_fail(p_output_len);
        }
        /* Consume the terminating '1'. */
        bit_index++;

        if (    (offset_count == 0u)
             || (offset_count > (CODEC_ZERO_SEGMENT_MAX + 1u)))
        {
            return codec_zero_fail(p_output_len);
        }
        offset = (uint8_t)(offset_count - 1u);

        nibble = (uint8_t)((segment << CODEC_ZERO_SEGMENT_BITS) | offset);

        /* Assemble bytes high nibble first. */

        if (has_pending == 0u)
        {
            pending_high = nibble;
            has_pending  = 1u;
        }
        else
        {
            if (out_len >= capacity)
            {
                return codec_zero_fail(p_output_len);
            }

            if (((uint16_t)(out_len + 1u)) >= limit_len)
            {
                return codec_zero_fail(p_output_len);
            }
            p_output[out_len] = (uint8_t)((pending_high << 4u) | nibble);
            out_len++;
            has_pending = 0u;
        }
    }

    if (has_pending != 0u)
    {
        /* A complete byte requires an even number of nibbles. */
        return codec_zero_fail(p_output_len);
    }

    if (    (out_len == 0u)
         || (out_len >= limit_len))
    {
        return codec_zero_fail(p_output_len);
    }

    *p_output_len = out_len;
    return 1;
}
