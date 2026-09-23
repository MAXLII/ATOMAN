// SPDX-License-Identifier: MIT
/**
 * @file comm_v1.c
 * @brief COMM v1 (0xE9) protocol module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Parse the byte-stream 0xE9 framed protocol (MSG, LEN, DATA, SUM)
 *          - Reject duplicate sequences, mismatched addresses, bad checksums, and illegal CODEC values
 *          - Decode DICT / RLE / LZSS compressed DATA after checksum validation
 *          - Dispatch local commands through the shared REG_COMM table or route validated frames
 *          - Build and send 0xE9 frames with candidate compression selection
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *          - v1 coexists with the legacy 0xE8 protocol; comm_send_data picks the
 *            encoding from the first byte of the supplied pack object
 *          - Each concurrent send owns its transmission slot, so candidate
 *            compression buffers are never shared without protection
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

#include <string.h>

/* =============================================================================
 * 常量
 * =============================================================================
 */

/* 数据帧固定前缀长度：SOP + MSG(4) + LEN(1) */
#define COMM_V1_FRAME_PREFIX 6u
/* 发送缓冲池容量 */
#define COMM_V1_TX_BUFFER_COUNT 4u
/* 压缩最低节省量阈值分段点 */
#define COMM_V1_MIN_SAVED_RAW_SMALL 16u

/* =============================================================================
 * 发送缓冲池（与 comm.c 相同策略：忙标志原子锁，每槽自带候选缓冲）
 * =============================================================================
 */

typedef struct
{
    volatile uint8_t busy;           ///< 忙标志：0 空闲，1 占用
    uint8_t data[COMM_V1_FRAME_MAX]; ///< 发送组包工作区（最大 263 B）
    uint8_t candidate[COMM_V1_MAX_DATA]; ///< 压缩候选临时缓冲（256 B）
} comm_v1_tx_buffer_t;

static comm_v1_tx_buffer_t s_comm_v1_tx_buffer[COMM_V1_TX_BUFFER_COUNT];

static comm_v1_tx_buffer_t *comm_v1_tx_buffer_acquire(void)
{
    for (;;)
    {
        for (uint32_t i = 0u; i < COMM_V1_TX_BUFFER_COUNT; ++i)
        {
            if (    (__LDREXB(&s_comm_v1_tx_buffer[i].busy) == 0u)
                 && (__STREXB(1u, &s_comm_v1_tx_buffer[i].busy) == 0u))
            {
                __DMB();
                return &s_comm_v1_tx_buffer[i];
            }
        }
    }
}

static void comm_v1_tx_buffer_release(comm_v1_tx_buffer_t *tx)
{
    if (tx == NULL)
    {
        return;
    }

    __DMB();
    tx->busy = 0u;
}

/* =============================================================================
 * SUM 校验
 * =============================================================================
 */

uint8_t comm_sum_encode(const uint8_t *p_data, uint16_t length)
{
    uint8_t sum = 0u;
    uint16_t i  = 0u;

    if (p_data == NULL)
    {
        return 0x55u;
    }

    for (i = 0u; i < length; ++i)
    {
        sum = (uint8_t)(sum + p_data[i]);
    }

    if (sum == 0x00u)
    {
        return 0x55u;
    }

    if (sum == 0xFFu)
    {
        return 0xAAu;
    }
    return sum;
}

/* =============================================================================
 * 命令查找与路由
 * =============================================================================
 */

static void (*comm_v1_find_func(uint8_t cmd_set, uint8_t cmd_word))(void *p_pack, DEC_MY_PRINTF)
{
    for (section_item_t *p_item = p_comm_command_first; p_item != NULL; p_item = p_item->p_next)
    {
        section_com_t *p = (section_com_t *)p_item->p_obj;

        if (    (p->cmd_set == cmd_set)
             && (p->cmd_word == cmd_word))
        {
            return p->func;
        }
    }
    return NULL;
}

