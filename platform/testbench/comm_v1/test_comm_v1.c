// SPDX-License-Identifier: MIT
/**
 * @file test_comm_v1.c
 * @brief COMM v1 protocol and codec host test suite.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Exercise the COMM v1 SUM, DICT, RLE, and LZSS codecs
 *          - Exercise the COMM v1 receive state machine, deduplication, and command dispatch
 *          - Exercise the COMM v1 send path, candidate compression, and range rejection
 *          - Verify the CODEC_SELECT codebook negotiation exchange
 *
 *          Design notes:
 *          - C11 compatible
 *          - Built with PLATFORM_TESTBENCH so no MCU hardware is required
 *          - Provides the command and link registry anchors normally owned by comm.c
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
#include "comm.h"
#include "section.h"

#include "codec_dict.h"
#include "codec_lzss.h"
#include "codec_rle.h"
#include "codec_zero.h"

#include <stdio.h>
#include <string.h>

/* =============================================================================
 * Test framework
 * =============================================================================
 */

static int g_check_failures = 0;

#define CHECK(condition, text)                                \
    do                                                        \
    {                                                         \
        if (!(condition))                                     \
        {                                                     \
            printf("FAIL: %s (line %d)\n", (text), __LINE__); \
            g_check_failures++;                               \
        }                                                     \
    } while (0)

/* =============================================================================
 * Registry anchors normally owned by comm.c
 * =============================================================================
 */

section_item_t *p_comm_command_first = NULL;
section_item_t *p_comm_route_first   = NULL;
section_item_t *p_link_first         = NULL;

/* =============================================================================
 * Test command registration and handler capture
 * =============================================================================
 */

static uint8_t g_handler_called = 0u;
static uint8_t g_last_seq       = 0u;
static uint8_t g_last_cmd_set   = 0u;
static uint8_t g_last_cmd_word  = 0u;
static uint8_t g_last_ack       = 0u;
static uint8_t g_last_src       = 0u;
static uint8_t g_last_dst       = 0u;
static uint16_t g_last_len      = 0u;
static uint8_t g_last_data[256];

static void test_command_handler(void *p_frame, DEC_MY_PRINTF)
{
    section_packform_t *p_pack = (section_packform_t *)p_frame;

    (void)my_printf;
    g_handler_called = 1u;
    g_last_seq       = p_pack->seq;
    g_last_cmd_set   = p_pack->cmd_set;
    g_last_cmd_word  = p_pack->cmd_word;
    g_last_ack       = p_pack->is_ack;
    g_last_src       = p_pack->src;
    g_last_dst       = p_pack->dst;
    g_last_len       = p_pack->len;

    if (    (p_pack->len > 0u)
         && (p_pack->p_data != NULL)
         && (p_pack->len <= sizeof(g_last_data)))
    {
        (void)memcpy(g_last_data, p_pack->p_data, p_pack->len);
    }
}

static section_com_t g_test_command = {
    .cmd_set  = 0x05u,
    .cmd_word = 0x21u,
    .func     = test_command_handler,
};

static section_item_t g_test_command_item = {
    .p_obj  = &g_test_command,
    .p_next = NULL,
};

/* =============================================================================
 * TX capture link
 * =============================================================================
 */

static uint8_t g_tx_bytes[512];
static uint32_t g_tx_len = 0u;

static void test_tx_by_dma_cb(char *ptr, int len)
{
    uint32_t copy_len = (len > 0) ? (uint32_t)len : 0u;

    if (    (ptr != NULL)
         && (copy_len <= sizeof(g_tx_bytes)))
    {
        (void)memcpy(g_tx_bytes, ptr, copy_len);
    }
    g_tx_len = copy_len;
}

static section_link_tx_func_t g_test_tx_func = {
    .my_printf = NULL,
    .tx_by_dma = test_tx_by_dma_cb,
};

#define TEST_LINK_ID    0u
#define TEST_LOCAL_ADDR 2u

DECLARE_COMM_V1_CTX(g_test_v1_ctx, TEST_LOCAL_ADDR, TEST_LINK_ID);

static void test_reset_rx(void)
{
    g_handler_called = 0u;
    g_last_len       = 0u;
    g_tx_len         = 0u;
    (void)memset(g_last_data, 0, sizeof(g_last_data));
    (void)memset(g_tx_bytes, 0, sizeof(g_tx_bytes));
}

/* =============================================================================
 * Wire helpers
 * =============================================================================
 */

