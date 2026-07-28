// SPDX-License-Identifier: MIT
/**
 * @file    fal_core.c
 * @brief   Platform-independent Flash Abstraction Layer state machine.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Validate mounted flash devices and non-overlapping logical zones
 *          - Translate zone-relative requests into bounded physical operations
 *          - Advance asynchronous read, program, erase, and sync work one step at a time
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; one execution context owns each fal_t instance
 *          - Hardware access is abstracted through fal_flash_ops_t
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

#include "fal_core.h"

#include <stddef.h>
#include <string.h>

static const fal_zone_cfg_t *zone_find(const fal_cfg_t *p_cfg,
                                       fal_zone_id_t zone_id,
                                       const fal_device_cfg_t **p_p_device,
                                       uint32_t *p_device_offset)
{
    uint16_t device_index = 0u; /* Physical Flash table index. */

    for (device_index = 0u; device_index < p_cfg->device_count; device_index++)
    {
        const fal_device_cfg_t *p_device = &p_cfg->p_devices[device_index];
        uint32_t device_offset = 0u; /* Running zone address on this Flash. */
        uint16_t zone_index = 0u;    /* Zone table index on this Flash. */

        for (zone_index = 0u; zone_index < p_device->zone_count; zone_index++)
        {
            const fal_zone_cfg_t *p_zone = &p_device->p_zones[zone_index];

            if (p_zone->zone_id == zone_id)
            {
                *p_p_device = p_device;
                *p_device_offset = device_offset;
                return p_zone;
            }
            device_offset += p_zone->size;
        }
    }

    return NULL;
}

static uint8_t zone_id_is_duplicate(const fal_cfg_t *p_cfg,
                                    uint16_t device_index,
                                    uint16_t zone_index)
{
    const fal_zone_id_t zone_id = p_cfg->p_devices[device_index].p_zones[zone_index].zone_id;
    uint16_t compare_device_index = 0u; /* Earlier physical Flash table index. */

    for (compare_device_index = 0u;
         compare_device_index <= device_index;
         compare_device_index++)
    {
        const fal_device_cfg_t *p_device = &p_cfg->p_devices[compare_device_index];
        const uint16_t compare_zone_count = (compare_device_index == device_index)
                                                ? zone_index
                                                : p_device->zone_count;
        uint16_t compare_zone_index = 0u;

        for (compare_zone_index = 0u;
             compare_zone_index < compare_zone_count;
             compare_zone_index++)
        {
            if (p_device->p_zones[compare_zone_index].zone_id == zone_id)
            {
                return 1u;
            }
        }
    }

    return 0u;
}

static fal_result_t device_validate(const fal_cfg_t *p_cfg, uint16_t device_index)
{
    const fal_device_cfg_t *p_device = &p_cfg->p_devices[device_index]; /* Device under validation. */
    uint16_t compare_index = 0u;                                        /* Earlier device checked for duplicate identifiers. */

    if ((p_device->capacity == 0u) ||                         /* A physical device must expose storage. */
        (p_device->program_page_size == 0u) ||                /* Programs require a finite page boundary. */
        (p_device->erase_block_size == 0u) ||                 /* Erases require a finite block boundary. */
        (p_device->program_page_size > p_device->capacity) || /* A page must fit in the device. */
        (p_device->erase_block_size > p_device->capacity) ||  /* A block must fit in the device. */
        (p_device->p_zones == NULL) ||                        /* Every managed Flash has a zone table. */
        (p_device->zone_count == 0u) ||                       /* Empty device zone tables are invalid. */
        (p_device->ops.p_init == NULL) ||                     /* Device initialization is mandatory. */
        (p_device->ops.p_get_state == NULL) ||                /* Async completion observation is mandatory. */
        (p_device->ops.p_read == NULL) ||                     /* Reads are part of the common FAL contract. */
        (p_device->ops.p_program == NULL) ||                  /* Programs are part of the common FAL contract. */
        (p_device->ops.p_erase == NULL))                      /* Erases are part of the common FAL contract. */
    {
        return FAL_RESULT_CONFIG_ERROR;
    }

    for (compare_index = 0u; compare_index < device_index; compare_index++)
    {
        if (p_cfg->p_devices[compare_index].device_id == p_device->device_id)
        {
            return FAL_RESULT_CONFIG_ERROR;
        }
    }

    return FAL_RESULT_SUCCESS;
}

