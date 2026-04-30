// SPDX-License-Identifier: MIT
/**
 * @file    perf_service.c
 * @brief   Perf communication service module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Handle Perf Viewer binary commands on cmd_set 0x01 using the existing 0xE8 frame protocol
 *          - Send record dictionaries and packed sample batches without exposing backend enum values directly
 *          - Provide shell commands for local task, interrupt, code, summary, info, and peak-reset inspection
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
#include "perf_service.h"

#include "record_dict.h"
#include "shell.h"

#include <stddef.h>
#include <string.h>

extern reg_task_t *p_task_first;

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
    uint8_t success;
    uint8_t reserved[3];
} perf_reset_peak_ack_t;

typedef struct
{
    uint8_t type_filter;
    uint8_t reserved[3];
    uint32_t known_dict_version;
} perf_dict_query_t;

typedef struct
{
    uint8_t accepted;
    uint8_t type_filter;
    uint16_t record_count;
    uint32_t sequence;
    uint32_t dict_version;
    uint8_t reject_reason;
    uint8_t reserved[3];
} perf_dict_ack_t;

typedef struct
{
    uint32_t sequence;
    uint16_t index;
    uint16_t record_count;
    uint16_t record_id;
    uint8_t record_type;
    uint8_t name_len;
} perf_dict_item_header_t;

typedef struct
{
    uint32_t sequence;
    uint16_t record_count;
    uint8_t status;
    uint8_t reserved;
    uint32_t dict_version;
} perf_dict_end_t;

typedef struct
{
    uint8_t type_filter;
    uint8_t flags;
    uint16_t reserved;
    uint32_t dict_version;
} perf_sample_query_t;

typedef struct
{
    uint8_t accepted;
    uint8_t type_filter;
    uint16_t record_count;
    uint32_t sequence;
    uint32_t dict_version;
    uint8_t reject_reason;
    uint8_t reserved[3];
} perf_sample_ack_t;

typedef struct
{
    uint32_t sequence;
    uint16_t record_count;
    uint16_t item_count;
} perf_sample_batch_header_t;

typedef struct
{
    uint16_t record_id;
    uint32_t time_us;
    uint32_t max_time_us;
    uint32_t period_us;
    float load_percent;
    float peak_percent;
} perf_sample_task_item_t;

typedef struct
{
    uint16_t record_id;
    uint32_t time_us;
    uint32_t max_time_us;
    float load_percent;
    float peak_percent;
} perf_sample_interrupt_item_t;

typedef struct
{
    uint16_t record_id;
    uint32_t time_us;
    uint32_t max_time_us;
} perf_sample_code_item_t;

typedef struct
{
    uint32_t sequence;
    uint16_t record_count;
    uint8_t status;
    uint8_t reserved;
} perf_sample_end_t;
#pragma pack(pop)

static perf_opt_service_t s_perf_opt_service;
static uint32_t s_perf_opt_sequence = 0u;

typedef struct
{
    section_perf_record_t *cur;
    DEC_MY_PRINTF;
    uint8_t active;
    uint32_t perf_max_name_len;
} perf_text_print_ctx_t;

static perf_text_print_ctx_t s_perf_text_print_ctx = {0};
static float s_perf_service_task_metric = 0.0f;
static float s_perf_service_task_metric_max = 0.0f;
static float s_perf_service_interrupt_metric = 0.0f;
static float s_perf_service_interrupt_metric_max = 0.0f;

static void perf_service_print_step(void);

#define PERF_SERVICE_PROTOCOL_VERSION 0x0001u
#define PERF_SERVICE_INFO_FLAGS ((uint8_t)((1u << 0) | (1u << 1)))

static uint8_t perf_opt_filter_is_valid(uint8_t type_filter)
{
    return record_dict_filter_is_valid(type_filter, PERF_OPT_TYPE_CODE);
}

static uint8_t perf_opt_record_type_to_protocol(uint8_t record_type)
{
    switch (record_type)
    {
    case SECTION_PERF_RECORD_TASK:
        return PERF_OPT_TYPE_TASK;
    case SECTION_PERF_RECORD_INTERRUPT:
        return PERF_OPT_TYPE_INTERRUPT;
    case SECTION_PERF_RECORD_CODE:
        return PERF_OPT_TYPE_CODE;
    default:
        return 0u;
    }
}

static void *perf_opt_record_next_get(void *record)
{
    if (record == NULL)
    {
        return NULL;
    }

    return ((section_perf_record_t *)record)->p_next;
}

static uint8_t perf_opt_record_protocol_type_get(void *record)
{
    if (record == NULL)
    {
        return 0u;
    }

    return perf_opt_record_type_to_protocol(((section_perf_record_t *)record)->record_type);
}

static uint16_t perf_opt_record_count(uint8_t type_filter)
{
    return record_dict_count(p_perf_record_first,
                             type_filter,
                             PERF_OPT_TYPE_ALL,
                             perf_opt_record_next_get,
                             perf_opt_record_protocol_type_get);
}

static section_perf_record_t *perf_opt_find_next(section_perf_record_t *record, uint8_t type_filter)
{
    return (section_perf_record_t *)record_dict_find_next(record,
                                                          type_filter,
                                                          PERF_OPT_TYPE_ALL,
                                                          perf_opt_record_next_get,
                                                          perf_opt_record_protocol_type_get);
}

static uint32_t perf_opt_task_period_us_get(section_perf_record_t *record)
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

static uint32_t perf_opt_time_us(uint32_t count)
{
    return (uint32_t)((float)count * PERF_COUNT_UNIT_US);
}

static void perf_opt_send_response(section_packform_t *p_req,
                                   uint8_t cmd_word,
                                   uint8_t is_ack,
                                   uint8_t *payload,
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
    pack.cmd_set = PERF_OPT_CMD_SET;
    pack.cmd_word = cmd_word;
    pack.is_ack = is_ack;
    pack.len = len;
    pack.p_data = payload;

    comm_send_data(&pack, my_printf);
}

static void perf_opt_send_active(perf_opt_service_t *self, uint8_t cmd_word, uint8_t *payload, uint16_t len)
{
    section_packform_t pack = {0};

    pack.src = self->src;
    pack.d_src = self->d_src;
    pack.dst = self->dst;
    pack.d_dst = self->d_dst;
    pack.cmd_set = PERF_OPT_CMD_SET;
    pack.cmd_word = cmd_word;
    pack.is_ack = 0u;
    pack.len = len;
    pack.p_data = payload;

    comm_send_data(&pack, self->my_printf);
}

static void perf_opt_capture_route(perf_opt_service_t *self, section_packform_t *p_pack, DEC_MY_PRINTF)
{
    self->my_printf = my_printf;
    self->src = p_pack->dst;
    self->d_src = p_pack->d_dst;
    self->dst = p_pack->src;
    self->d_dst = p_pack->d_src;
}

static uint32_t perf_opt_next_sequence(void)
{
    ++s_perf_opt_sequence;
    if (s_perf_opt_sequence == 0u)
    {
        ++s_perf_opt_sequence;
    }
    return s_perf_opt_sequence;
}

static uint8_t perf_opt_start_common(perf_opt_service_t *self,
                                     section_packform_t *p_pack,
                                     uint8_t type_filter,
                                     DEC_MY_PRINTF,
                                     uint8_t *reject_reason)
{
    if ((self == NULL) || (p_pack == NULL) || (reject_reason == NULL))
    {
        return 0u;
    }

    if (self->active != 0u)
    {
        *reject_reason = PERF_OPT_REJECT_BUSY;
        return 0u;
    }

    if (perf_opt_filter_is_valid(type_filter) == 0u)
    {
        *reject_reason = PERF_OPT_REJECT_INVALID_FILTER;
        return 0u;
    }

    if ((my_printf == NULL) || (my_printf->tx_by_dma == NULL))
    {
        *reject_reason = PERF_OPT_REJECT_NO_BUFFER;
        return 0u;
    }

    self->type_filter = type_filter;
    self->record_count = perf_opt_record_count(type_filter);
    self->index = 0u;
    self->sequence = perf_opt_next_sequence();
    self->dict_version = perf_dict_version_get();
    self->cur = p_perf_record_first;
    self->status = PERF_OPT_END_OK;
    perf_opt_capture_route(self, p_pack, my_printf);
    *reject_reason = PERF_OPT_REJECT_OK;
    return 1u;
}

static uint8_t perf_opt_start_dict(perf_opt_service_t *self,
                                   section_packform_t *p_pack,
                                   uint8_t type_filter,
                                   DEC_MY_PRINTF,
                                   uint8_t *reject_reason)
{
    if (perf_opt_start_common(self, p_pack, type_filter, my_printf, reject_reason) == 0u)
    {
        return 0u;
    }

    self->pull_type = PERF_OPT_PULL_DICT;
    self->active = 1u;
    return 1u;
}

static uint8_t perf_opt_start_sample(perf_opt_service_t *self,
                                     section_packform_t *p_pack,
                                     uint8_t type_filter,
                                     DEC_MY_PRINTF,
                                     uint8_t *reject_reason)
{
    if (perf_opt_start_common(self, p_pack, type_filter, my_printf, reject_reason) == 0u)
    {
        return 0u;
    }

    self->pull_type = PERF_OPT_PULL_SAMPLE;
    self->active = 1u;
    return 1u;
}

static void perf_opt_send_dict_end(perf_opt_service_t *self, uint8_t status)
{
    perf_dict_end_t end_pack = {0};

    end_pack.sequence = self->sequence;
    end_pack.record_count = self->index;
    end_pack.status = status;
    end_pack.dict_version = self->dict_version;
    perf_opt_send_active(self, PERF_OPT_CMD_DICT_END, (uint8_t *)&end_pack, (uint16_t)sizeof(end_pack));
}

static void perf_opt_send_sample_end(perf_opt_service_t *self, uint8_t status)
{
    perf_sample_end_t end_pack = {0};

    end_pack.sequence = self->sequence;
    end_pack.record_count = self->index;
    end_pack.status = status;
    perf_opt_send_active(self, PERF_OPT_CMD_SAMPLE_END, (uint8_t *)&end_pack, (uint16_t)sizeof(end_pack));
}

static void perf_opt_poll_dict(perf_opt_service_t *self)
{
    section_perf_record_t *record;
    perf_dict_item_header_t item = {0};
    size_t name_len;
    uint16_t len;

    record = perf_opt_find_next(self->cur, self->type_filter);
    if (record == NULL)
    {
        perf_opt_send_dict_end(self, (self->index == self->record_count) ? PERF_OPT_END_OK : PERF_OPT_END_INTERNAL_ERROR);
        self->active = 0u;
        self->pull_type = PERF_OPT_PULL_IDLE;
        return;
    }

    item.sequence = self->sequence;
    item.index = self->index;
    item.record_count = self->record_count;
    item.record_id = record->record_id;
    item.record_type = perf_opt_record_type_to_protocol(record->record_type);

    name_len = (record->p_name != NULL) ? strlen(record->p_name) : 0u;
    if (name_len > (PERF_OPT_MAX_PAYLOAD_SIZE - sizeof(item)))
    {
        name_len = PERF_OPT_MAX_PAYLOAD_SIZE - sizeof(item);
    }
    item.name_len = (uint8_t)name_len;

    (void)memcpy(self->payload, &item, sizeof(item));
    if (name_len > 0u)
    {
        (void)memcpy(&self->payload[sizeof(item)], record->p_name, name_len);
    }

    len = (uint16_t)(sizeof(item) + name_len);
    perf_opt_send_active(self, PERF_OPT_CMD_DICT_ITEM_REPORT, self->payload, len);
    self->cur = (section_perf_record_t *)record->p_next;
    ++self->index;

    if (self->index >= self->record_count)
    {
        perf_opt_send_dict_end(self, PERF_OPT_END_OK);
        self->active = 0u;
        self->pull_type = PERF_OPT_PULL_IDLE;
    }
}

static uint16_t perf_opt_sample_item_size(section_perf_record_t *record)
{
    switch (record->record_type)
    {
    case SECTION_PERF_RECORD_TASK:
        return (uint16_t)sizeof(perf_sample_task_item_t);
    case SECTION_PERF_RECORD_INTERRUPT:
        return (uint16_t)sizeof(perf_sample_interrupt_item_t);
    case SECTION_PERF_RECORD_CODE:
        return (uint16_t)sizeof(perf_sample_code_item_t);
    default:
        return 0u;
    }
}

static uint16_t perf_opt_fill_sample_item(section_perf_record_t *record, uint8_t *payload)
{
    if ((record == NULL) || (payload == NULL))
    {
        return 0u;
    }

    switch (record->record_type)
    {
    case SECTION_PERF_RECORD_TASK:
    {
        perf_sample_task_item_t item = {0};
        item.record_id = record->record_id;
        item.time_us = perf_opt_time_us(record->time);
        item.max_time_us = perf_opt_time_us(record->max_time);
        item.period_us = perf_opt_task_period_us_get(record);
        item.load_percent = record->load * 100.0f;
        item.peak_percent = record->load_max * 100.0f;
        (void)memcpy(payload, &item, sizeof(item));
        return (uint16_t)sizeof(item);
    }

    case SECTION_PERF_RECORD_INTERRUPT:
    {
        perf_sample_interrupt_item_t item = {0};
        item.record_id = record->record_id;
        item.time_us = perf_opt_time_us(record->time);
        item.max_time_us = perf_opt_time_us(record->max_time);
        item.load_percent = record->load * 100.0f;
        item.peak_percent = record->load_max * 100.0f;
        (void)memcpy(payload, &item, sizeof(item));
        return (uint16_t)sizeof(item);
    }

    case SECTION_PERF_RECORD_CODE:
    {
        perf_sample_code_item_t item = {0};
        item.record_id = record->record_id;
        item.time_us = perf_opt_time_us(record->time);
        item.max_time_us = perf_opt_time_us(record->max_time);
        (void)memcpy(payload, &item, sizeof(item));
        return (uint16_t)sizeof(item);
    }

    default:
        return 0u;
    }
}

static void perf_opt_poll_sample(perf_opt_service_t *self)
{
    perf_sample_batch_header_t header = {0};
    section_perf_record_t *record;
    uint16_t payload_len;
    uint16_t item_count;

    header.sequence = self->sequence;
    header.record_count = self->record_count;

    payload_len = (uint16_t)sizeof(header);
    item_count = 0u;

    while (self->index < self->record_count)
    {
        uint16_t item_size;

        record = perf_opt_find_next(self->cur, self->type_filter);
        if (record == NULL)
        {
            break;
        }

        item_size = perf_opt_sample_item_size(record);
        if (item_size == 0u)
        {
            self->cur = (section_perf_record_t *)record->p_next;
            continue;
        }

        if ((payload_len + item_size) > PERF_OPT_MAX_PAYLOAD_SIZE)
        {
            break;
        }

        payload_len += perf_opt_fill_sample_item(record, &self->payload[payload_len]);
        self->cur = (section_perf_record_t *)record->p_next;
        ++self->index;
        ++item_count;
    }

    if (item_count > 0u)
    {
        header.item_count = item_count;
        (void)memcpy(self->payload, &header, sizeof(header));
        perf_opt_send_active(self, PERF_OPT_CMD_SAMPLE_BATCH_REPORT, self->payload, payload_len);
    }

    if (self->index >= self->record_count)
    {
        perf_opt_send_sample_end(self, PERF_OPT_END_OK);
        self->active = 0u;
        self->pull_type = PERF_OPT_PULL_IDLE;
        return;
    }

    if (item_count == 0u)
    {
        perf_opt_send_sample_end(self, PERF_OPT_END_INTERNAL_ERROR);
        self->active = 0u;
        self->pull_type = PERF_OPT_PULL_IDLE;
    }
}

static void perf_opt_poll(perf_opt_service_t *self)
{
    if ((self == NULL) || (self->active == 0u))
    {
        return;
    }

    switch (self->pull_type)
    {
    case PERF_OPT_PULL_DICT:
        perf_opt_poll_dict(self);
        break;

    case PERF_OPT_PULL_SAMPLE:
        perf_opt_poll_sample(self);
        break;

    default:
        self->active = 0u;
        self->pull_type = PERF_OPT_PULL_IDLE;
        break;
    }
}

void perf_opt_service_init(perf_opt_service_t *self)
{
    if (self == NULL)
    {
        return;
    }

    (void)memset(self, 0, sizeof(*self));
    self->dict_version = perf_dict_version_get();
    self->start_dict = perf_opt_start_dict;
    self->start_sample = perf_opt_start_sample;
    self->poll = perf_opt_poll;
}

static void perf_opt_init_task(void)
{
    perf_opt_service_init(&s_perf_opt_service);
}

static void perf_opt_poll_task(void)
{
    s_perf_service_task_metric = perf_task_metric_get();
    s_perf_service_task_metric_max = perf_task_metric_max_get();
    s_perf_service_interrupt_metric = perf_interrupt_metric_get();
    s_perf_service_interrupt_metric_max = perf_interrupt_metric_max_get();
    s_perf_opt_service.poll(&s_perf_opt_service);
    perf_service_print_step();
}

static const char *perf_service_record_type_name(uint8_t record_type)
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

static void perf_service_print_record_item(section_perf_record_t *record, DEC_MY_PRINTF)
{
    if ((record == NULL) || (my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    if (record->record_type == SECTION_PERF_RECORD_CODE)
    {
        my_printf->my_printf("%s\t%s\t%lu\t%lu\t-\t-\r\n",
                             perf_service_record_type_name(record->record_type),
                             record->p_name,
                             (unsigned long)perf_count_to_us(record->time),
                             (unsigned long)perf_count_to_us(record->max_time));
    }
    else
    {
        my_printf->my_printf("%s\t%s\t%lu\t%lu\t%f\t%f\r\n",
                             perf_service_record_type_name(record->record_type),
                             record->p_name,
                             (unsigned long)perf_count_to_us(record->time),
                             (unsigned long)perf_count_to_us(record->max_time),
                             (double)(record->load * 100.0f),
                             (double)(record->load_max * 100.0f));
    }
}

static void perf_service_print_by_type(uint8_t record_type, DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("PERF_BEGIN type=%s count=%u unit_us=%f\r\n",
                         perf_service_record_type_name(record_type),
                         (unsigned)perf_record_count_by_type(record_type),
                         (double)PERF_COUNT_UNIT_US);
    my_printf->my_printf("Type\tPerf Name\tTime(us)\tMax(us)\tLoad(%%)\tPeak(%%)\r\n");
    for (section_perf_record_t *record = p_perf_record_first; record != NULL; record = (section_perf_record_t *)record->p_next)
    {
        if (record->record_type == record_type)
        {
            perf_service_print_record_item(record, my_printf);
        }
    }
    my_printf->my_printf("PERF_END\r\n");
}

static void perf_service_print_task(DEC_MY_PRINTF)
{
    perf_service_print_by_type(SECTION_PERF_RECORD_TASK, my_printf);
}

static void perf_service_print_interrupt(DEC_MY_PRINTF)
{
    perf_service_print_by_type(SECTION_PERF_RECORD_INTERRUPT, my_printf);
}

static void perf_service_print_code(DEC_MY_PRINTF)
{
    perf_service_print_by_type(SECTION_PERF_RECORD_CODE, my_printf);
}

static void perf_service_cpu_utilization(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("TASK CPU Load:%f%%,TASK CPU Peak:%f%%\r\n",
                         (double)(perf_task_metric_get() * 100.0f),
                         (double)(perf_task_metric_max_get() * 100.0f));
    my_printf->my_printf("INT CPU Load:%f%%,INT CPU Peak:%f%%\r\n",
                         (double)(perf_interrupt_metric_get() * 100.0f),
                         (double)(perf_interrupt_metric_max_get() * 100.0f));
}

static void perf_service_summary(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("PERF_SUMMARY task_load=%f task_peak=%f int_load=%f int_peak=%f\r\n",
                         (double)(perf_task_metric_get() * 100.0f),
                         (double)(perf_task_metric_max_get() * 100.0f),
                         (double)(perf_interrupt_metric_get() * 100.0f),
                         (double)(perf_interrupt_metric_max_get() * 100.0f));
}

static void perf_service_info(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("PERF_INFO unit_us=%f cnt_per_sys_tick=%lu cpu_window_ms=%lu record_count=%u\r\n",
                         (double)PERF_COUNT_UNIT_US,
                         (unsigned long)PERF_CNT_PER_SECTION_SYS_TICK,
                         (unsigned long)PERF_CPU_LOAD_PERIOD_MS,
                         (unsigned)perf_record_count_get());
}

static void perf_service_reset_peak(DEC_MY_PRINTF)
{
    perf_reset_peak_value();

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("PERF_RESET_OK\r\n");
    }
}

static void perf_service_print_start(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL) || (s_perf_text_print_ctx.active != 0u))
    {
        return;
    }

    s_perf_text_print_ctx.cur = p_perf_record_first;
    s_perf_text_print_ctx.my_printf = my_printf;
    s_perf_text_print_ctx.active = 1u;
    s_perf_text_print_ctx.perf_max_name_len = 0u;

    for (section_perf_record_t *record = p_perf_record_first; record != NULL; record = (section_perf_record_t *)record->p_next)
    {
        uint32_t len = (record->p_name != NULL) ? (uint32_t)strlen(record->p_name) : 0u;
        if (len > s_perf_text_print_ctx.perf_max_name_len)
        {
            s_perf_text_print_ctx.perf_max_name_len = len;
        }
    }

    my_printf->my_printf("PERF_BEGIN type=ALL count=%u unit_us=%f cnt_per_sys_tick=%lu cpu_window_ms=%lu\r\n",
                         (unsigned)perf_record_count_get(),
                         (double)PERF_COUNT_UNIT_US,
                         (unsigned long)PERF_CNT_PER_SECTION_SYS_TICK,
                         (unsigned long)PERF_CPU_LOAD_PERIOD_MS);
    my_printf->my_printf("Type\tPerf Name\tTime(us)\tMax(us)\tLoad(%%)\tPeak(%%)\r\n");
}

static void perf_service_print_step(void)
{
    if (s_perf_text_print_ctx.active == 0u)
    {
        return;
    }

    if (s_perf_text_print_ctx.cur == NULL)
    {
        if ((s_perf_text_print_ctx.my_printf != NULL) && (s_perf_text_print_ctx.my_printf->my_printf != NULL))
        {
            s_perf_text_print_ctx.my_printf->my_printf("PERF_END\r\n");
        }
        s_perf_text_print_ctx.active = 0u;
        return;
    }

    perf_service_print_record_item(s_perf_text_print_ctx.cur, s_perf_text_print_ctx.my_printf);
    s_perf_text_print_ctx.cur = (section_perf_record_t *)s_perf_text_print_ctx.cur->p_next;
}

static void perf_info_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_info_ack_t ack = {0};

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    ack.protocol_version = PERF_SERVICE_PROTOCOL_VERSION;
    ack.record_count = perf_record_count_get();
    ack.unit_us = PERF_COUNT_UNIT_US;
    ack.cnt_per_sys_tick = PERF_CNT_PER_SECTION_SYS_TICK;
    ack.cpu_window_ms = PERF_CPU_LOAD_PERIOD_MS;
    ack.flags = PERF_SERVICE_INFO_FLAGS;
    perf_opt_send_response(p_pack, PERF_OPT_CMD_INFO_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

static void perf_summary_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_summary_ack_t ack = {0};

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    ack.task_load_percent = perf_task_metric_get() * 100.0f;
    ack.task_peak_percent = perf_task_metric_max_get() * 100.0f;
    ack.int_load_percent = perf_interrupt_metric_get() * 100.0f;
    ack.int_peak_percent = perf_interrupt_metric_max_get() * 100.0f;
    perf_opt_send_response(p_pack, PERF_OPT_CMD_SUMMARY_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

static void perf_reset_peak_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_reset_peak_ack_t ack = {0};

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    perf_reset_peak_value();
    ack.success = 1u;
    perf_opt_send_response(p_pack, PERF_OPT_CMD_RESET_PEAK, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

static void perf_dict_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_dict_ack_t ack = {0};
    uint8_t reject_reason = PERF_OPT_REJECT_OK;
    uint8_t type_filter = 0xFFu;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len >= sizeof(perf_dict_query_t))
    {
        const perf_dict_query_t *query = (const perf_dict_query_t *)p_pack->p_data;
        type_filter = query->type_filter;
        (void)query->known_dict_version;
    }

    ack.type_filter = type_filter;
    ack.dict_version = perf_dict_version_get();
    if (s_perf_opt_service.start_dict(&s_perf_opt_service, p_pack, type_filter, my_printf, &reject_reason) != 0u)
    {
        ack.accepted = 1u;
        ack.record_count = s_perf_opt_service.record_count;
        ack.sequence = s_perf_opt_service.sequence;
        ack.reject_reason = PERF_OPT_REJECT_OK;
    }
    else
    {
        ack.accepted = 0u;
        ack.reject_reason = reject_reason;
    }

    perf_opt_send_response(p_pack, PERF_OPT_CMD_DICT_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

static void perf_sample_query_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    perf_sample_ack_t ack = {0};
    uint8_t reject_reason = PERF_OPT_REJECT_OK;
    uint8_t type_filter = 0xFFu;
    uint32_t query_dict_version = 0u;

    if ((p_pack == NULL) || (p_pack->is_ack == 1u))
    {
        return;
    }

    if (p_pack->len >= sizeof(perf_sample_query_t))
    {
        const perf_sample_query_t *query = (const perf_sample_query_t *)p_pack->p_data;
        type_filter = query->type_filter;
        (void)query->flags;
        query_dict_version = query->dict_version;
    }

    ack.type_filter = type_filter;
    ack.dict_version = perf_dict_version_get();
    if (perf_opt_filter_is_valid(type_filter) == 0u)
    {
        ack.accepted = 0u;
        ack.reject_reason = PERF_OPT_REJECT_INVALID_FILTER;
        perf_opt_send_response(p_pack, PERF_OPT_CMD_SAMPLE_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
        return;
    }

    if (query_dict_version != ack.dict_version)
    {
        ack.accepted = 0u;
        ack.reject_reason = PERF_OPT_REJECT_DICT_MISMATCH;
        perf_opt_send_response(p_pack, PERF_OPT_CMD_SAMPLE_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
        return;
    }

    if (s_perf_opt_service.start_sample(&s_perf_opt_service, p_pack, type_filter, my_printf, &reject_reason) != 0u)
    {
        ack.accepted = 1u;
        ack.record_count = s_perf_opt_service.record_count;
        ack.sequence = s_perf_opt_service.sequence;
        ack.reject_reason = PERF_OPT_REJECT_OK;
    }
    else
    {
        ack.accepted = 0u;
        ack.reject_reason = reject_reason;
    }

    perf_opt_send_response(p_pack, PERF_OPT_CMD_SAMPLE_QUERY, 1u, (uint8_t *)&ack, (uint16_t)sizeof(ack), my_printf);
}

REG_INIT(0, perf_opt_init_task)
REG_TASK_MS(1, perf_opt_poll_task)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_INFO_QUERY, perf_info_query_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_SUMMARY_QUERY, perf_summary_query_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_RESET_PEAK, perf_reset_peak_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_DICT_QUERY, perf_dict_query_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_SAMPLE_QUERY, perf_sample_query_act)
REG_SHELL_CMD(perf_print_record, perf_service_print_start)
REG_SHELL_CMD(perf_print_task, perf_service_print_task)
REG_SHELL_CMD(perf_print_interrupt, perf_service_print_interrupt)
REG_SHELL_CMD(perf_print_code, perf_service_print_code)
REG_SHELL_CMD(CPU_Utilization, perf_service_cpu_utilization)
REG_SHELL_CMD(perf_summary, perf_service_summary)
REG_SHELL_CMD(perf_info, perf_service_info)
REG_SHELL_CMD(perf_reset_peak, perf_service_reset_peak)
REG_SHELL_VAR(TASK_METRIC, s_perf_service_task_metric, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TASK_METRIC_MAX, s_perf_service_task_metric_max, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(INTERRUPT_METRIC, s_perf_service_interrupt_metric, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(INTERRUPT_METRIC_MAX, s_perf_service_interrupt_metric_max, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
