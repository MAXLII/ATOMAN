// SPDX-License-Identifier: MIT
/**
 * @file    perf.h
 * @brief   Perf backend public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define perf base and record objects shared by code, task, and interrupt instrumentation
 *          - Provide registration macros that bind perf records into linker sections
 *          - Expose backend metrics, record ids, dictionary version, and peak-reset APIs to service layers
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
#ifndef __PERF_H__
#define __PERF_H__

#include <stdint.h>

typedef enum
{
    SECTION_PERF_RECORD = 0,
    SECTION_PERF_BASE,
} SECTION_PERF_E;

typedef enum
{
    SECTION_PERF_RECORD_CODE = 0,
    SECTION_PERF_RECORD_TASK,
    SECTION_PERF_RECORD_INTERRUPT,
} SECTION_PERF_RECORD_E;

typedef struct
{
    uint32_t *p_cnt;
} section_perf_base_t;

typedef struct
{
    uint32_t perf_type;
    void *p_perf;
} section_perf_t;

typedef struct
{
    const char *p_name;
    uint16_t start;
    uint16_t end;
    uint16_t time;
    uint16_t reserved;
    uint32_t max_time;
    uint32_t run_time;
    float load;
    float load_max;
    uint16_t record_id;
    uint8_t record_type;
    uint32_t **p_cnt;
    void *p_next;
} section_perf_record_t;

extern section_perf_record_t *p_perf_record_first;
extern uint32_t perf_dict_version;

#ifndef PERF_CNT_PER_SECTION_SYS_TICK
#define PERF_CNT_PER_SECTION_SYS_TICK 200UL
#endif

#ifndef PERF_CPU_LOAD_PERIOD_MS
#define PERF_CPU_LOAD_PERIOD_MS 500UL
#endif

#ifndef PERF_COUNT_UNIT_US
#define PERF_COUNT_UNIT_US 0.5f
#endif

uint32_t perf_base_cnt_get(void);
uint8_t perf_base_is_ready(void);
float perf_task_metric_get(void);
float perf_task_metric_max_get(void);
float perf_interrupt_metric_get(void);
float perf_interrupt_metric_max_get(void);
uint32_t perf_dict_version_get(void);
uint16_t perf_record_count_get(void);
uint16_t perf_record_count_by_type(uint8_t record_type);
uint32_t perf_count_to_us(uint32_t count);
uint32_t perf_task_period_us_get(section_perf_record_t *record);
void perf_reset_peak_value(void);

#define REG_PERF_BASE_CNT(timer_cnt)                \
    section_perf_base_t section_perf_base_timer = { \
        .p_cnt = (timer_cnt),                       \
    };                                              \
    section_perf_t section_timer_cnt_perf = {       \
        .perf_type = SECTION_PERF_BASE,             \
        .p_perf = (void *)&section_perf_base_timer, \
    };                                              \
    REG_SECTION_FUNC(SECTION_PERF, section_timer_cnt_perf)

#define PERF_RECORD_ENABLE 1

#if (PERF_RECORD_ENABLE == 1)

#define PERF_START(name)                                                           \
    do                                                                             \
    {                                                                              \
        if (*section_perf_record_##name.p_cnt != NULL)                             \
        {                                                                          \
            section_perf_record_##name.start = **section_perf_record_##name.p_cnt; \
        }                                                                          \
    } while (0)

#define PERF_END(name)                                                                                                       \
    do                                                                                                                       \
    {                                                                                                                        \
        if (*section_perf_record_##name.p_cnt != NULL)                                                                       \
        {                                                                                                                    \
            section_perf_record_##name.end = **section_perf_record_##name.p_cnt;                                             \
            section_perf_record_##name.time = (uint16_t)(section_perf_record_##name.end - section_perf_record_##name.start); \
            if (section_perf_record_##name.time > section_perf_record_##name.max_time)                                       \
            {                                                                                                                \
                section_perf_record_##name.max_time = section_perf_record_##name.time;                                       \
            }                                                                                                                \
        }                                                                                                                    \
    } while (0)

#define P_RECORD_PERF(name) ((section_perf_record_t *)&section_perf_record_##name)

#define REG_PERF_RECORD(name)                            \
    section_perf_record_t section_perf_record_##name = { \
        .p_name = #name,                                 \
        .start = 0,                                      \
        .end = 0,                                        \
        .time = 0,                                       \
        .reserved = 0,                                   \
        .max_time = 0,                                   \
        .run_time = 0,                                   \
        .load = 0.0f,                                    \
        .load_max = 0.0f,                                \
        .record_id = 0,                                  \
        .record_type = SECTION_PERF_RECORD_CODE,         \
        .p_cnt = NULL,                                   \
        .p_next = NULL,                                  \
    };                                                   \
    section_perf_t section_perf_record_##name##_perf = { \
        .perf_type = SECTION_PERF_RECORD,                \
        .p_perf = (void *)&section_perf_record_##name,   \
    };                                                   \
    REG_SECTION_FUNC(SECTION_PERF, section_perf_record_##name##_perf)

#define REG_TASK_PERF_RECORD(name)                       \
    section_perf_record_t section_perf_record_##name = { \
        .p_name = #name,                                 \
        .start = 0,                                      \
        .end = 0,                                        \
        .time = 0,                                       \
        .reserved = 0,                                   \
        .max_time = 0,                                   \
        .run_time = 0,                                   \
        .load = 0.0f,                                    \
        .load_max = 0.0f,                                \
        .record_id = 0,                                  \
        .record_type = SECTION_PERF_RECORD_TASK,         \
        .p_cnt = NULL,                                   \
        .p_next = NULL,                                  \
    };                                                   \
    section_perf_t section_perf_record_##name##_perf = { \
        .perf_type = SECTION_PERF_RECORD,                \
        .p_perf = (void *)&section_perf_record_##name,   \
    };                                                   \
    REG_SECTION_FUNC(SECTION_PERF, section_perf_record_##name##_perf)

#define REG_INTERRUPT_PERF_RECORD(name)                  \
    section_perf_record_t section_perf_record_##name = { \
        .p_name = #name,                                 \
        .start = 0,                                      \
        .end = 0,                                        \
        .time = 0,                                       \
        .reserved = 0,                                   \
        .max_time = 0,                                   \
        .run_time = 0,                                   \
        .load = 0.0f,                                    \
        .load_max = 0.0f,                                \
        .record_id = 0,                                  \
        .record_type = SECTION_PERF_RECORD_INTERRUPT,    \
        .p_cnt = NULL,                                   \
        .p_next = NULL,                                  \
    };                                                   \
    section_perf_t section_perf_record_##name##_perf = { \
        .perf_type = SECTION_PERF_RECORD,                \
        .p_perf = (void *)&section_perf_record_##name,   \
    };                                                   \
    REG_SECTION_FUNC(SECTION_PERF, section_perf_record_##name##_perf)

#else

#define PERF_START(name)
#define PERF_END(name)
#define P_RECORD_PERF(name) NULL
#define REG_PERF_RECORD(name)
#define REG_TASK_PERF_RECORD(name)
#define REG_INTERRUPT_PERF_RECORD(name)

#endif

#endif
