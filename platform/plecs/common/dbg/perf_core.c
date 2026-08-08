// SPDX-License-Identifier: MIT
/**
 * @file    perf_core.c
 * @brief   Perf backend module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Collect perf base counters and perf records registered through linker sections
 *          - Assign stable record ids and maintain the static dictionary version used by Perf Viewer
 *          - Calculate 500 ms task and interrupt CPU-load windows from accumulated run time
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-08-02
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "perf_core.h"

#include "record_dict.h"

#include <stddef.h>

static record_dict_t s_perf_dict;
static volatile uint32_t *s_perf_cnt = NULL;
static float s_perf_cnt_period_s = PERF_COUNT_UNIT_US * 1.0e-6f;
static float s_perf_task_metric = 0.0f;
static float s_perf_task_metric_max = 0.0f;
static float s_perf_interrupt_metric = 0.0f;
static float s_perf_interrupt_metric_max = 0.0f;
static uint32_t s_perf_metric_last_sys_tick = 0u;
static uint32_t s_perf_system_tick_unit_us = 100u;
static volatile perf_core_record_t *s_running_task_perf_record = NULL;
static volatile uint32_t s_running_task_interrupt_time = 0u;

typedef struct
{
    const perf_core_list_t *p_list;
    void *p_cursor;
} perf_core_iterator_t;

static perf_core_record_t *perf_core_list_begin(const perf_core_list_t *p_list,
                                                perf_core_iterator_t *p_iterator)
{
    if (p_iterator == NULL)
    {
        return NULL;
    }

    p_iterator->p_list = p_list;
    p_iterator->p_cursor = NULL;
    if ((p_list == NULL) || (p_list->p_first == NULL))
    {
        return NULL;
    }

    return (perf_core_record_t *)p_list->p_first(p_list->p_context,
                                                 &p_iterator->p_cursor);
}

static perf_core_record_t *perf_core_list_next(perf_core_iterator_t *p_iterator)
{
    if ((p_iterator == NULL) ||
        (p_iterator->p_list == NULL) ||
        (p_iterator->p_list->p_next == NULL) ||
        (p_iterator->p_cursor == NULL))
    {
        return NULL;
    }

    return (perf_core_record_t *)p_iterator->p_list->p_next(
        p_iterator->p_list->p_context,
        &p_iterator->p_cursor);
}

void perf_core_init(const perf_core_list_t *p_list,
                    const perf_core_base_t *p_base,
                    uint32_t system_tick,
                    uint32_t system_tick_unit_us)
{
    s_perf_cnt = NULL;
    s_perf_cnt_period_s = PERF_COUNT_UNIT_US * 1.0e-6f;
    s_perf_task_metric = 0.0f;
    s_perf_task_metric_max = 0.0f;
    s_perf_interrupt_metric = 0.0f;
    s_perf_interrupt_metric_max = 0.0f;
    s_perf_metric_last_sys_tick = system_tick;
    s_perf_system_tick_unit_us = system_tick_unit_us;
    s_running_task_perf_record = NULL;
    s_running_task_interrupt_time = 0u;
    record_dict_init(&s_perf_dict, 1u);

    if ((p_base != NULL) && (p_base->p_cnt != NULL) && (p_base->cnt_period_s > 0.0f))
    {
        s_perf_cnt = p_base->p_cnt;
        s_perf_cnt_period_s = p_base->cnt_period_s;
    }

    perf_core_iterator_t iterator = {0};
    perf_core_record_t *p_record = perf_core_list_begin(p_list, &iterator);
    while (p_record != NULL)
    {
        p_record->p_cnt = &s_perf_cnt;
        p_record->record_id = record_dict_alloc_id(&s_perf_dict);
        p_record = perf_core_list_next(&iterator);
    }
}

uint32_t perf_core_base_cnt_get(void)
{
    return (s_perf_cnt != NULL) ? *s_perf_cnt : 0u;
}

uint8_t perf_core_base_is_ready(void)
{
    return (s_perf_cnt != NULL) ? 1u : 0u;
}

float perf_core_count_period_s_get(void)
{
    return s_perf_cnt_period_s;
}

float perf_core_count_unit_us_get(void)
{
    return s_perf_cnt_period_s * 1.0e6f;
}

uint32_t perf_core_count_unit_ns_get(void)
{
    const float count_unit_ns = s_perf_cnt_period_s * 1.0e9f; /* Raw hardware counter period in nanoseconds. */

    if (count_unit_ns <= 0.0f)
    {
        return 0u;
    }

    if (count_unit_ns >= (float)UINT32_MAX)
    {
        return UINT32_MAX;
    }

    return (uint32_t)(count_unit_ns + 0.5f);
}

