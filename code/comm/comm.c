// SPDX-License-Identifier: MIT
/**
 * @file    comm.c
 * @brief   comm communication module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement the 0xE8 framed protocol parser as a byte-by-byte state machine
 *          - Build and send frames with address fields, command set/word, ACK flag, payload length, and CRC16
 *          - Dispatch local registered commands or route frames across registered communication links
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
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

#include <string.h>

/* =============================================================================
 * 全局注册表（由 section 扫描填充）
 * =============================================================================
 */

section_com_t *p_com_first = NULL;
comm_route_t *p_comm_route_first = NULL;

/* section 链路表在 section.c 内维护，这里只使用其首指针 */
extern section_link_t *p_link_first;

/* 使用 tail 指针，将 insert 从 O(n) 降到 O(1) */
static section_com_t *s_com_tail = NULL;
static comm_route_t *s_route_tail = NULL;

/* =============================================================================
 * Comm / Route 插入
 * =============================================================================
 */

static void comm_insert(section_com_t *com)
{
    if (!com)
        return;

    com->p_next = NULL;
    if (!p_com_first)
    {
        p_com_first = com;
        s_com_tail = com;
    }
    else
    {
        s_com_tail->p_next = com;
        s_com_tail = com;
    }
}

static void comm_route_insert(comm_route_t *route)
{
    if (!route)
        return;

    route->p_next = NULL;
    if (!p_comm_route_first)
    {
        p_comm_route_first = route;
        s_route_tail = route;
    }
    else
    {
        s_route_tail->p_next = route;
        s_route_tail = route;
    }
}

static void comm_init(void)
{
    /* 允许重复调用：清空锚点避免链表串接 */
    p_com_first = NULL;
    p_comm_route_first = NULL;
    s_com_tail = NULL;
    s_route_tail = NULL;

    for (reg_section_t *p = (reg_section_t *)&SECTION_START;
         p < (reg_section_t *)&SECTION_STOP;
         ++p)
    {
        switch (p->section_type)
        {
        case SECTION_COMM:
            comm_insert((section_com_t *)p->p_str);
            break;
        case SECTION_COMM_ROUTE:
            comm_route_insert((comm_route_t *)p->p_str);
            break;
        default:
            break;
        }
    }
}

REG_INIT(0, comm_init)

/* =============================================================================
 * CRC16（CCITT）
 * =============================================================================
 */

static uint16_t crc16_table[256];
static uint8_t s_crc16_table_ready = 0;

static void crc16_init_table(void)
{
    if (s_crc16_table_ready)
        return;

    for (uint32_t i = 0; i < 256u; i++)
    {
        uint16_t crc = 0u;
        uint16_t c = (uint16_t)(i << 8);

        for (uint32_t j = 0; j < 8u; j++)
        {
            if (((crc ^ c) & 0x8000u) != 0u)
            {
                crc = (uint16_t)((crc << 1) ^ CRC16_CCITT_POLY);
            }
            else
            {
                crc = (uint16_t)(crc << 1);
            }
            c = (uint16_t)(c << 1);
        }
        crc16_table[i] = crc;
    }

    s_crc16_table_ready = 1;
}

REG_INIT(0, crc16_init_table)

uint16_t crc16_init(void) { return CRC16_CCITT_INIT; }

uint16_t crc16_update(uint16_t crc, uint8_t data)
{
    const uint8_t table_index = (uint8_t)((crc >> 8) ^ data);
    return (uint16_t)((crc << 8) ^ crc16_table[table_index]);
}

uint16_t crc16_final(uint16_t crc) { return crc; }

uint16_t section_crc16(uint8_t *p_data, uint32_t len)
{
    uint16_t crc = crc16_init();
    for (uint32_t i = 0; i < len; i++)
        crc = crc16_update(crc, p_data[i]);
    return crc16_final(crc);
}

uint16_t section_crc16_with_crc(uint8_t *p_data, uint32_t len, uint16_t crc_in)
{
    uint16_t crc = crc_in;
    for (uint32_t i = 0; i < len; i++)
        crc = crc16_update(crc, p_data[i]);
    return crc;
}

/* =============================================================================
 * COMM 命令查找（move-to-front：对热点命令友好）
 * =============================================================================
 */