static const section_link_t *comm_v1_find_link_by_id(uint8_t link_id)
{
    for (section_item_t *p_item = p_link_first; p_item != NULL; p_item = p_item->p_next)
    {
        const section_link_t *p = (const section_link_t *)p_item->p_obj;

        if (p->link_id == link_id)
        {
            return p;
        }
    }

    return NULL;
}

static uint8_t comm_v1_is_route_target(uint8_t link_id, uint8_t dst_addr)
{
    for (section_item_t *p_item = p_comm_route_first; p_item != NULL; p_item = p_item->p_next)
    {
        comm_route_t *r = (comm_route_t *)p_item->p_obj;

        if (    (r->src_link_id == link_id)
             && (r->dst_addr == dst_addr))
        {
            return 1u;
        }
    }
    return 0u;
}

static void comm_v1_route_forward(comm_v1_ctx_t *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    for (section_item_t *p_item = p_comm_route_first; p_item != NULL; p_item = p_item->p_next)
    {
        comm_route_t *r = (comm_route_t *)p_item->p_obj;

        if (    (ctx->link_id == r->src_link_id)
             && (ctx->pack.dst == r->dst_addr))
        {
            const section_link_t *dst_link = comm_v1_find_link_by_id(r->dst_link_id);

            if (    (dst_link != NULL)
                 && (dst_link->my_printf != NULL)
                 && (dst_link->my_printf->tx_by_dma != NULL))
            {
                /* 转发保留原始编码字节：SOP..SUM 原样透传。 */
                dst_link->my_printf->tx_by_dma((char *)ctx->p_wire_buffer, (int)ctx->wire_len);
            }
            break;
        }
    }
}

/* =============================================================================
 * 解析复位与解压
 * =============================================================================
 */

static void comm_v1_reset(comm_v1_ctx_t *ctx)
{
    ctx->status   = COMM_V1_STA_IDLE;
    ctx->index    = 0u;
    ctx->len      = 0u;
    ctx->sum      = 0u;
    ctx->msg      = 0u;
    ctx->msg_cnt  = 0u;
    ctx->wire_len = 0u;
    ctx->is_route = 0u;
    ctx->is_local = 0u;
    ctx->func     = NULL;
    /* 上一有效包 SEQ 保留：残帧、校验失败不更新去重历史。 */
}

void comm_v1_reset_ctx(comm_v1_ctx_t *p_ctx)
{
    if (p_ctx == NULL)
    {
        return;
    }

    comm_v1_reset(p_ctx);
}

void comm_v1_reset_session(comm_v1_ctx_t *p_ctx)
{
    if (p_ctx == NULL)
    {
        return;
    }

    comm_v1_reset(p_ctx);
    /* 新会话清除上一有效 SEQ，避免把断线前的序号当成本会话的重复包。 */
    p_ctx->last_seq       = 0u;
    p_ctx->last_seq_valid = 0u;
}

static int8_t comm_v1_decode_data(uint8_t codec,
                                  uint16_t input_len,
                                  const uint8_t *p_input,
                                  uint16_t *p_output_len,
                                  uint8_t *p_output)
{
    /* v1 解压限制：允许完整输出 256 B，因此 limit_len=257。 */

    switch (codec)
    {
    case COMM_V1_CODEC_DICT:
        return codec_dict_decode(input_len, p_input, p_output_len, p_output, 257u);
    case COMM_V1_CODEC_RLE:
        return codec_rle_decode(input_len, p_input, p_output_len, p_output, 257u);
    case COMM_V1_CODEC_LZSS:
        return codec_lzss_decode(input_len, p_input, p_output_len, p_output, 257u);
    case COMM_V1_CODEC_ZERO:
        return codec_zero_decode(input_len, p_input, p_output_len, p_output, 257u);
    default:
        return 0;
    }
}

