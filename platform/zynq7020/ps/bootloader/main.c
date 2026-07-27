// SPDX-License-Identifier: MIT
/**
 * @file    main.c
 * @brief   Independent Zynq-7020 bootloader program entry.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Mount Zynq QSPI cfg into FAL and FAL into Bootloader Core
 *          - Initialize the dedicated FRAME protocol and Section service
 *          - Keep communication and the bounded upgrade state machine running while resident
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Bootloader and IAP are linked as independent programs
 *          - Platform flash and jump access are mounted through operation tables
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bootloader_core.h"
#include "bootloader_fal_adapter.h"
#include "bootloader_protocol.h"
#include "bootloader_section_service.h"
#include "bsp_interrupt.h"
#include "bsp_qspi_flash.h"
#include "bsp_timer.h"
#include "bsp_usart.h"
#include "comm.h"
#include "comm_addr.h"
#include "fal_cfg.h"
#include "section.h"
#include "xstatus.h"
#include "zynq_boot_platform.h"

#include <stdint.h>

static fal_t s_fal;
static bootloader_fal_adapter_t s_adapter;
static bootloader_flash_ops_t s_flash_ops;
static bootloader_t s_bootloader;
static bootloader_protocol_t s_protocol;
static uint8_t s_packet_buffer[BOOTLOADER_PACKET_DATA_SIZE];
static uint8_t s_copy_buffer[BSP_QSPI_FLASH_ERASE_SIZE];

static uint16_t firmware_crc_init(void)
{
    return crc16_init();
}

static uint16_t firmware_crc_update(const uint8_t *p_data, uint32_t length, uint16_t crc)
{
    uint32_t index = 0u;

    for (index = 0u; index < length; index++)
    {
        crc = crc16_update(crc, p_data[index]);
    }
    return crc;
}

static uint16_t packet_crc(const uint8_t *p_data, uint32_t length)
{
    return firmware_crc_update(p_data, length, firmware_crc_init());
}

int main(void)
{
    const bootloader_config_t config = {
        .expected_module_id = HOST_ADDR,
        .default_mode = BOOTLOADER_UPGRADE_MODE_STAGED,
        .image_header_length = 8u,
        .p_packet_buffer = s_packet_buffer,
        .packet_buffer_size = (uint32_t)sizeof(s_packet_buffer),
        .p_copy_buffer = s_copy_buffer,
        .copy_buffer_size = (uint32_t)sizeof(s_copy_buffer),
        .p_crc16_init = firmware_crc_init,
        .p_crc16_update = firmware_crc_update,
    };
    bootloader_platform_ops_t platform_ops;
    int32_t status = XST_FAILURE;

    bsp_timer_init();
    section_port_init();
    status = bsp_usart_init();
    if (status == XST_SUCCESS)
    {
        status = bsp_interrupt_init();
    }
    if (status != XST_SUCCESS)
    {
        for (;;)
        {
        }
    }
    if (bootloader_fal_adapter_mount(&s_adapter,
                                      &s_fal,
                                      &g_fal_api,
                                      &g_zynq7020_fal_cfg,
                                      g_zynq7020_bootloader_zone_map,
                                      (uint16_t)BOOTLOADER_FLASH_ZONE_COUNT,
                                      &s_flash_ops) != BOOTLOADER_RESULT_SUCCESS)
    {
        for (;;)
        {
        }
    }
    platform_ops = zynq_boot_platform_ops_make(&s_bootloader);
    if ((bootloader_init(&s_bootloader, &config, &s_flash_ops, &platform_ops) !=
         BOOTLOADER_RESULT_SUCCESS) ||
        (bootloader_protocol_init(&s_protocol,
                                  &s_bootloader,
                                  packet_crc,
                                  config.default_mode) != BOOTLOADER_RESULT_SUCCESS) ||
        (bootloader_section_service_mount(&s_bootloader, &s_protocol) !=
         BOOTLOADER_RESULT_SUCCESS))
    {
        for (;;)
        {
        }
    }

    section_init();
    if (bsp_timer_interrupt_start(10000u) != XST_SUCCESS)
    {
        for (;;)
        {
        }
    }
    for (;;)
    {
        run_task();
    }
}
