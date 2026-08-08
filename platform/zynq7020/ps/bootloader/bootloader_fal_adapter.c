// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_fal_adapter.c
 * @brief   Zynq-7020 FAL-to-Bootloader Flash adapter implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Convert Bootloader logical zones into Zynq FAL zones
 *          - Adapt FAL operations and results to Bootloader Flash callbacks
 *          - Mount platform, Flash, and protocol services into Bootloader Core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Bootloader Core receives only bootloader_flash_ops_t
 *          - FAL initialization remains owned by the platform FAL service
 *
 * @author  Max.Li
 * @date    2026-07-29
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bootloader_core.h"
#include "bootloader_protocol.h"
#include "bsp_qspi_flash.h"
#include "comm.h"
#include "comm_addr.h"
#include "fal_cfg.h"
#include "section.h"
#include "zynq_boot_platform.h"

#include <stddef.h>

static bootloader_t bootloader = {0}; /* Platform Bootloader Core instance. */
static bootloader_protocol_t protocol = {0}; /* FRAME upgrade protocol instance. */
static uint8_t packet_buffer[BOOTLOADER_PACKET_DATA_SIZE] = {0}; /* Upgrade packet workspace. */
static uint8_t copy_buffer[BSP_QSPI_FLASH_ERASE_SIZE] = {0}; /* Staging-copy block workspace. */

static uint16_t firmware_crc_init(void)
{
    return crc16_init();
}

static uint16_t firmware_crc_update(const uint8_t *p_data, uint32_t length, uint16_t crc)
{
    uint32_t index = 0u; /* Byte currently accumulated into the image CRC. */

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

static const bootloader_config_t config = {
    .expected_module_id = HOST_ADDR,
    .default_mode = BOOTLOADER_UPGRADE_MODE_STAGED_E,
    .image_header_length = 8u,
    .p_packet_buffer = packet_buffer,
    .packet_buffer_size = (uint32_t)sizeof(packet_buffer),
    .p_copy_buffer = copy_buffer,
    .copy_buffer_size = (uint32_t)sizeof(copy_buffer),
    .p_crc16_init = firmware_crc_init,
    .p_crc16_update = firmware_crc_update,
}; /* Platform-independent Bootloader Core configuration. */

static bootloader_result_t result_convert(fal_result_t result)
{
    switch (result)
    {
    case FAL_RESULT_SUCCESS:
        return BOOTLOADER_RESULT_SUCCESS_E;
    case FAL_RESULT_IN_PROGRESS:
        return BOOTLOADER_RESULT_IN_PROGRESS_E;
    case FAL_RESULT_BUSY:
        return BOOTLOADER_RESULT_BUSY_E;
    case FAL_RESULT_INVALID_ARGUMENT:
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    case FAL_RESULT_OUT_OF_RANGE:
        return BOOTLOADER_RESULT_OUT_OF_RANGE_E;
    case FAL_RESULT_CONFIG_ERROR:
        return BOOTLOADER_RESULT_CONFIG_ERROR_E;
    case FAL_RESULT_PERMISSION_DENIED:
        return BOOTLOADER_RESULT_PERMISSION_DENIED_E;
    case FAL_RESULT_DRIVER_ERROR:
    case FAL_RESULT_STOPPED:
    default:
        return BOOTLOADER_RESULT_STORAGE_ERROR_E;
    }
}

static bootloader_result_t fal_zone_get(bootloader_flash_zone_t zone,
                                        fal_zone_id_t *p_fal_zone)
{
    if (p_fal_zone == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }

    switch (zone)
    {
    case BOOTLOADER_FLASH_ZONE_IAP_E:
        *p_fal_zone = FAL_ZONE_ZYNQ_IAP;
        break;
    case BOOTLOADER_FLASH_ZONE_STAGING_E:
        *p_fal_zone = FAL_ZONE_ZYNQ_IAP_STAGING;
        break;
    case BOOTLOADER_FLASH_ZONE_META_A_E:
        *p_fal_zone = FAL_ZONE_ZYNQ_UPDATE_META_A;
        break;
    case BOOTLOADER_FLASH_ZONE_META_B_E:
        *p_fal_zone = FAL_ZONE_ZYNQ_UPDATE_META_B;
        break;
    case BOOTLOADER_FLASH_ZONE_LAYOUT_E:
        *p_fal_zone = FAL_ZONE_ZYNQ_LAYOUT;
        break;
    case BOOTLOADER_FLASH_ZONE_COUNT_E:
    default:
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }

    return BOOTLOADER_RESULT_SUCCESS_E;
}

static bootloader_result_t zone_info_get(bootloader_flash_zone_t zone,
                                         bootloader_flash_zone_info_t *p_info)
{
    fal_zone_info_t fal_info = {0}; /* Geometry and permissions reported by FAL. */
    fal_zone_id_t fal_zone = 0u; /* Platform FAL zone selected for the logical zone. */
    fal_result_t result = FAL_RESULT_INVALID_ARGUMENT; /* FAL request result. */

    if (p_info == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }
    if (fal_zone_get(zone, &fal_zone) != BOOTLOADER_RESULT_SUCCESS_E)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }

    result = fal_zone_info_get(&g_zynq7020_fal, fal_zone, &fal_info);
    if (result != FAL_RESULT_SUCCESS)
    {
        return result_convert(result);
    }

    p_info->size = fal_info.size;
    p_info->program_page_size = fal_info.program_page_size;
    p_info->erase_block_size = fal_info.erase_block_size;
    p_info->readable = ((fal_info.permissions & FAL_ZONE_PERMISSION_READ) != 0u) ? 1u : 0u;
    p_info->writable = ((fal_info.permissions & FAL_ZONE_PERMISSION_WRITE) != 0u) ? 1u : 0u;
    p_info->erasable = ((fal_info.permissions & FAL_ZONE_PERMISSION_ERASE) != 0u) ? 1u : 0u;
    return BOOTLOADER_RESULT_SUCCESS_E;
}