static fal_result_t zone_validate(const fal_cfg_t *p_cfg,
                                  uint16_t device_index,
                                  uint16_t zone_index)
{
    const fal_device_cfg_t *p_device = &p_cfg->p_devices[device_index];
    const fal_zone_cfg_t *p_zone = &p_device->p_zones[zone_index];
    uint32_t device_offset = 0u; /* Address accumulated from preceding zones. */
    uint16_t preceding_index = 0u;

    for (preceding_index = 0u; preceding_index < zone_index; preceding_index++)
    {
        const uint32_t preceding_size = p_device->p_zones[preceding_index].size;

        if (preceding_size > (p_device->capacity - device_offset))
        {
            return FAL_RESULT_CONFIG_ERROR;
        }
        device_offset += preceding_size;
    }

    if ((p_zone->size == 0u) ||                                                /* Empty logical partitions are invalid. */
        ((p_zone->permissions & (uint8_t)(~FAL_ZONE_PERMISSION_ALL)) != 0u) || /* Reject unknown rights. */
        (p_zone->permissions == 0u) ||                                         /* A zone must expose at least one operation. */
        (p_zone->size > (p_device->capacity - device_offset)) ||               /* Cumulative end must remain in bounds. */
        ((device_offset % p_device->erase_block_size) != 0u) ||                /* Accumulated start is erase aligned. */
        ((p_zone->size % p_device->erase_block_size) != 0u))                   /* Zones contain whole blocks. */
    {
        return FAL_RESULT_CONFIG_ERROR;
    }

    if (zone_id_is_duplicate(p_cfg, device_index, zone_index) == 1u)
    {
        return FAL_RESULT_CONFIG_ERROR;
    }

    return FAL_RESULT_SUCCESS;
}

static void operation_clear(fal_t *p_fal)
{
    p_fal->p_read_data = NULL;
    p_fal->p_write_data = NULL;
    p_fal->physical_address = 0u;
    p_fal->remaining = 0u;
    p_fal->chunk_length = 0u;
    p_fal->active_device_id = 0u;
    p_fal->operation_state = FAL_STATE_IDLE;
    p_fal->operation = FAL_OPERATION_NONE;
    p_fal->sync_issued = 0u;
}

static void operation_finish(fal_t *p_fal, fal_result_t result)
{
    operation_clear(p_fal);
    p_fal->result = result;
    if (p_fal->stop_requested == 1u)
    {
        p_fal->state = FAL_STATE_STOPPED;
    }
    else
    {
        p_fal->state = FAL_STATE_IDLE;
    }
}

static fal_result_t request_validate(fal_t *p_fal,
                                     fal_zone_id_t zone_id,
                                     uint32_t offset,
                                     uint32_t length,
                                     uint8_t permission,
                                     const fal_zone_cfg_t **p_p_zone,
                                     const fal_device_cfg_t **p_p_device,
                                     uint32_t *p_device_offset)
{
    const fal_zone_cfg_t *p_zone = NULL;     /* Logical zone selected for the request. */
    const fal_device_cfg_t *p_device = NULL; /* Physical device selected for the request. */

    if ((p_fal == NULL) ||
        (p_p_zone == NULL) ||
        (p_p_device == NULL) ||
        (p_device_offset == NULL))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    if (p_fal->state == FAL_STATE_STOPPED)
    {
        return FAL_RESULT_STOPPED;
    }
    if ((p_fal->state == FAL_STATE_UNINITIALIZED) ||
        (p_fal->state == FAL_STATE_ERROR) ||
        (p_fal->p_cfg == NULL))
    {
        return FAL_RESULT_CONFIG_ERROR;
    }
    if (fal_is_busy(p_fal) == 1u)
    {
        return FAL_RESULT_BUSY;
    }

    p_zone = zone_find(p_fal->p_cfg, zone_id, &p_device, p_device_offset);
    if (p_zone == NULL)
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    if ((p_zone->permissions & permission) == 0u)
    {
        return FAL_RESULT_PERMISSION_DENIED;
    }
    if ((offset > p_zone->size) ||          /* Offset may equal size only for an empty request. */
        (length > (p_zone->size - offset))) /* Request end must remain inside the zone. */
    {
        return FAL_RESULT_OUT_OF_RANGE;
    }

    *p_p_zone = p_zone;
    *p_p_device = p_device;
    return FAL_RESULT_SUCCESS;
}