/* =============================================================================
 * CODEC_SELECT 码本协商（CMD_SET=0、CMD_WORD=0）
 * =============================================================================
 */

static void comm_v1_codec_select_handle(comm_v1_ctx_t *ctx, DEC_MY_PRINTF)
{
    uint8_t response_data[8]    = {0};
    section_packform_t response = {0};
    uint32_t codebook_crc       = 0u;

    /* 初版只应答协商请求；设备收到协商响应时不发起新的业务。 */

    if (ctx->pack.is_ack != 0u)
    {
        return;
    }

    codebook_crc = codec_dict_codebook_crc32();

    /* 响应 DATA：result(1) | codec(1) | codebook_id(1) | version(1) | crc32(4, LE) */
    response_data[0] = COMM_V1_CODEC_SELECT_OK;
    response_data[1] = (uint8_t)COMM_V1_CODEC_DICT;
    response_data[2] = codec_dict_codebook_id();
    response_data[3] = codec_dict_codebook_version();
    response_data[4] = (uint8_t)(codebook_crc & 0xFFu);
    response_data[5] = (uint8_t)((codebook_crc >> 8u) & 0xFFu);
    response_data[6] = (uint8_t)((codebook_crc >> 16u) & 0xFFu);
    response_data[7] = (uint8_t)((codebook_crc >> 24u) & 0xFFu);

    /* 单播响应：SRC/D_SRC 与请求 DST/D_DST 对调，SEQ 回显。 */
    response.sop      = COMM_V1_SOP;
    response.version  = COMM_V1_VERSION;
    response.src      = ctx->pack.dst;
    response.d_src    = ctx->pack.d_dst;
    response.dst      = ctx->pack.src;
    response.d_dst    = ctx->pack.d_src;
    response.cmd_set  = COMM_V1_CODEC_SELECT_SET;
    response.cmd_word = COMM_V1_CODEC_SELECT_WORD;
    response.is_ack   = 1u;
    response.seq      = ctx->pack.seq;
    response.len      = (uint16_t)sizeof(response_data);
    response.p_data   = response_data;

    comm_v1_send(&response, my_printf);
}

/* =============================================================================
 * 接收状态机
 * =============================================================================
 */