static uint32_t test_build_msg(uint8_t seq,
                               uint8_t dst,
                               uint8_t d_dst,
                               uint8_t src,
                               uint8_t d_src,
                               uint8_t cmd_set,
                               uint8_t cmd_word,
                               uint8_t codec,
                               uint8_t cmd,
                               uint8_t ack)
{
    return (uint32_t)seq | ((uint32_t)dst << 3u) | ((uint32_t)d_dst << 7u) | ((uint32_t)src << 10u)
         | ((uint32_t)d_src << 14u) | ((uint32_t)cmd_set << 17u) | ((uint32_t)cmd_word << 21u)
         | ((uint32_t)codec << 27u) | ((uint32_t)cmd << 30u) | ((uint32_t)ack << 31u);
}

static uint16_t test_build_frame(uint8_t *p_out,
                                 uint32_t msg,
                                 uint8_t cmd,
                                 const uint8_t *p_data,
                                 uint16_t data_len)
{
    uint16_t frame_len = 0u;

    p_out[0] = COMM_V1_SOP;
    p_out[1] = (uint8_t)(msg & 0xFFu);
    p_out[2] = (uint8_t)((msg >> 8u) & 0xFFu);
    p_out[3] = (uint8_t)((msg >> 16u) & 0xFFu);
    p_out[4] = (uint8_t)((msg >> 24u) & 0xFFu);

    if (cmd == 0u)
    {
        p_out[5] = (data_len == 256u) ? 0u : (uint8_t)data_len;
        (void)memcpy(&p_out[6], p_data, data_len);
        frame_len = (uint16_t)(6u + data_len);
    }
    else
    {
        frame_len = 5u;
    }

    p_out[frame_len] = comm_sum_encode(p_out, frame_len);
    frame_len++;
    return frame_len;
}

/* =============================================================================
 * SUM tests
 * =============================================================================
 */

static void test_sum(void)
{
    const uint8_t data_zero_sum[2] = {0x01u, 0xFFu};
    const uint8_t data_ff_sum[2]   = {0xFFu, 0x01u};
    const uint8_t data_plain[2]    = {0x12u, 0x34u};

    CHECK(comm_sum_encode(data_zero_sum, 2u) == 0x55u, "SUM zero maps to 0x55");
    CHECK(comm_sum_encode(data_ff_sum, 2u) == 0x55u, "SUM 0x100 also wraps to zero");
    CHECK(comm_sum_encode(data_plain, 2u) == 0x46u, "SUM plain value");
}

/* =============================================================================
 * Codec tests
 * =============================================================================
 */

static void test_codec_rle(void)
{
    /* Document example: 12 34 00 00 00 00 00 AB -> 01 12 34 82 00 00 AB */
    const uint8_t input[8]    = {0x12u, 0x34u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xABu};
    const uint8_t expected[7] = {0x01u, 0x12u, 0x34u, 0x82u, 0x00u, 0x00u, 0xABu};
    uint8_t encoded[32]       = {0};
    uint8_t decoded[32]       = {0};
    uint16_t out_len          = 0u;

    out_len = (uint16_t)sizeof(encoded);
    CHECK(codec_rle_encode(8u, input, &out_len, encoded, 300u) == 1, "RLE document example encodes");
    CHECK(out_len == 7u, "RLE document example length");
    CHECK(memcmp(encoded, expected, 7u) == 0, "RLE document example bytes");

    out_len = (uint16_t)sizeof(decoded);
    CHECK(codec_rle_decode(7u, encoded, &out_len, decoded, 300u) == 1, "RLE document example decodes");
    CHECK(out_len == 8u, "RLE decode length");
    CHECK(memcmp(decoded, input, 8u) == 0, "RLE roundtrip bytes");

    /* Long literal run and long repeat run roundtrips. */
    {
        uint8_t long_input[300]   = {0};
        uint8_t long_encoded[512] = {0};
        uint8_t long_decoded[512] = {0};
        uint16_t enc_len          = (uint16_t)sizeof(long_encoded);
        uint16_t dec_len          = (uint16_t)sizeof(long_decoded);
        uint16_t i                = 0u;

        for (i = 0u; i < 300u; ++i)
        {
            long_input[i] = (uint8_t)(i & 0xFFu);
        }
        CHECK(codec_rle_encode(300u, long_input, &enc_len, long_encoded, 600u) == 1, "RLE long literal encodes");
        CHECK(codec_rle_decode(enc_len, long_encoded, &dec_len, long_decoded, 600u) == 1, "RLE long literal decodes");
        CHECK(dec_len == 300u, "RLE long literal decode length");
        CHECK(memcmp(long_decoded, long_input, 300u) == 0, "RLE long literal bytes");

        for (i = 0u; i < 300u; ++i)
        {
            long_input[i] = 0xAAu;
        }
        enc_len = (uint16_t)sizeof(long_encoded);
        CHECK(codec_rle_encode(300u, long_input, &enc_len, long_encoded, 600u) == 1, "RLE long repeat encodes");
        CHECK(enc_len == 6u, "RLE long repeat length");
        dec_len = (uint16_t)sizeof(long_decoded);
        CHECK(codec_rle_decode(enc_len, long_encoded, &dec_len, long_decoded, 600u) == 1, "RLE long repeat decodes");
        CHECK(dec_len == 300u, "RLE long repeat decode length");
        CHECK(memcmp(long_decoded, long_input, 300u) == 0, "RLE long repeat bytes");
    }
}

