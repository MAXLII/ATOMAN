#ifndef __PERF_SERVICE_H__
#define __PERF_SERVICE_H__

#include "comm.h"
#include "perf.h"
#include "section.h"

#include <stdint.h>

#ifndef PERF_CNT_PER_SECTION_SYS_TICK
#define PERF_CNT_PER_SECTION_SYS_TICK 200UL
#endif

#ifndef PERF_COUNT_UNIT_US
#define PERF_COUNT_UNIT_US 0.5f
#endif

#define PERF_OPT_CMD_SET 0x01u
#define PERF_OPT_CMD_INFO_QUERY 0x20u
#define PERF_OPT_CMD_SUMMARY_QUERY 0x21u
#define PERF_OPT_CMD_RESET_PEAK 0x25u
#define PERF_OPT_CMD_DICT_QUERY 0x26u
#define PERF_OPT_CMD_DICT_ITEM_REPORT 0x27u
#define PERF_OPT_CMD_DICT_END 0x28u
#define PERF_OPT_CMD_SAMPLE_QUERY 0x29u
#define PERF_OPT_CMD_SAMPLE_BATCH_REPORT 0x2Au
#define PERF_OPT_CMD_SAMPLE_END 0x2Bu

#define PERF_OPT_TYPE_ALL 0u
#define PERF_OPT_TYPE_TASK 1u
#define PERF_OPT_TYPE_INTERRUPT 2u
#define PERF_OPT_TYPE_CODE 3u

#define PERF_OPT_REJECT_OK 0u
#define PERF_OPT_REJECT_BUSY 1u
#define PERF_OPT_REJECT_INVALID_FILTER 2u
#define PERF_OPT_REJECT_NO_BUFFER 3u
#define PERF_OPT_REJECT_UNSUPPORTED 4u
#define PERF_OPT_REJECT_DICT_MISMATCH 5u

#define PERF_OPT_END_OK 0u
#define PERF_OPT_END_CANCELLED 1u
#define PERF_OPT_END_OVERFLOW 2u
#define PERF_OPT_END_INTERNAL_ERROR 3u

#define PERF_OPT_MAX_PAYLOAD_SIZE 480u

typedef enum
{
    PERF_OPT_PULL_IDLE = 0,
    PERF_OPT_PULL_DICT,
    PERF_OPT_PULL_SAMPLE,
} perf_opt_pull_type_t;

typedef struct perf_opt_service perf_opt_service_t;

typedef uint8_t (*perf_opt_start_f)(perf_opt_service_t *self,
                                    section_packform_t *p_pack,
                                    uint8_t type_filter,
                                    DEC_MY_PRINTF,
                                    uint8_t *reject_reason);
typedef void (*perf_opt_poll_f)(perf_opt_service_t *self);

struct perf_opt_service
{
    uint8_t active;
    uint8_t type_filter;
    uint8_t status;
    perf_opt_pull_type_t pull_type;
    uint16_t record_count;
    uint16_t index;
    uint32_t sequence;
    uint32_t dict_version;
    section_perf_record_t *cur;
    DEC_MY_PRINTF;
    uint8_t src;
    uint8_t d_src;
    uint8_t dst;
    uint8_t d_dst;
    uint8_t payload[PERF_OPT_MAX_PAYLOAD_SIZE];
    perf_opt_start_f start_dict;
    perf_opt_start_f start_sample;
    perf_opt_poll_f poll;
};

void perf_opt_service_init(perf_opt_service_t *self);

#endif