static void (*find_comm_func(uint8_t cmd_set, uint8_t cmd_word))(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_com_t *p = p_com_first;
    section_com_t *p_last = NULL;

    while (p)
    {
        if ((p->cmd_set == cmd_set) && (p->cmd_word == cmd_word))
        {
            if (p_last)
            {
                /* move-to-front：减少后续查找成本 */
                p_last->p_next = p->p_next;
                p->p_next = p_com_first;
                p_com_first = p;
            }
            return p->func;
        }
        p_last = p;
        p = p->p_next;
    }
    return NULL;
}

/* =============================================================================
 * 路由：根据 link_id + dst_addr 转发到目标链路
 * =============================================================================
 */

static section_link_t *find_link_by_id(uint8_t link_id)
{
    for (section_link_t *p = p_link_first; p; p = p->p_next)
    {
        if (p->link_id == link_id)
            return p;
    }
    return NULL;
}

static void comm_route_run(comm_ctx_t *ctx)
{
    if (!ctx)
        return;

    for (comm_route_t *r = p_comm_route_first; r; r = r->p_next)
    {
        if ((ctx->link_id == r->src_link_id) && (ctx->pack.dst == r->dst_addr))
        {
            section_link_t *dst_link = find_link_by_id(r->dst_link_id);
            if (dst_link)
            {
                comm_send_data(&ctx->pack, dst_link->my_printf);
            }
            /* 同一帧命中多个路由规则是否需要多播？
             * 若你希望多播，就删掉 break。
             */
            break;
        }
    }
}

/* =============================================================================
 * 协议解析状态机
 * =============================================================================
 */

/* 固定字段 */
#define COMM_SOP_BYTE 0xE8u
#define COMM_VER_1 0x01u
#define COMM_EOP_WORD 0x0A0Du
#define COMM_FRAME_TIMEOUT_TICK (1000u)

/* 对 dst/d_dst 的“本机接收”判定 */
static inline uint8_t is_addr_match(uint8_t addr, uint8_t local)
{
    return (uint8_t)((addr == 0x00u) || (addr == local));
}

static inline void comm_reset_ctx(comm_ctx_t *ctx)
{
    ctx->status = SECTION_PACKFORM_STA_SOP;
    ctx->index = 0;
    ctx->len = 0;
    ctx->crc = 0;
    ctx->func = NULL;
    ctx->src_flag = 0;
    ctx->dst_flag = 0;
    ctx->cmd_flag = 0;
    ctx->len_flag = 0;
    ctx->eop_flag = 0;
    ctx->is_route = 0;
}