static void test_codec_dict(void)
{
    /* "counter ok 12" contains the preset entries "counter" and "ok". */
    const char *input        = "counter ok 12";
    const uint16_t input_len = (uint16_t)strlen(input);
    uint8_t encoded[64]      = {0};
    uint8_t decoded[64]      = {0};
    uint16_t enc_len         = (uint16_t)sizeof(encoded);
    uint16_t dec_len         = (uint16_t)sizeof(decoded);

    CHECK(codec_dict_encode(input_len, (const uint8_t *)input, &enc_len, encoded, 300u) == 1, "DICT encodes");
    CHECK(enc_len < input_len, "DICT compresses known text");

    CHECK(codec_dict_decode(enc_len, encoded, &dec_len, decoded, 300u) == 1, "DICT decodes");
    CHECK(dec_len == input_len, "DICT decode length");
    CHECK(memcmp(decoded, input, input_len) == 0, "DICT roundtrip bytes");

    /* Invalid token 0xC0 must fail. */
    {
        const uint8_t bad[1] = {0xC0u};
        uint16_t out_len     = (uint16_t)sizeof(decoded);
        CHECK(codec_dict_decode(1u, bad, &out_len, decoded, 300u) == 0, "DICT rejects invalid token");
        CHECK(out_len == 0u, "DICT failure zeroes output length");
    }

    /* Empty input and NULL length pointer must fail. */
    {
        uint16_t out_len = (uint16_t)sizeof(encoded);
        CHECK(codec_dict_encode(0u, (const uint8_t *)input, &out_len, encoded, 300u) == 0, "DICT rejects empty input");
    }
    CHECK(codec_dict_encode(input_len, (const uint8_t *)input, NULL, encoded, 300u) == 0,
          "DICT rejects NULL length pointer");
    /* Golden CRC32 shared with the FRAME host implementation. */
    CHECK(codec_dict_codebook_crc32() == 0xB6DD009Du, "DICT codebook CRC32 golden");
}

static void test_codec_lzss(void)
{
    /* Periodic content forces overlapping back references. */
    uint8_t input[200]   = {0};
    uint8_t encoded[300] = {0};
    uint8_t decoded[300] = {0};
    uint16_t enc_len     = (uint16_t)sizeof(encoded);
    uint16_t dec_len     = (uint16_t)sizeof(decoded);
    uint16_t i           = 0u;

    for (i = 0u; i < 200u; ++i)
    {
        input[i] = (uint8_t)("abcd"[i % 4u]);
    }

    CHECK(codec_lzss_encode(200u, input, &enc_len, encoded, 300u) == 1, "LZSS encodes periodic data");
    CHECK(enc_len < 200u, "LZSS compresses periodic data");

    CHECK(codec_lzss_decode(enc_len, encoded, &dec_len, decoded, 300u) == 1, "LZSS decodes");
    CHECK(dec_len == 200u, "LZSS decode length");
    CHECK(memcmp(decoded, input, 200u) == 0, "LZSS roundtrip bytes");

    /* Invalid offset must fail. */
    {
        const uint8_t bad[2] = {0x83u, 0x7Fu}; /* Offset 128 > output length. */
        uint16_t out_len     = (uint16_t)sizeof(decoded);
        CHECK(codec_lzss_decode(2u, bad, &out_len, decoded, 300u) == 0, "LZSS rejects offset beyond history");
    }
}

