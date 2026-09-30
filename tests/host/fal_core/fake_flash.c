// SPDX-License-Identifier: MIT
/**
 * @file    fake_flash.c
 * @brief   Host fake flash driver implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Execute physical reads, programs, and erases against test memory
 *          - Expose controllable asynchronous busy and failure behavior
 *          - Capture every accepted operation for address and chunk verification
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Test-only, single-threaded implementation
 *          - Hardware access is replaced by an in-memory model
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

#include "fake_flash.h"

#include <stddef.h>
#include <string.h>

static fal_result_t next_result_take(fake_flash_t *p_flash)
{
    fal_result_t result = p_flash->next_result; /* One-shot injected driver result. */

    p_flash->next_result = FAL_RESULT_SUCCESS;
    return result;
}

static fal_result_t call_record(fake_flash_t *p_flash,
                                fake_flash_call_type_t type,
                                uint32_t address,
                                uint32_t length)
{
    if ((address > FAKE_FLASH_CAPACITY) ||
        (length > (FAKE_FLASH_CAPACITY - address)) ||
        (p_flash->call_count >= FAKE_FLASH_MAX_CALLS))
    {
        return FAL_RESULT_DRIVER_ERROR;
    }

    p_flash->calls[p_flash->call_count].type = type;
    p_flash->calls[p_flash->call_count].address = address;
    p_flash->calls[p_flash->call_count].length = length;
    p_flash->call_count++;
    p_flash->busy_polls_remaining = p_flash->busy_polls_per_operation;
    return next_result_take(p_flash);
}

static fal_result_t fake_init(void *p_context)
{
    fake_flash_t *p_flash = (fake_flash_t *)p_context; /* Fake device being initialized. */

    if (p_flash == NULL)
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    p_flash->initialized = 1u;
    return FAL_RESULT_SUCCESS;
}

static fal_device_state_t fake_state_get(void *p_context)
{
    fake_flash_t *p_flash = (fake_flash_t *)p_context; /* Fake device queried by FAL. */

    if ((p_flash == NULL) || (p_flash->initialized == 0u))
    {
        return FAL_DEVICE_STATE_ERROR;
    }
    if (p_flash->busy_polls_remaining != 0u)
    {
        p_flash->busy_polls_remaining--;
        return FAL_DEVICE_STATE_BUSY;
    }
    return FAL_DEVICE_STATE_READY;
}

static fal_result_t fake_read(void *p_context,
                              uint32_t address,
                              uint32_t length,
                              uint8_t *p_data)
{
    fake_flash_t *p_flash = (fake_flash_t *)p_context; /* Fake device supplying read data. */
    fal_result_t result = FAL_RESULT_SUCCESS;          /* Read call acceptance result. */

    if ((p_flash == NULL) || (p_data == NULL))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    result = call_record(p_flash, FAKE_FLASH_CALL_READ, address, length);
    if (result == FAL_RESULT_SUCCESS)
    {
        (void)memcpy(p_data, &p_flash->data[address], length);
    }
    return result;
}

static fal_result_t fake_program(void *p_context,
                                 uint32_t address,
                                 uint32_t length,
                                 const uint8_t *p_data)
{
    fake_flash_t *p_flash = (fake_flash_t *)p_context; /* Fake device accepting program data. */
    fal_result_t result = FAL_RESULT_SUCCESS;          /* Program call acceptance result. */
    uint32_t index = 0u;                               /* Program byte index. */

    if ((p_flash == NULL) || (p_data == NULL))
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    result = call_record(p_flash, FAKE_FLASH_CALL_PROGRAM, address, length);
    if (result == FAL_RESULT_SUCCESS)
    {
        for (index = 0u; index < length; index++)
        {
            p_flash->data[address + index] &= p_data[index];
        }
    }
    return result;
}

static fal_result_t fake_erase(void *p_context, uint32_t address, uint32_t length)
{
    fake_flash_t *p_flash = (fake_flash_t *)p_context; /* Fake device accepting an erase. */
    fal_result_t result = FAL_RESULT_SUCCESS;          /* Erase call acceptance result. */

    if (p_flash == NULL)
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    result = call_record(p_flash, FAKE_FLASH_CALL_ERASE, address, length);
    if (result == FAL_RESULT_SUCCESS)
    {
        (void)memset(&p_flash->data[address], 0xFF, length);
    }
    return result;
}

static fal_result_t fake_sync(void *p_context)
{
    fake_flash_t *p_flash = (fake_flash_t *)p_context; /* Fake device accepting a sync. */

    if (p_flash == NULL)
    {
        return FAL_RESULT_INVALID_ARGUMENT;
    }
    return call_record(p_flash, FAKE_FLASH_CALL_SYNC, 0u, 0u);
}

void fake_flash_reset(fake_flash_t *p_flash)
{
    if (p_flash == NULL)
    {
        return;
    }
    (void)memset(p_flash, 0, sizeof(*p_flash));
    (void)memset(p_flash->data, 0xFF, sizeof(p_flash->data));
    p_flash->next_result = FAL_RESULT_SUCCESS;
}

fal_flash_ops_t fake_flash_ops_make(fake_flash_t *p_flash)
{
    fal_flash_ops_t ops = {
        .p_context = p_flash,
        .p_init = fake_init,
        .p_get_state = fake_state_get,
        .p_read = fake_read,
        .p_program = fake_program,
        .p_erase = fake_erase,
        .p_sync = fake_sync,
    }; /* Operation table mounted by the real FAL core. */

    return ops;
}
