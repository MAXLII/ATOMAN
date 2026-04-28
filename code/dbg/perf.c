#include "perf.h"

#include "comm.h"
#include "section.h"
#include "shell.h"

#include <stddef.h>
#include <string.h>

#ifndef PERF_CNT_PER_SECTION_SYS_TICK
#define PERF_CNT_PER_SECTION_SYS_TICK 200UL
#endif

#ifndef PERF_CPU_LOAD_PERIOD_MS
#define PERF_CPU_LOAD_PERIOD_MS 500UL
#endif

#ifndef PERF_COUNT_UNIT_US
#define PERF_COUNT_UNIT_US 0.5f
#endif

#define CMD_SET_PERF 0x01u
#define CMD_WORD_PERF_INFO_QUERY 0x20u
#define CMD_WORD_PERF_SUMMARY_QUERY 0x21u
#define CMD_WORD_PERF_RECORD_LIST_QUERY 0x22u
#define CMD_WORD_PERF_RECORD_ITEM_REPORT 0x23u
#define CMD_WORD_PERF_RECORD_LIST_END 0x24u
#define CMD_WORD_PERF_RESET_PEAK 0x25u

#define PERF_PROTOCOL_VERSION 0x0001u
#define PERF_PULL_NAME_MAX 48u

#define PERF_PULL_TYPE_ALL 0u
#define PERF_PULL_TYPE_TASK 1u
#define PERF_PULL_TYPE_INTERRUPT 2u
#define PERF_PULL_TYPE_CODE 3u

#define PERF_PULL_REJECT_NONE 0u
#define PERF_PULL_REJECT_BUSY 1u
#define PERF_PULL_REJECT_INVALID_TYPE 2u
#define PERF_PULL_REJECT_NO_PERF_BASE 3u
#define PERF_PULL_REJECT_NO_RECORD 4u

#define PERF_PULL_END_OK 0u
#define PERF_PULL_END_OVERFLOW 2u
#define PERF_PULL_END_INTERNAL_ERROR 3u

#define PERF_INFO_FLAG_RESET_PEAK (1u << 0)
#define PERF_INFO_FLAG_TYPE_FILTER (1u << 1)
#define PERF_INFO_FLAGS (PERF_INFO_FLAG_RESET_PEAK | PERF_INFO_FLAG_TYPE_FILTER)

#pragma pack(push, 1)
typedef struct
{
    uint16_t protocol_version;
    uint16_t record_count;
    float unit_us;
    uint32_t cnt_per_sys_tick;
    uint32_t cpu_window_ms;
    uint8_t flags;
    uint8_t reserved[3];
} perf_info_ack_t;

typedef struct
{
    float task_load_percent;
    float task_peak_percent;
    float int_load_percent;
    float int_peak_percent;
} perf_summary_ack_t;

typedef struct
{
    uint8_t type_filter;
    uint8_t reserved[3];
} perf_record_list_query_t;

typedef struct
{
    uint8_t accepted;
    uint8_t type_filter;
    uint16_t record_count;
    uint32_t sequence;
    uint8_t reject_reason;
    uint8_t reserved[3];
} perf_record_list_ack_t;

typedef struct
{
    uint32_t sequence;
    uint16_t index;
    uint16_t record_count;
    uint8_t record_type;
    uint8_t name_len;
    uint16_t reserved;
    uint32_t time_us;
    uint32_t max_time_us;
    uint32_t period_us;
    float load_percent;
    float peak_percent;
    char name[PERF_PULL_NAME_MAX];
} perf_record_item_report_t;

typedef struct
{
    uint32_t sequence;
    uint16_t record_count;
    uint8_t status;
    uint8_t reserved;
} perf_record_list_end_t;

typedef struct
{
    uint8_t success;
    uint8_t reserved[3];
} perf_simple_ack_t;
#pragma pack(pop)