static void test_codec_limits(void)
{
    uint8_t input[64]   = {0};
    uint8_t output[128] = {0};
    uint16_t out_len    = 0u;
    uint16_t i          = 0u;

    for (i = 0u; i < 64u; ++i)
    {
        input[i] = 0x00u;
    }

    /* limit_len=1 is unreachable: encoding must stop and fail. */
    out_len = (uint16_t)sizeof(output);
    CHECK(codec_rle_encode(64u, input, &out_len, output, 1u) == 0, "RLE aborts at limit");
    CHECK(out_len == 0u, "RLE limit failure zeroes length");

    /* Output capacity smaller than the minimal token must fail. */
    out_len = 0u;
    CHECK(codec_rle_encode(64u, input, &out_len, output, 300u) == 0, "RLE rejects zero capacity");

    /* limit_len=0 must fail. */
    out_len = (uint16_t)sizeof(output);
    CHECK(codec_rle_encode(64u, input, &out_len, output, 0u) == 0, "RLE rejects zero limit");
}

static void test_codec_zero(void)
{
    /* Golden: bytes 01 23 45 67 89 AB CD EF carry nibbles 0..F, which encode
     * to the 80-bit table stream and pad to 10 bytes. */
    const uint8_t input[8]   = {0x01u, 0x23u, 0x45u, 0x67u, 0x89u, 0xABu, 0xCDu, 0xEFu};
    const uint8_t golden[10] = {0x48u, 0x86u, 0xCCu, 0x61u, 0xDCu, 0xE3u, 0x87u, 0xBCu, 0xF1u, 0xE1u};
    uint8_t encoded[32]      = {0};
    uint8_t decoded[32]      = {0};
    uint16_t out_len         = (uint16_t)sizeof(encoded);

    CHECK(codec_zero_encode(8u, input, &out_len, encoded, 300u) == 1, "ZERO encodes nibble table");
    CHECK(out_len == 10u, "ZERO golden length");
    CHECK(memcmp(encoded, golden, 10u) == 0, "ZERO golden bytes");

    out_len = (uint16_t)sizeof(decoded);
    CHECK(codec_zero_decode(10u, encoded, &out_len, decoded, 300u) == 1, "ZERO decodes golden");
    CHECK(out_len == 8u, "ZERO decode length");
    CHECK(memcmp(decoded, input, 8u) == 0, "ZERO roundtrip bytes");

    /* Small-value dense data compresses well: 16 zero bytes -> 8 bytes. */
    {
        uint8_t zeros[16] = {0};
        uint8_t small[32] = {0};
        uint8_t back[32]  = {0};
        out_len           = (uint16_t)sizeof(small);
        CHECK(codec_zero_encode(16u, zeros, &out_len, small, 300u) == 1, "ZERO compresses zeros");
        CHECK(out_len == 8u, "ZERO zeros length");
        out_len = (uint16_t)sizeof(back);
        CHECK(codec_zero_decode(8u, small, &out_len, back, 300u) == 1, "ZERO decodes zeros");
        CHECK(    out_len == 16u
                   && memcmp(back, zeros, 16u) == 0,
                  "ZERO zeros roundtrip");
    }

    /* Random-ish bytes: nibbles 4..F expand, so the result may exceed RAW;
     * the codec still roundtrips for any input. */
    {
        uint8_t high[16]  = {0};
        uint8_t small[64] = {0};
        uint8_t back[64]  = {0};
        uint16_t enc_len  = 0u;

        for (uint16_t i = 0u; i < 16u; ++i)
        {
            high[i] = (uint8_t)(0xEEu);
        }
        enc_len = (uint16_t)sizeof(small);
        CHECK(codec_zero_encode(16u, high, &enc_len, small, 300u) == 1, "ZERO encodes high nibbles");
        out_len = (uint16_t)sizeof(back);
        CHECK(codec_zero_decode(enc_len, small, &out_len, back, 300u) == 1, "ZERO decodes high nibbles");
        CHECK(    out_len == 16u
                   && memcmp(back, high, 16u) == 0,
                  "ZERO high nibble roundtrip");
    }

    /* Truncated nibble (padding without a terminating separator) must fail. */
    {
        /* "11000000": overflow prefix runs into the end -> padding, zero output. */
        const uint8_t only_padding[1] = {0xFFu};
        out_len                       = (uint16_t)sizeof(decoded);
        CHECK(codec_zero_decode(1u, only_padding, &out_len, decoded, 300u) == 0, "ZERO rejects all-padding input");
        /* "00000000": zero run without a terminator is corruption. */
        const uint8_t unterminated[1] = {0x00u};
        out_len                       = (uint16_t)sizeof(decoded);
        CHECK(codec_zero_decode(1u, unterminated, &out_len, decoded, 300u) == 0, "ZERO rejects unterminated run");
    }

    /* Limit bound aborts encoding. */
    {
        uint8_t zeros[16] = {0};
        out_len           = (uint16_t)sizeof(encoded);
        CHECK(codec_zero_encode(16u, zeros, &out_len, encoded, 8u) == 0, "ZERO aborts at limit");
        CHECK(codec_zero_encode(16u, zeros, NULL, encoded, 300u) == 0, "ZERO rejects NULL length");
    }
}

