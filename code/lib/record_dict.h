// SPDX-License-Identifier: MIT
/**
 * @file    record_dict.h
 * @brief   record_dict library public interface.
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
#ifndef __RECORD_DICT_H__
#define __RECORD_DICT_H__

#include <stdint.h>

/* Callback used to walk an application-owned record list. */
typedef void *(*record_dict_next_get_f)(void *record);

/* Callback used to read the protocol-facing type from an application record. */
typedef uint8_t (*record_dict_type_get_f)(void *record);

typedef struct
{
    /* Dictionary version. Bump it when record count/order/type/name changes. */
    uint32_t version;

    /* Next allocatable id. Id 0 is reserved as invalid. */
    uint16_t next_id;
} record_dict_t;

/* Initialize a dictionary context with a caller-owned version value. */
void record_dict_init(record_dict_t *dict, uint32_t version);

/* Allocate a stable non-zero record id for the current dictionary version. */
uint16_t record_dict_alloc_id(record_dict_t *dict);

/* Validate that a type filter is within the caller-defined protocol range. */
uint8_t record_dict_filter_is_valid(uint8_t type_filter, uint8_t type_max);

/* Match one record type against a filter; type_all means wildcard. */
uint8_t record_dict_match(uint8_t record_type, uint8_t type_filter, uint8_t type_all);

/* Count records that match a filter without knowing the concrete record type. */
uint16_t record_dict_count(void *first,
                           uint8_t type_filter,
                           uint8_t type_all,
                           record_dict_next_get_f next_get,
                           record_dict_type_get_f type_get);

/* Find the next record that matches a filter in a caller-owned list. */
void *record_dict_find_next(void *record,
                            uint8_t type_filter,
                            uint8_t type_all,
                            record_dict_next_get_f next_get,
                            record_dict_type_get_f type_get);

#endif
