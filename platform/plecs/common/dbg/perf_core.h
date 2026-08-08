// SPDX-License-Identifier: MIT
/**
 * @file    perf_core.h
 * @brief   Perf backend public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define perf base and record objects shared by code, task, and interrupt instrumentation
 *          - Provide pure-software record definitions and instrumentation macros
 *          - Expose explicit initialization, registration, run, iteration, and metric APIs
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
#ifndef __PERF_CORE_H__
#define __PERF_CORE_H__

#include <stdint.h>

#define PERF_TASK_ENABLE 1u
#define PERF_INTERRUPT_ENABLE 1u
#define PERF_CODE_ENABLE 1u

#if ((PERF_TASK_ENABLE != 0u) && (PERF_TASK_ENABLE != 1u))
#error "PERF_TASK_ENABLE must be 0 or 1."
#endif

#if ((PERF_INTERRUPT_ENABLE != 0u) && (PERF_INTERRUPT_ENABLE != 1u))
#error "PERF_INTERRUPT_ENABLE must be 0 or 1."
#endif

#if ((PERF_CODE_ENABLE != 0u) && (PERF_CODE_ENABLE != 1u))
#error "PERF_CODE_ENABLE must be 0 or 1."
#endif

#define PERF_ENABLE ((PERF_TASK_ENABLE == 1u) || \
                     (PERF_INTERRUPT_ENABLE == 1u) || \
                     (PERF_CODE_ENABLE == 1u))

typedef struct perf_core_record perf_core_record_t;

typedef enum
{
    PERF_CORE_RECORD_CODE = 0,
    PERF_CORE_RECORD_TASK,
    PERF_CORE_RECORD_INTERRUPT,
} perf_core_record_type_t;

typedef struct
{
    volatile uint32_t *p_cnt;
    float cnt_period_s;
} perf_core_base_t;

struct perf_core_record
{
    const char *p_name;
    uint32_t start;
    uint32_t end;
    uint32_t time;
    uint32_t max_time;
    uint32_t run_time;
    uint32_t start_to_start_time; /* Raw counts between consecutive task-entry timestamps. */
    uint32_t end_to_start_time;   /* Raw counts from the previous task exit to the next entry. */
    uint32_t period_us;
    float load;
    float load_max;
    uint16_t record_id;
    uint8_t record_type;
    uint8_t start_valid; /* Indicates that start contains a previous task-entry timestamp. */
    uint8_t end_valid;   /* Indicates that end contains a previous task-exit timestamp. */
    volatile uint32_t **p_cnt;
};

#ifndef PERF_CPU_LOAD_PERIOD_MS
#define PERF_CPU_LOAD_PERIOD_MS 500UL
#endif

#ifndef PERF_COUNT_UNIT_US
#define PERF_COUNT_UNIT_US 0.5f
#endif

#define PERF_REPORT_UNIT_NS 100u /* Time quantum used by the Perf text report. */

typedef void *(*perf_core_list_first_f)(void *p_context, void **pp_cursor);
typedef void *(*perf_core_list_next_f)(void *p_context, void **pp_cursor);

typedef struct
{
    void *p_context;
    perf_core_list_first_f p_first;
    perf_core_list_next_f p_next;
} perf_core_list_t;

void perf_core_init(const perf_core_list_t *p_list,
                    const perf_core_base_t *p_base,
                    uint32_t system_tick,
                    uint32_t system_tick_unit_us);
void perf_core_run(const perf_core_list_t *p_list, uint32_t system_tick);
uint32_t perf_core_base_cnt_get(void);
uint8_t perf_core_base_is_ready(void);
float perf_core_count_period_s_get(void);
float perf_core_count_unit_us_get(void);
/**
 * @brief Get the raw Perf counter period rounded to nanoseconds.
 * @return Raw hardware counter period in nanoseconds.
 */
uint32_t perf_core_count_unit_ns_get(void);
uint32_t perf_core_cnt_per_sys_tick_get(void);
float perf_core_task_metric_get(void);
float perf_core_task_metric_max_get(void);
float perf_core_interrupt_metric_get(void);
float perf_core_interrupt_metric_max_get(void);
uint32_t perf_core_dict_version_get(void);
uint16_t perf_core_record_count_get(const perf_core_list_t *p_list);
uint16_t perf_core_record_count_by_type(const perf_core_list_t *p_list, uint8_t record_type);
uint32_t perf_core_count_to_us(uint32_t count);
/**
 * @brief Convert a raw Perf counter difference to 100 ns report units.
 * @param[in] count Raw hardware counter difference.
 * @return Converted duration in 100 ns units, saturated to UINT32_MAX.
 */
uint32_t perf_core_count_to_100ns(uint32_t count);
uint32_t perf_core_task_period_us_get(perf_core_record_t *p_record);
void perf_core_reset_peak_value(const perf_core_list_t *p_list);
uint32_t perf_core_task_begin(perf_core_record_t *p_record);
void perf_core_task_end(perf_core_record_t *p_record, uint32_t start_cnt);
void perf_core_task_period_set(perf_core_record_t *p_record, uint32_t period_us);
uint32_t perf_core_interrupt_begin(perf_core_record_t *p_record);
void perf_core_interrupt_end(perf_core_record_t *p_record, uint32_t start_cnt);

#endif /* __PERF_CORE_H__ */