/* =============================================================================
 * Receive state machine tests
 * =============================================================================
 */

static void test_rx_pure_command(void)
{
    uint8_t frame[8]   = {0};
    uint32_t msg       = 0u;
    uint16_t frame_len = 0u;
    uint32_t i         = 0u;

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    msg       = test_build_msg(3u, 2u, 0u, 1u, 0u, 0x05u, 0x21u, COMM_V1_CODEC_RAW, 1u, 0u);
    frame_len = test_build_frame(frame, msg, 1u, NULL, 0u);
    CHECK(frame_len == 6u, "pure command frame length");

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }

    CHECK(g_handler_called == 1u, "pure command dispatched");
    CHECK(g_last_seq == 3u, "pure command seq extracted");
    CHECK(g_last_cmd_set == 0x05u, "pure command cmd_set extracted");
    CHECK(g_last_cmd_word == 0x21u, "pure command cmd_word extracted");
    CHECK(g_last_len == 0u, "pure command has no payload");
    CHECK(g_test_v1_ctx.last_seq_valid == 1u, "last_seq updated after valid frame");
}

static void test_rx_data_raw(void)
{
    uint8_t frame[16]     = {0};
    const uint8_t data[4] = {0x11u, 0x22u, 0x33u, 0x44u};
    uint32_t msg          = 0u;
    uint16_t frame_len    = 0u;
    uint32_t i            = 0u;

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    msg       = test_build_msg(5u, 2u, 0u, 1u, 0u, 0x05u, 0x21u, COMM_V1_CODEC_RAW, 0u, 0u);
    frame_len = test_build_frame(frame, msg, 0u, data, 4u);

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }

    CHECK(g_handler_called == 1u, "data frame dispatched");
    CHECK(g_last_len == 4u, "data frame payload length");
    CHECK(memcmp(g_last_data, data, 4u) == 0, "data frame payload bytes");
}

static void test_rx_compressed(void)
{
    uint8_t frame[32]    = {0};
    uint8_t data[16]     = {0};
    uint8_t encoded[16]  = {0};
    uint16_t encoded_len = (uint16_t)sizeof(encoded);
    uint32_t msg         = 0u;
    uint16_t frame_len   = 0u;
    uint32_t i           = 0u;

    /* 16 identical bytes compress to one RLE repeat token (2 bytes). */
    (void)memset(data, 0x77u, sizeof(data));
    CHECK(codec_rle_encode(16u, data, &encoded_len, encoded, 300u) == 1, "test data compresses");

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    msg       = test_build_msg(0u, 2u, 0u, 1u, 0u, 0x05u, 0x21u, COMM_V1_CODEC_RLE, 0u, 0u);
    frame_len = test_build_frame(frame, msg, 0u, encoded, encoded_len);

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }

    CHECK(g_handler_called == 1u, "compressed frame dispatched");
    CHECK(g_last_len == 16u, "compressed frame decoded length");
    CHECK(memcmp(g_last_data, data, 16u) == 0, "compressed frame decoded bytes");
}

static void test_rx_bad_sum(void)
{
    uint8_t frame[16]     = {0};
    const uint8_t data[4] = {0x11u, 0x22u, 0x33u, 0x44u};
    uint32_t msg          = 0u;
    uint16_t frame_len    = 0u;
    uint32_t i            = 0u;

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    msg       = test_build_msg(1u, 2u, 0u, 1u, 0u, 0x05u, 0x21u, COMM_V1_CODEC_RAW, 0u, 0u);
    frame_len = test_build_frame(frame, msg, 0u, data, 4u);
    frame[frame_len - 1u] ^= 0x01u; /* Corrupt SUM. */

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }

    CHECK(g_handler_called == 0u, "bad SUM frame dropped");
    CHECK(g_test_v1_ctx.last_seq_valid == 0u, "bad SUM does not update last_seq");
}

