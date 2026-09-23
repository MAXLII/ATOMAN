// SPDX-License-Identifier: MIT
/**
 * @file codec_dict.c
 * @brief COMM v1 static dictionary codec implementation.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Encode a byte stream into dictionary reference and literal tokens
 *          - Decode dictionary tokens back into the original byte stream
 *          - Build a per-first-byte lookup table during framework initialization
 *          - Provide the codebook identity and CRC32 used by CODEC_SELECT
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Both peers preset the same codebook; up to 64 entries of 1..256 bytes
 *          - Token layout: 0x00..0x7F literal run of token+1 bytes,
 *            0x80..0xBF dictionary reference index = token & 0x3F,
 *            0xC0..0xFF invalid
 *          - The encoder prefers the longest match and then the smallest index
 *          - Literal runs hold at most 128 bytes (token 0x7F)
 *          - No per-frame history is kept between encode or decode calls
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
#include "codec_dict.h"

#include "section.h"

#include <string.h>

/* Maximum literal run length expressed by one token. */
#define CODEC_DICT_LITERAL_MAX 128u
/* Maximum dictionary reference token payload value. */
#define CODEC_DICT_REF_TOKEN_MAX 0xBFu
/* First invalid dictionary token value. */
#define CODEC_DICT_TOKEN_INVALID_MIN 0xC0u
/* Mask selecting the dictionary entry index from a reference token. */
#define CODEC_DICT_INDEX_MASK 0x3Fu

/* Built-in codebook content. Both peers must preset the identical table. */
static const char s_codec_dict_entry_data[] = "counter\0"
                                              "led_mask\0"
                                              "temperature\0"
                                              "voltage\0"
                                              "current\0"
                                              "power\0"
                                              "frequency\0"
                                              "status\0"
                                              "error\0"
                                              "scope\0"
                                              "sfra\0"
                                              "perf\0"
                                              "trace\0"
                                              "ok\0"
                                              "fail\0"
                                              "version";

typedef struct
{
    uint16_t offset; /**< Byte offset of this entry inside the codebook blob. */
    uint8_t length;  /**< Entry length in bytes, 1..256. */
} codec_dict_entry_t;

static const codec_dict_entry_t s_codec_dict_entries[] = {
    {.offset = 0u, .length = 7u},   /* "counter" */
    {.offset = 8u, .length = 8u},   /* "led_mask" */
    {.offset = 17u, .length = 11u}, /* "temperature" */
    {.offset = 29u, .length = 7u},  /* "voltage" */
    {.offset = 37u, .length = 7u},  /* "current" */
    {.offset = 45u, .length = 5u},  /* "power" */
    {.offset = 51u, .length = 9u},  /* "frequency" */
    {.offset = 61u, .length = 6u},  /* "status" */
    {.offset = 68u, .length = 5u},  /* "error" */
    {.offset = 74u, .length = 5u},  /* "scope" */
    {.offset = 80u, .length = 4u},  /* "sfra" */
    {.offset = 85u, .length = 4u},  /* "perf" */
    {.offset = 90u, .length = 5u},  /* "trace" */
    {.offset = 96u, .length = 2u},  /* "ok" */
    {.offset = 99u, .length = 4u},  /* "fail" */
    {.offset = 104u, .length = 7u}, /* "version" */
};

#define CODEC_DICT_ENTRY_COUNT (sizeof(s_codec_dict_entries) / sizeof(s_codec_dict_entries[0]))
#define CODEC_DICT_ID          0u
#define CODEC_DICT_VERSION     1u

/* One bit per dictionary entry for each possible first byte. */
static uint64_t s_codec_dict_first_byte_bucket[256];
/* Initialization completion flag for the lazy fallback path. */
static uint8_t s_codec_dict_ready = 0u;

static void codec_dict_init(void)
{
    uint16_t byte_index  = 0u;
    uint16_t entry_index = 0u;

    for (byte_index = 0u; byte_index < 256u; ++byte_index)
    {
        s_codec_dict_first_byte_bucket[byte_index] = 0ULL;
    }

    for (entry_index = 0u; entry_index < CODEC_DICT_ENTRY_COUNT; ++entry_index)
    {
        const uint8_t first_byte = (uint8_t)s_codec_dict_entry_data[s_codec_dict_entries[entry_index].offset];
        s_codec_dict_first_byte_bucket[first_byte] |= (1ULL << entry_index);
    }

    s_codec_dict_ready = 1u;
}

REG_INIT(0, codec_dict_init)

static int8_t codec_dict_fail(uint16_t *p_output_len)
{
    *p_output_len = 0u;
    return 0;
}