static void request_begin(fal_t *p_fal,
                          const fal_device_cfg_t *p_device,
                          fal_operation_type_t operation,
                          fal_state_t operation_state,
                          uint32_t physical_address,
                          uint32_t length)
{
    p_fal->active_device_id = p_device->device_id;
    p_fal->physical_address = physical_address;
    p_fal->remaining = length;
    p_fal->chunk_length = 0u;
    p_fal->operation = operation;
    p_fal->operation_state = operation_state;
    p_fal->state = operation_state;
    p_fal->result = FAL_RESULT_IN_PROGRESS;
    p_fal->sync_issued = 0u;
}

static uint32_t read_chunk_get(const fal_t *p_fal, const fal_device_cfg_t *p_device)
{
    uint32_t chunk = p_fal->remaining; /* Bytes selected for the next physical read. */

    if ((p_device->max_read_size != 0u) &&
        (chunk > p_device->max_read_size))
    {
        chunk = p_device->max_read_size;
    }
    return chunk;
}

static uint32_t write_chunk_get(const fal_t *p_fal, const fal_device_cfg_t *p_device)
{
    uint32_t page_offset = p_fal->physical_address % p_device->program_page_size; /* Page offset. */
    uint32_t page_available = p_device->program_page_size - page_offset;          /* Remaining page bytes. */

    return (p_fal->remaining < page_available) ? p_fal->remaining : page_available;
}

static fal_result_t operation_issue(fal_t *p_fal, const fal_device_cfg_t *p_device)
{
    fal_result_t result = FAL_RESULT_DRIVER_ERROR; /* Platform operation submission result. */
    uint32_t chunk = 0u;                           /* Bytes submitted in this state-machine step. */

    if (p_fal->operation == FAL_OPERATION_READ)
    {
        chunk = read_chunk_get(p_fal, p_device);
        result = p_device->ops.p_read(p_device->ops.p_context,
                                      p_fal->physical_address,
                                      chunk,
                                      p_fal->p_read_data);
    }
    else if (p_fal->operation == FAL_OPERATION_WRITE)
    {
        chunk = write_chunk_get(p_fal, p_device);
        result = p_device->ops.p_program(p_device->ops.p_context,
                                         p_fal->physical_address,
                                         chunk,
                                         p_fal->p_write_data);
    }
    else if (p_fal->operation == FAL_OPERATION_ERASE)
    {
        chunk = p_device->erase_block_size;
        result = p_device->ops.p_erase(p_device->ops.p_context,
                                       p_fal->physical_address,
                                       chunk);
    }
    else
    {
        return FAL_RESULT_CONFIG_ERROR;
    }

    if (result == FAL_RESULT_SUCCESS)
    {
        p_fal->chunk_length = chunk;
        p_fal->state = FAL_STATE_WAIT_DEVICE;
    }
    return result;
}

static void completed_chunk_commit(fal_t *p_fal)
{
    p_fal->physical_address += p_fal->chunk_length;
    p_fal->remaining -= p_fal->chunk_length;
    if (p_fal->operation == FAL_OPERATION_READ)
    {
        p_fal->p_read_data += p_fal->chunk_length;
    }
    else if (p_fal->operation == FAL_OPERATION_WRITE)
    {
        p_fal->p_write_data += p_fal->chunk_length;
    }
    else
    {
        /* Erase operations have no caller buffer to advance. */
    }
    p_fal->chunk_length = 0u;
}