static void test_rx_duplicate_seq(void)
{
    uint8_t frame[8]   = {0};
    uint32_t msg       = 0u;
    uint16_t frame_len = 0u;
    uint32_t i         = 0u;

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    msg       = test_build_msg(2u, 2u, 0u, 1u, 0u, 0x05u, 0x21u, COMM_V1_CODEC_RAW, 1u, 0u);
    frame_len = test_build_frame(frame, msg, 1u, NULL, 0u);

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }
    CHECK(g_handler_called == 1u, "first frame dispatched");

    test_reset_rx();

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }
    CHECK(g_handler_called == 0u, "duplicate SEQ frame dropped");
}

static void test_rx_bad_dst(void)
{
    uint8_t frame[8]   = {0};
    uint32_t msg       = 0u;
    uint16_t frame_len = 0u;
    uint32_t i         = 0u;

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    msg       = test_build_msg(0u, 7u, 0u, 1u, 0u, 0x05u, 0x21u, COMM_V1_CODEC_RAW, 1u, 0u);
    frame_len = test_build_frame(frame, msg, 1u, NULL, 0u);

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }
    CHECK(g_handler_called == 0u, "non-local DST frame dropped");
}

static void test_rx_bad_codec(void)
{
    uint8_t frame[16]     = {0};
    const uint8_t data[4] = {0x11u, 0x22u, 0x33u, 0x44u};
    uint32_t msg          = 0u;
    uint16_t frame_len    = 0u;
    uint32_t i            = 0u;

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    msg       = test_build_msg(0u, 2u, 0u, 1u, 0u, 0x05u, 0x21u, 5u, 0u, 0u);
    frame_len = test_build_frame(frame, msg, 0u, data, 4u);

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }
    CHECK(g_handler_called == 0u, "reserved CODEC frame dropped");
}

static void test_rx_256_bytes(void)
{
    uint8_t frame[263] = {0};
    uint8_t data[256]  = {0};
    uint32_t msg       = 0u;
    uint16_t frame_len = 0u;
    uint32_t i         = 0u;
    uint16_t j         = 0u;

    for (j = 0u; j < 256u; ++j)
    {
        data[j] = (uint8_t)(j * 3u);
    }

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    msg       = test_build_msg(4u, 2u, 0u, 1u, 0u, 0x05u, 0x21u, COMM_V1_CODEC_RAW, 0u, 0u);
    frame_len = test_build_frame(frame, msg, 0u, data, 256u);
    CHECK(frame_len == 263u, "256 B frame length");
    CHECK(frame[5] == 0x00u, "256 B LEN encodes as zero");

    for (i = 0u; i < frame_len; ++i)
    {
        comm_v1_run(frame[i], &g_test_tx_func, &g_test_v1_ctx);
    }
    CHECK(g_handler_called == 1u, "256 B frame dispatched");
    CHECK(g_last_len == 256u, "256 B payload length");
    CHECK(memcmp(g_last_data, data, 256u) == 0, "256 B payload bytes");
}

static void test_rx_fragmented(void)
{
    uint8_t frame[16]     = {0};
    const uint8_t data[4] = {0xAAu, 0xBBu, 0xCCu, 0xDDu};
    uint32_t msg          = 0u;
    uint16_t frame_len    = 0u;
    uint16_t split        = 0u;

    for (split = 0u; split <= 11u; ++split)
    {
        test_reset_rx();
        g_test_v1_ctx.last_seq_valid = 0u;

        msg       = test_build_msg(0u, 2u, 0u, 1u, 0u, 0x05u, 0x21u, COMM_V1_CODEC_RAW, 0u, 0u);
        frame_len = test_build_frame(frame, msg, 0u, data, 4u);

        if (split > frame_len)
        {
            break;
        }

        comm_v1_run_buffer(frame, split, &g_test_tx_func, &g_test_v1_ctx);
        comm_v1_run_buffer(&frame[split], (uint32_t)(frame_len - split), &g_test_tx_func, &g_test_v1_ctx);
        CHECK(g_handler_called == 1u, "fragmented frame dispatched");
        CHECK(memcmp(g_last_data, data, 4u) == 0, "fragmented frame bytes");
    }
}

/* =============================================================================
 * Send path tests
 * =============================================================================
 */