void comm_v1_run(uint8_t data, DEC_MY_PRINTF, void *p_context)
{
    comm_v1_ctx_t *ctx = (comm_v1_ctx_t *)p_context;
    uint32_t now       = 0u;

    if (ctx == NULL)
    {
        return;
    }

    /* 半帧超时复位：保留上一有效 SEQ，等待下一个 0xE9。 */
    now = SECTION_SYS_TICK;

    if (    (ctx->status != COMM_V1_STA_IDLE)
         && ((uint32_t)(now - ctx->last_rx_tick) > COMM_V1_FRAME_TIMEOUT_TICK))
    {
        comm_v1_reset(ctx);
    }
    ctx->last_rx_tick = now;

    switch (ctx->status)
    {
    case COMM_V1_STA_IDLE:

        if (data == COMM_V1_SOP)
        {
            ctx->p_wire_buffer[0] = data;
            ctx->wire_len         = 1u;
            ctx->sum              = data;
            ctx->msg              = 0u;
            ctx->msg_cnt          = 0u;
            ctx->index            = 0u;
            ctx->len              = 0u;
            ctx->is_route         = 0u;
            ctx->is_local         = 0u;
            ctx->func             = NULL;
            ctx->status           = COMM_V1_STA_MSG_L0;
        }
        break;

    case COMM_V1_STA_MSG_L0:
    {
        const uint8_t seq = (uint8_t)(data & COMM_V1_SEQ_MASK);
        const uint8_t dst = (uint8_t)((data >> 3u) & 0x0Fu);
        const uint8_t is_duplicate = (uint8_t)(    (ctx->last_seq_valid == 1u)
                                                && (seq == ctx->last_seq));
        const uint8_t is_local_static = (uint8_t)(    (dst == 0u)
                                                   || (dst == ctx->src));
        const uint8_t is_route_target = comm_v1_is_route_target(ctx->link_id, dst);
        const uint8_t accept_header = (uint8_t)(    (is_duplicate == 0u)
                                                 && (    (is_local_static == 1u)
                                                      || (is_route_target == 1u)));

        if (accept_header == 0u)
        {
            /* 重复 SEQ，或目的地址既不匹配本机/广播也不匹配路由：立即返回空闲。 */
            comm_v1_reset(ctx);
            return;
        }

        ctx->is_local = is_local_static;
        ctx->is_route = (uint8_t)(    (is_local_static == 0u)
                                   && (is_route_target == 1u));

        ctx->msg                          = (uint32_t)data;
        ctx->msg_cnt                      = 1u;
        ctx->sum                          = (uint8_t)(ctx->sum + data);
        ctx->p_wire_buffer[ctx->wire_len] = data;
        ctx->wire_len++;
        ctx->status = COMM_V1_STA_MSG_L1;
        break;
    }

    case COMM_V1_STA_MSG_L1:
        ctx->msg |= (uint32_t)data << 8u;
        ctx->msg_cnt                      = 2u;
        ctx->sum                          = (uint8_t)(ctx->sum + data);
        ctx->p_wire_buffer[ctx->wire_len] = data;
        ctx->wire_len++;
        ctx->status = COMM_V1_STA_MSG_L2;
        break;

    case COMM_V1_STA_MSG_L2:
        ctx->msg |= (uint32_t)data << 16u;
        ctx->msg_cnt                      = 3u;
        ctx->sum                          = (uint8_t)(ctx->sum + data);
        ctx->p_wire_buffer[ctx->wire_len] = data;
        ctx->wire_len++;
        ctx->status = COMM_V1_STA_MSG_L3;
        break;

    case COMM_V1_STA_MSG_L3:
    {
        const uint8_t msg_cmd = (uint8_t)((data >> 6u) & 0x01u);
        const uint8_t msg_codec = (uint8_t)((data >> 3u) & 0x07u);
        uint8_t msg_d_dst = 0u;

        ctx->msg |= (uint32_t)data << 24u;
        ctx->msg_cnt                      = 4u;
        ctx->sum                          = (uint8_t)(ctx->sum + data);
        ctx->p_wire_buffer[ctx->wire_len] = data;
        ctx->wire_len++;

        /* 从 MSG 显式提取全部字段（线上格式与位域无关）。 */
        ctx->pack.sop     = COMM_V1_SOP;
        ctx->pack.version = COMM_V1_VERSION;
        ctx->pack.seq     = (uint8_t)(ctx->msg & 0x07u);
        ctx->pack.dst = (uint8_t)((ctx->msg >> 3u) & 0x0Fu);
        msg_d_dst = (uint8_t)(((ctx->msg >> 7u) & 0x01u) | (((ctx->msg >> 8u) & 0x03u) << 1u));
        ctx->pack.d_dst = msg_d_dst;
        ctx->pack.src = (uint8_t)((ctx->msg >> 10u) & 0x0Fu);
        ctx->pack.d_src = (uint8_t)(((ctx->msg >> 14u) & 0x03u) | (((ctx->msg >> 16u) & 0x01u) << 2u));
        ctx->pack.cmd_set = (uint8_t)((ctx->msg >> 17u) & 0x0Fu);
        ctx->pack.cmd_word = (uint8_t)(((ctx->msg >> 21u) & 0x07u) | (((ctx->msg >> 24u) & 0x07u) << 3u));
        ctx->pack.is_ack = (uint8_t)((ctx->msg >> 31u) & 0x01u);

        /* 编码合法性：纯命令帧必须 RAW，保留 CODEC 必须拒绝。 */

        if (    (    (msg_cmd == 1u)
                  && (msg_codec != COMM_V1_CODEC_RAW))
             || (msg_codec > COMM_V1_CODEC_ZERO))
        {
            comm_v1_reset(ctx);
            return;
        }

        /* 本地投递要求静态、动态目的地址均匹配；路由帧不在此检查。 */

        if (ctx->is_route == 0u)
        {
            const uint8_t d_dst_ok = (uint8_t)(    (msg_d_dst == 0u)
                                                || (msg_d_dst == ctx->d_src));

            if (d_dst_ok == 0u)
            {
                comm_v1_reset(ctx);
                return;
            }
        }

        if (msg_cmd == 1u)
        {
            /* 纯命令帧固定 6 B：直接等待 SUM。 */
            ctx->pack.p_data = NULL;
            ctx->pack.len    = 0u;
            ctx->status      = COMM_V1_STA_SUM;
        }
        else
        {
            ctx->status = COMM_V1_STA_LEN;
        }
        break;
    }

    case COMM_V1_STA_LEN:
    {
        /* LEN：0x01~0xFF 表示 1~255 B，0x00 表示 256 B。 */
        ctx->len = (data == 0u) ? COMM_V1_MAX_DATA : (uint16_t)data;
        ctx->index                        = 0u;
        ctx->sum                          = (uint8_t)(ctx->sum + data);
        ctx->p_wire_buffer[ctx->wire_len] = data;
        ctx->wire_len++;
        ctx->status = COMM_V1_STA_DATA;
        break;
    }

    case COMM_V1_STA_DATA:
        ctx->p_data_buffer[ctx->index] = data;
        ctx->index++;
        ctx->sum                          = (uint8_t)(ctx->sum + data);
        ctx->p_wire_buffer[ctx->wire_len] = data;
        ctx->wire_len++;
        ctx->len--;

        if (ctx->len == 0u)
        {
            ctx->status = COMM_V1_STA_SUM;
        }
        break;

    case COMM_V1_STA_SUM:
    {
        const uint8_t expected_sum = comm_sum_encode(ctx->p_wire_buffer, ctx->wire_len);
        uint8_t msg_codec = (uint8_t)((ctx->msg >> 27u) & 0x07u);

        if (data != expected_sum)
        {
            /* SUM 不相等则丢弃整帧，不更新上一有效 SEQ。 */
            comm_v1_reset(ctx);
            return;
        }

        if (msg_codec != COMM_V1_CODEC_RAW)
        {
            /* 先校验 SUM，再解压 DATA；解压输出放入候选缓冲。 */
            uint16_t output_len = ctx->candidate_size;
            int8_t decode_result =
                comm_v1_decode_data(msg_codec, ctx->index, ctx->p_data_buffer, &output_len, ctx->p_candidate_buffer);

            if (decode_result != 1)
            {
                /* 解压输出为 0 B、超限、未消费完或非法 Token：丢弃整帧。 */
                comm_v1_reset(ctx);
                return;
            }

            ctx->pack.p_data = ctx->p_candidate_buffer;
            ctx->pack.len    = output_len;
        }
        else
        {
            ctx->pack.p_data = ctx->p_data_buffer;
            ctx->pack.len    = ctx->index;
        }

        ctx->pack.crc = 0u;
        ctx->pack.eop = 0u;

        /* 码本协商由 comm 层直接处理，不进入业务命令表。 */

        if (    (ctx->pack.cmd_set == COMM_V1_CODEC_SELECT_SET)
             && (ctx->pack.cmd_word == COMM_V1_CODEC_SELECT_WORD))
        {
            ctx->last_seq       = ctx->pack.seq;
            ctx->last_seq_valid = 1u;
            comm_v1_codec_select_handle(ctx, my_printf);
            comm_v1_reset(ctx);
            return;
        }

        if (ctx->is_route == 1u)
        {
            /* 路由包：完整帧校验后按路由表转发原始编码字节。 */
            ctx->last_seq       = ctx->pack.seq;
            ctx->last_seq_valid = 1u;
            comm_v1_route_forward(ctx);
            comm_v1_reset(ctx);
            return;
        }

        /* 本地包：查 REG_COMM 表，仅有效命令更新上一有效 SEQ。 */
        ctx->func = comm_v1_find_func(ctx->pack.cmd_set, ctx->pack.cmd_word);

        if (ctx->func == NULL)
        {
            comm_v1_reset(ctx);
            return;
        }

        ctx->last_seq       = ctx->pack.seq;
        ctx->last_seq_valid = 1u;
        ctx->func(&ctx->pack, my_printf);
        comm_v1_reset(ctx);
        break;
    }

    default:
        comm_v1_reset(ctx);
        break;
    }
}