void comm_run(uint8_t data, DEC_MY_PRINTF, void *p)
{
    (void)my_printf;

    comm_ctx_t *ctx = (comm_ctx_t *)p;
    uint32_t now;
    if (!ctx)
        return;

    now = SECTION_SYS_TICK;
    if ((ctx->status != SECTION_PACKFORM_STA_SOP) &&
        ((uint32_t)(now - ctx->last_rx_tick) > COMM_FRAME_TIMEOUT_TICK))
    {
        comm_reset_ctx(ctx);
    }
    ctx->last_rx_tick = now;

    switch (ctx->status)
    {
    case SECTION_PACKFORM_STA_SOP:
        if (data != COMM_SOP_BYTE)
            return;

        /* start */
        ctx->crc = crc16_init();
        ctx->crc = crc16_update(ctx->crc, data);
        ctx->pack.sop = data;
        ctx->pack.p_data = (uint8_t *)ctx->p_data_buffer;
        ctx->index = 0;
        ctx->len = 0;
        ctx->func = NULL;
        ctx->is_route = 0;
        ctx->src_flag = 0;
        ctx->dst_flag = 0;
        ctx->cmd_flag = 0;
        ctx->len_flag = 0;
        ctx->eop_flag = 0;
        ctx->status = SECTION_PACKFORM_STA_VER;
        break;

    case SECTION_PACKFORM_STA_VER:
        ctx->pack.version = data;
        ctx->crc = crc16_update(ctx->crc, data);
        if (ctx->pack.version != COMM_VER_1)
        {
            comm_reset_ctx(ctx);
            return;
        }
        ctx->status = SECTION_PACKFORM_STA_SRC;
        ctx->src_flag = 0;
        break;

    case SECTION_PACKFORM_STA_SRC:
        if (ctx->src_flag == 0u)
        {
            ctx->pack.src = data;
            ctx->crc = crc16_update(ctx->crc, data);
            ctx->src_flag = 1u;
        }
        else
        {
            ctx->pack.d_src = data;
            ctx->crc = crc16_update(ctx->crc, data);
            ctx->dst_flag = 0u;
            ctx->status = SECTION_PACKFORM_STA_DST;
        }
        break;

    case SECTION_PACKFORM_STA_DST:
        if (ctx->dst_flag == 0u)
        {
            ctx->pack.dst = data;
            ctx->crc = crc16_update(ctx->crc, data);

            /* dst 不匹配本机则标记路由 */
            ctx->is_route = (uint8_t)(!is_addr_match(ctx->pack.dst, ctx->src));
            ctx->dst_flag = 1u;
        }
        else
        {
            ctx->pack.d_dst = data;
            ctx->crc = crc16_update(ctx->crc, data);

            /* d_dst 匹配本机动态地址则接收，否则若 dst 不匹配则允许路由 */
            if (is_addr_match(ctx->pack.d_dst, ctx->d_src) || (ctx->is_route == 1u))
            {
                ctx->cmd_flag = 0u;
                ctx->status = SECTION_PACKFORM_STA_CMD;
            }
            else
            {
                comm_reset_ctx(ctx);
                return;
            }
        }
        break;

    case SECTION_PACKFORM_STA_CMD:
        if (ctx->cmd_flag == 0u)
        {
            ctx->pack.cmd_set = data;
            ctx->crc = crc16_update(ctx->crc, data);
            ctx->cmd_flag = 1u;
        }
        else
        {
            ctx->pack.cmd_word = data;
            ctx->crc = crc16_update(ctx->crc, data);

            if (ctx->is_route == 0u)
            {
                ctx->func = find_comm_func(ctx->pack.cmd_set, ctx->pack.cmd_word);
                if (!ctx->func)
                {
                    comm_reset_ctx(ctx);
                    return;
                }
            }
            ctx->status = SECTION_PACKFORM_STA_ACK;
        }
        break;

    case SECTION_PACKFORM_STA_ACK:
        ctx->pack.is_ack = data;
        ctx->crc = crc16_update(ctx->crc, data);
        ctx->pack.len = 0u;
        ctx->len_flag = 0u;
        ctx->status = SECTION_PACKFORM_STA_LEN;
        break;

    case SECTION_PACKFORM_STA_LEN:
        if (ctx->len_flag == 0u)
        {
            ctx->pack.len = (uint16_t)(ctx->pack.len | (uint16_t)data);
            ctx->crc = crc16_update(ctx->crc, data);
            ctx->len_flag = 1u;
        }
        else
        {
            ctx->pack.len = (uint16_t)(ctx->pack.len | (uint16_t)(data << 8));
            ctx->crc = crc16_update(ctx->crc, data);

            ctx->len = ctx->pack.len;
            ctx->index = 0u;
            ctx->pack.p_data = ctx->p_data_buffer;
            ctx->len_flag = 0u;

            if (ctx->len > ctx->buffer_size)
            {
                comm_reset_ctx(ctx);
                return;
            }

            ctx->status = SECTION_PACKFORM_STA_DATA;
        }
        break;

    case SECTION_PACKFORM_STA_DATA:
        if (ctx->len != 0u)
        {
            ctx->p_data_buffer[ctx->index++] = data;
            ctx->crc = crc16_update(ctx->crc, data);
            ctx->len--;
        }
        else
        {
            /* len==0：当前字节是 CRC low */
            ctx->pack.crc = (uint16_t)data;
            ctx->status = SECTION_PACKFORM_STA_CRC;
        }
        break;

    case SECTION_PACKFORM_STA_CRC:
        /* 当前字节是 CRC high */
        ctx->pack.crc = (uint16_t)(ctx->pack.crc | (uint16_t)(data << 8));
        ctx->crc = crc16_final(ctx->crc);

        if (ctx->crc != ctx->pack.crc)
        {
            comm_reset_ctx(ctx);
            return;
        }

        ctx->pack.eop = 0u;
        ctx->eop_flag = 0u;
        ctx->status = SECTION_PACKFORM_STA_EOP;
        break;

    case SECTION_PACKFORM_STA_EOP:
        if (ctx->eop_flag == 0u)
        {
            ctx->pack.eop = (uint16_t)(ctx->pack.eop | (uint16_t)data);
            ctx->eop_flag = 1u;
        }
        else
        {
            ctx->pack.eop = (uint16_t)(ctx->pack.eop | (uint16_t)(data << 8));
            /* EOP 必须校验：否则“路由帧”会在任意 1 字节后触发转发（原代码的 bug） */
            if (ctx->pack.eop == COMM_EOP_WORD)
            {
                if (ctx->is_route == 1u)
                {
                    comm_route_run(ctx);
                }
                else
                {
                    if (ctx->func)
                        ctx->func(&ctx->pack, my_printf);
                }
            }
            comm_reset_ctx(ctx);
        }
        break;

    case SECTION_PACKFORM_STA_ROUTE:
    default:
        comm_reset_ctx(ctx);
        break;
    }
}

