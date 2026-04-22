#ifndef __PERF_SECTION_H__
#define __PERF_SECTION_H__

#include <stdint.h>

typedef enum
{
    SECTION_PERF_RECORD = 0,
    SECTION_PERF_BASE,
} SECTION_PERF_E;

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
    uint32_t **p_cnt;
    void *p_next;
} section_perf_record_t;

extern section_perf_record_t *p_perf_record_first;

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

#define REG_PERF_RECORD(name)                                  \
    section_perf_record_t section_perf_record_##name = {       \
        .p_name = #name,                                       \
        .start = 0,                                            \
        .end = 0,                                              \
        .time = 0,                                             \
        .reserved = 0,                                         \
        .max_time = 0,                                         \
        .p_cnt = NULL,                                         \
        .p_next = NULL,                                        \
    };                                                         \
    section_perf_t section_perf_record_##name##_perf = {       \
        .perf_type = SECTION_PERF_RECORD,                      \
        .p_perf = (void *)&section_perf_record_##name,         \
    };                                                         \
    REG_SECTION_FUNC(SECTION_PERF, section_perf_record_##name##_perf)

#else

#define PERF_START(name)
#define PERF_END(name)
#define P_RECORD_PERF(name) NULL
#define REG_PERF_RECORD(name)

#endif

#endif
