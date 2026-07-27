// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_fal_adapter.c
 * @brief   FAL-backed bootloader flash service implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Validate a complete and unique bootloader-to-FAL zone map
 *          - Forward logical storage requests into a mounted FAL API table
 *          - Convert storage results and permissions at the module boundary
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; the owning bootloader task serializes calls
 *          - Hardware access remains behind the mounted FAL configuration
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

#include "bootloader_fal_adapter.h"

#include <stddef.h>
#include <string.h>

static bootloader_result_t result_convert(fal_result_t result)
{
    switch (result)
    {
    case FAL_RESULT_SUCCESS:
        return BOOTLOADER_RESULT_SUCCESS;
    case FAL_RESULT_IN_PROGRESS:
        return BOOTLOADER_RESULT_IN_PROGRESS;
    case FAL_RESULT_BUSY:
        return BOOTLOADER_RESULT_BUSY;
    case FAL_RESULT_INVALID_ARGUMENT:
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    case FAL_RESULT_OUT_OF_RANGE:
        return BOOTLOADER_RESULT_OUT_OF_RANGE;
    case FAL_RESULT_CONFIG_ERROR:
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    case FAL_RESULT_PERMISSION_DENIED:
        return BOOTLOADER_RESULT_PERMISSION_DENIED;
    case FAL_RESULT_DRIVER_ERROR:
    case FAL_RESULT_STOPPED:
    default:
        return BOOTLOADER_RESULT_STORAGE_ERROR;
    }
}

static const bootloader_fal_zone_map_t *mapping_find(const bootloader_fal_adapter_t *p_adapter,
                                                      bootloader_flash_zone_t zone)
{
    uint16_t index = 0u; /* Mapping entry being inspected. */

    for (index = 0u; index < p_adapter->zone_map_count; index++)
    {
        if (p_adapter->p_zone_map[index].bootloader_zone == zone)
        {
            return &p_adapter->p_zone_map[index];
        }
    }
    return NULL;
}

