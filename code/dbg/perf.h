// SPDX-License-Identifier: MIT
/**
 * @file    perf.h
 * @brief   Perf Section adapter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Preserve existing Perf instrumentation and query APIs
 *          - Register counter bases and records in SECTION_PERF
 *          - Adapt the Section-owned record list to the portable Perf core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Timing calculations are implemented by perf_core.c
 *          - Protocol handling belongs to perf_service.c
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
#ifndef __PERF_H__
#define __PERF_H__

#include "perf_core.h"

typedef struct section_item section_item_t;

typedef perf_core_record_t section_perf_record_t;
typedef perf_core_base_t section_perf_base_t;

#define SECTION_PERF_RECORD_CODE PERF_CORE_RECORD_CODE
#define SECTION_PERF_RECORD_TASK PERF_CORE_RECORD_TASK
#define SECTION_PERF_RECORD_INTERRUPT PERF_CORE_RECORD_INTERRUPT

typedef enum
{
    SECTION_PERF_RECORD = 0,
    SECTION_PERF_BASE,
} SECTION_PERF_E;

typedef struct
{
    uint32_t perf_type;
    void *p_perf;
} section_perf_t;

extern section_item_t *p_perf_first;

section_perf_record_t *perf_record_from_item(const section_item_t *p_item);
uint32_t perf_base_cnt_get(void);
uint8_t perf_base_is_ready(void);
float perf_count_period_s_get(void);
float perf_count_unit_us_get(void);
uint32_t perf_count_unit_ns_get(void);
uint32_t perf_cnt_per_sys_tick_get(void);
float perf_task_metric_get(void);
float perf_task_metric_max_get(void);
float perf_interrupt_metric_get(void);
float perf_interrupt_metric_max_get(void);
uint32_t perf_dict_version_get(void);
uint16_t perf_record_count_get(void);
uint16_t perf_record_count_by_type(uint8_t record_type);
uint32_t perf_count_to_us(uint32_t count);
uint32_t perf_count_to_100ns(uint32_t count);
uint32_t perf_task_period_us_get(section_perf_record_t *p_record);
void perf_reset_peak_value(void);
uint32_t section_perf_task_begin(section_perf_record_t *p_record);
void section_perf_task_end(section_perf_record_t *p_record, uint32_t start_cnt);
void section_perf_task_period_set(section_perf_record_t *p_record, uint32_t period_us);
uint32_t section_perf_interrupt_begin(section_perf_record_t *p_record);
void section_perf_interrupt_end(section_perf_record_t *p_record, uint32_t start_cnt);

#if (PERF_ENABLE)
#define P_RECORD_PERF(name) ((section_perf_record_t *)&section_perf_record_##name)

#define PERF_RECORD_DEFINE_EX(name, _record_type)           \
    section_perf_record_t section_perf_record_##name = {    \
        .p_name = #name,                                    \
        .start = 0u,                                        \
        .end = 0u,                                          \
        .time = 0u,                                         \
        .max_time = 0u,                                     \
        .run_time = 0u,                                     \
        .start_to_start_time = 0u,                          \
        .end_to_start_time = 0u,                            \
        .period_us = 0u,                                    \
        .load = 0.0f,                                       \
        .load_max = 0.0f,                                   \
        .record_id = 0u,                                    \
        .record_type = (uint8_t)(_record_type),             \
        .start_valid = 0u,                                  \
        .end_valid = 0u,                                    \
        .p_cnt = NULL,                                      \
    };                                                      \
    section_perf_t section_perf_registration_##name = {     \
        .perf_type = (uint32_t)SECTION_PERF_RECORD,         \
        .p_perf = &section_perf_record_##name,               \
    };                                                      \
    REG_SECTION_FUNC(SECTION_PERF, section_perf_registration_##name)

#define REG_PERF_BASE_CNT(timer_cnt, period_s)              \
    section_perf_base_t section_perf_base_timer = {         \
        .p_cnt = (volatile uint32_t *)(timer_cnt),          \
        .cnt_period_s = (period_s),                         \
    };                                                      \
    section_perf_t section_perf_base_registration = {       \
        .perf_type = (uint32_t)SECTION_PERF_BASE,           \
        .p_perf = &section_perf_base_timer,                  \
    };                                                      \
    REG_SECTION_FUNC(SECTION_PERF, section_perf_base_registration)
#else
#define P_RECORD_PERF(name) NULL
#define PERF_RECORD_DEFINE_EX(name, _record_type)
#define REG_PERF_BASE_CNT(timer_cnt, period_s)
#endif

#if (PERF_CODE_ENABLE == 1u)
#define PERF_START(name)                                                           \
    do                                                                             \
    {                                                                              \
        if ((section_perf_record_##name.p_cnt != NULL) &&                          \
            (*section_perf_record_##name.p_cnt != NULL))                           \
        {                                                                          \
            section_perf_record_##name.start = **section_perf_record_##name.p_cnt; \
        }                                                                          \
    } while (0)

#define PERF_END(name)                                                                                  \
    do                                                                                                  \
    {                                                                                                   \
        if ((section_perf_record_##name.p_cnt != NULL) &&                                               \
            (*section_perf_record_##name.p_cnt != NULL))                                                \
        {                                                                                               \
            uint32_t perf_delta = 0u;                                                                   \
            section_perf_record_##name.end = **section_perf_record_##name.p_cnt;                        \
            perf_delta = (uint32_t)(section_perf_record_##name.end - section_perf_record_##name.start); \
            section_perf_record_##name.time = perf_delta;                                               \
            if (perf_delta > section_perf_record_##name.max_time)                                       \
            {                                                                                           \
                section_perf_record_##name.max_time = perf_delta;                                       \
            }                                                                                           \
            section_perf_record_##name.run_time += perf_delta;                                          \
        }                                                                                               \
    } while (0)

#define REG_PERF_RECORD(name) PERF_RECORD_DEFINE_EX(name, SECTION_PERF_RECORD_CODE)
#else
#define PERF_START(name) ((void)0)
#define PERF_END(name) ((void)0)
#define REG_PERF_RECORD(name)
#endif

#if (PERF_TASK_ENABLE == 1u)
#define REG_TASK_PERF_RECORD(name) PERF_RECORD_DEFINE_EX(name, SECTION_PERF_RECORD_TASK)
#else
#define REG_TASK_PERF_RECORD(name)
#endif

#if (PERF_INTERRUPT_ENABLE == 1u)
#define REG_INTERRUPT_PERF_RECORD(name) PERF_RECORD_DEFINE_EX(name, SECTION_PERF_RECORD_INTERRUPT)
#else
#define REG_INTERRUPT_PERF_RECORD(name)
#endif

#include "section.h"

#endif /* __PERF_H__ */