static void test_tx_pure_command(void)
{
    section_packform_t pack = {0};

    test_reset_rx();

    pack.sop      = COMM_V1_SOP;
    pack.version  = COMM_V1_VERSION;
    pack.src      = 2u;
    pack.dst      = 1u;
    pack.cmd_set  = 0x05u;
    pack.cmd_word = 0x21u;
    pack.seq      = 6u;
    pack.len      = 0u;
    pack.p_data   = NULL;

    comm_v1_send(&pack, &g_test_tx_func);

    CHECK(g_tx_len == 6u, "sent pure command length");
    CHECK(g_tx_bytes[0] == COMM_V1_SOP, "sent SOP");
    CHECK((g_tx_bytes[4] & 0x40u) != 0u, "sent CMD bit set");
    CHECK((g_tx_bytes[4] & 0x38u) == 0u, "sent CODEC is RAW");
    /* Cross-checked golden bytes shared with the FRAME host test:
     * seq=6, dst=1, d_dst=0, src=2, d_src=0, cmd_set=5, cmd_word=0x21,
     * codec=RAW, cmd=1, ack=0. */
    {
        static const uint8_t golden[6] = {0xE9u, 0x0Eu, 0x08u, 0x2Au, 0x44u, 0x6Du};
        CHECK(memcmp(g_tx_bytes, golden, 6u) == 0, "sent pure command golden bytes");
    }
}

static void test_tx_256_bytes(void)
{
    section_packform_t pack = {0};
    uint8_t data[256]       = {0};
    uint16_t j              = 0u;

    for (j = 0u; j < 256u; ++j)
    {
        data[j] = (uint8_t)(255u - j);
    }

    test_reset_rx();

    pack.sop      = COMM_V1_SOP;
    pack.version  = COMM_V1_VERSION;
    pack.src      = 2u;
    pack.dst      = 1u;
    pack.cmd_set  = 0x05u;
    pack.cmd_word = 0x21u;
    pack.seq      = 1u;
    pack.len      = 256u;
    pack.p_data   = data;

    comm_v1_send(&pack, &g_test_tx_func);

    CHECK(g_tx_len == 263u, "sent 256 B frame length");
    CHECK(g_tx_bytes[5] == 0x00u, "sent LEN zero for 256 B");
    CHECK(memcmp(&g_tx_bytes[6], data, 256u) == 0, "sent 256 B payload");
}

static void test_tx_compression(void)
{
    section_packform_t pack = {0};
    uint8_t data[100]       = {0};
    uint32_t msg            = 0u;
    uint8_t codec           = 0u;

    (void)memset(data, 0x55u, sizeof(data));

    test_reset_rx();

    pack.sop      = COMM_V1_SOP;
    pack.version  = COMM_V1_VERSION;
    pack.src      = 2u;
    pack.dst      = 1u;
    pack.cmd_set  = 0x05u;
    pack.cmd_word = 0x21u;
    pack.seq      = 2u;
    pack.len      = 100u;
    pack.p_data   = data;

    comm_v1_send(&pack, &g_test_tx_func);

    msg = (uint32_t)g_tx_bytes[1] | ((uint32_t)g_tx_bytes[2] << 8u) | ((uint32_t)g_tx_bytes[3] << 16u)
        | ((uint32_t)g_tx_bytes[4] << 24u);
    codec = (uint8_t)((msg >> 27u) & 0x07u);
    CHECK(codec != COMM_V1_CODEC_RAW, "100 identical bytes selected compression");
    CHECK(g_tx_len < 107u, "compressed frame shorter than RAW");
}

static void test_tx_range_reject(void)
{
    section_packform_t pack = {0};

    test_reset_rx();

    pack.sop      = COMM_V1_SOP;
    pack.version  = COMM_V1_VERSION;
    pack.src      = 2u;
    pack.dst      = 1u;
    pack.cmd_set  = 0x30u;
    pack.cmd_word = 0x40u; /* Out of 0..63 range. */
    pack.seq      = 0u;
    pack.len      = 0u;

    comm_v1_send(&pack, &g_test_tx_func);
    CHECK(g_tx_len == 0u, "out-of-range cmd_word rejected");

    pack.cmd_word = 0x01u;
    pack.src      = 0x10u; /* Out of 0..15 range. */
    comm_v1_send(&pack, &g_test_tx_func);
    CHECK(g_tx_len == 0u, "out-of-range src rejected");
}

