// SPDX-License-Identifier: MIT
/**
 * @file demo_comm.c
 * @brief Demo E8/E9/Shell ownership and binary echo command.
 * @details Entity: per-stream protocol state. Prior: serialized Link callbacks.
 *          Time: bounded byte dispatch; session events clear previous protocol state.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "demo_comm.h"
#include "comm_link.h"
#include "comm.h"
#include "shell.h"
#include <stddef.h>
#define DEMO_ADDRESS 2u /**< Local address shared by USB and TCP. */

typedef enum
{
    PARSER_IDLE = 0, /**< Select a parser at the start of a transaction. */
    PARSER_TEXT,     /**< Preserve binary-looking bytes inside text lines. */
    PARSER_E8,       /**< Preserve E8 payload ownership after early rejection. */
    PARSER_E9        /**< Preserve E9 payload ownership after early rejection. */
} PARSER_E;

typedef struct
{
    PARSER_E parser;       /**< Current frame or text-line owner. */
    uint32_t last_rx_tick; /**< Partial-frame timeout in SECTION ticks. */
    uint32_t wire_index;   /**< Offset in the current envelope. */
    uint32_t wire_size;    /**< Complete envelope length. */
    uint32_t payload_size; /**< E8 low length byte. */
    uint8_t rejected;      /**< Drain rejected binary envelopes without Shell dispatch. */
} demo_link_t;
static demo_link_t links[2]; /**< Main-task-owned USB and TCP parser state. */

DECLARE_SHELL_CTX(usb_shell);
DECLARE_SHELL_CTX(wifi_shell);
DECLARE_COMM_CTX(usb_comm, 256u, DEMO_ADDRESS, USART_DBG_LINK);
DECLARE_COMM_CTX(wifi_comm, 256u, DEMO_ADDRESS, WIFI_DBG_LINK);
DECLARE_COMM_V1_CTX(usb_comm_v1, DEMO_ADDRESS, USART_DBG_LINK);
DECLARE_COMM_V1_CTX(wifi_comm_v1, DEMO_ADDRESS, WIFI_DBG_LINK);

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

void demo_comm_usb_receive(uint8_t data, DEC_MY_PRINTF, void *p_context)
{
    (void)p_context;
    receive(data, my_printf, &links[0]);
}
void demo_comm_wifi_receive(uint8_t data, DEC_MY_PRINTF, void *p_context)
{
    (void)p_context;
    receive(data, my_printf, &links[1]);
}
void demo_comm_wifi_session_reset(void)
{
    links[1]         = (demo_link_t){0};
    wifi_comm.status = SECTION_PACKFORM_STA_SOP;
    comm_v1_reset_session(&wifi_comm_v1);
    wifi_shell = (shell_ctx_t){0};
}
/** @param p_context Validated packet from the generic COMM registration. @param my_printf Requesting Link output. */
static void loopback(void *p_context, DEC_MY_PRINTF)
{
    section_packform_t *p_pack = p_context; /* Recover the generic framework payload at its boundary. */
    section_packform_t reply   = {0};       /* Direct same-command acknowledgment. */

    if (p_pack == NULL)
    {
        return;
    }
    reply        = *p_pack;
    reply.src    = p_pack->dst;
    reply.d_src  = p_pack->d_dst;
    reply.dst    = p_pack->src;
    reply.d_dst  = p_pack->d_src;
    reply.is_ack = 1u;
    comm_send_data(&reply, my_printf);
}

REG_COMM(0x01, 0x17, loopback)