static void operation_complete_or_sync(fal_t *p_fal, const fal_device_cfg_t *p_device)
{
    fal_result_t result = FAL_RESULT_SUCCESS; /* Optional sync submission result. */

    if ((p_device->ops.p_sync != NULL) &&
        (p_fal->sync_issued == 0u))
    {
        result = p_device->ops.p_sync(p_device->ops.p_context);
        if (result != FAL_RESULT_SUCCESS)
        {
            operation_finish(p_fal, FAL_RESULT_DRIVER_ERROR);
            return;
        }
        p_fal->sync_issued = 1u;
        p_fal->chunk_length = 0u;
        p_fal->state = FAL_STATE_WAIT_DEVICE;
        return;
    }

    operation_finish(p_fal, FAL_RESULT_SUCCESS);
}

fal_result_t fal_init(fal_t *p_fal, const fal_cfg_t *p_cfg)
{
    uint16_t index = 0u;                      /* Configuration entry being initialized. */
    fal_result_t result = FAL_RESULT_SUCCESS; /* Validation or driver initialization result. */

    if (p_fal == NULL)
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }

    (void)memset(p_fal, 0, sizeof(*p_fal));
    p_fal->state = FAL_STATE_UNINITIALIZED;
    p_fal->result = FAL_RESULT_CONFIG_ERROR;

    if ((p_cfg == NULL) ||
        (p_cfg->p_devices == NULL) ||
        (p_cfg->device_count == 0u))
    {
        p_fal->state = FAL_STATE_ERROR;
        return FAL_RESULT_CONFIG_ERROR;
    }

    for (index = 0u; index < p_cfg->device_count; index++)
    {
        result = device_validate(p_cfg, index);
        if (result != FAL_RESULT_SUCCESS)
        {
            p_fal->state = FAL_STATE_ERROR;
            return result;
        }
    }
    for (index = 0u; index < p_cfg->device_count; index++)
    {
        uint16_t zone_index = 0u;

        for (zone_index = 0u;
             zone_index < p_cfg->p_devices[index].zone_count;
             zone_index++)
        {
            result = zone_validate(p_cfg, index, zone_index);
            if (result != FAL_RESULT_SUCCESS)
            {
                p_fal->state = FAL_STATE_ERROR;
                return result;
            }
        }
    }
    for (index = 0u; index < p_cfg->device_count; index++)
    {
        result = p_cfg->p_devices[index].ops.p_init(p_cfg->p_devices[index].ops.p_context);
        if (result != FAL_RESULT_SUCCESS)
        {
            p_fal->state = FAL_STATE_ERROR;
            p_fal->result = FAL_RESULT_DRIVER_ERROR;
            return FAL_RESULT_DRIVER_ERROR;
        }
    }

    p_fal->p_cfg = p_cfg;
    p_fal->state = FAL_STATE_IDLE;
    p_fal->operation_state = FAL_STATE_IDLE;
    p_fal->result = FAL_RESULT_SUCCESS;
    return FAL_RESULT_SUCCESS;
}

static void device_process(fal_t *p_fal, const fal_device_cfg_t *p_device)
{
    fal_device_state_t device_state = FAL_DEVICE_STATE_ERROR; /* Active device state snapshot. */
    fal_result_t result = FAL_RESULT_SUCCESS;                 /* Operation issue result. */

    device_state = p_device->ops.p_get_state(p_device->ops.p_context);
    if (device_state == FAL_DEVICE_STATE_ERROR)
    {
        operation_finish(p_fal, FAL_RESULT_DRIVER_ERROR);
        return;
    }

    if (p_fal->state == FAL_STATE_WAIT_DEVICE)
    {
        if (device_state == FAL_DEVICE_STATE_BUSY)
        {
            return;
        }
        if (p_fal->chunk_length != 0u)
        {
            completed_chunk_commit(p_fal);
        }
        else if (p_fal->sync_issued == 1u)
        {
            operation_finish(p_fal, FAL_RESULT_SUCCESS);
            return;
        }
        else
        {
            operation_finish(p_fal, FAL_RESULT_CONFIG_ERROR);
            return;
        }

        if (p_fal->remaining == 0u)
        {
            operation_complete_or_sync(p_fal, p_device);
        }
        else
        {
            p_fal->state = p_fal->operation_state;
        }
        return;
    }

    if (device_state == FAL_DEVICE_STATE_BUSY)
    {
        return;
    }

    result = operation_issue(p_fal, p_device);
    if (result != FAL_RESULT_SUCCESS)
    {
        operation_finish(p_fal, FAL_RESULT_DRIVER_ERROR);
    }
}

