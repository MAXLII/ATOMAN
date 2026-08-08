// SPDX-License-Identifier: MIT
/**
 * @file    perf.c
 * @brief   Perf Section adapter implementation.
 * @details
 *          This file is part of the base PLECS platform project.
 *
 *          Module responsibilities:
 *          - Discover SECTION_PERF registrations between Windows linker sentinels
 *          - Adapt section_item_t traversal to the portable Perf core
 *          - Preserve existing Perf query and instrumentation symbols
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Windows linker-section and PLECS counter access are isolated to this file
 *          - Protocol handling belongs to perf_service.c
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "perf.h"

#include <stddef.h>

section_item_t *p_perf_first = NULL;
static section_item_t *p_perf_tail = NULL;
static section_perf_base_t *p_perf_base = NULL;
REG_DBG_LIST(perf, p_perf_first)

section_perf_record_t *perf_record_from_item(const section_item_t *p_item)
{
    section_perf_t *p_registration = NULL;

    if ((p_item == NULL) || (p_item->p_obj == NULL))
    {
        return NULL;
    }

    p_registration = (section_perf_t *)p_item->p_obj;
    if ((p_registration->perf_type != (uint32_t)SECTION_PERF_RECORD) ||
        (p_registration->p_perf == NULL))
    {
        return NULL;
    }

    return (section_perf_record_t *)p_registration->p_perf;
}

static void *perf_list_first(void *p_context, void **pp_cursor)
{
    section_item_t *p_item = *(section_item_t **)p_context;

    *pp_cursor = p_item;
    return perf_record_from_item(p_item);
}

static void *perf_list_next(void *p_context, void **pp_cursor)
{
    section_item_t *p_item = (section_item_t *)*pp_cursor;

    (void)p_context;
    p_item = (p_item == NULL) ? NULL : p_item->p_next;
    *pp_cursor = p_item;
    return perf_record_from_item(p_item);
}

static const perf_core_list_t perf_list = {
    .p_context = &p_perf_first,
    .p_first = perf_list_first,
    .p_next = perf_list_next,
};

static void perf_init(void)
{
    const reg_section_t *p_section_first = NULL;
    const reg_section_t *p_section_last = NULL;

    p_perf_first = NULL;
    p_perf_tail = NULL;
    p_perf_base = NULL;

#if defined(SECTION_SENTINEL_REG_SECTION)
    extern const reg_section_t section_reg_start;
    extern const reg_section_t section_reg_stop;
    p_section_first = &section_reg_start + 1;
    p_section_last = &section_reg_stop;
#else
    p_section_first = (const reg_section_t *)&SECTION_START;
    p_section_last = (const reg_section_t *)&SECTION_STOP;
#endif

    for (const reg_section_t *p_section = p_section_first;
         p_section < p_section_last;
         ++p_section)
    {
        if (p_section->section_type == SECTION_PERF)
        {
            section_item_t *p_item = (section_item_t *)p_section->p_str;
            section_perf_t *p_registration = NULL;

            if ((p_item == NULL) || (p_item->p_obj == NULL))
            {
                continue;
            }

            p_registration = (section_perf_t *)p_item->p_obj;
            if (p_registration->perf_type == (uint32_t)SECTION_PERF_BASE)
            {
                p_perf_base = (section_perf_base_t *)p_registration->p_perf;
                continue;
            }
            if ((p_registration->perf_type != (uint32_t)SECTION_PERF_RECORD) ||
                (p_registration->p_perf == NULL))
            {
                continue;
            }

            p_item->p_next = NULL;
            if (p_perf_first == NULL)
            {
                p_perf_first = p_item;
            }
            else
            {
                p_perf_tail->p_next = p_item;
            }
            p_perf_tail = p_item;
        }
    }

    perf_core_init(&perf_list,
                   p_perf_base,
                   SECTION_SYS_TICK,
                   SECTION_SYS_TICK_UNIT_US);
}

static void perf_run(void)
{
    PLATFORM_PERF_COUNTER_REFRESH();
    perf_core_run(&perf_list, SECTION_SYS_TICK);
}

uint32_t perf_base_cnt_get(void)
{
    PLATFORM_PERF_COUNTER_REFRESH();
    return perf_core_base_cnt_get();
}
uint8_t perf_base_is_ready(void) { return perf_core_base_is_ready(); }
float perf_count_period_s_get(void) { return perf_core_count_period_s_get(); }
float perf_count_unit_us_get(void) { return perf_core_count_unit_us_get(); }
uint32_t perf_count_unit_ns_get(void) { return perf_core_count_unit_ns_get(); }
uint32_t perf_cnt_per_sys_tick_get(void) { return perf_core_cnt_per_sys_tick_get(); }
float perf_task_metric_get(void) { return perf_core_task_metric_get(); }
float perf_task_metric_max_get(void) { return perf_core_task_metric_max_get(); }
float perf_interrupt_metric_get(void) { return perf_core_interrupt_metric_get(); }
float perf_interrupt_metric_max_get(void) { return perf_core_interrupt_metric_max_get(); }
uint32_t perf_dict_version_get(void) { return perf_core_dict_version_get(); }
uint16_t perf_record_count_get(void) { return perf_core_record_count_get(&perf_list); }
uint16_t perf_record_count_by_type(uint8_t record_type)
{
    return perf_core_record_count_by_type(&perf_list, record_type);
}
uint32_t perf_count_to_us(uint32_t count) { return perf_core_count_to_us(count); }
uint32_t perf_count_to_100ns(uint32_t count) { return perf_core_count_to_100ns(count); }
uint32_t perf_task_period_us_get(section_perf_record_t *p_record)
{
    return perf_core_task_period_us_get(p_record);
}
void perf_reset_peak_value(void) { perf_core_reset_peak_value(&perf_list); }
uint32_t section_perf_task_begin(section_perf_record_t *p_record)
{
    PLATFORM_PERF_COUNTER_REFRESH();
    return perf_core_task_begin(p_record);
}
void section_perf_task_end(section_perf_record_t *p_record, uint32_t start_cnt)
{
    PLATFORM_PERF_COUNTER_REFRESH();
    perf_core_task_end(p_record, start_cnt);
}
void section_perf_task_period_set(section_perf_record_t *p_record, uint32_t period_us)
{
    perf_core_task_period_set(p_record, period_us);
}
uint32_t section_perf_interrupt_begin(section_perf_record_t *p_record)
{
    PLATFORM_PERF_COUNTER_REFRESH();
    return perf_core_interrupt_begin(p_record);
}
void section_perf_interrupt_end(section_perf_record_t *p_record, uint32_t start_cnt)
{
    PLATFORM_PERF_COUNTER_REFRESH();
    perf_core_interrupt_end(p_record, start_cnt);
}

REG_INIT(0, perf_init)
REG_TASK_MS(PERF_CPU_LOAD_PERIOD_MS, perf_run)