/* =============================================================================
 * 发送
 * =============================================================================
 */

static uint8_t tx_buffer[512];
static uint16_t tx_len = 0;

static inline int tx_push(uint8_t b)
{
    if (tx_len >= (uint16_t)sizeof(tx_buffer))
        return 0;
    tx_buffer[tx_len++] = b;
    return 1;
}

void comm_send_data(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (!p_pack)
        return;

    /* 基础帧长：15 字节（含 EOP、CRC），payload 不能超过 tx_buffer */
    const uint32_t need = 15u + (uint32_t)p_pack->len;
    if (need > sizeof(tx_buffer))
        return;

    tx_len = 0;

    p_pack->version = COMM_VER_1;
    p_pack->crc = crc16_init();

    /* SOP */
    if (!tx_push(COMM_SOP_BYTE))
        return;
    p_pack->crc = crc16_update(p_pack->crc, COMM_SOP_BYTE);

    /* VERSION */
    if (!tx_push(p_pack->version))
        return;
    p_pack->crc = crc16_update(p_pack->crc, p_pack->version);

    /* SRC / D_SRC */
    if (!tx_push(p_pack->src))
        return;
    p_pack->crc = crc16_update(p_pack->crc, p_pack->src);

    if (!tx_push(p_pack->d_src))
        return;
    p_pack->crc = crc16_update(p_pack->crc, p_pack->d_src);

    /* DST / D_DST */
    if (!tx_push(p_pack->dst))
        return;
    p_pack->crc = crc16_update(p_pack->crc, p_pack->dst);

    if (!tx_push(p_pack->d_dst))
        return;
    p_pack->crc = crc16_update(p_pack->crc, p_pack->d_dst);

    /* CMD */
    if (!tx_push(p_pack->cmd_set))
        return;
    p_pack->crc = crc16_update(p_pack->crc, p_pack->cmd_set);

    if (!tx_push(p_pack->cmd_word))
        return;
    p_pack->crc = crc16_update(p_pack->crc, p_pack->cmd_word);

    /* ACK */
    if (!tx_push(p_pack->is_ack))
        return;
    p_pack->crc = crc16_update(p_pack->crc, p_pack->is_ack);

    /* LEN (LE) */
    const uint8_t len_lo = (uint8_t)(p_pack->len & 0xFFu);
    const uint8_t len_hi = (uint8_t)((p_pack->len >> 8) & 0xFFu);
    if (!tx_push(len_lo))
        return;
    if (!tx_push(len_hi))
        return;
    p_pack->crc = crc16_update(p_pack->crc, len_lo);
    p_pack->crc = crc16_update(p_pack->crc, len_hi);

    /* DATA */
    if (p_pack->p_data && p_pack->len)
    {
        for (uint32_t i = 0; i < (uint32_t)p_pack->len; i++)
        {
            const uint8_t b = p_pack->p_data[i];
            if (!tx_push(b))
                return;
            p_pack->crc = crc16_update(p_pack->crc, b);
        }
    }

    /* CRC (LE) */
    const uint16_t crc = crc16_final(p_pack->crc);
    if (!tx_push((uint8_t)(crc & 0xFFu)))
        return;
    if (!tx_push((uint8_t)((crc >> 8) & 0xFFu)))
        return;

    /* EOP 0x0D 0x0A => word 0x0A0D */
    if (!tx_push(0x0Du))
        return;
    if (!tx_push(0x0Au))
        return;

    if (my_printf && my_printf->tx_by_dma)
    {
        my_printf->tx_by_dma((char *)tx_buffer, (int)tx_len);
    }
}
