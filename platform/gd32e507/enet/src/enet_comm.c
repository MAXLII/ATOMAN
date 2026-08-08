// SPDX-License-Identifier: MIT
/**
 * @file    enet_comm.c
 * @brief   GD32E507 Ethernet communication service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Start the standalone LwIP IPv4 stack on the GD32E507 ENET BSP
 *          - Poll received Ethernet frames and service protocol timers
 *          - Carry the existing FRAME binary protocol over a TCP byte stream
 *          - Provide a UDP echo endpoint for independent network-path testing
 *
 *          Design notes:
 *          - C11 compatible
 *          - LwIP uses its fixed-size internal heap
 *          - All raw-API callbacks run from the 1 ms background service task
 *          - Hardware access is abstracted through the GD32E507 ENET BSP
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_enet.h"
#include "bsp_usart.h"
#include "comm.h"
#include "comm_addr.h"
#include "comm_link.h"
#include "enet_config.h"
#include "section.h"
#include "systick.h"
#include "ethernetif.h"

#include "lwip/init.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "netif/ethernet.h"

#include <stdint.h>

#define ENET_COMM_RX_BUDGET 8u          /* Maximum frames processed by one 1 ms task invocation. */
#define ENET_COMM_LINK_PERIOD_MS 250u   /* PHY link polling period. */
#define ENET_COMM_RETRY_PERIOD_MS 1000u /* Hardware initialization retry period. */
#define ENET_COMM_TCP_RX_RING_SIZE 2048u /* FRAME bytes waiting for the Section communication task. */
#define ENET_COMM_TCP_TX_RING_SIZE 4096u /* FRAME bytes waiting for the LwIP TCP service task. */
#define ENET_COMM_FRAME_PAYLOAD_SIZE 512u /* Maximum incoming FRAME payload accepted on Ethernet. */

static struct netif network_interface;       /* LwIP interface bound to the GD32 ENET MAC. */
static struct tcp_pcb *p_tcp_listener = NULL; /* TCP FRAME listener owned by this service. */
static struct tcp_pcb *p_tcp_connection = NULL; /* Active FRAME client; only one client is served. */
static struct udp_pcb *p_udp_endpoint = NULL; /* UDP echo endpoint owned by this service. */
static uint8_t tcp_rx_ring[ENET_COMM_TCP_RX_RING_SIZE]; /* Single-producer/single-consumer receive ring. */
static volatile uint16_t tcp_rx_head = 0u; /* Next receive-ring position written by LwIP. */
static volatile uint16_t tcp_rx_tail = 0u; /* Next receive-ring position consumed by Section. */
static uint8_t tcp_tx_ring[ENET_COMM_TCP_TX_RING_SIZE]; /* Serialized protocol response queue. */
static volatile uint16_t tcp_tx_head = 0u; /* Next transmit-ring position reserved by producers. */
static volatile uint16_t tcp_tx_tail = 0u; /* Next transmit-ring position submitted to LwIP. */
static uint8_t network_ready = 0u;            /* Network stack and endpoints are ready when set. */
static uint8_t link_state = 0u;               /* Last reported PHY link state. */
static uint32_t last_link_check_ms = 0u;       /* Millisecond time of the latest PHY poll. */
static uint32_t last_retry_ms = 0u;            /* Millisecond time of the latest initialization attempt. */

DECLARE_COMM_CTX(ethernet_comm_ctx, ENET_COMM_FRAME_PAYLOAD_SIZE, HOST_ADDR, ETHERNET_LINK);

volatile uint32_t g_enet_rx_frame_count = 0u;        /* Ethernet frames accepted by LwIP. */
volatile uint32_t g_enet_rx_error_count = 0u;        /* Ethernet frames rejected by the adapter or LwIP. */
volatile uint32_t g_enet_tcp_connection_count = 0u;  /* Accepted TCP FRAME connections. */
volatile uint32_t g_enet_tcp_rx_byte_count = 0u;     /* FRAME bytes received by the TCP service. */
volatile uint32_t g_enet_tcp_rx_drop_count = 0u;     /* TCP connections aborted after receive overflow. */
volatile uint32_t g_enet_tcp_tx_byte_count = 0u;     /* FRAME bytes accepted by the TCP send queue. */
volatile uint32_t g_enet_tcp_tx_drop_count = 0u;     /* Complete FRAME responses dropped before enqueue. */
volatile uint32_t g_enet_tcp_tx_error_count = 0u;    /* TCP write or flush failures. */
volatile uint32_t g_enet_udp_datagram_count = 0u;    /* Datagrams received by the UDP echo service. */
volatile uint32_t g_enet_udp_rx_byte_count = 0u;     /* Bytes received by the UDP echo service. */
volatile uint32_t g_enet_udp_tx_error_count = 0u;    /* UDP echo send failures. */