void fal_process(fal_t *p_fal)
{
    uint16_t device_index = 0u; /* Platform Flash device visited by the state machine. */

    if ((p_fal == NULL) ||
        (p_fal->p_cfg == NULL) ||
        (p_fal->operation == FAL_OPERATION_NONE))
    {
        return;
    }

    for (device_index = 0u; device_index < p_fal->p_cfg->device_count; device_index++)
    {
        const fal_device_cfg_t *p_device = &p_fal->p_cfg->p_devices[device_index];

        if (p_device->device_id == p_fal->active_device_id)
        {
            device_process(p_fal, p_device);
            return;
        }
    }

    operation_finish(p_fal, FAL_RESULT_CONFIG_ERROR);
}

fal_result_t fal_zone_info_get(const fal_t *p_fal, fal_zone_id_t zone_id, fal_zone_info_t *p_info)
{
    const fal_zone_cfg_t *p_zone = NULL;     /* Requested zone configuration. */
    const fal_device_cfg_t *p_device = NULL; /* Device containing the requested zone. */
    uint32_t device_offset = 0u;             /* Address accumulated from preceding zones. */

    if ((p_fal == NULL) || (p_info == NULL))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    if (p_fal->p_cfg == NULL)
    {
        return FAL_RESULT_CONFIG_ERROR;
    }
    p_zone = zone_find(p_fal->p_cfg, zone_id, &p_device, &device_offset);
    if (p_zone == NULL)
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    p_info->zone_id = p_zone->zone_id;
    p_info->device_id = p_device->device_id;
    p_info->size = p_zone->size;
    p_info->program_page_size = p_device->program_page_size;
    p_info->erase_block_size = p_device->erase_block_size;
    p_info->permissions = p_zone->permissions;
    return FAL_RESULT_SUCCESS;
}

fal_result_t fal_read(fal_t *p_fal,
                      fal_zone_id_t zone_id,
                      uint32_t offset,
                      uint32_t length,
                      uint8_t *p_data)
{
    const fal_zone_cfg_t *p_zone = NULL;      /* Validated read zone. */
    const fal_device_cfg_t *p_device = NULL;  /* Device containing the read zone. */
    fal_result_t result = FAL_RESULT_SUCCESS; /* Request validation result. */
    uint32_t device_offset = 0u;              /* Address accumulated from preceding zones. */

    if ((p_data == NULL) && (length != 0u))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    result = request_validate(p_fal,
                              zone_id,
                              offset,
                              length,
                              FAL_ZONE_PERMISSION_READ,
                              &p_zone,
                              &p_device,
                              &device_offset);
    if (result != FAL_RESULT_SUCCESS)
    {
        return result;
    }
    if (length == 0u)
    {
        p_fal->result = FAL_RESULT_SUCCESS;
        return FAL_RESULT_SUCCESS;
    }

    request_begin(p_fal,
                  p_device,
                  FAL_OPERATION_READ,
                  FAL_STATE_READ,
                  device_offset + offset,
                  length);
    p_fal->p_read_data = p_data;
    return FAL_RESULT_IN_PROGRESS;
}

