// SPDX-License-Identifier: MIT
/**
 * @file    main.c
 * @brief   Independent HC32F334 bootloader entry point.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Mount HC32 FAL devices into the platform-independent Bootloader Core
 *          - Initialize the dedicated USART2 FRAME link and Section service
 *          - Keep communication and bounded upgrade processing active while resident
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Bootloader and IAP are independent linked images
 *          - Hardware access is mounted through BSP and platform operation tables
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

#include "boot_uart.h"
#include "bootloader_core.h"
#include "bootloader_fal_adapter.h"
#include "bootloader_protocol.h"
#include "bootloader_section_service.h"
#include "bsp_clk.h"
#include "bsp_w25q64.h"
#include "comm.h"
#include "comm_addr.h"
#include "fal_cfg.h"
#include "hc32_boot_platform.h"
#include "hc32_ll.h"
#include "section.h"
#include "systick.h"

#include <stdint.h>

#define HC32_BOOT_LL_PERIPH_SEL (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                                 LL_PERIPH_EFM | LL_PERIPH_SRAM)

#ifndef HC32_BOOTLOADER_DEFAULT_MODE
#define HC32_BOOTLOADER_DEFAULT_MODE BOOTLOADER_UPGRADE_MODE_STAGED
#endif

#ifndef HC32_BOOT_W25Q64_SELF_TEST
#define HC32_BOOT_W25Q64_SELF_TEST 0
#endif

static fal_t s_fal;
static bootloader_fal_adapter_t s_adapter;
static bootloader_flash_ops_t s_flash_ops;
static bootloader_t s_bootloader;
static bootloader_protocol_t s_protocol;
static uint8_t s_packet_buffer[BOOTLOADER_PACKET_DATA_SIZE];
static uint8_t s_copy_buffer[BSP_W25Q64_SECTOR_SIZE];
#if (HC32_BOOT_W25Q64_SELF_TEST == 1)
static volatile int32_t s_w25q64_self_test_result;
static volatile uint32_t s_w25q64_jedec_id;
#endif

static uint16_t firmware_crc_init(void)
{
    return crc16_init();
}

static uint16_t firmware_crc_update(const uint8_t *p_data, uint32_t length, uint16_t crc)
{
    uint32_t index;

    for (index = 0UL; index < length; index++)
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
        HOST_ADDR,
        HC32_BOOTLOADER_DEFAULT_MODE,
        8UL,
        s_packet_buffer,
        (uint32_t)sizeof(s_packet_buffer),
        s_copy_buffer,
        (uint32_t)sizeof(s_copy_buffer),
        firmware_crc_init,
        firmware_crc_update,
    };
    bootloader_platform_ops_t platform_ops;
#if (HC32_BOOT_W25Q64_SELF_TEST == 1)
    uint32_t jedec_id = 0UL;
#endif

    LL_PERIPH_WE(HC32_BOOT_LL_PERIPH_SEL);
    BSP_CLK_Init();
    systick_config();
    if (LL_OK != hc32_boot_uart_init())
    {
        for (;;)
        {
        }
    }
#if (HC32_BOOT_W25Q64_SELF_TEST == 1)
    s_w25q64_self_test_result = (int32_t)bsp_w25q64_init();
    if (BSP_W25Q64_RESULT_SUCCESS == s_w25q64_self_test_result)
    {
        s_w25q64_self_test_result = (int32_t)bsp_w25q64_self_test();
    }
    else
    {
        (void)bsp_w25q64_read_jedec_id(&jedec_id);
        s_w25q64_jedec_id = jedec_id;
    }
#endif
    if (BOOTLOADER_RESULT_SUCCESS !=
        bootloader_fal_adapter_mount(&s_adapter,
                                     &s_fal,
                                     &g_fal_api,
                                     &g_hc32f334_fal_cfg,
                                     g_hc32f334_bootloader_zone_map,
                                     (uint16_t)BOOTLOADER_FLASH_ZONE_COUNT,
                                     &s_flash_ops))
    {
        for (;;)
        {
        }
    }
    platform_ops = hc32_boot_platform_ops_make();
    if ((BOOTLOADER_RESULT_SUCCESS != bootloader_init(&s_bootloader,
                                                       &config,
                                                       &s_flash_ops,
                                                       &platform_ops)) ||
        (BOOTLOADER_RESULT_SUCCESS != bootloader_protocol_init(&s_protocol,
                                                                &s_bootloader,
                                                                packet_crc,
                                                                config.default_mode)) ||
        (BOOTLOADER_RESULT_SUCCESS != bootloader_section_service_mount(&s_bootloader,
                                                                        &s_protocol)))
    {
        for (;;)
        {
        }
    }

    section_init();
    for (;;)
    {
        run_task();
    }
}