static bootloader_result_t read(bootloader_flash_zone_t zone,
                                uint32_t offset,
                                uint32_t length,
                                uint8_t *p_data)
{
    fal_zone_id_t fal_zone = 0u; /* Platform FAL zone selected for the logical zone. */

    if (fal_zone_get(zone, &fal_zone) != BOOTLOADER_RESULT_SUCCESS_E)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }
    return result_convert(fal_read(&g_zynq7020_fal, fal_zone, offset, length, p_data));
}

static bootloader_result_t write(bootloader_flash_zone_t zone,
                                 uint32_t offset,
                                 uint32_t length,
                                 const uint8_t *p_data)
{
    fal_zone_id_t fal_zone = 0u; /* Platform FAL zone selected for the logical zone. */

    if (fal_zone_get(zone, &fal_zone) != BOOTLOADER_RESULT_SUCCESS_E)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }
    return result_convert(fal_write(&g_zynq7020_fal, fal_zone, offset, length, p_data));
}

static bootloader_result_t erase(bootloader_flash_zone_t zone,
                                 uint32_t offset,
                                 uint32_t length)
{
    fal_zone_id_t fal_zone = 0u; /* Platform FAL zone selected for the logical zone. */

    if (fal_zone_get(zone, &fal_zone) != BOOTLOADER_RESULT_SUCCESS_E)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }
    return result_convert(fal_erase(&g_zynq7020_fal, fal_zone, offset, length));
}

static uint8_t is_busy(void)
{
    return fal_is_busy(&g_zynq7020_fal);
}

static bootloader_result_t result_get(void)
{
    return result_convert(fal_result_get(&g_zynq7020_fal));
}

static const bootloader_flash_ops_t flash_ops = {
    .p_zone_info_get = zone_info_get,
    .p_read = read,
    .p_write = write,
    .p_erase = erase,
    .p_is_busy = is_busy,
    .p_result_get = result_get,
}; /* FAL callbacks mounted into Bootloader Core. */

static void halt(void)
{
    for (;;)
    {
    }
}

static void bootloader_fal_adapter_init(void)
{
    bootloader_platform_ops_t platform_ops = {0}; /* Zynq boot-reason and IAP handoff callbacks. */
    fal_state_t fal_state = fal_state_get(&g_zynq7020_fal); /* FAL state established by priority 0 init. */
    if ((fal_state == FAL_STATE_UNINITIALIZED) ||
        (fal_state == FAL_STATE_ERROR) ||
        (fal_state == FAL_STATE_STOPPED))
    {
        halt();
    }

    platform_ops = zynq_boot_platform_ops_make(&bootloader);
    if ((bootloader_flash_ops_init(&bootloader, &flash_ops) != BOOTLOADER_RESULT_SUCCESS_E) ||
        (bootloader_init(&bootloader, &config, &platform_ops) != BOOTLOADER_RESULT_SUCCESS_E) ||
        (bootloader_protocol_init(&protocol,
                                  &bootloader,
                                  packet_crc,
                                  config.default_mode) != BOOTLOADER_RESULT_SUCCESS_E) ||
        (bootloader_protocol_mount(&protocol) != BOOTLOADER_RESULT_SUCCESS_E))
    {
        halt();
    }
}

REG_INIT(1, bootloader_fal_adapter_init)
