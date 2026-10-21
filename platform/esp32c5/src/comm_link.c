// SPDX-License-Identifier: MIT
/**
 * @file comm_link.c
 * @brief Demo Link binding for independent USB and Wi-Fi E8/E9/Shell contexts.
 * @details Part of base. Each complete binary frame/text line has 1 parser owner.
 *          Per-Link TX queues copy caller bytes and preserve partial-send tails.
 *          All state is private to the SECTION/main task, with no ISR writers.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "esp32c5_platform.h"
#include "bsp_usart.h"
#include "bsp_wifi.h"
#include "bsp_wifi_socket.h"
#include "comm.h"
#include "shell.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define DEMO_ADDRESS  2u    /**< Both transports access the same base demo. */
#define DEMO_TX_DEPTH 16u   /**< Bounded number of copied frames per Link. */
#define DEMO_PORT     5000u /**< Demo TCP/UDP listening port. */

typedef enum
{
    PARSER_IDLE = 0, /**< Select a parser at the start of a transaction. */
    PARSER_TEXT,     /**< Preserve binary-looking bytes inside text lines. */
    PARSER_E8,       /**< Preserve E8 payload ownership after early rejection. */
    PARSER_E9        /**< Preserve E9 payload ownership after early rejection. */
} PARSER_E;

typedef struct demo_link
{
    PARSER_E parser;       /**< Current private receive owner. */
    uint32_t last_rx_tick; /**< Incomplete-frame timeout. */
    uint32_t wire_index;   /**< Current binary byte offset. */
    uint32_t wire_size;    /**< Complete size from the binary header. */
    uint32_t payload_size; /**< E8 low length byte. */
    uint8_t rejected;      /**< Drain a rejected binary envelope without parsing its payload. */
    uint8_t tx[DEMO_TX_DEPTH][512];  /**< Private copied TX frames. */
    uint32_t tx_size[DEMO_TX_DEPTH]; /**< Valid bytes per queued frame. */
    uint32_t tx_head;    /**< Next queue slot to drain. */
    uint32_t tx_count;   /**< Occupied queue slots. */
    uint32_t tx_offset;  /**< Bytes already accepted from the first frame. */
    uint32_t tx_dropped; /**< Rejected whole frames, available through the debugger. */
} demo_link_t;

static demo_link_t links[2];     /**< USB index 0 and TCP index 1; no cross-Link parser state. */
static int32_t listener = -1;    /**< Active TCP listener, owned by the main task. */
static int32_t client   = -1;    /**< Current TCP session, replaced only after cleanup. */
static int32_t datagram = -1;    /**< UDP raw byte-loopback socket. */
static uint32_t session_ip;      /**< IP identity used to retire listeners after reconnect. */
static uint8_t udp_data[512];    /**< Pending complete UDP echo payload. */
static uint32_t udp_size;        /**< Pending UDP payload bytes. */
static bsp_wifi_peer_t udp_peer; /**< Sender of the pending UDP echo. */

DECLARE_SHELL_CTX(usb_shell);
DECLARE_SHELL_CTX(wifi_shell);
DECLARE_COMM_CTX(usb_comm, 256u, DEMO_ADDRESS, 0u);
DECLARE_COMM_CTX(wifi_comm, 256u, DEMO_ADDRESS, 1u);
DECLARE_COMM_V1_CTX(usb_comm_v1, DEMO_ADDRESS, 0u);
DECLARE_COMM_V1_CTX(wifi_comm_v1, DEMO_ADDRESS, 1u);

/** @param index Link index. @param p_data Bytes copied before return. @param length Signed byte count. */
static void enqueue(uint32_t index, const char *p_data, int length)
{
    demo_link_t *p_link = &links[index]; /* Main-task-only queue owner. */
    uint32_t tail = (p_link->tx_head + p_link->tx_count) % DEMO_TX_DEPTH; /* Next free slot. */

    if (    (p_data == NULL) /* Reject an absent source. */
         || (length <= 0)    /* Do not convert an invalid signed size. */
         || (length > 512)   /* Match one bounded BSP transfer. */
         || (p_link->tx_count == DEMO_TX_DEPTH) /* Reject whole messages rather than corrupting a frame. */
         || (    (index == 1u)
              && (client < 0))) /* Do not retain replies for a future TCP peer. */
    {
        ++p_link->tx_dropped;
        return;
    }
    memcpy(p_link->tx[tail], p_data, (size_t)length);
    p_link->tx_size[tail] = (uint32_t)length;
    ++p_link->tx_count;
}

/** @param index Link index. @param p_format Format string. @param arguments Formatting arguments. */
static void format_output(uint32_t index, const char *p_format, va_list arguments)
{
    char buffer[512] = {0}; /* Bounded stack output, copied to the Link queue. */
    int length       = 0;   /* Required formatted length. */

    if (p_format == NULL)
    {
        return;
    }
    length = vsnprintf(buffer, sizeof(buffer), p_format, arguments);

    if (length >= (int)sizeof(buffer))
    {
        length = (int)sizeof(buffer) - 1;
    }

    if (length > 0)
    {
        enqueue(index, buffer, length);
    }
}