void comm_v1_run_buffer(const uint8_t *p_data,
                        uint32_t length,
                        DEC_MY_PRINTF,
                        void *p_context)
{
    comm_v1_ctx_t *p_ctx = (comm_v1_ctx_t *)p_context; /* Parser context owned by the active communication link. */
    uint32_t offset      = 0u; /* Next byte in the supplied transport block. */

    if (    (p_data == NULL)
         || /* The transport did not provide a readable block. */
            (p_ctx == NULL)) /* The link has no parser state or payload storage. */
    {
        return;
    }

    while (offset < length)
    {
        if (p_ctx->status == COMM_V1_STA_IDLE)
        {
            const uint8_t *p_sop = (const uint8_t *)memchr(&p_data[offset], /* Next possible frame marker. */
                                                           COMM_V1_SOP,
                                                           length - offset);

            if (p_sop == NULL)
            {
                return;
            }
            offset = (uint32_t)(p_sop - p_data);
        }

        if (    (p_ctx->status == COMM_V1_STA_DATA)
             && /* A validated payload is being received. */
                (p_ctx->len > 0u)) /* At least one payload byte remains. */
        {
            uint32_t available_length = length - offset; /* Bytes remaining in the transport block. */
            uint32_t copy_length = (available_length < p_ctx->len) ? available_length : p_ctx->len;
            uint32_t sum_index = 0u;

            (void)memcpy(&p_ctx->p_data_buffer[p_ctx->index], &p_data[offset], copy_length);
            (void)memcpy(&p_ctx->p_wire_buffer[p_ctx->wire_len], &p_data[offset], copy_length);

            for (sum_index = 0u; sum_index < copy_length; ++sum_index)
            {
                p_ctx->sum = (uint8_t)(p_ctx->sum + p_data[offset + sum_index]);
            }
            p_ctx->index    = (uint16_t)((uint32_t)p_ctx->index + copy_length);
            p_ctx->wire_len = (uint16_t)((uint32_t)p_ctx->wire_len + copy_length);
            p_ctx->len      = (uint16_t)((uint32_t)p_ctx->len - copy_length);

            if (p_ctx->len == 0u)
            {
                /* 批量路径不经过逐字节 DATA case，须显式推进到 SUM。 */
                p_ctx->status = COMM_V1_STA_SUM;
            }
            p_ctx->last_rx_tick = SECTION_SYS_TICK;
            offset += copy_length;
            continue;
        }

        comm_v1_run(p_data[offset], my_printf, p_context);
        offset++;
    }
}