typedef struct
{
    uint8_t active;
    uint8_t type_filter;
    uint16_t record_count;
    uint16_t index;
    uint32_t sequence;
    section_perf_record_t *cur;
    uint8_t status;
    DEC_MY_PRINTF;
    uint8_t src;
    uint8_t d_src;
    uint8_t dst;
    uint8_t d_dst;
} perf_pull_ctx_t;

section_perf_record_t *p_perf_record_first = NULL;
static uint32_t *s_perf_cnt = NULL;
static float s_perf_task_metric = 0.0f;
static float s_perf_task_metric_max = 0.0f;
static float s_perf_interrupt_metric = 0.0f;
static float s_perf_interrupt_metric_max = 0.0f;
static uint32_t s_perf_metric_last_sys_tick = 0u;
static perf_pull_ctx_t s_perf_pull_ctx = {0};
static uint32_t s_perf_pull_sequence = 0u;

extern reg_task_t *p_task_first;

static void perf_insert(section_perf_t *perf)
{
    section_perf_base_t *base;
    section_perf_record_t *rec;
    static section_perf_record_t *s_perf_record_tail = NULL;

    if (!perf)
        return;

    switch (perf->perf_type)
    {
    case SECTION_PERF_BASE:
        base = (section_perf_base_t *)perf->p_perf;
        if (base && base->p_cnt)
            s_perf_cnt = base->p_cnt;
        break;

    case SECTION_PERF_RECORD:
        rec = (section_perf_record_t *)perf->p_perf;
        if (rec)
        {
            rec->p_cnt = &s_perf_cnt;
            rec->p_next = NULL;

            if (!p_perf_record_first)
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
    s_perf_task_metric = 0.0f;
    s_perf_task_metric_max = 0.0f;
    s_perf_interrupt_metric = 0.0f;
    s_perf_interrupt_metric_max = 0.0f;
    s_perf_metric_last_sys_tick = SECTION_SYS_TICK;

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

static uint8_t perf_protocol_type_is_valid(uint8_t type_filter)
{
    return (type_filter <= PERF_PULL_TYPE_CODE) ? 1u : 0u;
}

static uint8_t perf_record_type_to_protocol(uint8_t record_type)
{
    switch (record_type)
    {
    case SECTION_PERF_RECORD_TASK:
        return PERF_PULL_TYPE_TASK;
    case SECTION_PERF_RECORD_INTERRUPT:
        return PERF_PULL_TYPE_INTERRUPT;
    case SECTION_PERF_RECORD_CODE:
        return PERF_PULL_TYPE_CODE;
    default:
        return 0u;
    }
}

static uint8_t perf_record_match_filter(section_perf_record_t *p, uint8_t type_filter)
{
    if (p == NULL)
    {
        return 0u;
    }

    if (type_filter == PERF_PULL_TYPE_ALL)
    {
        return 1u;
    }

    return (perf_record_type_to_protocol(p->record_type) == type_filter) ? 1u : 0u;
}

static uint32_t perf_record_count_by_type(uint8_t record_type, uint8_t filter_enabled)
{
    uint32_t count = 0u;

    for (section_perf_record_t *p = p_perf_record_first; p != NULL; p = (section_perf_record_t *)p->p_next)
    {
        if (!filter_enabled || p->record_type == record_type)
        {
            ++count;
        }
    }

    return count;
}

static uint16_t perf_record_count_by_filter(uint8_t type_filter)
{
    uint16_t count = 0u;

    for (section_perf_record_t *p = p_perf_record_first; p != NULL; p = (section_perf_record_t *)p->p_next)
    {
        if (perf_record_match_filter(p, type_filter))
        {
            if (count != UINT16_MAX)
            {
                ++count;
            }
        }
    }

    return count;
}

static section_perf_record_t *perf_find_next_record(section_perf_record_t *p, uint8_t type_filter)
{
    while (p != NULL)
    {
        if (perf_record_match_filter(p, type_filter))
        {
            return p;
        }
        p = (section_perf_record_t *)p->p_next;
    }

    return NULL;
}

static uint32_t perf_task_period_us_get(section_perf_record_t *record)
{
    if (record == NULL)
    {
        return 0u;
    }

    for (reg_task_t *task = p_task_first; task != NULL; task = (reg_task_t *)task->p_next)
    {
        if (task->p_perf_record == record)
        {
            return (uint32_t)((float)(task->t_period * PERF_CNT_PER_SECTION_SYS_TICK) * PERF_COUNT_UNIT_US);
        }
    }

    return 0u;
}

static void perf_reset_peak_value(void)
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

static void perf_send_response(section_packform_t *p_req,
                               uint8_t cmd_word,
                               uint8_t is_ack,
                               uint8_t *p_data,
                               uint16_t len,
                               DEC_MY_PRINTF)
{
    section_packform_t pack = {0};

    if (p_req == NULL)
    {
        return;
    }

    pack.src = p_req->dst;
    pack.d_src = p_req->d_dst;
    pack.dst = p_req->src;
    pack.d_dst = p_req->d_src;
    pack.cmd_set = CMD_SET_PERF;
    pack.cmd_word = cmd_word;
    pack.is_ack = is_ack;
    pack.len = len;
    pack.p_data = p_data;

    comm_send_data(&pack, my_printf);
}

static void perf_send_pull_pack(uint8_t cmd_word, uint8_t *p_data, uint16_t len)
{
    section_packform_t pack = {0};

    pack.src = s_perf_pull_ctx.src;
    pack.d_src = s_perf_pull_ctx.d_src;
    pack.dst = s_perf_pull_ctx.dst;
    pack.d_dst = s_perf_pull_ctx.d_dst;
    pack.cmd_set = CMD_SET_PERF;
    pack.cmd_word = cmd_word;
    pack.is_ack = 0u;
    pack.len = len;
    pack.p_data = p_data;

    comm_send_data(&pack, s_perf_pull_ctx.my_printf);
}

static void perf_send_pull_end(uint8_t status)
{
    perf_record_list_end_t end_pack = {0};

    end_pack.sequence = s_perf_pull_ctx.sequence;
    end_pack.record_count = s_perf_pull_ctx.index;
    end_pack.status = status;
    perf_send_pull_pack(CMD_WORD_PERF_RECORD_LIST_END, (uint8_t *)&end_pack, (uint16_t)sizeof(end_pack));
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
    elapsed_perf_cnt = elapsed_sys_tick * PERF_CNT_PER_SECTION_SYS_TICK;
    if (elapsed_perf_cnt == 0u)
    {
        return;
    }

    for (section_perf_record_t *p = p_perf_record_first; p != NULL; p = (section_perf_record_t *)p->p_next)
    {
        switch (p->record_type)
        {
        case SECTION_PERF_RECORD_TASK:
            p->load = (float)p->run_time / (float)elapsed_perf_cnt;
            if (p->load > p->load_max)
            {
                p->load_max = p->load;
            }
            task_run_time += p->run_time;
            p->run_time = 0u;
            break;

        case SECTION_PERF_RECORD_INTERRUPT:
            p->load = (float)p->run_time / (float)elapsed_perf_cnt;
            if (p->load > p->load_max)
            {
                p->load_max = p->load;
            }
            interrupt_run_time += p->run_time;
            p->run_time = 0u;
            break;

        default:
            break;
        }
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

static void CPU_Utilization(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("TASK CPU Load:%f%%,TASK CPU Peak:%f%%\r\n",
                         (double)(s_perf_task_metric * 100.0f),
                         (double)(s_perf_task_metric_max * 100.0f));
    my_printf->my_printf("INT CPU Load:%f%%,INT CPU Peak:%f%%\r\n",
                         (double)(s_perf_interrupt_metric * 100.0f),
                         (double)(s_perf_interrupt_metric_max * 100.0f));
}

static void perf_summary(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("PERF_SUMMARY task_load=%f task_peak=%f int_load=%f int_peak=%f\r\n",
                         (double)(s_perf_task_metric * 100.0f),
                         (double)(s_perf_task_metric_max * 100.0f),
                         (double)(s_perf_interrupt_metric * 100.0f),
                         (double)(s_perf_interrupt_metric_max * 100.0f));
}

static void perf_info(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("PERF_INFO unit_us=%f cnt_per_sys_tick=%lu cpu_window_ms=%lu record_count=%lu\r\n",
                         (double)PERF_COUNT_UNIT_US,
                         (unsigned long)PERF_CNT_PER_SECTION_SYS_TICK,
                         (unsigned long)PERF_CPU_LOAD_PERIOD_MS,
                         (unsigned long)perf_record_count_by_type(0u, 0u));
}

static void perf_reset_peak(DEC_MY_PRINTF)
{
    perf_reset_peak_value();

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("PERF_RESET_OK\r\n");
    }
}

static const char *perf_record_type_name(uint8_t record_type)
{
    switch (record_type)
    {
    case SECTION_PERF_RECORD_TASK:
        return "TASK";
    case SECTION_PERF_RECORD_INTERRUPT:
        return "INT";
    case SECTION_PERF_RECORD_CODE:
        return "CODE";
    default:
        return "UNKNOWN";
    }
}

static void print_perf_record_item(section_perf_record_t *p, DEC_MY_PRINTF)
{
    if ((p == NULL) || (my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    if (p->record_type == SECTION_PERF_RECORD_CODE)
    {
        my_printf->my_printf("%s\t%s\t%u\t%u\t-\t-\r\n",
                             perf_record_type_name(p->record_type),
                             p->p_name,
                             (unsigned)(p->time * PERF_COUNT_UNIT_US),
                             (unsigned)(p->max_time * PERF_COUNT_UNIT_US));
    }
    else
    {
        my_printf->my_printf("%s\t%s\t%u\t%u\t%f\t%f\r\n",
                             perf_record_type_name(p->record_type),
                             p->p_name,
                             (unsigned)(p->time * PERF_COUNT_UNIT_US),
                             (unsigned)(p->max_time * PERF_COUNT_UNIT_US),
                             (double)(p->load * 100.0f),
                             (double)(p->load_max * 100.0f));
    }
}

static void print_perf_record_by_type(uint8_t record_type, DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("PERF_BEGIN type=%s count=%lu unit_us=%f\r\n",
                         perf_record_type_name(record_type),
                         (unsigned long)perf_record_count_by_type(record_type, 1u),
                         (double)PERF_COUNT_UNIT_US);
    my_printf->my_printf("Type\tPerf Name\tTime(us)\tMax(us)\tLoad(%%)\tPeak(%%)\r\n");
    for (section_perf_record_t *p = p_perf_record_first; p != NULL; p = (section_perf_record_t *)p->p_next)
    {
        if (p->record_type == record_type)
        {
            print_perf_record_item(p, my_printf);
        }
    }
    my_printf->my_printf("PERF_END\r\n");
}

static void print_perf_task_record(DEC_MY_PRINTF)
{
    print_perf_record_by_type(SECTION_PERF_RECORD_TASK, my_printf);
}

static void print_perf_interrupt_record(DEC_MY_PRINTF)
{
    print_perf_record_by_type(SECTION_PERF_RECORD_INTERRUPT, my_printf);
}

static void print_perf_code_record(DEC_MY_PRINTF)
{
    print_perf_record_by_type(SECTION_PERF_RECORD_CODE, my_printf);
}

void print_perf_record(DEC_MY_PRINTF)
{
    if (!my_printf)
        return;

    section_perf_record_t *p = p_perf_record_first;
    my_printf->my_printf("PERF_BEGIN type=ALL count=%lu unit_us=%f\r\n",
                         (unsigned long)perf_record_count_by_type(0u, 0u),
                         (double)PERF_COUNT_UNIT_US);
    my_printf->my_printf("Type\tPerf Name\tTime(us)\tMax(us)\tLoad(%%)\tPeak(%%)\r\n");
    while (p)
    {
        print_perf_record_item(p, my_printf);
        p = (section_perf_record_t *)p->p_next;
    }
    my_printf->my_printf("PERF_END\r\n");
}

typedef struct
{
    section_perf_record_t *cur;
    DEC_MY_PRINTF;
    uint8_t active;
    uint32_t perf_max_name_len;
} perf_print_ctx_t;

static perf_print_ctx_t g_perf_print_ctx = {0};

void print_perf_record_start(DEC_MY_PRINTF)
{
    if (!my_printf || !my_printf->my_printf)
        return;

    if (g_perf_print_ctx.active)
    {
        my_printf->my_printf("PERF_BUSY\r\n");
        return;
    }

    g_perf_print_ctx.cur = p_perf_record_first;
    g_perf_print_ctx.my_printf = my_printf;
    g_perf_print_ctx.active = 1;

    int max_len = 0;
    for (section_perf_record_t *s = p_perf_record_first; s; s = s->p_next)
    {
        int len = (int)strlen(s->p_name);
        if (len > max_len)
            max_len = len;
    }

    g_perf_print_ctx.perf_max_name_len = (uint32_t)max_len;

    int name_len = (int)strlen("Perf Name");
    int tab_size = 8;
    int tab_count = ((int)g_perf_print_ctx.perf_max_name_len / tab_size) - (name_len / tab_size) + 1;

    my_printf->my_printf("PERF_BEGIN type=ALL count=%lu unit_us=%f cnt_per_sys_tick=%lu cpu_window_ms=%lu\r\n",
                         (unsigned long)perf_record_count_by_type(0u, 0u),
                         (double)PERF_COUNT_UNIT_US,
                         (unsigned long)PERF_CNT_PER_SECTION_SYS_TICK,
                         (unsigned long)PERF_CPU_LOAD_PERIOD_MS);
    my_printf->my_printf("Type\tPerf Name");
    for (int i = 0; i < tab_count; i++)
        my_printf->my_printf("\t");
    my_printf->my_printf("Time(us)\tMax(us)\tLoad(%%)\tPeak(%%)\r\n");
}

int print_perf_record_step(void)
{
    if (!g_perf_print_ctx.active)
        return 0;

    section_perf_record_t *p = g_perf_print_ctx.cur;
    if (!p)
    {
        if (g_perf_print_ctx.my_printf && g_perf_print_ctx.my_printf->my_printf)
        {
            g_perf_print_ctx.my_printf->my_printf("PERF_END\r\n");
        }
        g_perf_print_ctx.active = 0;
        return 0;
    }

    int name_len = (int)strlen(p->p_name);
    int tab_size = 8;
    int tab_count = ((int)g_perf_print_ctx.perf_max_name_len / tab_size) - (name_len / tab_size) + 1;

    g_perf_print_ctx.my_printf->my_printf("%s\t%s", perf_record_type_name(p->record_type), p->p_name);
    for (int i = 0; i < tab_count; i++)
        g_perf_print_ctx.my_printf->my_printf("\t");
    if (p->record_type == SECTION_PERF_RECORD_CODE)
    {
        g_perf_print_ctx.my_printf->my_printf("%u\t%u\t-\t-\r\n",
                                              (unsigned)(p->time * PERF_COUNT_UNIT_US),
                                              (unsigned)(p->max_time * PERF_COUNT_UNIT_US));
    }
    else
    {
        g_perf_print_ctx.my_printf->my_printf("%u\t%u\t%f\t%f\r\n",
                                              (unsigned)(p->time * PERF_COUNT_UNIT_US),
                                              (unsigned)(p->max_time * PERF_COUNT_UNIT_US),
                                              (double)(p->load * 100.0f),
                                              (double)(p->load_max * 100.0f));
    }
    g_perf_print_ctx.cur = (section_perf_record_t *)p->p_next;
    return 1;
}

static void perf_pull_report_step(void)
{
    section_perf_record_t *p;
    perf_record_item_report_t item = {0};
    size_t name_len;

    if (s_perf_pull_ctx.active == 0u)
    {
        return;
    }

    p = perf_find_next_record(s_perf_pull_ctx.cur, s_perf_pull_ctx.type_filter);
    if (p == NULL)
    {
        perf_send_pull_end((s_perf_pull_ctx.index == s_perf_pull_ctx.record_count) ? PERF_PULL_END_OK : PERF_PULL_END_INTERNAL_ERROR);
        s_perf_pull_ctx.active = 0u;
        return;
    }

    item.sequence = s_perf_pull_ctx.sequence;
    item.index = s_perf_pull_ctx.index;
    item.record_count = s_perf_pull_ctx.record_count;
    item.record_type = perf_record_type_to_protocol(p->record_type);
    item.time_us = (uint32_t)((float)p->time * PERF_COUNT_UNIT_US);
    item.max_time_us = (uint32_t)((float)p->max_time * PERF_COUNT_UNIT_US);
    item.period_us = 0u;

    if (p->record_type == SECTION_PERF_RECORD_CODE)
    {
        item.load_percent = 0.0f;
        item.peak_percent = 0.0f;
    }
    else
    {
        item.load_percent = p->load * 100.0f;
        item.peak_percent = p->load_max * 100.0f;
        if (p->record_type == SECTION_PERF_RECORD_TASK)
        {
            item.period_us = perf_task_period_us_get(p);
        }
    }

    name_len = (p->p_name != NULL) ? strlen(p->p_name) : 0u;
    if (name_len > PERF_PULL_NAME_MAX)
    {
        name_len = PERF_PULL_NAME_MAX;
    }
    item.name_len = (uint8_t)name_len;
    if (name_len > 0u)
    {
        memcpy(item.name, p->p_name, name_len);
    }

    perf_send_pull_pack(CMD_WORD_PERF_RECORD_ITEM_REPORT,
                        (uint8_t *)&item,
                        (uint16_t)(offsetof(perf_record_item_report_t, name) + name_len));

    s_perf_pull_ctx.cur = (section_perf_record_t *)p->p_next;
    ++s_perf_pull_ctx.index;

    if (s_perf_pull_ctx.index >= s_perf_pull_ctx.record_count)
    {
        perf_send_pull_end(PERF_PULL_END_OK);
        s_perf_pull_ctx.active = 0u;
    }
}

static void perf_info_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_info_ack_t ack = {0};

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    ack.protocol_version = PERF_PROTOCOL_VERSION;
    ack.record_count = perf_record_count_by_filter(PERF_PULL_TYPE_ALL);
    ack.unit_us = PERF_COUNT_UNIT_US;
    ack.cnt_per_sys_tick = PERF_CNT_PER_SECTION_SYS_TICK;
    ack.cpu_window_ms = PERF_CPU_LOAD_PERIOD_MS;
    ack.flags = PERF_INFO_FLAGS;

    perf_send_response(p_pack, CMD_WORD_PERF_INFO_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

static void perf_summary_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_summary_ack_t ack = {0};

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    ack.task_load_percent = s_perf_task_metric * 100.0f;
    ack.task_peak_percent = s_perf_task_metric_max * 100.0f;
    ack.int_load_percent = s_perf_interrupt_metric * 100.0f;
    ack.int_peak_percent = s_perf_interrupt_metric_max * 100.0f;

    perf_send_response(p_pack, CMD_WORD_PERF_SUMMARY_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

static void perf_record_list_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_record_list_ack_t ack = {0};
    uint8_t type_filter;
    uint16_t record_count;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    type_filter = (p_pack->len >= sizeof(perf_record_list_query_t)) ? ((perf_record_list_query_t *)p_pack->p_data)->type_filter : 0xFFu;
    ack.type_filter = type_filter;

    if (s_perf_pull_ctx.active != 0u)
    {
        ack.accepted = 0u;
        ack.reject_reason = PERF_PULL_REJECT_BUSY;
        perf_send_response(p_pack, CMD_WORD_PERF_RECORD_LIST_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
        return;
    }

    if (perf_protocol_type_is_valid(type_filter) == 0u)
    {
        ack.accepted = 0u;
        ack.reject_reason = PERF_PULL_REJECT_INVALID_TYPE;
        perf_send_response(p_pack, CMD_WORD_PERF_RECORD_LIST_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
        return;
    }

    if (s_perf_cnt == NULL)
    {
        ack.accepted = 0u;
        ack.reject_reason = PERF_PULL_REJECT_NO_PERF_BASE;
        perf_send_response(p_pack, CMD_WORD_PERF_RECORD_LIST_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
        return;
    }

    record_count = perf_record_count_by_filter(type_filter);
    if (record_count == 0u)
    {
        ack.accepted = 0u;
        ack.reject_reason = PERF_PULL_REJECT_NO_RECORD;
        perf_send_response(p_pack, CMD_WORD_PERF_RECORD_LIST_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
        return;
    }

    ++s_perf_pull_sequence;
    if (s_perf_pull_sequence == 0u)
    {
        ++s_perf_pull_sequence;
    }

    s_perf_pull_ctx.active = 1u;
    s_perf_pull_ctx.type_filter = type_filter;
    s_perf_pull_ctx.record_count = record_count;
    s_perf_pull_ctx.index = 0u;
    s_perf_pull_ctx.sequence = s_perf_pull_sequence;
    s_perf_pull_ctx.cur = p_perf_record_first;
    s_perf_pull_ctx.status = PERF_PULL_END_OK;
    s_perf_pull_ctx.my_printf = my_printf;
    s_perf_pull_ctx.src = p_pack->dst;
    s_perf_pull_ctx.d_src = p_pack->d_dst;
    s_perf_pull_ctx.dst = p_pack->src;
    s_perf_pull_ctx.d_dst = p_pack->d_src;

    ack.accepted = 1u;
    ack.record_count = record_count;
    ack.sequence = s_perf_pull_ctx.sequence;
    ack.reject_reason = PERF_PULL_REJECT_NONE;
    perf_send_response(p_pack, CMD_WORD_PERF_RECORD_LIST_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

static void perf_reset_peak_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_simple_ack_t ack = {0};

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    perf_reset_peak_value();
    ack.success = 1u;
    perf_send_response(p_pack, CMD_WORD_PERF_RESET_PEAK, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

static void perf_print_task(void)
{
    if (g_perf_print_ctx.active)
        (void)print_perf_record_step();

    perf_pull_report_step();
}

REG_INIT(0, perf_init)
REG_TASK_MS(PERF_CPU_LOAD_PERIOD_MS, perf_cpu_load_calculate)
REG_TASK_MS(5, perf_print_task)
REG_COMM(CMD_SET_PERF, CMD_WORD_PERF_INFO_QUERY, perf_info_query_act)
REG_COMM(CMD_SET_PERF, CMD_WORD_PERF_SUMMARY_QUERY, perf_summary_query_act)
REG_COMM(CMD_SET_PERF, CMD_WORD_PERF_RECORD_LIST_QUERY, perf_record_list_query_act)
REG_COMM(CMD_SET_PERF, CMD_WORD_PERF_RESET_PEAK, perf_reset_peak_act)
REG_SHELL_CMD(perf_print_record, print_perf_record_start)
REG_SHELL_CMD(perf_print_task, print_perf_task_record)
REG_SHELL_CMD(perf_print_interrupt, print_perf_interrupt_record)
REG_SHELL_CMD(perf_print_code, print_perf_code_record)
REG_SHELL_CMD(CPU_Utilization, CPU_Utilization)
REG_SHELL_CMD(perf_summary, perf_summary)
REG_SHELL_CMD(perf_info, perf_info)
REG_SHELL_CMD(perf_reset_peak, perf_reset_peak)
REG_SHELL_VAR(TASK_METRIC, s_perf_task_metric, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TASK_METRIC_MAX, s_perf_task_metric_max, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(INTERRUPT_METRIC, s_perf_interrupt_metric, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(INTERRUPT_METRIC_MAX, s_perf_interrupt_metric_max, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