/** @param p_format printf-style Link message. @param ... Formatting arguments. */
static void usb_printf(const char *p_format, ...)
{
    va_list arguments; /* Initialized below before the first use. */
    va_start(arguments, p_format);
    format_output(0u, p_format, arguments);
    va_end(arguments);
}
/** @param p_format printf-style Link message. @param ... Formatting arguments. */
static void wifi_printf(const char *p_format, ...)
{
    va_list arguments; /* Initialized below before the first use. */
    va_start(arguments, p_format);
    format_output(1u, p_format, arguments);
    va_end(arguments);
}
/** @param p_data Binary response bytes. @param length Signed byte count. */
static void usb_tx(char *p_data, int length)
{
    enqueue(0u, p_data, length);
}
/** @param p_data Binary response bytes. @param length Signed byte count. */
static void wifi_tx(char *p_data, int length)
{
    enqueue(1u, p_data, length);
}

static section_link_tx_func_t usb_output = {.my_printf = usb_printf, .tx_by_dma = usb_tx};    /**< Stable USB binding. */
static section_link_tx_func_t wifi_output = {.my_printf = wifi_printf, .tx_by_dma = wifi_tx}; /**< Stable TCP binding. */

/** @param data Byte from this physical stream. @param my_printf Current Link output.
 * @param p_context Private Link state, supplied by its registration.
 */
static void receive(uint8_t data, DEC_MY_PRINTF, void *p_context)
{
    demo_link_t *p_link = p_context; /* Context belongs to exactly 1 registered Link. */
    uint32_t index = p_link == &links[0] ? 0u : 1u; /* Select this stream's parser instances. */
    uint32_t now = SECTION_SYS_TICK; /* Monotonic 100 us units. */

    if (    (p_link->parser != PARSER_IDLE) /* Recover only an unfinished transaction. */
         && ((now - p_link->last_rx_tick) > COMM_V1_FRAME_TIMEOUT_TICK)) /* Match shared parser timeout. */
    {
        p_link->parser = PARSER_IDLE;
    }
    p_link->last_rx_tick = now;

    if (p_link->parser == PARSER_IDLE)
    {
        p_link->wire_index   = 0u;
        p_link->wire_size    = 0u;
        p_link->payload_size = 0u;
        p_link->rejected     = 0u;

        if (data == 0xE8u)
        {
            p_link->parser = PARSER_E8;
        }
        else if (data == COMM_V1_SOP)
        {
            p_link->parser = PARSER_E9;
        }
        else
        {
            p_link->parser = PARSER_TEXT;
        }
    }

    if (p_link->parser == PARSER_TEXT)
    {
        shell_run(data, my_printf, index == 0u ? (void *)&usb_shell : (void *)&wifi_shell);

        if (data == (uint8_t)'\n')
        {
            p_link->parser = PARSER_IDLE;
        }
        return;
    }

    if (p_link->parser == PARSER_E8)
    {
        if (p_link->wire_index == 9u)
        {
            p_link->payload_size = data;
        }

        if (p_link->wire_index == 10u)
        {
            p_link->wire_size = 15u + p_link->payload_size + ((uint32_t)data << 8u);
        }

        if (p_link->rejected == 0u)
        {
            comm_run(data, my_printf, index == 0u ? (void *)&usb_comm : (void *)&wifi_comm);

            if ((index == 0u ? usb_comm.status : wifi_comm.status) == SECTION_PACKFORM_STA_SOP)
            {
                p_link->rejected = 1u;
            }
        }
    }
    else
    {
        if (    (p_link->wire_index == 4u) /* MSG header identifies command-only E9 frames. */
             && ((data & 0x40u) != 0u))    /* Those frames have no LEN byte. */
        {
            p_link->wire_size = COMM_V1_PURE_CMD_LEN;
        }

        if (    (p_link->wire_index == 5u) /* Data frame length is available. */
             && (p_link->wire_size == 0u)) /* Keep command-only frame size intact. */
        {
            p_link->wire_size = 7u + (data == 0u ? COMM_V1_MAX_DATA : (uint32_t)data);
        }

        if (p_link->rejected == 0u)
        {
            comm_v1_run(data, my_printf, index == 0u ? (void *)&usb_comm_v1 : (void *)&wifi_comm_v1);

            if ((index == 0u ? usb_comm_v1.status : wifi_comm_v1.status) == COMM_V1_STA_IDLE)
            {
                p_link->rejected = 1u;
            }
        }
    }
    ++p_link->wire_index;

    if (    (p_link->wire_size != 0u) /* Header has declared the complete envelope. */
         && (p_link->wire_index >= p_link->wire_size)) /* Drain even rejected binary payloads. */
    {
        p_link->parser = PARSER_IDLE;
    }
}

/** @param p_data TCP receive destination. @return 1 if 1 byte was read, else 0. */
static uint8_t wifi_rx(uint8_t *p_data)
{
    if (    (client < 0) /* No live session. */
         || (links[1].tx_count == DEMO_TX_DEPTH)) /* Pause requests while responses are backpressured. */
    {
        return 0u;
    }
    return bsp_wifi_tcp_read(client, p_data, 1u) == 1 ? 1u : 0u;
}