uint32_t perf_core_cnt_per_sys_tick_get(void)
{
    const float cnt_per_tick = ((float)s_perf_system_tick_unit_us * 1.0e-6f) / s_perf_cnt_period_s;

    if (cnt_per_tick <= 0.0f)
    {
        return 0u;
    }

    return (uint32_t)(cnt_per_tick + 0.5f);
}

static inline uint32_t perf_cnt_read(volatile uint32_t *const *pp_cnt)
{
    if ((pp_cnt == NULL) || (*pp_cnt == NULL))
    {
        return 0u;
    }

    return **pp_cnt;
}

uint32_t perf_core_task_begin(perf_core_record_t *record)
{
    uint32_t now = 0u; /* Raw counter captured at this task entry. */

    if (record == NULL)
    {
        return 0u;
    }

    now = perf_cnt_read(record->p_cnt);
    if (record->start_valid != 0u)
    {
        record->start_to_start_time = (uint32_t)(now - record->start);
    }
    if (record->end_valid != 0u)
    {
        record->end_to_start_time = (uint32_t)(now - record->end);
    }
    record->start = now;
    record->start_valid = 1u;

    s_running_task_interrupt_time = 0u;
    s_running_task_perf_record = record;

    return now;
}

void perf_core_task_end(perf_core_record_t *record, uint32_t start_cnt)
{
    uint32_t delta;
    uint32_t end_cnt = 0u; /* Raw counter captured at this task exit. */
    uint32_t interrupt_time;

    if (record == NULL)
    {
        return;
    }

    end_cnt = perf_cnt_read(record->p_cnt);
    delta = (uint32_t)(end_cnt - start_cnt);
    interrupt_time = s_running_task_interrupt_time;
    s_running_task_perf_record = NULL;
    s_running_task_interrupt_time = 0u;

    if (delta > interrupt_time)
    {
        delta -= interrupt_time;
    }
    else
    {
        delta = 0u;
    }

    record->time = delta;
    record->max_time = (delta > record->max_time) ? delta : record->max_time;
    record->run_time += delta;
    record->end = end_cnt;
    record->end_valid = 1u;
}

void perf_core_task_period_set(perf_core_record_t *record, uint32_t period_us)
{
    if (record != NULL)
    {
        record->period_us = period_us;
    }
}

uint32_t perf_core_interrupt_begin(perf_core_record_t *record)
{
    if (record == NULL)
    {
        return 0u;
    }

    return perf_cnt_read(record->p_cnt);
}

void perf_core_interrupt_end(perf_core_record_t *record, uint32_t start_cnt)
{
    uint32_t delta;

    if (record == NULL)
    {
        return;
    }

    delta = (uint32_t)(perf_cnt_read(record->p_cnt) - start_cnt);
    record->time = delta;
    record->max_time = (delta > record->max_time) ? delta : record->max_time;
    record->run_time += delta;

    if (s_running_task_perf_record != NULL)
    {
        s_running_task_interrupt_time += delta;
    }
}

float perf_core_task_metric_get(void)
{
    return s_perf_task_metric;
}

float perf_core_task_metric_max_get(void)
{
    return s_perf_task_metric_max;
}

float perf_core_interrupt_metric_get(void)
{
    return s_perf_interrupt_metric;
}

float perf_core_interrupt_metric_max_get(void)
{
    return s_perf_interrupt_metric_max;
}

uint32_t perf_core_dict_version_get(void)
{
    return s_perf_dict.version;
}

