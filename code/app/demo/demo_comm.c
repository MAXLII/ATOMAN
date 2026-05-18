// SPDX-License-Identifier: MIT
/**
 * @file    demo_comm.c
 * @brief   comm section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register protocol handlers with SECTION_COMM
 *          - Parse payload length by compatible copy size
 *          - Send a direct ACK for the same cmd_set/cmd_word request
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-18
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "comm.h"
#include "demo.h"

#include <string.h>

static demo_comm_frame_t s_demo_comm_last_frame;

static void demo_comm_loopback(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_packform_t ack = {0};
    uint16_t copy_len;

    if ((p_pack == NULL) || (p_pack->p_data == NULL))
    {
        return;
    }

    memset(&s_demo_comm_last_frame, 0, sizeof(s_demo_comm_last_frame));
    copy_len = (p_pack->len < sizeof(s_demo_comm_last_frame)) ? p_pack->len : sizeof(s_demo_comm_last_frame);
    memcpy(&s_demo_comm_last_frame, p_pack->p_data, copy_len);

    ack.src = p_pack->dst;
    ack.d_src = p_pack->d_dst;
    ack.dst = p_pack->src;
    ack.d_dst = p_pack->d_src;
    ack.cmd_set = p_pack->cmd_set;
    ack.cmd_word = p_pack->cmd_word;
    ack.is_ack = 1u;
    ack.len = sizeof(s_demo_comm_last_frame);
    ack.p_data = (uint8_t *)&s_demo_comm_last_frame;
    comm_send_data(&ack, my_printf);
}

REG_COMM(DEMO_CMD_SET_LOOPBACK, DEMO_CMD_WORD_LOOPBACK, demo_comm_loopback)