/** @brief Retire TCP descriptors and every piece of old-session parser/TX state. */
static void close_tcp(void)
{
    if (client >= 0)
    {
        (void)bsp_wifi_socket_close(client);
        client = -1;
    }
    links[1]            = (demo_link_t){0};
    wifi_comm.status    = SECTION_PACKFORM_STA_SOP;
    wifi_comm_v1.status = COMM_V1_STA_IDLE;
    wifi_shell          = (shell_ctx_t){0};
}

/** @param index Link whose pending response should advance. */
static void drain_tx(uint32_t index)
{
    demo_link_t *p_link = &links[index]; /* Main-task-only TX state. */
    uint32_t remaining  = 0u;            /* Bytes not yet accepted by BSP. */
    int32_t sent        = 0;             /* Partial acceptance or transport error. */

    if (p_link->tx_count == 0u)
    {
        return;
    }
    remaining = p_link->tx_size[p_link->tx_head] - p_link->tx_offset;

    if (index == 0u)
    {
        sent = bsp_usart_write(&p_link->tx[p_link->tx_head][p_link->tx_offset], remaining);
    }
    else
    {
        sent = bsp_wifi_tcp_write(client, &p_link->tx[p_link->tx_head][p_link->tx_offset], remaining);

        if (    (sent < 0) /* A nonblocking wait retains the same response bytes. */
             && (errno != EAGAIN)
             && (errno != EWOULDBLOCK)) /* Real failures retire the session. */
        {
            close_tcp();
            return;
        }
    }

    if (sent > 0)
    {
        p_link->tx_offset += (uint32_t)sent;

        if (p_link->tx_offset == p_link->tx_size[p_link->tx_head])
        {
            p_link->tx_offset = 0u;
            p_link->tx_head = (p_link->tx_head + 1u) % DEMO_TX_DEPTH;
            --p_link->tx_count;
        }
    }
}

/** @brief Refresh radio state independently of incoming protocol requests. */
static void poll_radio(void)
{
    (void)bsp_wifi_poll();
}

void esp32c5_link_poll(void)
{
    bsp_wifi_status_t status = {0}; /* Last radio poll snapshot. */
    int32_t received         = -1;  /* Complete UDP payload size or socket result. */
    drain_tx(0u);
    bsp_wifi_status_get(&status);

    if (    (status.ready == 0u)            /* Retire transports while disconnected. */
         || (session_ip != status.ipv4_be)) /* A new address means a new socket lifetime. */
    {
        close_tcp();

        if (listener >= 0)
        {
            (void)bsp_wifi_socket_close(listener);
            listener = -1;
        }

        if (datagram >= 0)
        {
            (void)bsp_wifi_socket_close(datagram);
            datagram = -1;
        }
        udp_size   = 0u;
        session_ip = status.ipv4_be;
    }

    if (status.ready == 0u)
    {
        return;
    }

    if (listener < 0)
    {
        listener = bsp_wifi_tcp_listen(DEMO_PORT);
    }

    if (datagram < 0)
    {
        datagram = bsp_wifi_udp_bind(DEMO_PORT);
    }

    if (    (client < 0)
         && (listener >= 0)) /* Accept only when no existing client owns the Link. */
    {
        client = bsp_wifi_tcp_accept(listener);
    }

    if (client >= 0)
    {
        /* The BSP parser byte source returns no byte for EOF. Probe once here so
         * a quiet disconnected client cannot retain the only Link indefinitely. */
        received = bsp_wifi_tcp_probe(client);

        if (    (received == 0) /* Orderly close. */
             || (    (received < 0)
                  && (errno != EAGAIN)
                  && (errno != EWOULDBLOCK))) /* Reset/error. */
        {
            close_tcp();
        }
        else
        {
            drain_tx(1u);
        }
    }

    if (datagram >= 0)
    {
        if (udp_size == 0u)
        {
            received = bsp_wifi_udp_read(datagram, udp_data, sizeof(udp_data), &udp_peer);

            if (received > 0)
            {
                udp_size = (uint32_t)received;
            }
        }

        if (udp_size != 0u)
        {
            received = bsp_wifi_udp_write(datagram, udp_data, udp_size, &udp_peer);

            if (    (received >= 0) /* Whole UDP datagram was accepted. */
                 || (    (errno != EAGAIN)
                      && (errno != EWOULDBLOCK))) /* Retire only permanent errors. */
            {
                udp_size = 0u;
            }
        }
    }
}

static const section_link_handler_item_t usb_handlers[] = {{.func = receive, .ctx = &links[0]}};  /**< USB owner. */
static const section_link_handler_item_t wifi_handlers[] = {{.func = receive, .ctx = &links[1]}}; /**< TCP owner. */
REG_LINK(0u, usb_output, bsp_usart_dbg_rx_get_byte, usb_handlers, 1u)
REG_LINK(1u, wifi_output, wifi_rx, wifi_handlers, 1u)
REG_TASK_MS(100u, poll_radio)
