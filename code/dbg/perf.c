// SPDX-License-Identifier: MIT
/**
 * @file    perf.c
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
 * @date    2026-04-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "perf.h"

#include "record_dict.h"
#include "section.h"

section_perf_record_t *p_perf_record_first = NULL;
uint32_t perf_dict_version = 1u;
static record_dict_t s_perf_dict;
static volatile uint32_t *s_perf_cnt = NULL;
static float s_perf_cnt_period_s = PERF_COUNT_UNIT_US * 1.0e-6f;
static float s_perf_task_metric = 0.0f;
static float s_perf_task_metric_max = 0.0f;
static float s_perf_interrupt_metric = 0.0f;
static float s_perf_interrupt_metric_max = 0.0f;
static uint32_t s_perf_metric_last_sys_tick = 0u;
static volatile section_perf_record_t *s_running_task_perf_record = NULL;
static volatile uint32_t s_running_task_interrupt_time = 0u;

static void perf_insert(section_perf_t *perf)
{
    section_perf_base_t *base;
    section_perf_record_t *rec;
    static section_perf_record_t *s_perf_record_tail = NULL;

    if (perf == NULL)
    {
        return;
    }

    switch (perf->perf_type)
    {
    case SECTION_PERF_BASE:
        base = (section_perf_base_t *)perf->p_perf;
        if ((base != NULL) && (base->p_cnt != NULL) && (base->cnt_period_s > 0.0f))
        {
            s_perf_cnt = base->p_cnt;
            s_perf_cnt_period_s = base->cnt_period_s;
        }
        break;

    case SECTION_PERF_RECORD:
        rec = (section_perf_record_t *)perf->p_perf;
        if (rec != NULL)
        {
            rec->p_cnt = &s_perf_cnt;
            rec->p_next = NULL;
            rec->record_id = record_dict_alloc_id(&s_perf_dict);

            if (p_perf_record_first == NULL)
            {
                p_perf_record_first = rec;
                s_perf_record_tail = rec;
            }
            else
            {
                s_perf_record_tail->p_next = rec;
                s_perf_record_tail = rec;
            }
        }
        break;

    default:
        break;
    }
}

static void perf_init(void)
{
    p_perf_record_first = NULL;
    s_perf_cnt = NULL;
    s_perf_cnt_period_s = PERF_COUNT_UNIT_US * 1.0e-6f;
    s_perf_task_metric = 0.0f;
    s_perf_task_metric_max = 0.0f;
    s_perf_interrupt_metric = 0.0f;
    s_perf_interrupt_metric_max = 0.0f;
    s_perf_metric_last_sys_tick = SECTION_SYS_TICK;
    s_running_task_perf_record = NULL;
    s_running_task_interrupt_time = 0u;
    perf_dict_version = 1u;
    record_dict_init(&s_perf_dict, perf_dict_version);

    for (reg_section_t *p = (reg_section_t *)&SECTION_START;
         p < (reg_section_t *)&SECTION_STOP;
         ++p)
    {
        switch (p->section_type)
        {
        case SECTION_PERF:
            perf_insert((section_perf_t *)p->p_str);
            break;
        default:
            break;
        }
    }
}

uint32_t perf_base_cnt_get(void)
{
    return (s_perf_cnt != NULL) ? *s_perf_cnt : 0u;
}

uint8_t perf_base_is_ready(void)
{
    return (s_perf_cnt != NULL) ? 1u : 0u;
}

float perf_count_period_s_get(void)
{
    return s_perf_cnt_period_s;
}

float perf_count_unit_us_get(void)
{
    return s_perf_cnt_period_s * 1.0e6f;
}

uint32_t perf_cnt_per_sys_tick_get(void)
{
    const float cnt_per_tick = ((float)SECTION_SYS_TICK_UNIT_US * 1.0e-6f) / s_perf_cnt_period_s;

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

uint32_t section_perf_task_begin(section_perf_record_t *record)
{
    if (record == NULL)
    {
        return 0u;
    }

    s_running_task_interrupt_time = 0u;
    s_running_task_perf_record = record;

    return perf_cnt_read(record->p_cnt);
}

void section_perf_task_end(section_perf_record_t *record, uint32_t start_cnt)
{
    uint32_t delta;
    uint32_t interrupt_time;

    if (record == NULL)
    {
        return;
    }

    delta = (uint32_t)(perf_cnt_read(record->p_cnt) - start_cnt);
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
}

void section_perf_task_period_set(section_perf_record_t *record, uint32_t period_us)
{
    if (record != NULL)
    {
        record->period_us = period_us;
    }
}

uint32_t section_perf_interrupt_begin(section_perf_record_t *record)
{
    if (record == NULL)
    {
        return 0u;
    }

    return perf_cnt_read(record->p_cnt);
}

void section_perf_interrupt_end(section_perf_record_t *record, uint32_t start_cnt)
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

float perf_task_metric_get(void)
{
    return s_perf_task_metric;
}

float perf_task_metric_max_get(void)
{
    return s_perf_task_metric_max;
}

float perf_interrupt_metric_get(void)
{
    return s_perf_interrupt_metric;
}

float perf_interrupt_metric_max_get(void)
{
    return s_perf_interrupt_metric_max;
}

uint32_t perf_dict_version_get(void)
{
    return s_perf_dict.version;
}

uint16_t perf_record_count_get(void)
{
    uint16_t count = 0u;

    for (section_perf_record_t *p = p_perf_record_first; p != NULL; p = (section_perf_record_t *)p->p_next)
    {
        if (count != UINT16_MAX)
        {
            ++count;
        }
    }

    return count;
}

uint16_t perf_record_count_by_type(uint8_t record_type)
{
    uint16_t count = 0u;

    for (section_perf_record_t *p = p_perf_record_first; p != NULL; p = (section_perf_record_t *)p->p_next)
    {
        if (p->record_type == record_type)
        {
            if (count != UINT16_MAX)
            {
                ++count;
            }
        }
    }

    return count;
}

uint32_t perf_count_to_us(uint32_t count)
{
    return (uint32_t)(((float)count * perf_count_unit_us_get()) + 0.5f);
}

uint32_t perf_task_period_us_get(section_perf_record_t *record)
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

void perf_reset_peak_value(void)
{
    for (section_perf_record_t *p = p_perf_record_first; p != NULL; p = (section_perf_record_t *)p->p_next)
    {
        p->max_time = 0u;
        if ((p->record_type == SECTION_PERF_RECORD_TASK) ||
            (p->record_type == SECTION_PERF_RECORD_INTERRUPT))
        {
            p->load_max = 0.0f;
        }
    }

    s_perf_task_metric_max = 0.0f;
    s_perf_interrupt_metric_max = 0.0f;
}

static void perf_cpu_load_calculate(void)
{
    uint32_t now;
    uint32_t elapsed_sys_tick;
    uint32_t elapsed_perf_cnt;
    uint32_t task_run_time = 0u;
    uint32_t interrupt_run_time = 0u;

    now = SECTION_SYS_TICK;
    elapsed_sys_tick = (uint32_t)(now - s_perf_metric_last_sys_tick);
    if (elapsed_sys_tick == 0u)
    {
        return;
    }

    s_perf_metric_last_sys_tick = now;
    elapsed_perf_cnt = elapsed_sys_tick * perf_cnt_per_sys_tick_get();
    if (elapsed_perf_cnt == 0u)
    {
        return;
    }

    for (section_perf_record_t *p = p_perf_record_first; p != NULL; p = (section_perf_record_t *)p->p_next)
    {
        p->load = (float)p->run_time / (float)elapsed_perf_cnt;
        if (p->load > p->load_max)
        {
            p->load_max = p->load;
        }

        switch (p->record_type)
        {
        case SECTION_PERF_RECORD_TASK:
            task_run_time += p->run_time;
            break;

        case SECTION_PERF_RECORD_INTERRUPT:
            interrupt_run_time += p->run_time;
            break;

        default:
            break;
        }
        p->run_time = 0u;
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

REG_INIT(0, perf_init)
REG_TASK_MS(PERF_CPU_LOAD_PERIOD_MS, perf_cpu_load_calculate)