int8_t codec_dict_encode(uint16_t input_len,
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
        return codec_dict_fail(p_output_len);
    }

    capacity      = *p_output_len;
    *p_output_len = 0u;

    if (capacity == 0u)
    {
        return 0;
    }

    if (s_codec_dict_ready == 0u)
    {
        /* Host builds that skip section_init still get a usable lookup table.
         * Rebuilding is idempotent, so a concurrent duplicate build is harmless. */
        codec_dict_init();
    }

    while (index < input_len)
    {
        /* Find the longest matching entry and prefer the smallest index on ties. */
        uint16_t best_len    = 0u;
        uint8_t best_index   = 0u;
        uint64_t candidates  = s_codec_dict_first_byte_bucket[p_input[index]];
        uint16_t entry_index = 0u;

        for (entry_index = 0u; entry_index < CODEC_DICT_ENTRY_COUNT; ++entry_index)
        {
            uint16_t entry_len;

            if ((candidates & (1ULL << entry_index)) == 0ULL)
            {
                /* The entry cannot match the current first byte. */
                continue;
            }

            entry_len = (uint16_t)s_codec_dict_entries[entry_index].length;

            if (entry_len > (input_len - index))
            {
                /* The remaining input is shorter than this entry. */
                continue;
            }

            if (memcmp(&p_input[index], &s_codec_dict_entry_data[s_codec_dict_entries[entry_index].offset], entry_len) != 0)
            {
                continue;
            }

            if (    (entry_len > best_len)
                 || (    (entry_len == best_len)
                      && (entry_index < best_index)))
            {
                best_len   = entry_len;
                best_index = (uint8_t)entry_index;
            }
        }

        if (best_len == 0u)
        {
            /* No dictionary match here; collect this byte into the literal run. */
            literal_len++;
            index++;

            if (literal_len == CODEC_DICT_LITERAL_MAX)
            {
                if ((out_len + 1u + literal_len) > capacity)
                {
                    return 0;
                }

                if ((out_len + 1u + literal_len) >= limit_len)
                {
                    return codec_dict_fail(p_output_len);
                }

                p_output[out_len] = (uint8_t)(literal_len - 1u);
                out_len++;
                (void)memcpy(&p_output[out_len], &p_input[literal_start], literal_len);
                out_len += literal_len;
                literal_len   = 0u;
                literal_start = index;
            }
        }
        else
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
                    return codec_dict_fail(p_output_len);
                }

                p_output[out_len] = (uint8_t)(literal_len - 1u);
                out_len++;
                (void)memcpy(&p_output[out_len], &p_input[literal_start], literal_len);
                out_len += literal_len;
                literal_len = 0u;
            }

            if ((out_len + 1u) > capacity)
            {
                return 0;
            }

            if ((out_len + 1u) >= limit_len)
            {
                return codec_dict_fail(p_output_len);
            }

            p_output[out_len] = (uint8_t)(0x80u | best_index);
            out_len++;
            index += best_len;
            literal_start = index;
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
            return codec_dict_fail(p_output_len);
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
        return codec_dict_fail(p_output_len);
    }

    *p_output_len = out_len;
    return 1;
}

int8_t codec_dict_decode(uint16_t input_len,
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
        return codec_dict_fail(p_output_len);
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
                return codec_dict_fail(p_output_len);
            }

            if ((out_len + literal_len) > capacity)
            {
                return codec_dict_fail(p_output_len);
            }

            if ((out_len + literal_len) >= limit_len)
            {
                return codec_dict_fail(p_output_len);
            }

            (void)memcpy(&p_output[out_len], &p_input[index], literal_len);
            out_len += literal_len;
            index += literal_len;
        }
        else if (token <= CODEC_DICT_REF_TOKEN_MAX)
        {
            const uint8_t entry_index = (uint8_t)(token & CODEC_DICT_INDEX_MASK);
            uint16_t entry_len;

            if (entry_index >= CODEC_DICT_ENTRY_COUNT)
            {
                /* The reference points past the preset codebook. */
                return codec_dict_fail(p_output_len);
            }

            entry_len = (uint16_t)s_codec_dict_entries[entry_index].length;

            if ((out_len + entry_len) > capacity)
            {
                return codec_dict_fail(p_output_len);
            }

            if ((out_len + entry_len) >= limit_len)
            {
                return codec_dict_fail(p_output_len);
            }

            (void)memcpy(&p_output[out_len],
                         &s_codec_dict_entry_data[s_codec_dict_entries[entry_index].offset],
                         entry_len);
            out_len += entry_len;
        }
        else
        {
            /* Tokens 0xC0..0xFF are invalid in CODEC=001. */
            return codec_dict_fail(p_output_len);
        }
    }

    /* A successful result must be non-empty and strictly shorter than the limit. */

    if (    (out_len == 0u)
         || (out_len >= limit_len))
    {
        return codec_dict_fail(p_output_len);
    }

    *p_output_len = out_len;
    return 1;
}

uint8_t codec_dict_codebook_id(void)
{
    return (uint8_t)CODEC_DICT_ID;
}

uint8_t codec_dict_codebook_version(void)
{
    return (uint8_t)CODEC_DICT_VERSION;
}

uint32_t codec_dict_codebook_crc32(void)
{
    uint32_t crc         = 0xFFFFFFFFu;
    uint16_t entry_index = 0u;
    uint16_t byte_index  = 0u;

    /* 仅覆盖条目内容字节，不含分隔符，与上位机实现保持一致。 */

    for (entry_index = 0u; entry_index < CODEC_DICT_ENTRY_COUNT; ++entry_index)
    {
        const uint8_t *p_entry = (const uint8_t *)&s_codec_dict_entry_data[s_codec_dict_entries[entry_index].offset];

        for (byte_index = 0u; byte_index < s_codec_dict_entries[entry_index].length; ++byte_index)
        {
            uint32_t bit_index = 0u;

            crc ^= (uint32_t)p_entry[byte_index];

            for (bit_index = 0u; bit_index < 8u; ++bit_index)
            {
                const uint32_t mask = (uint32_t)(0u - (crc & 1u));
                crc = (crc >> 1u) ^ (0xEDB88320u & mask);
            }
        }
    }

    return crc ^ 0xFFFFFFFFu;
}