uint16_t perf_core_record_count_get(const perf_core_list_t *p_list)
{
    uint16_t count = 0u;

    perf_core_iterator_t iterator = {0};
    perf_core_record_t *p = perf_core_list_begin(p_list, &iterator);
    while (p != NULL)
    {
        if (count != UINT16_MAX)
        {
            ++count;
        }
        p = perf_core_list_next(&iterator);
    }

    return count;
}

uint16_t perf_core_record_count_by_type(const perf_core_list_t *p_list, uint8_t record_type)
{
    uint16_t count = 0u;

    perf_core_iterator_t iterator = {0};
    perf_core_record_t *p = perf_core_list_begin(p_list, &iterator);
    while (p != NULL)
    {
        if (p->record_type == record_type)
        {
            if (count != UINT16_MAX)
            {
                ++count;
            }
        }
        p = perf_core_list_next(&iterator);
    }

    return count;
}

uint32_t perf_core_count_to_us(uint32_t count)
{
    return (uint32_t)(((float)count * perf_core_count_unit_us_get()) + 0.5f);
}

uint32_t perf_core_count_to_100ns(uint32_t count)
{
    const double ticks_100ns = ((double)count * (double)s_perf_cnt_period_s) * 1.0e7; /* Report-unit value. */

    if (ticks_100ns <= 0.0)
    {
        return 0u;
    }

    if (ticks_100ns >= (double)UINT32_MAX)
    {
        return UINT32_MAX;
    }

    return (uint32_t)(ticks_100ns + 0.5);
}

uint32_t perf_core_task_period_us_get(perf_core_record_t *record)
{
    if (record == NULL)
    {
        return 0u;
    }

    if (record->period_us != 0u)
    {
        return record->period_us;
    }

    return 0u;
}

void perf_core_reset_peak_value(const perf_core_list_t *p_list)
{
    perf_core_iterator_t iterator = {0};
    perf_core_record_t *p = perf_core_list_begin(p_list, &iterator);
    while (p != NULL)
    {
        p->max_time = 0u;
        if ((p->record_type == PERF_CORE_RECORD_TASK) ||
            (p->record_type == PERF_CORE_RECORD_INTERRUPT))
        {
            p->load_max = 0.0f;
        }
        p = perf_core_list_next(&iterator);
    }

    s_perf_task_metric_max = 0.0f;
    s_perf_interrupt_metric_max = 0.0f;
}

void perf_core_run(const perf_core_list_t *p_list, uint32_t system_tick)
{
    uint32_t now;
    uint32_t elapsed_sys_tick;
    uint32_t elapsed_perf_cnt;
    uint32_t task_run_time = 0u;
    uint32_t interrupt_run_time = 0u;

    now = system_tick;
    elapsed_sys_tick = (uint32_t)(now - s_perf_metric_last_sys_tick);
    if (elapsed_sys_tick == 0u)
    {
        return;
    }

    s_perf_metric_last_sys_tick = now;
    elapsed_perf_cnt = elapsed_sys_tick * perf_core_cnt_per_sys_tick_get();
    if (elapsed_perf_cnt == 0u)
    {
        return;
    }

    perf_core_iterator_t iterator = {0};
    perf_core_record_t *p = perf_core_list_begin(p_list, &iterator);
    while (p != NULL)
    {
        p->load = (float)p->run_time / (float)elapsed_perf_cnt;
        if (p->load > p->load_max)
        {
            p->load_max = p->load;
        }

        switch (p->record_type)
        {
        case PERF_CORE_RECORD_TASK:
            task_run_time += p->run_time;
            break;

        case PERF_CORE_RECORD_INTERRUPT:
            interrupt_run_time += p->run_time;
            break;

        default:
            break;
        }
        p->run_time = 0u;
        p = perf_core_list_next(&iterator);
    }

    s_perf_task_metric = (float)task_run_time / (float)elapsed_perf_cnt;
    if (s_perf_task_metric > s_perf_task_metric_max)
    {
        s_perf_task_metric_max = s_perf_task_metric;
    }

    s_perf_interrupt_metric = (float)interrupt_run_time / (float)elapsed_perf_cnt;
    if (s_perf_interrupt_metric > s_perf_interrupt_metric_max)
    {
        s_perf_interrupt_metric_max = s_perf_interrupt_metric;
    }
}
