// SPDX-License-Identifier: MIT
/**
 * @file test.c
 * @brief Real Ethernet port and LwIP regression under exhausted DMA descriptors.
 * @details Hardware operations alone are faked. Busy output must return without
 *          copying or claiming a descriptor; real TCP must retain and retry data.
 *          This host test is separate from physical board acceptance.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "gd32e50x_enet.h"
#include "ethernetif.h"
#include "lwip/init.h"
#include "lwip/tcp.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enet_descriptors_struct rxdesc_tab[ENET_RXBUF_NUM], txdesc_tab[ENET_TXBUF_NUM];
uint8_t rx_buff[ENET_RXBUF_NUM][ENET_RXBUF_SIZE], tx_buff[ENET_TXBUF_NUM][ENET_TXBUF_SIZE];
enet_descriptors_struct *dma_current_txdesc = &txdesc_tab[0];
enet_descriptors_struct *dma_current_rxdesc = &rxdesc_tab[0];
static uint32_t submitted, last_length;

u32_t sys_now(void);
u32_t sys_now(void)
{
    return 0u;
}

void test_enet_transmit(uint32_t length)
{
    assert((dma_current_txdesc->status & ENET_TDES0_DAV) == 0u);
    dma_current_txdesc->status |= ENET_TDES0_DAV;
    last_length = length;
    ++submitted;
}

static err_t direct_output(struct netif *p_netif, struct pbuf *p_packet, const ip4_addr_t *p_address)
{
    (void)p_address;
    return p_netif->linkoutput(p_netif, p_packet);
}

int main(void)
{
    struct netif network = {0};
    ip4_addr_t address = {0}, mask = {0}, gateway = {0};
    IP4_ADDR(&address, 169, 254, 2, 7);
    IP4_ADDR(&mask, 255, 255, 0, 0);
    lwip_init();
    assert(netif_add(&network, &address, &mask, &gateway, NULL, ethernetif_init, NULL) != NULL);
    network.output = direct_output;
    netif_set_default(&network);
    netif_set_up(&network);
    netif_set_link_up(&network);
    txdesc_tab[0].buffer = (uintptr_t)tx_buff[0];
    memset(tx_buff[0], 0xA5, sizeof(tx_buff[0]));

    uint8_t bytes[] = {1u, 2u, 3u, 4u, 5u};
    struct pbuf second = {.payload = &bytes[3], .tot_len = 2u, .len = 2u};
    struct pbuf first = {.next = &second, .payload = bytes, .tot_len = 5u, .len = 3u};
    txdesc_tab[0].status = ENET_TDES0_DAV;

    for (uint32_t i = 0u; i < 10000u; ++i)
        assert(network.linkoutput(&network, &first) == ERR_MEM);
    assert(    submitted == 0u
            && tx_buff[0][0] == 0xA5u);
    txdesc_tab[0].status = 0u;
    first.tot_len        = ENET_TXBUF_SIZE + 1u;
    assert(network.linkoutput(&network, &first) == ERR_BUF);
    assert(    submitted == 0u
            && tx_buff[0][0] == 0xA5u);
    first.tot_len = 5u;
    assert(network.linkoutput(&network, &first) == ERR_OK);
    assert(    last_length == 5u
            && memcmp(tx_buff[0], bytes, 5u) == 0);

    struct tcp_pcb *p_peer = tcp_new();
    assert(p_peer != NULL);
    p_peer->state = ESTABLISHED;
    ip_addr_copy_from_ip4(p_peer->local_ip, address);
    IP_ADDR4(&p_peer->remote_ip, 169, 254, 2, 8);
    p_peer->local_port  = 5000u;
    p_peer->remote_port = 50000u;
    p_peer->snd_wnd     = TCP_SND_BUF;
    p_peer->snd_wnd_max = TCP_SND_BUF;
    p_peer->cwnd        = TCP_SND_BUF;
    tcp_nagle_disable(p_peer);
    uint8_t payload[64] = {0};

    for (uint32_t i = 0u; i < sizeof(payload); ++i)
        payload[i] = (uint8_t)i;
    assert(tcp_write(p_peer, payload, sizeof(payload), 0u) == ERR_OK);

    for (uint32_t i = 0u; i < 10000u; ++i)
        assert(tcp_output(p_peer) == ERR_MEM);
    assert(    p_peer->unsent != NULL
            && p_peer->unacked == NULL
            && submitted == 1u);
    txdesc_tab[0].status = 0u;
    assert(tcp_output(p_peer) == ERR_OK);
    assert(    p_peer->unsent == NULL
            && p_peer->unacked != NULL
            && submitted == 2u);
    assert(last_length >= sizeof(payload));
    assert(memcmp(&tx_buff[0][last_length - sizeof(payload)], payload, sizeof(payload)) == 0);
    assert(    tcp_output(p_peer) == ERR_OK
            && submitted == 2u);
    puts(
        "PASS real Ethernet/LwIP: busy DMA returns immediately, no overwrite, chained copy, size guard, TCP retry without new data");
    return 0;
}
