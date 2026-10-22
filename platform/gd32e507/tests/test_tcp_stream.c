// SPDX-License-Identifier: MIT
/**
 * @file test_tcp_stream.c
 * @brief Exercise the production raw-TCP BSP against delayed ACKs and faults.
 * @details Host-only fake replaces LwIP/CPU operations, never the BSP ring logic.
 *          No scope protocol or simulated acquisition source is used for board acceptance.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "bsp_tcp_stream.h"
#include "lwip/tcp.h"
#include "lwip/sys.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

uint32_t test_primask;
static struct tcp_pcb listener, peer;
static err_t (*accept_cb)(void *, struct tcp_pcb *, err_t);
static err_t (*recv_cb)(void *, struct tcp_pcb *, struct pbuf *, err_t);
static err_t (*sent_cb)(void *, struct tcp_pcb *, u16_t);
static void (*error_cb)(void *, err_t);
static struct
{
    const uint8_t *data;
    uint16_t size;
} writes[32];
static uint32_t generated, delivered, write_count, aborts, frees;
static uint16_t available = 4096u;
static bool blocked, retire_on_read, oversize;
static uint32_t now_ms;

uint32_t sys_now(void)
{
    return now_ms;
}
static void tick(void)
{
    ++now_ms;
    bsp_tcp_stream_poll(true);
}

struct tcp_pcb *tcp_new_ip_type(int type)
{
    (void)type;
    return &listener;
}
err_t tcp_bind(struct tcp_pcb *p, const void *address, uint16_t port)
{
    assert(    p == &listener
            && address == NULL
            && port == 9001u);
    return ERR_OK;
}
struct tcp_pcb *tcp_listen(struct tcp_pcb *p)
{
    return p;
}
void tcp_accept(struct tcp_pcb *p, err_t (*cb)(void *, struct tcp_pcb *, err_t))
{
    (void)p;
    accept_cb = cb;
}
void tcp_arg(struct tcp_pcb *p, void *argument)
{
    (void)p;
    (void)argument;
}
void tcp_recv(struct tcp_pcb *p, err_t (*cb)(void *, struct tcp_pcb *, struct pbuf *, err_t))
{
    (void)p;
    recv_cb = cb;
}
void tcp_sent(struct tcp_pcb *p, err_t (*cb)(void *, struct tcp_pcb *, u16_t))
{
    (void)p;
    sent_cb = cb;
}
void tcp_err(struct tcp_pcb *p, void (*cb)(void *, err_t))
{
    (void)p;
    error_cb = cb;
}
void tcp_abort(struct tcp_pcb *p)
{
    (void)p;
    ++aborts;
}
err_t tcp_close(struct tcp_pcb *p)
{
    (void)p;
    return ERR_OK;
}
void tcp_nagle_disable(struct tcp_pcb *p)
{
    (void)p;
}
u16_t tcp_sndbuf(const struct tcp_pcb *p)
{
    (void)p;
    return available;
}
err_t tcp_write(struct tcp_pcb *p,
                const void *data,
                u16_t length,
                uint8_t flags)
{
    assert(    p == &peer
            && flags == 0u
            && length <= available);

    if (blocked)
        return ERR_MEM;
    assert(write_count < 32u);
    writes[write_count].data   = data;
    writes[write_count++].size = length;
    available                  = (uint16_t)(available - length);
    return ERR_OK;
}
err_t tcp_output(struct tcp_pcb *p)
{
    assert(p == &peer);
    return ERR_OK;
}
void pbuf_free(struct pbuf *p)
{
    assert(p != NULL);
    ++frees;
}
static uint32_t source(uint8_t *data, uint32_t capacity)
{
    if (retire_on_read)
        bsp_tcp_stream_reset();

    if (oversize)
        return capacity + 1u;
    uint32_t size = capacity > 317u ? 317u : capacity;

    for (uint32_t i = 0u; i < size; ++i)
        data[i] = (uint8_t)generated++;
    return size;
}
static void acknowledge(void)
{
    uint32_t total = 0u;

    for (uint32_t i = 0u; i < write_count; ++i)
    {
        for (uint16_t j = 0u; j < writes[i].size; ++j)
            assert(writes[i].data[j] == (uint8_t)delivered++);
        total += writes[i].size;
    }
    write_count = 0u;

    if (total != 0u)
    {
        assert(sent_cb(NULL, &peer, (u16_t)total) == ERR_OK);
        available = (uint16_t)(available + total);
    }
}
static void connect_peer(void)
{
    bsp_tcp_stream_poll(true);
    assert(accept_cb(NULL, &peer, ERR_OK) == ERR_OK);
    generated = delivered = write_count = 0u;
    available                           = 4096u;
    assert(bsp_tcp_stream_connected());
}
int main(void)
{
    assert(!bsp_tcp_stream_configure(0u, source));
    assert(!bsp_tcp_stream_configure(9001u, NULL));
    assert(bsp_tcp_stream_configure(9001u, source));
    assert(!bsp_tcp_stream_configure(9001u, source));
    bsp_tcp_stream_poll(false);
    assert(accept_cb == NULL);
    connect_peer();
    uint32_t session = bsp_tcp_stream_session();
    assert(accept_cb(NULL, &listener, ERR_OK) == ERR_ABRT);
    blocked = true;

    bsp_tcp_stream_poll(true);
    assert(generated == 0u); /* No batch before the first millisecond. */
    tick();
    uint32_t first_batch = generated;
    assert(first_batch > 0u);
    bsp_tcp_stream_poll(true);
    assert(generated == first_batch); /* Repeated polls in the same ms. */

    for (unsigned i = 0u; i < 40u; ++i)
        tick();
    assert(    write_count == 0u
            && generated <= 2499u);
    blocked = false;

    for (unsigned i = 0u; i < 1000u; ++i)
    {
        tick();

        if (i % 3u == 0u)
            acknowledge();
    }
    acknowledge();
    assert(delivered > 100000u);
    assert(    test_primask == 0u
            && bsp_tcp_stream_session() == session);
    bsp_tcp_stream_reset();
    assert(!bsp_tcp_stream_connected());
    connect_peer();
    assert(bsp_tcp_stream_session() > session);
    retire_on_read = true;
    tick();
    retire_on_read = false;
    assert(    !bsp_tcp_stream_connected()
            && write_count == 0u);
    connect_peer();
    oversize = true;
    tick();
    oversize = false;
    assert(    !bsp_tcp_stream_connected()
            && bsp_tcp_stream_error() == 4u);
    connect_peer();
    struct pbuf packet;
    assert(recv_cb(NULL, &peer, &packet, ERR_OK) == ERR_ABRT);
    assert(    frees == 1u
            &&!bsp_tcp_stream_connected());
    connect_peer();
    error_cb(NULL, ERR_ABRT);
    assert(!bsp_tcp_stream_connected());
    connect_peer();
    bsp_tcp_stream_poll(false);
    assert(!bsp_tcp_stream_connected());
    now_ms = UINT32_MAX - 1u;
    connect_peer();
    tick();
    acknowledge();
    uint32_t before_wrap = delivered;
    tick(); /* Millisecond clock wraps to zero. */
    acknowledge();
    assert(delivered > before_wrap);
    bsp_tcp_stream_poll(false);
    assert(aborts > 0u);
    puts(
        "PASS: 2500-byte TCP ring, 1 ms pacing/clock wrap, delayed ACK, ERR_MEM, session retirement, source fault and forbidden receive");
    return 0;
}