static bootloader_result_t adapter_init(void *p_context)
{
    bootloader_fal_adapter_t *p_adapter = (bootloader_fal_adapter_t *)p_context; /* Adapter being initialized. */

    if (p_adapter == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    return result_convert(p_adapter->p_fal_api->p_init(p_adapter->p_fal, p_adapter->p_fal_cfg));
}

static void adapter_process(void *p_context)
{
    bootloader_fal_adapter_t *p_adapter = (bootloader_fal_adapter_t *)p_context; /* Adapter being advanced. */

    if (p_adapter != NULL)
    {
        p_adapter->p_fal_api->p_process(p_adapter->p_fal);
    }
}

static bootloader_result_t adapter_zone_info_get(void *p_context,
                                                  bootloader_flash_zone_t zone,
                                                  bootloader_flash_zone_info_t *p_info)
{
    bootloader_fal_adapter_t *p_adapter = (bootloader_fal_adapter_t *)p_context; /* Adapter serving the query. */
    const bootloader_fal_zone_map_t *p_mapping = NULL; /* Selected logical zone mapping. */
    fal_zone_info_t fal_info = {0};                    /* FAL geometry converted for the caller. */
    fal_result_t result = FAL_RESULT_SUCCESS;          /* FAL query result. */

    if ((p_adapter == NULL) || (p_info == NULL))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    p_mapping = mapping_find(p_adapter, zone);
    if (p_mapping == NULL)
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }
    result = p_adapter->p_fal_api->p_zone_info_get(p_adapter->p_fal, p_mapping->fal_zone, &fal_info);
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
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t adapter_read(void *p_context,
                                        bootloader_flash_zone_t zone,
                                        uint32_t offset,
                                        uint32_t length,
                                        uint8_t *p_data)
{
    bootloader_fal_adapter_t *p_adapter = (bootloader_fal_adapter_t *)p_context; /* Adapter serving the read. */
    const bootloader_fal_zone_map_t *p_mapping = NULL; /* Selected logical zone mapping. */

    if (p_adapter == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    p_mapping = mapping_find(p_adapter, zone);
    if (p_mapping == NULL)
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }
    return result_convert(p_adapter->p_fal_api->p_read(p_adapter->p_fal,
                                                       p_mapping->fal_zone,
                                                       offset,
                                                       length,
                                                       p_data));
}

static bootloader_result_t adapter_write(void *p_context,
                                         bootloader_flash_zone_t zone,
                                         uint32_t offset,
                                         uint32_t length,
                                         const uint8_t *p_data)
{
    bootloader_fal_adapter_t *p_adapter = (bootloader_fal_adapter_t *)p_context; /* Adapter serving the write. */
    const bootloader_fal_zone_map_t *p_mapping = NULL; /* Selected logical zone mapping. */

    if (p_adapter == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    p_mapping = mapping_find(p_adapter, zone);
    if (p_mapping == NULL)
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }
    return result_convert(p_adapter->p_fal_api->p_write(p_adapter->p_fal,
                                                        p_mapping->fal_zone,
                                                        offset,
                                                        length,
                                                        p_data));
}

static bootloader_result_t adapter_erase(void *p_context,
                                         bootloader_flash_zone_t zone,
                                         uint32_t offset,
                                         uint32_t length)
{
    bootloader_fal_adapter_t *p_adapter = (bootloader_fal_adapter_t *)p_context; /* Adapter serving the erase. */
    const bootloader_fal_zone_map_t *p_mapping = NULL; /* Selected logical zone mapping. */

    if (p_adapter == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    p_mapping = mapping_find(p_adapter, zone);
    if (p_mapping == NULL)
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }
    return result_convert(p_adapter->p_fal_api->p_erase(p_adapter->p_fal,
                                                        p_mapping->fal_zone,
                                                        offset,
                                                        length));
}

static uint8_t adapter_is_busy(void *p_context)
{
    const bootloader_fal_adapter_t *p_adapter = (const bootloader_fal_adapter_t *)p_context; /* Queried adapter. */

    return (p_adapter == NULL) ? 0u : p_adapter->p_fal_api->p_is_busy(p_adapter->p_fal);
}

static bootloader_result_t adapter_result_get(void *p_context)
{
    const bootloader_fal_adapter_t *p_adapter = (const bootloader_fal_adapter_t *)p_context; /* Queried adapter. */

    if (p_adapter == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    return result_convert(p_adapter->p_fal_api->p_result_get(p_adapter->p_fal));
}

bootloader_result_t bootloader_fal_adapter_mount(
    bootloader_fal_adapter_t *p_adapter,
    fal_t *p_fal,
    const fal_api_t *p_fal_api,
    const fal_cfg_t *p_fal_cfg,
    const bootloader_fal_zone_map_t *p_zone_map,
    uint16_t zone_map_count,
    bootloader_flash_ops_t *p_flash_ops)
{
    uint16_t index = 0u;         /* Mapping entry being validated. */
    uint16_t compare_index = 0u; /* Earlier mapping checked for duplicates. */

    if ((p_adapter == NULL) ||
        (p_fal == NULL) ||
        (p_fal_api == NULL) ||
        (p_fal_cfg == NULL) ||
        (p_zone_map == NULL) ||
        (p_flash_ops == NULL) ||
        (zone_map_count != (uint16_t)BOOTLOADER_FLASH_ZONE_COUNT) ||
        (p_fal_api->p_init == NULL) ||
        (p_fal_api->p_process == NULL) ||
        (p_fal_api->p_zone_info_get == NULL) ||
        (p_fal_api->p_read == NULL) ||
        (p_fal_api->p_write == NULL) ||
        (p_fal_api->p_erase == NULL) ||
        (p_fal_api->p_is_busy == NULL) ||
        (p_fal_api->p_result_get == NULL))
    {
        return BOOTLOADER_RESULT_CONFIG_ERROR;
    }

    for (index = 0u; index < zone_map_count; index++)
    {
        if (p_zone_map[index].bootloader_zone >= BOOTLOADER_FLASH_ZONE_COUNT)
        {
            return BOOTLOADER_RESULT_CONFIG_ERROR;
        }
        for (compare_index = 0u; compare_index < index; compare_index++)
        {
            if (p_zone_map[compare_index].bootloader_zone == p_zone_map[index].bootloader_zone)
            {
                return BOOTLOADER_RESULT_CONFIG_ERROR;
            }
        }
    }

    (void)memset(p_adapter, 0, sizeof(*p_adapter));
    p_adapter->p_fal = p_fal;
    p_adapter->p_fal_api = p_fal_api;
    p_adapter->p_fal_cfg = p_fal_cfg;
    p_adapter->p_zone_map = p_zone_map;
    p_adapter->zone_map_count = zone_map_count;

    *p_flash_ops = (bootloader_flash_ops_t){
        .p_context = p_adapter,
        .p_init = adapter_init,
        .p_process = adapter_process,
        .p_zone_info_get = adapter_zone_info_get,
        .p_read = adapter_read,
        .p_write = adapter_write,
        .p_erase = adapter_erase,
        .p_is_busy = adapter_is_busy,
        .p_result_get = adapter_result_get,
    };
    return BOOTLOADER_RESULT_SUCCESS;
}
