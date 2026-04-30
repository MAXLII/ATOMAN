// SPDX-License-Identifier: MIT
/**
 * @file    record_dict.c
 * @brief   record_dict library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Assign stable record ids within a dictionary version
 *          - Count and find records through caller-provided list and type callbacks
 *          - Provide reusable filtering helpers for dictionary-based reporting services
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "record_dict.h"

#include <stddef.h>

void record_dict_init(record_dict_t *dict, uint32_t version)
{
    if (dict == NULL)
    {
        return;
    }

    dict->version = version;
    dict->next_id = 1u;
}

/* Id 0 is reserved, so allocation starts at 1 and saturates at UINT16_MAX. */
uint16_t record_dict_alloc_id(record_dict_t *dict)
{
    uint16_t id;

    if (dict == NULL)
    {
        return 0u;
    }

    id = dict->next_id;
    if ((dict->next_id != 0u) && (dict->next_id != UINT16_MAX))
    {
        ++dict->next_id;
    }

    return id;
}

uint8_t record_dict_filter_is_valid(uint8_t type_filter, uint8_t type_max)
{
    return (type_filter <= type_max) ? 1u : 0u;
}

uint8_t record_dict_match(uint8_t record_type, uint8_t type_filter, uint8_t type_all)
{
    /* The caller defines which filter value represents "all records". */
    if (type_filter == type_all)
    {
        return 1u;
    }

    return (record_type == type_filter) ? 1u : 0u;
}

uint16_t record_dict_count(void *first,
                           uint8_t type_filter,
                           uint8_t type_all,
                           record_dict_next_get_f next_get,
                           record_dict_type_get_f type_get)
{
    uint16_t count = 0u;

    if ((next_get == NULL) || (type_get == NULL))
    {
        return 0u;
    }

    for (void *record = first; record != NULL; record = next_get(record))
    {
        /* Saturate instead of wrapping when the list is unexpectedly large. */
        if ((record_dict_match(type_get(record), type_filter, type_all) != 0u) &&
            (count != UINT16_MAX))
        {
            ++count;
        }
    }

    return count;
}

void *record_dict_find_next(void *record,
                            uint8_t type_filter,
                            uint8_t type_all,
                            record_dict_next_get_f next_get,
                            record_dict_type_get_f type_get)
{
    if ((next_get == NULL) || (type_get == NULL))
    {
        return NULL;
    }

    while (record != NULL)
    {
        if (record_dict_match(type_get(record), type_filter, type_all) != 0u)
        {
            return record;
        }
        record = next_get(record);
    }

    return NULL;
}