/* =============================================================================
 * 发送：压缩择优 + 组包
 * =============================================================================
 */

static uint8_t comm_v1_is_codec_select(const section_packform_t *p_pack)
{
    return (uint8_t)(    (p_pack->cmd_set == COMM_V1_CODEC_SELECT_SET)
                      && (p_pack->cmd_word == COMM_V1_CODEC_SELECT_WORD));
}

void comm_v1_send(void *p_frame, DEC_MY_PRINTF)
{
    section_packform_t *p_pack = (section_packform_t *)p_frame;
    comm_v1_tx_buffer_t *tx    = NULL;
    uint8_t *tx_buffer         = NULL;
    uint16_t raw_len           = 0u;
    uint16_t best_len          = 0u;
    uint8_t best_codec         = COMM_V1_CODEC_RAW;
    uint16_t min_saved         = 0u;
    uint8_t cmd                = 0u;
    uint8_t ack                = 0u;
    uint32_t msg               = 0u;
    uint16_t frame_len         = 0u;

    if (p_pack == NULL)
    {
        return;
    }

    /* v1 范围检查：不截断任何超范围字段，直接拒绝发送。 */

    if (    (p_pack->version != COMM_V1_VERSION)
         || (p_pack->len > COMM_V1_MAX_DATA)
         || (    (p_pack->len > 0u)
              && (p_pack->p_data == NULL))
         || (p_pack->seq > COMM_V1_SEQ_MASK)
         || (p_pack->src > COMM_V1_ADDR_MAX)
         || (p_pack->dst > COMM_V1_ADDR_MAX)
         || (p_pack->d_src > COMM_V1_DADDR_MAX)
         || (p_pack->d_dst > COMM_V1_DADDR_MAX)
         || (p_pack->cmd_set > COMM_V1_CMD_SET_MAX)
         || (p_pack->cmd_word > COMM_V1_CMD_WORD_MAX))
    {
        return;
    }

    tx        = comm_v1_tx_buffer_acquire();
    tx_buffer = tx->data;

    raw_len = p_pack->len;
    cmd = (raw_len == 0u) ? 1u : 0u;
    ack = (p_pack->is_ack != 0u) ? 1u : 0u;
    best_len   = raw_len;
    best_codec = COMM_V1_CODEC_RAW;

    if (raw_len == 0u)
    {
        /* 纯命令帧：CMD=1、CODEC=RAW，不写 LEN 和 DATA。 */
    }
    else
    {
        /* 初始最优编码为 RAW：原始 DATA 复制到发送缓冲 DATA 区。 */
        (void)memcpy(&tx_buffer[COMM_V1_FRAME_PREFIX], p_pack->p_data, raw_len);

        /* 最低节省量相对 RAW 计算；raw_len <= min_saved 时直接 RAW。 */

        if (raw_len <= COMM_V1_MIN_SAVED_RAW_SMALL)
        {
            min_saved = 2u;
        }
        else
        {
            min_saved = (uint16_t)((raw_len + 15u) / 16u);

            if (min_saved < 2u)
            {
                min_saved = 2u;
            }
        }

        /* CODEC_SELECT 协商帧强制 RAW，避免协商未完成时使用压缩。 */

        if (    (raw_len > min_saved)
             && (comm_v1_is_codec_select(p_pack) == 0u))
        {
            uint16_t limit_len     = 0u;
            uint16_t candidate_len = 0u;
            int8_t codec_result    = 0;

            /* 候选 1：DICT（所有长度都尝试）。 */
            candidate_len = COMM_V1_MAX_DATA;
            limit_len = (uint16_t)((best_len < (raw_len - min_saved + 1u)) ? best_len : (raw_len - min_saved + 1u));
            codec_result = codec_dict_encode(raw_len, p_pack->p_data, &candidate_len, tx->candidate, limit_len);

            if (    (codec_result == 1)
                 && (candidate_len < best_len))
            {
                (void)memcpy(&tx_buffer[COMM_V1_FRAME_PREFIX], tx->candidate, candidate_len);
                best_len   = candidate_len;
                best_codec = COMM_V1_CODEC_DICT;
            }

            /* 候选 2：RLE（17 B 及以上）。 */

            if (raw_len > COMM_V1_MIN_SAVED_RAW_SMALL)
            {
                candidate_len = COMM_V1_MAX_DATA;
                limit_len = (uint16_t)((best_len < (raw_len - min_saved + 1u)) ? best_len : (raw_len - min_saved + 1u));
                codec_result = codec_rle_encode(raw_len, p_pack->p_data, &candidate_len, tx->candidate, limit_len);

                if (    (codec_result == 1)
                     && (candidate_len < best_len))
                {
                    (void)memcpy(&tx_buffer[COMM_V1_FRAME_PREFIX], tx->candidate, candidate_len);
                    best_len   = candidate_len;
                    best_codec = COMM_V1_CODEC_RLE;
                }
            }

            /* 候选 3：LZSS（64 B 及以上）。 */

            if (raw_len >= 64u)
            {
                candidate_len = COMM_V1_MAX_DATA;
                limit_len = (uint16_t)((best_len < (raw_len - min_saved + 1u)) ? best_len : (raw_len - min_saved + 1u));
                codec_result = codec_lzss_encode(raw_len, p_pack->p_data, &candidate_len, tx->candidate, limit_len);

                if (    (codec_result == 1)
                     && (candidate_len < best_len))
                {
                    (void)memcpy(&tx_buffer[COMM_V1_FRAME_PREFIX], tx->candidate, candidate_len);
                    best_len   = candidate_len;
                    best_codec = COMM_V1_CODEC_LZSS;
                }
            }

            /* 候选 4：ZERO 数零法（所有长度，小值密集数据受益）。 */
            {
                candidate_len = COMM_V1_MAX_DATA;
                limit_len = (uint16_t)((best_len < (raw_len - min_saved + 1u)) ? best_len : (raw_len - min_saved + 1u));
                codec_result = codec_zero_encode(raw_len, p_pack->p_data, &candidate_len, tx->candidate, limit_len);

                if (    (codec_result == 1)
                     && (candidate_len < best_len))
                {
                    (void)memcpy(&tx_buffer[COMM_V1_FRAME_PREFIX], tx->candidate, candidate_len);
                    best_len   = candidate_len;
                    best_codec = COMM_V1_CODEC_ZERO;
                }
            }
        }
    }

    /* 填写 MSG（小端线上顺序，显式移位保证）。 */
    msg = (uint32_t)p_pack->seq | ((uint32_t)p_pack->dst << 3u) | ((uint32_t)p_pack->d_dst << 7u)
        | ((uint32_t)p_pack->src << 10u) | ((uint32_t)p_pack->d_src << 14u) | ((uint32_t)p_pack->cmd_set << 17u)
        | ((uint32_t)p_pack->cmd_word << 21u) | ((uint32_t)best_codec << 27u) | ((uint32_t)cmd << 30u)
        | ((uint32_t)ack << 31u);

    tx_buffer[0] = COMM_V1_SOP;
    tx_buffer[1] = (uint8_t)(msg & 0xFFu);
    tx_buffer[2] = (uint8_t)((msg >> 8u) & 0xFFu);
    tx_buffer[3] = (uint8_t)((msg >> 16u) & 0xFFu);
    tx_buffer[4] = (uint8_t)((msg >> 24u) & 0xFFu);

    if (cmd == 1u)
    {
        /* 纯命令帧固定 6 B。 */
        frame_len = COMM_V1_PURE_CMD_LEN;
    }
    else
    {
        /* 数据帧：LEN（0x00 表示 256 B）+ DATA + SUM。 */
        tx_buffer[5] = (best_len == COMM_V1_MAX_DATA) ? 0u : (uint8_t)best_len;
        frame_len = (uint16_t)(COMM_V1_FRAME_PREFIX + best_len + 1u);
    }

    tx_buffer[frame_len - 1u] = comm_sum_encode(tx_buffer, (uint16_t)(frame_len - 1u));

    if (    (my_printf != NULL)
         && (my_printf->tx_by_dma != NULL))
    {
        my_printf->tx_by_dma((char *)tx_buffer, (int)frame_len);
    }

    comm_v1_tx_buffer_release(tx);
}