static void test_tx_codec_select(void)
{
    section_packform_t pack   = {0};
    uint8_t request_data[7]   = {0};
    uint8_t request_frame[16] = {0};
    uint32_t request_len      = 0u;
    uint32_t msg              = 0u;
    uint8_t codec             = 0u;
    uint8_t response_codec    = 0u;
    uint8_t response_result   = 0u;
    uint32_t response_crc     = 0u;
    uint16_t j                = 0u;

    /* 请求 DATA：codec(1) | codebook_id(1) | version(1) | codebook_crc32(4 LE) */
    request_data[0] = (uint8_t)COMM_V1_CODEC_DICT;
    request_data[1] = 0u;
    request_data[2] = 1u;
    request_data[3] = 0x11u;
    request_data[4] = 0x22u;
    request_data[5] = 0x33u;
    request_data[6] = 0x44u;

    test_reset_rx();
    g_test_v1_ctx.last_seq_valid = 0u;

    pack.sop      = COMM_V1_SOP;
    pack.version  = COMM_V1_VERSION;
    pack.src      = 1u; /* Request from host. */
    pack.dst      = 2u; /* Device local address. */
    pack.cmd_set  = COMM_V1_CODEC_SELECT_SET;
    pack.cmd_word = COMM_V1_CODEC_SELECT_WORD;
    pack.seq      = 5u;
    pack.len      = 7u;
    pack.p_data   = request_data;

    comm_v1_send(&pack, &g_test_tx_func);

    /* 校验协商请求帧：CODEC_SELECT 强制 RAW。 */
    msg = (uint32_t)g_tx_bytes[1] | ((uint32_t)g_tx_bytes[2] << 8u) | ((uint32_t)g_tx_bytes[3] << 16u)
        | ((uint32_t)g_tx_bytes[4] << 24u);
    codec = (uint8_t)((msg >> 27u) & 0x07u);
    CHECK(codec == COMM_V1_CODEC_RAW, "CODEC_SELECT request stays RAW");

    /* 拷贝请求帧：设备应答会覆盖 TX 捕获缓冲。 */
    request_len = g_tx_len;
    (void)memcpy(request_frame, g_tx_bytes, request_len);

    /* 设备端解析请求并应答。 */

    for (j = 0u; j < request_len; ++j)
    {
        comm_v1_run(request_frame[j], &g_test_tx_func, &g_test_v1_ctx);
    }
    CHECK(g_tx_len == 15u, "CODEC_SELECT response frame length (8 B payload RAW)");

    /* 解析响应帧内容。 */
    msg = (uint32_t)g_tx_bytes[1] | ((uint32_t)g_tx_bytes[2] << 8u) | ((uint32_t)g_tx_bytes[3] << 16u)
        | ((uint32_t)g_tx_bytes[4] << 24u);
    CHECK(((msg >> 31u) & 0x01u) == 1u, "CODEC_SELECT response ACK set");
    CHECK(((msg & 0x07u)) == 5u, "CODEC_SELECT response echoes SEQ");
    response_result = g_tx_bytes[6];
    response_codec  = g_tx_bytes[7];
    response_crc = (uint32_t)g_tx_bytes[10] | ((uint32_t)g_tx_bytes[11] << 8u) | ((uint32_t)g_tx_bytes[12] << 16u)
                 | ((uint32_t)g_tx_bytes[13] << 24u);
    CHECK(response_result == 0u, "CODEC_SELECT response result OK");
    CHECK(response_codec == (uint8_t)COMM_V1_CODEC_DICT, "CODEC_SELECT response codec DICT");
    CHECK(response_crc == codec_dict_codebook_crc32(), "CODEC_SELECT response CRC32 matches codebook");
}

/* =============================================================================
 * Runner
 * =============================================================================
 */

int main(void)
{
    p_comm_command_first = &g_test_command_item;

    test_sum();
    test_codec_rle();
    test_codec_dict();
    test_codec_lzss();
    test_codec_limits();
    test_codec_zero();
    test_rx_pure_command();
    test_rx_data_raw();
    test_rx_compressed();
    test_rx_bad_sum();
    test_rx_duplicate_seq();
    test_rx_bad_dst();
    test_rx_bad_codec();
    test_rx_256_bytes();
    test_rx_fragmented();
    test_tx_pure_command();
    test_tx_256_bytes();
    test_tx_compression();
    test_tx_range_reject();
    test_tx_codec_select();

    if (g_check_failures != 0)
    {
        printf("FAIL %d checks\n", g_check_failures);
        return 1;
    }

    printf("PASS all comm_v1 checks\n");
    return 0;
}
