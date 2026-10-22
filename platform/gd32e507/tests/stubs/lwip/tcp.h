// SPDX-License-Identifier: MIT
/** @file tcp.h @brief Minimal raw TCP contract for the real BSP host test. */
#ifndef TEST_LWIP_TCP_H
#define TEST_LWIP_TCP_H
#include <stdint.h>
typedef int err_t;
typedef uint16_t u16_t;
#define ERR_OK         0
#define ERR_MEM        (-1)
#define ERR_ABRT       (-13)
#define IPADDR_TYPE_V4 0
#define IP_ANY_TYPE    ((void *)0)
struct pbuf
{
    int unused;
};
struct tcp_pcb
{
    int unused;
};
struct tcp_pcb *tcp_new_ip_type(int type);
err_t tcp_bind(struct tcp_pcb *peer, const void *address, uint16_t port);
struct tcp_pcb *tcp_listen(struct tcp_pcb *peer);
void tcp_accept(struct tcp_pcb *peer, err_t (*callback)(void *, struct tcp_pcb *, err_t));
void tcp_arg(struct tcp_pcb *peer, void *argument);
void tcp_recv(struct tcp_pcb *peer, err_t (*callback)(void *, struct tcp_pcb *, struct pbuf *, err_t));
void tcp_sent(struct tcp_pcb *peer, err_t (*callback)(void *, struct tcp_pcb *, u16_t));
void tcp_err(struct tcp_pcb *peer, void (*callback)(void *, err_t));
void tcp_abort(struct tcp_pcb *peer);
err_t tcp_close(struct tcp_pcb *peer);
void tcp_nagle_disable(struct tcp_pcb *peer);
u16_t tcp_sndbuf(const struct tcp_pcb *peer);
err_t tcp_write(struct tcp_pcb *peer,
                const void *data,
                u16_t length,
                uint8_t flags);
err_t tcp_output(struct tcp_pcb *peer);
void pbuf_free(struct pbuf *packet);
#endif