static void service_init(void);
static void service_task(void);
static uint8_t stack_start(void);
static void link_update(uint32_t now_ms);
static void tcp_connection_error_callback(void *p_argument, err_t error);
static err_t tcp_accept_callback(void *p_argument, struct tcp_pcb *p_connection, err_t error);
static err_t tcp_receive_callback(void *p_argument, struct tcp_pcb *p_connection, struct pbuf *p_packet, err_t error);
static void tcp_transmit_service(void);
static void udp_receive_callback(void *p_argument,
                                 struct udp_pcb *p_endpoint,
                                 struct pbuf *p_packet,
                                 const ip_addr_t *p_remote_address,
                                 u16_t remote_port);

u32_t sys_now(void)
{
    return (u32_t)(systick_gettime_100us() / 10u);
}

static uint32_t enet_comm_irq_lock(void)
{
    const uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

static void enet_comm_irq_unlock(uint32_t primask)
{
    if ((primask & 1u) == 0u)
    {
        __enable_irq();
    }
}

static uint16_t ring_free(uint16_t head, uint16_t tail, uint16_t size)
{
    if (head >= tail)
    {
        return (uint16_t)((uint32_t)size - (uint32_t)(head - tail) - 1u);
    }

    return (uint16_t)((uint32_t)tail - (uint32_t)head - 1u);
}

static void tcp_rings_reset(void)
{
    uint32_t primask = enet_comm_irq_lock();

    tcp_rx_head = 0u;
    tcp_rx_tail = 0u;
    tcp_tx_head = 0u;
    tcp_tx_tail = 0u;
    ethernet_comm_ctx.status = SECTION_PACKFORM_STA_SOP;
    ethernet_comm_ctx.index = 0u;
    ethernet_comm_ctx.len = 0u;
    ethernet_comm_ctx.crc = 0u;
    ethernet_comm_ctx.func = NULL;
    ethernet_comm_ctx.src_flag = 0u;
    ethernet_comm_ctx.dst_flag = 0u;
    ethernet_comm_ctx.cmd_flag = 0u;
    ethernet_comm_ctx.len_flag = 0u;
    ethernet_comm_ctx.eop_flag = 0u;
    ethernet_comm_ctx.is_route = 0u;
    enet_comm_irq_unlock(primask);
}

static uint8_t ethernet_rx_get_byte(uint8_t *p_data)
{
    uint16_t tail = tcp_rx_tail;

    if ((p_data == NULL) || (tail == tcp_rx_head))
    {
        return 0u;
    }

    *p_data = tcp_rx_ring[tail];
    tail = (uint16_t)(((uint32_t)tail + 1u) % ENET_COMM_TCP_RX_RING_SIZE);
    __DMB();
    tcp_rx_tail = tail;
    return 1u;
}

static void ethernet_tx_by_dma(char *p_data, int length)
{
    uint32_t primask = 0u;
    uint16_t head = 0u;
    uint16_t index = 0u;
    uint16_t transfer_length = 0u;

    if ((p_data == NULL) || (length <= 0) || ((uint32_t)length >= ENET_COMM_TCP_TX_RING_SIZE))
    {
        return;
    }

    transfer_length = (uint16_t)length;
    primask = enet_comm_irq_lock();
    head = tcp_tx_head;
    if ((p_tcp_connection == NULL) ||
        (ring_free(head, tcp_tx_tail, ENET_COMM_TCP_TX_RING_SIZE) < transfer_length))
    {
        g_enet_tcp_tx_drop_count++;
        enet_comm_irq_unlock(primask);
        return;
    }

    for (index = 0u; index < transfer_length; index++)
    {
        tcp_tx_ring[head] = (uint8_t)p_data[index];
        head = (uint16_t)(((uint32_t)head + 1u) % ENET_COMM_TCP_TX_RING_SIZE);
    }
    __DMB();
    tcp_tx_head = head;
    g_enet_tcp_tx_byte_count += transfer_length;
    enet_comm_irq_unlock(primask);
}

static section_link_tx_func_t ethernet_tx_func = {
    .my_printf = NULL,
    .tx_by_dma = ethernet_tx_by_dma,
};

static const section_link_handler_item_t ethernet_handler_arr[] = {
    {.func = comm_run, .ctx = (void *)&ethernet_comm_ctx},
};

REG_LINK(ETHERNET_LINK,
         ethernet_tx_func,
         ethernet_rx_get_byte,
         ethernet_handler_arr,
         sizeof(ethernet_handler_arr) / sizeof(ethernet_handler_arr[0]))

static void tcp_connection_error_callback(void *p_argument, err_t error)
{
    LWIP_UNUSED_ARG(error);

    if (p_tcp_connection == (struct tcp_pcb *)p_argument)
    {
        p_tcp_connection = NULL;
        tcp_rings_reset();
    }
}

static err_t tcp_receive_callback(void *p_argument, struct tcp_pcb *p_connection, struct pbuf *p_packet, err_t error)
{
    struct pbuf *p_segment = NULL; /* Current pbuf segment being copied into the protocol receive queue. */
    uint16_t head = tcp_rx_head;   /* Private producer position published after the complete packet copy. */
    uint16_t index = 0u;           /* Current byte within one pbuf segment. */
    err_t close_status = ERR_OK;   /* Graceful close result after a peer shutdown. */

    LWIP_UNUSED_ARG(p_argument);

    if (p_packet == NULL)
    {
        if (p_tcp_connection == p_connection)
        {
            p_tcp_connection = NULL;
            tcp_rings_reset();
        }
        tcp_arg(p_connection, NULL);
        tcp_recv(p_connection, NULL);
        tcp_err(p_connection, NULL);
        close_status = tcp_close(p_connection);
        if (close_status != ERR_OK)
        {
            tcp_abort(p_connection);
            return ERR_ABRT;
        }
        return ERR_OK;
    }

    if (error != ERR_OK)
    {
        pbuf_free(p_packet);
        if (p_tcp_connection == p_connection)
        {
            p_tcp_connection = NULL;
            tcp_rings_reset();
        }
        tcp_abort(p_connection);
        return ERR_ABRT;
    }

    if (ring_free(head, tcp_rx_tail, ENET_COMM_TCP_RX_RING_SIZE) < p_packet->tot_len)
    {
        g_enet_tcp_rx_drop_count++;
        pbuf_free(p_packet);
        if (p_tcp_connection == p_connection)
        {
            p_tcp_connection = NULL;
            tcp_rings_reset();
        }
        tcp_abort(p_connection);
        return ERR_ABRT;
    }

    for (p_segment = p_packet; p_segment != NULL; p_segment = p_segment->next)
    {
        const uint8_t *p_bytes = (const uint8_t *)p_segment->payload;

        for (index = 0u; index < p_segment->len; index++)
        {
            tcp_rx_ring[head] = p_bytes[index];
            head = (uint16_t)(((uint32_t)head + 1u) % ENET_COMM_TCP_RX_RING_SIZE);
        }
    }

    __DMB();
    tcp_rx_head = head;
    tcp_recved(p_connection, p_packet->tot_len);
    g_enet_tcp_rx_byte_count += (uint32_t)p_packet->tot_len;
    pbuf_free(p_packet);
    return ERR_OK;
}

static err_t tcp_accept_callback(void *p_argument, struct tcp_pcb *p_connection, err_t error)
{
    LWIP_UNUSED_ARG(p_argument);

    if (error != ERR_OK)
    {
        return error;
    }

    if (p_tcp_connection != NULL)
    {
        tcp_abort(p_tcp_connection);
    }

    g_enet_tcp_connection_count++;
    p_tcp_connection = p_connection;
    tcp_rings_reset();
    tcp_arg(p_connection, p_connection);
    tcp_recv(p_connection, tcp_receive_callback);
    tcp_err(p_connection, tcp_connection_error_callback);
    tcp_nagle_disable(p_connection);
    return ERR_OK;
}

static void tcp_transmit_service(void)
{
    uint16_t tail = tcp_tx_tail;
    uint16_t queued_length = 0u;
    uint16_t send_length = 0u;
    u16_t send_capacity = 0u;
    err_t write_status = ERR_OK;

    if ((p_tcp_connection == NULL) || (tail == tcp_tx_head))
    {
        return;
    }

    if (tcp_tx_head > tail)
    {
        queued_length = (uint16_t)(tcp_tx_head - tail);
    }
    else
    {
        queued_length = (uint16_t)(ENET_COMM_TCP_TX_RING_SIZE - tail);
    }

    send_capacity = tcp_sndbuf(p_tcp_connection);
    send_length = (queued_length < send_capacity) ? queued_length : send_capacity;
    if (send_length == 0u)
    {
        return;
    }

    write_status = tcp_write(p_tcp_connection,
                             &tcp_tx_ring[tail],
                             send_length,
                             TCP_WRITE_FLAG_COPY);
    if (write_status == ERR_MEM)
    {
        return;
    }
    if (write_status != ERR_OK)
    {
        g_enet_tcp_tx_error_count++;
        tcp_abort(p_tcp_connection);
        p_tcp_connection = NULL;
        tcp_rings_reset();
        return;
    }

    tail = (uint16_t)(((uint32_t)tail + send_length) % ENET_COMM_TCP_TX_RING_SIZE);
    __DMB();
    tcp_tx_tail = tail;
    write_status = tcp_output(p_tcp_connection);
    if ((write_status != ERR_OK) && (write_status != ERR_MEM))
    {
        g_enet_tcp_tx_error_count++;
    }
}

static void udp_receive_callback(void *p_argument,
                                 struct udp_pcb *p_endpoint,
                                 struct pbuf *p_packet,
                                 const ip_addr_t *p_remote_address,
                                 u16_t remote_port)
{
    err_t send_status = ERR_OK; /* UDP echo send result. */

    LWIP_UNUSED_ARG(p_argument);

    if ((p_packet == NULL) || /* LwIP did not provide a received datagram. */
        (p_remote_address == NULL)) /* The source address is unavailable. */
    {
        if (p_packet != NULL)
        {
            pbuf_free(p_packet);
        }
        return;
    }

    g_enet_udp_datagram_count++;
    g_enet_udp_rx_byte_count += (uint32_t)p_packet->tot_len;
    send_status = udp_sendto(p_endpoint, p_packet, p_remote_address, remote_port);
    if (send_status != ERR_OK)
    {
        g_enet_udp_tx_error_count++;
    }
    pbuf_free(p_packet);
}

static uint8_t stack_start(void)
{
    ip4_addr_t ip_address = {0};      /* Static local IPv4 address. */
    ip4_addr_t network_mask = {0};    /* Static IPv4 subnet mask. */
    ip4_addr_t gateway_address = {0}; /* Static IPv4 gateway; zero for a direct link. */
    err_t bind_status = ERR_OK;       /* Endpoint bind result. */

    if (bsp_enet_init() == 0u)
    {
        bsp_usart_dbg_printf("enet: PHY/MAC init failed, retrying\r\n");
        return 0u;
    }

    lwip_init();
    IP4_ADDR(&ip_address,
             ENET_CONFIG_IP_ADDR0,
             ENET_CONFIG_IP_ADDR1,
             ENET_CONFIG_IP_ADDR2,
             ENET_CONFIG_IP_ADDR3);
    IP4_ADDR(&network_mask,
             ENET_CONFIG_NETMASK_ADDR0,
             ENET_CONFIG_NETMASK_ADDR1,
             ENET_CONFIG_NETMASK_ADDR2,
             ENET_CONFIG_NETMASK_ADDR3);
    IP4_ADDR(&gateway_address,
             ENET_CONFIG_GATEWAY_ADDR0,
             ENET_CONFIG_GATEWAY_ADDR1,
             ENET_CONFIG_GATEWAY_ADDR2,
             ENET_CONFIG_GATEWAY_ADDR3);

    if (netif_add(&network_interface,
                  &ip_address,
                  &network_mask,
                  &gateway_address,
                  NULL,
                  ethernetif_init,
                  ethernet_input) == NULL)
    {
        bsp_usart_dbg_printf("enet: netif add failed\r\n");
        return 0u;
    }

    netif_set_default(&network_interface);
    netif_set_up(&network_interface);
    link_state = bsp_enet_link_is_up();
    if (link_state == 1u)
    {
        netif_set_link_up(&network_interface);
    }
    else
    {
        netif_set_link_down(&network_interface);
    }

    p_tcp_listener = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (p_tcp_listener == NULL)
    {
        bsp_usart_dbg_printf("enet: TCP allocation failed\r\n");
        return 0u;
    }

    bind_status = tcp_bind(p_tcp_listener, IP_ANY_TYPE, ENET_CONFIG_TCP_FRAME_PORT);
    if (bind_status != ERR_OK)
    {
        tcp_close(p_tcp_listener);
        p_tcp_listener = NULL;
        bsp_usart_dbg_printf("enet: TCP bind failed=%d\r\n", (int)bind_status);
        return 0u;
    }

    p_tcp_listener = tcp_listen(p_tcp_listener);
    if (p_tcp_listener == NULL)
    {
        bsp_usart_dbg_printf("enet: TCP listen failed\r\n");
        return 0u;
    }
    tcp_accept(p_tcp_listener, tcp_accept_callback);

    p_udp_endpoint = udp_new_ip_type(IPADDR_TYPE_V4);
    if (p_udp_endpoint == NULL)
    {
        bsp_usart_dbg_printf("enet: UDP allocation failed\r\n");
        return 0u;
    }

    bind_status = udp_bind(p_udp_endpoint, IP_ANY_TYPE, ENET_CONFIG_UDP_ECHO_PORT);
    if (bind_status != ERR_OK)
    {
        udp_remove(p_udp_endpoint);
        p_udp_endpoint = NULL;
        bsp_usart_dbg_printf("enet: UDP bind failed=%d\r\n", (int)bind_status);
        return 0u;
    }
    udp_recv(p_udp_endpoint, udp_receive_callback, NULL);

    etharp_gratuitous(&network_interface); /* Announce the static address and verify the RMII transmit path. */

    bsp_usart_dbg_printf("enet: ready IP 192.168.1.101, TCP FRAME/UDP echo port 5000\r\n");
    bsp_usart_dbg_printf("enet: link %s\r\n", (link_state == 1u) ? "up" : "down");
    return 1u;
}

static void link_update(uint32_t now_ms)
{
    uint8_t current_link_state = 0u; /* Current PHY link state. */

    if ((uint32_t)(now_ms - last_link_check_ms) < ENET_COMM_LINK_PERIOD_MS)
    {
        return;
    }
    last_link_check_ms = now_ms;
    current_link_state = bsp_enet_link_is_up();
    if (current_link_state == link_state)
    {
        return;
    }

    link_state = current_link_state;
    if (link_state == 1u)
    {
        netif_set_link_up(&network_interface);
        etharp_gratuitous(&network_interface); /* Refresh the peer ARP cache after link recovery. */
        bsp_usart_dbg_printf("enet: link up\r\n");
    }
    else
    {
        netif_set_link_down(&network_interface);
        bsp_usart_dbg_printf("enet: link down\r\n");
    }
}

static void service_init(void)
{
    last_retry_ms = sys_now();
    network_ready = stack_start();
}

static void service_task(void)
{
    uint32_t now_ms = sys_now(); /* Current monotonic time in milliseconds. */
    uint32_t frame_count = 0u;   /* Frames handled during this task invocation. */

    if (network_ready == 0u)
    {
        if ((uint32_t)(now_ms - last_retry_ms) >= ENET_COMM_RETRY_PERIOD_MS)
        {
            last_retry_ms = now_ms;
            network_ready = stack_start();
        }
        return;
    }

    while ((frame_count < ENET_COMM_RX_BUDGET) && /* Bound the service time for cooperative scheduling. */
           (bsp_enet_rx_frame_pending() == 1u))   /* The DMA owns at least one complete received frame. */
    {
        if (ethernetif_input(&network_interface) == ERR_OK)
        {
            g_enet_rx_frame_count++;
        }
        else
        {
            g_enet_rx_error_count++;
        }
        frame_count++;
    }

    sys_check_timeouts();
    tcp_transmit_service();
    link_update(now_ms);
}

REG_INIT(20, service_init)
REG_TASK_MS(1, service_task)