fal_result_t fal_write(fal_t *p_fal,
                       fal_zone_id_t zone_id,
                       uint32_t offset,
                       uint32_t length,
                       const uint8_t *p_data)
{
    const fal_zone_cfg_t *p_zone = NULL;      /* Validated program zone. */
    const fal_device_cfg_t *p_device = NULL;  /* Device containing the program zone. */
    fal_result_t result = FAL_RESULT_SUCCESS; /* Request validation result. */
    uint32_t device_offset = 0u;              /* Address accumulated from preceding zones. */

    if ((p_data == NULL) && (length != 0u))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    result = request_validate(p_fal,
                              zone_id,
                              offset,
                              length,
                              FAL_ZONE_PERMISSION_WRITE,
                              &p_zone,
                              &p_device,
                              &device_offset);
    if (result != FAL_RESULT_SUCCESS)
    {
        return result;
    }
    if (length == 0u)
    {
        p_fal->result = FAL_RESULT_SUCCESS;
        return FAL_RESULT_SUCCESS;
    }

    request_begin(p_fal,
                  p_device,
                  FAL_OPERATION_WRITE,
                  FAL_STATE_WRITE,
                  device_offset + offset,
                  length);
    p_fal->p_write_data = p_data;
    return FAL_RESULT_IN_PROGRESS;
}

fal_result_t fal_erase(fal_t *p_fal,
                       fal_zone_id_t zone_id,
                       uint32_t offset,
                       uint32_t length)
{
    const fal_zone_cfg_t *p_zone = NULL;      /* Validated erase zone. */
    const fal_device_cfg_t *p_device = NULL;  /* Device containing the erase zone. */
    fal_result_t result = FAL_RESULT_SUCCESS; /* Request validation result. */
    uint32_t erase_offset = 0u;               /* Zone-relative first erase block address. */
    uint32_t erase_end = 0u;                  /* Zone-relative exclusive erase end. */
    uint32_t requested_end = 0u;              /* Zone-relative exclusive caller request end. */
    uint32_t device_offset = 0u;              /* Address accumulated from preceding zones. */

    result = request_validate(p_fal,
                              zone_id,
                              offset,
                              length,
                              FAL_ZONE_PERMISSION_ERASE,
                              &p_zone,
                              &p_device,
                              &device_offset);
    if (result != FAL_RESULT_SUCCESS)
    {
        return result;
    }
    if (length == 0u)
    {
        p_fal->result = FAL_RESULT_SUCCESS;
        return FAL_RESULT_SUCCESS;
    }

    requested_end = offset + length;
    erase_offset = offset - (offset % p_device->erase_block_size);
    erase_end = requested_end;
    if ((erase_end % p_device->erase_block_size) != 0u)
    {
        erase_end += p_device->erase_block_size - (erase_end % p_device->erase_block_size);
    }
    if (erase_end > p_zone->size)
    {
        return FAL_RESULT_OUT_OF_RANGE;
    }

    request_begin(p_fal,
                  p_device,
                  FAL_OPERATION_ERASE,
                  FAL_STATE_ERASE,
                  device_offset + erase_offset,
                  erase_end - erase_offset);
    return FAL_RESULT_IN_PROGRESS;
}

uint8_t fal_is_busy(const fal_t *p_fal)
{
    if (p_fal == NULL)
    {
        return 0u;
    }
    return (p_fal->operation != FAL_OPERATION_NONE) ? 1u : 0u;
}

fal_state_t fal_state_get(const fal_t *p_fal)
{
    return (p_fal == NULL) ? FAL_STATE_ERROR : p_fal->state;
}

fal_result_t fal_result_get(const fal_t *p_fal)
{
    return (p_fal == NULL) ? FAL_RESULT_INVALID_ARGUMENT : p_fal->result;
}

fal_result_t fal_stop_request(fal_t *p_fal)
{
    if (p_fal == NULL)
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    if ((p_fal->state == FAL_STATE_UNINITIALIZED) ||
        (p_fal->state == FAL_STATE_ERROR))
    {
        return FAL_RESULT_CONFIG_ERROR;
    }
    p_fal->stop_requested = 1u;
    if (fal_is_busy(p_fal) == 0u)
    {
        p_fal->state = FAL_STATE_STOPPED;
    }
    return FAL_RESULT_SUCCESS;
}

uint8_t fal_is_stopped(const fal_t *p_fal)
{
    return ((p_fal != NULL) && (p_fal->state == FAL_STATE_STOPPED)) ? 1u : 0u;
}
