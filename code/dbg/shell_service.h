// SPDX-License-Identifier: MIT
/**
 * @file    shell_service.h
 * @brief   shell reporting service public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define shell reporting command words and communication payload layouts
 *          - Provide service task declarations for list output, parameter reporting, and wave streaming
 *          - Keep binary reporting protocol definitions separate from the shell parser core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __SHELL_SERVICE_H__
#define __SHELL_SERVICE_H__

#include "shell.h"

#define CMD_SET_SHELL_DATA_NUM 0x01
#define CMD_WORD_SHELL_DATA_NUM 0x01

#define CMD_SET_SHELL_REPORT_LIST 0x01
#define CMD_WORD_SHELL_REPORT_LIST 0x04

#define CMD_SET_SHELL_READ_DATA 0x01
#define CMD_WORD_SHELL_READ_DATA 0x02

#define CMD_SET_SHELL_WRITE_DATA 0x01
#define CMD_WORD_SHELL_WRITE_DATA 0x03

#define CMD_SET_SHELL_WAVE_ENABLE_PARAM 0x01
#define CMD_WORD_SHELL_WAVE_ENABLE_PARAM 0x05

#define CMD_SET_SHELL_WAVE_START 0x01
#define CMD_WORD_SHELL_WAVE_START 0x0C

#define CMD_SET_SHELL_WAVE_PERIOD 0x01
#define CMD_WORD_SHELL_WAVE_PERIOD 0x06

#define CMD_SET_SHELL_WAVE_PARAM 0x01
#define CMD_WORD_SHELL_WAVE_PARAM 0x07

typedef struct
{
    uint8_t active;
    DEC_MY_PRINTF;
    uint8_t src;
    uint8_t d_src;
    uint8_t dst;
    uint8_t d_dst;
    section_shell_t *p_shell;
} shell_report_ctx_t;

#pragma pack(push, 1)
typedef struct
{
    uint8_t name_len;
    uint8_t type;
    uint32_t data;
    uint32_t data_max;
    uint32_t data_min;
    uint8_t auto_report;
    char name[SHELL_STR_SIZE_MAX];
} shell_report_list_t;

typedef struct
{
    uint8_t name_len;
    char name[SHELL_STR_SIZE_MAX];
} shell_read_data_t;

typedef struct
{
    uint8_t name_len;
    uint8_t type;
    uint32_t data;
    char name[SHELL_STR_SIZE_MAX];
} shell_read_data_ret_t;

typedef struct
{
    uint8_t name_len;
    uint32_t data;
    uint32_t data_max;
    uint32_t data_min;
    char name[SHELL_STR_SIZE_MAX];
} shell_write_data_t;

typedef struct
{
    uint8_t name_len;
    uint8_t type;
    uint32_t data;
    uint32_t data_max;
    uint32_t data_min;
    char name[SHELL_STR_SIZE_MAX];
} shell_write_data_ret_t;

typedef struct
{
    uint8_t name_len;
    uint8_t auto_report;
    char name[SHELL_STR_SIZE_MAX];
} shell_wave_enable_param_t;

typedef struct
{
    uint8_t ok;
} shell_wave_enable_param_ack_t;

typedef struct
{
    uint8_t name_len;
    uint8_t type;
    uint32_t data;
    char name[SHELL_STR_SIZE_MAX];
} shell_wave_param_t;

typedef struct
{
    uint8_t start_report;
} shell_wave_start_t;

typedef struct
{
    uint32_t reprot_period;
} shell_wave_period_t;

typedef struct
{
    uint32_t reprot_period;
} shell_wave_period_ack_t;
#pragma pack(pop)

void shell_status_run(void);
#ifdef SHELL_STRING_PARSE
void list_print_start(DEC_MY_PRINTF);
int list_print_step(void);
#endif

#endif /* __SHELL_SERVICE_H__ */