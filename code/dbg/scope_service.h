// SPDX-License-Identifier: MIT
/**
 * @file    scope_service.h
 * @brief   Scope service public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define scope communication payloads and command ids
 *          - Provide scope service registration helpers
 *          - Provide shell-facing scope registration macros
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

#ifndef __SCOPE_SERVICE_H__
#define __SCOPE_SERVICE_H__

#include "scope.h"

#define CMD_SET_SCOPE 0x01u

#define CMD_WORD_SCOPE_LIST_QUERY 0x18u
#define CMD_WORD_SCOPE_INFO_QUERY 0x19u
#define CMD_WORD_SCOPE_VAR_QUERY 0x1Au
#define CMD_WORD_SCOPE_START 0x1Bu
#define CMD_WORD_SCOPE_TRIGGER 0x1Cu
#define CMD_WORD_SCOPE_STOP 0x1Du
#define CMD_WORD_SCOPE_RESET 0x1Eu
#define CMD_WORD_SCOPE_SAMPLE_QUERY 0x1Fu

#ifndef SCOPE_ENABLE_PRINTF
#define SCOPE_ENABLE_PRINTF 0
#endif

typedef struct scope_service_obj_t
{
    uint8_t scope_id;
    const char *p_name;
    scope_t *p_scope;
    uint32_t sample_period_us;
    uint32_t capture_tag;
    uint8_t data_ready;
    scope_state_e last_state;
    struct scope_service_obj_t *p_next;
} scope_service_obj_t;

typedef enum
{
    SCOPE_READ_MODE_NORMAL = 0,
    SCOPE_READ_MODE_FORCE = 1,
} scope_read_mode_e;

typedef enum
{
    SCOPE_TOOL_STATUS_OK = 0,
    SCOPE_TOOL_STATUS_SCOPE_ID_INVALID = 1,
    SCOPE_TOOL_STATUS_VAR_INDEX_INVALID = 2,
    SCOPE_TOOL_STATUS_SAMPLE_INDEX_INVALID = 3,
    SCOPE_TOOL_STATUS_RUNNING_DENIED = 4,
    SCOPE_TOOL_STATUS_DATA_NOT_READY = 5,
    SCOPE_TOOL_STATUS_BUSY = 6,
    SCOPE_TOOL_STATUS_CAPTURE_CHANGED = 7,
} scope_tool_status_e;

#pragma pack(push, 1)
typedef struct
{
    uint8_t reserved;
} scope_list_query_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t is_last;
    uint8_t name_len;
    uint8_t reserved;
} scope_list_item_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t reserved[3];
} scope_info_query_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t status;
    uint8_t state;
    uint8_t data_ready;
    uint8_t var_count;
    uint8_t reserved[3];
    uint32_t sample_count;
    uint32_t write_index;
    uint32_t trigger_index;
    uint32_t trigger_post_cnt;
    uint32_t trigger_display_index;
    uint32_t sample_period_us;
    uint32_t capture_tag;
} scope_info_ack_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t var_index;
    uint8_t reserved[2];
} scope_var_query_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t status;
    uint8_t var_index;
    uint8_t is_last;
    uint8_t name_len;
    uint8_t reserved[3];
} scope_var_ack_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t status;
    uint8_t state;
    uint8_t data_ready;
    uint32_t capture_tag;
} scope_ctrl_ack_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t read_mode;
    uint8_t reserved[2];
    uint32_t sample_index;
    uint32_t expected_capture_tag;
} scope_sample_query_t;

typedef struct
{
    uint8_t scope_id;
    uint8_t status;
    uint8_t read_mode;
    uint8_t var_count;
    uint32_t sample_index;
    uint32_t capture_tag;
    uint8_t is_last_sample;
    uint8_t reserved[3];
} scope_sample_ack_t;
#pragma pack(pop)

void scope_service_register(scope_service_obj_t *p_obj);
void scope_printf_status(scope_t *scope, DEC_MY_PRINTF);
void scope_printf_data_start(scope_t *scope, DEC_MY_PRINTF);
int scope_printf_data_step(void);
int scope_printf_data_is_active(void);

#if SCOPE_ENABLE_PRINTF
#define REG_SCOPE_STATUS_CMD(name)                     \
    static void scope_status_##name(DEC_MY_PRINTF)     \
    {                                                  \
        scope_printf_status(&scope_##name, my_printf); \
    }                                                  \
    REG_SHELL_CMD(scp_sta_##name, scope_status_##name)

#define REG_SCOPE_START_CMD(name)                  \
    static void scope_start_##name(DEC_MY_PRINTF)  \
    {                                              \
        scope_start(&scope_##name);                \
        my_printf->my_printf("Scope started\r\n"); \
    }                                              \
    REG_SHELL_CMD(scp_start_##name, scope_start_##name)

#define SCOPE_DATA_STEP_START(name, my_printf) scope_printf_data_start(&scope_##name, my_printf)

#define REG_SCOPE_DATA_STEP_CMD(name)                                                             \
    static void scope_data_step_##name(DEC_MY_PRINTF) { SCOPE_DATA_STEP_START(name, my_printf); } \
    REG_SHELL_CMD(scp_pf_##name, scope_data_step_##name)
#else
#define REG_SCOPE_STATUS_CMD(name)
#define REG_SCOPE_START_CMD(name)
#define SCOPE_DATA_STEP_START(name, my_printf) ((void)0)
#define REG_SCOPE_DATA_STEP_CMD(name)
#endif

#define REG_SCOPE_EX(name, buf_size, trig_post_cnt, _sample_period_us, ...) \
    SCOPE_DEFINE(name, buf_size, trig_post_cnt, __VA_ARGS__);               \
    scope_service_obj_t scope_service_obj_##name = {                        \
        .scope_id = 0u,                                                      \
        .p_name = #name,                                                     \
        .p_scope = &scope_##name,                                            \
        .sample_period_us = (_sample_period_us),                             \
        .capture_tag = 0u,                                                   \
        .data_ready = 0u,                                                    \
        .last_state = SCOPE_STATE_IDLE,                                      \
        .p_next = NULL,                                                      \
    };                                                                       \
    static void scope_service_auto_reg_##name(void)                          \
    {                                                                        \
        scope_service_register(&scope_service_obj_##name);                   \
    }                                                                        \
    REG_INIT(1, scope_service_auto_reg_##name)                               \
    REG_SCOPE_STATUS_CMD(name)                                               \
    REG_SCOPE_START_CMD(name)                                                \
    REG_SCOPE_DATA_STEP_CMD(name)

#define REG_SCOPE(name, buf_size, trig_post_cnt, ...) \
    REG_SCOPE_EX(name, buf_size, trig_post_cnt, (uint32_t)(CTRL_TS * 1000000.0f + 0.5f), __VA_ARGS__)

#define SCOPE_DATA_STEP_RUN()              \
    do                                     \
    {                                      \
        if (scope_printf_data_is_active()) \
        {                                  \
            (void)scope_printf_data_step();\
        }                                  \
    } while (0)

#endif
