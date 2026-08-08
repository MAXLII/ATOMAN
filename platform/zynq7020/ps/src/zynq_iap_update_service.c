// SPDX-License-Identifier: MIT
/**
 * @file    zynq_iap_update_service.c
 * @brief   Independent Zynq IAP upgrade-trigger implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register and directly acknowledge only FRAME command 0x08
 *          - Invoke the mounted preparation callback and publish the boot request
 *          - Drain the active response before transferring to the Bootloader image
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The boot request occupies a fixed on-chip-memory address
 *          - Cache and interrupt shutdown occurs immediately before transfer
 *
 * @author  Max.Li
 * @date    2026-07-28
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "zynq_iap_update_service.h"

#include "bsp_ethernet.h"
#include "bsp_timer.h"
#include "comm.h"
#include "section.h"
#include "xil_cache.h"
#include "xil_exception.h"

#include <stddef.h>
#include <stdint.h>

#define ZYNQ_IAP_CMD_SET             0x01
#define ZYNQ_IAP_CMD_INFO            0x08
#define ZYNQ_IAP_INFO_LENGTH         10u
#define ZYNQ_IAP_ACK_LENGTH          3u
#define ZYNQ_IAP_ACK_ACCEPTED        1u
#define ZYNQ_IAP_ACK_REJECTED        2u
#define ZYNQ_BOOT_REASON_ADDRESS     0x0002FFF0u
#define ZYNQ_BOOT_REASON_MAGIC       0x42544C44u
#define ZYNQ_BOOT_REASON_IAP_REQUEST 1u
#define ZYNQ_BOOTLOADER_ENTRY        0x04000000u
#define ZYNQ_IAP_HANDOFF_DELAY_TICKS 1000u

typedef struct
{
    uint32_t magic;
    uint32_t reason;
    uint32_t inverted_magic;
} zynq_iap_boot_reason_record_t;

static zynq_iap_prepare_t p_prepare_callback;
static void *p_prepare_context;
static uint8_t transfer_pending;
static uint8_t transfer_called;
static uint32_t transfer_request_tick;

static uint32_t read_u32_le(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] |
           ((uint32_t)p_data[1] << 8u) |
           ((uint32_t)p_data[2] << 16u) |
           ((uint32_t)p_data[3] << 24u);
}

static zynq_iap_update_result_t default_prepare(void *p_context,
                                                 const zynq_iap_update_info_t *p_info)
{
    (void)p_context;
    (void)p_info;
    return ZYNQ_IAP_UPDATE_RESULT_SUCCESS;
}

static void boot_reason_set(void)
{
    volatile zynq_iap_boot_reason_record_t *p_record =
        (volatile zynq_iap_boot_reason_record_t *)(uintptr_t)ZYNQ_BOOT_REASON_ADDRESS;

    p_record->reason = ZYNQ_BOOT_REASON_IAP_REQUEST;
    p_record->inverted_magic = (uint32_t)(~ZYNQ_BOOT_REASON_MAGIC);
    __asm__ volatile("dmb sy" ::: "memory");
    p_record->magic = ZYNQ_BOOT_REASON_MAGIC;
    __asm__ volatile("dmb sy" ::: "memory");
}

static void info_handle(section_packform_t *p_request, DEC_MY_PRINTF)
{
    section_packform_t response = {0};
    zynq_iap_update_info_t info = {0};
    uint8_t ack[ZYNQ_IAP_ACK_LENGTH] = {ZYNQ_IAP_ACK_REJECTED, 0u, 0u};

    if ((p_request == NULL) || (my_printf == NULL))
    {
        return;
    }
    if ((p_request->p_data != NULL) &&
        (p_request->len >= ZYNQ_IAP_INFO_LENGTH) &&
        (transfer_pending == 0u))
    {
        info.module_id = p_request->p_data[0];
        info.version = read_u32_le(&p_request->p_data[1]);
        info.file_size = read_u32_le(&p_request->p_data[5]);
        info.update_type = p_request->p_data[9];
        if (p_prepare_callback(p_prepare_context, &info) == ZYNQ_IAP_UPDATE_RESULT_SUCCESS)
        {
            boot_reason_set();
            transfer_pending = 1u;
            transfer_called = 0u;
            transfer_request_tick = bsp_timer_gettime_100us();
            ack[0] = ZYNQ_IAP_ACK_ACCEPTED;
        }
    }
    response.cmd_set = p_request->cmd_set;
    response.cmd_word = p_request->cmd_word;
    response.dst = p_request->src;
    response.d_dst = p_request->d_src;
    response.src = p_request->dst;
    response.d_src = p_request->d_dst;
    response.is_ack = 1u;
    response.len = ZYNQ_IAP_ACK_LENGTH;
    response.p_data = ack;
    comm_send_data(&response, my_printf);
}

static void process(void)
{
    if ((transfer_pending == 1u) &&
        (transfer_called == 0u) &&
        ((bsp_timer_gettime_100us() - transfer_request_tick) >=
         ZYNQ_IAP_HANDOFF_DELAY_TICKS))
    {
        transfer_called = 1u;
        bsp_ethernet_prepare_handoff();
        Xil_DCacheFlush();
        Xil_ExceptionDisable();
        Xil_DCacheDisable();
        Xil_ICacheDisable();
        ((void (*)(void))(uintptr_t)ZYNQ_BOOTLOADER_ENTRY)();
        transfer_pending = 0u;
    }
}

static void init(void)
{
    if (p_prepare_callback == NULL)
    {
        p_prepare_callback = default_prepare;
    }
}

zynq_iap_update_result_t zynq_iap_update_prepare_mount(zynq_iap_prepare_t p_prepare,
                                                        void *p_context)
{
    if (p_prepare == NULL)
    {
        return ZYNQ_IAP_UPDATE_RESULT_INVALID_ARGUMENT;
    }
    p_prepare_callback = p_prepare;
    p_prepare_context = p_context;
    return ZYNQ_IAP_UPDATE_RESULT_SUCCESS;
}

REG_INIT(1, init)
REG_TASK_MS(1u, process)
REG_COMM(ZYNQ_IAP_CMD_SET, ZYNQ_IAP_CMD_INFO, info_handle)
