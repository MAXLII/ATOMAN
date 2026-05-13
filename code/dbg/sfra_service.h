// SPDX-License-Identifier: MIT
/**
 * @file    sfra_service.h
 * @brief   SFRA service public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define SFRA service command words, payload layouts, and status codes
 *          - Keep communication protocol definitions separate from the SFRA core
 *          - Expose the service-owned linked list head for registered SFRA instances
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef __SFRA_SERVICE_H__
#define __SFRA_SERVICE_H__

#include "sfra.h"

extern sfra_t *g_sfra_first;

#define CMD_SET_SFRA 0x01u

#define CMD_WORD_SFRA_LIST_QUERY   0x2Fu
#define CMD_WORD_SFRA_INFO_QUERY   0x30u
#define CMD_WORD_SFRA_CFG_SET      0x31u
#define CMD_WORD_SFRA_START        0x32u
#define CMD_WORD_SFRA_STOP         0x33u
#define CMD_WORD_SFRA_RESET        0x34u
#define CMD_WORD_SFRA_POINT_QUERY  0x35u
#define CMD_WORD_SFRA_POINT_REPORT 0x36u
#define CMD_WORD_SFRA_DONE_REPORT  0x37u

#define SFRA_CFG_APPLY_FREQ      0x01u
#define SFRA_CFG_APPLY_AMPLITUDE 0x02u

typedef enum
{
    SFRA_TOOL_STATUS_OK = 0,
    SFRA_TOOL_STATUS_SFRA_ID_INVALID = 1,
    SFRA_TOOL_STATUS_POINT_INDEX_INVALID = 2,
    SFRA_TOOL_STATUS_BUSY = 3,
    SFRA_TOOL_STATUS_DATA_NOT_READY = 4,
    SFRA_TOOL_STATUS_SWEEP_CHANGED = 5,
    SFRA_TOOL_STATUS_INVALID_PARAM = 6,
    SFRA_TOOL_STATUS_CORE_ERROR = 7,
} sfra_tool_status_e;

#pragma pack(push, 1)
typedef struct
{
    uint8_t reserved;
} sfra_list_query_t;

typedef struct
{
    uint8_t sfra_id;
    uint8_t is_last;
    uint8_t name_len;
    uint8_t reserved;
} sfra_list_item_t;

typedef struct
{
    uint8_t sfra_id;
    uint8_t reserved[3];
} sfra_info_query_t;

typedef struct
{
    uint8_t sfra_id;
    uint8_t status;
    uint8_t state;
    uint8_t busy;
    uint8_t done;
    uint8_t data_ready;
    uint8_t reserved[2];
    uint16_t freq_index;
    uint16_t freq_length;
    uint16_t table_length;
    uint16_t inject_delay_tick;
    uint32_t sweep_tag;
    float current_freq_hz;
    float isr_freq_hz;
    float freq_start_hz;
    float freq_end_hz;
    float inject_amplitude;
    float settle_cycle_count;
    float collect_cycle_count;
} sfra_info_ack_t;

typedef struct
{
    uint8_t sfra_id;
    uint8_t apply_mask;
    uint8_t reserved[2];
    float freq_start_hz;
    float freq_end_hz;
    float inject_amplitude;
} sfra_cfg_set_t;

typedef struct
{
    uint8_t sfra_id;
    uint8_t status;
    uint8_t state;
    uint8_t busy;
    uint8_t done;
    uint8_t data_ready;
    uint8_t reserved[2];
    uint16_t freq_index;
    uint16_t freq_length;
    uint16_t table_length;
    uint16_t reserved16;
    uint32_t sweep_tag;
} sfra_ctrl_ack_t;

typedef struct
{
    uint8_t sfra_id;
    uint8_t reserved;
    uint16_t point_index;
    uint32_t expected_sweep_tag;
} sfra_point_query_t;

typedef struct
{
    uint8_t sfra_id;
    uint8_t status;
    uint8_t is_last;
    uint8_t reserved;
    uint16_t point_index;
    uint16_t point_count;
    uint32_t sweep_tag;
    float freq_hz;
    float magnitude;
    float phase_deg;
} sfra_point_ack_t;

typedef sfra_point_ack_t sfra_point_report_t;
#pragma pack(pop)

void sfra_service_init(void);

#endif /* __SFRA_SERVICE_H__ */
