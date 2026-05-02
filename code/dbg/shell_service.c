// SPDX-License-Identifier: MIT
/**
 * @file    shell_service.c
 * @brief   shell reporting service module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Register shell service commands for list output, remote parameter access, and wave reporting
 *          - Stream shell item metadata and selected variable values through the 0xE8 communication protocol
 *          - Keep reporting protocol state machines outside the shell parser and registry core
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
#include "shell_service.h"

#include "comm.h"
#include "my_math.h"

#include <string.h>

#if SHELL_STRING_PARSE == 1

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
void shell_status_run(void)
{
    /* Periodically service status-triggered shell items. */
    for (section_shell_t *p = shell_first_get(); p != NULL; p = p->p_next)
    {
        if (p->status == 0u)
        {
            continue;
        }

        if ((p->my_printf == NULL) || (p->my_printf->my_printf == NULL))
        {
            continue;
        }

        if (p->status & (1u << 0))
        {
            /* Bit0 means "print me periodically" for variable-style entries. */
            if (p->type != SHELL_CMD)
            {
                shell_item_print(p, p->my_printf);
            }
        }

        if (p->status & (1u << 1))
        {
            /* Bit1 means "run callback periodically". */
            if (p->func)
                p->func(p->my_printf);
        }
    }
}

REG_TASK_MS(1000, shell_status_run)


typedef struct
{
    section_shell_t *cur;
    DEC_MY_PRINTF;
    uint8_t active;
    int max_name_len;
    int tab_count;
} list_print_ctx_t;

static list_print_ctx_t g_list_print_ctx = {0};

void list_print_start(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (g_list_print_ctx.active != 0u))
    {
        return;
    }
    g_list_print_ctx.cur = shell_first_get();
    g_list_print_ctx.my_printf = my_printf;
    g_list_print_ctx.active = 1u;
    int max_len = 0;
    for (section_shell_t *s = shell_first_get(); s != NULL; s = s->p_next)
    {
        int len = strlen(s->p_name);
        if (len > max_len)
        {
            max_len = len;
        }
    }
    g_list_print_ctx.max_name_len = max_len;
    my_printf->my_printf("\r\n==================== SHELL COMMANDS AND VARIABLES ====================\r\n");
}

REG_SHELL_CMD(list, list_print_start)

int list_print_step(void)
{
    static uint8_t print_flag = 0u;

    if ((g_list_print_ctx.active == 0u) ||
        (g_list_print_ctx.my_printf == NULL) ||
        (g_list_print_ctx.my_printf->my_printf == NULL))
    {
        return 0;
    }

    if (print_flag == 0u)
    {
        section_shell_t *s = g_list_print_ctx.cur;
        if (s == NULL)
        {
            g_list_print_ctx.active = 0u;
            print_flag = 0u;
            return 0;
        }

        switch (s->type)
        {
        case SHELL_CMD:
            g_list_print_ctx.my_printf->my_printf("%s\tCMD\r\n", s->p_name);
            break;

        case SHELL_UINT8:
            g_list_print_ctx.my_printf->my_printf("%s\tU8\t(%u)\t(%u)\t%u\r\n",
                                                  s->p_name,
                                                  (unsigned)(*(uint8_t *)s->p_max),
                                                  (unsigned)(*(uint8_t *)s->p_min),
                                                  (unsigned)(*(uint8_t *)s->p_var));
            break;

        case SHELL_UINT16:
            g_list_print_ctx.my_printf->my_printf("%s\tU16\t(%u)\t(%u)\t%u\r\n",
                                                  s->p_name,
                                                  (unsigned)(*(uint16_t *)s->p_max),
                                                  (unsigned)(*(uint16_t *)s->p_min),
                                                  (unsigned)(*(uint16_t *)s->p_var));
            break;

        case SHELL_UINT32:
            g_list_print_ctx.my_printf->my_printf("%s\tU32\t(%lu)\t(%lu)\t%lu\r\n",
                                                  s->p_name,
                                                  (unsigned long)(*(uint32_t *)s->p_max),
                                                  (unsigned long)(*(uint32_t *)s->p_min),
                                                  (unsigned long)(*(uint32_t *)s->p_var));
            break;

        case SHELL_INT8:
            g_list_print_ctx.my_printf->my_printf("%s\tI8\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  (int)(*(int8_t *)s->p_max),
                                                  (int)(*(int8_t *)s->p_min),
                                                  (int)(*(int8_t *)s->p_var));
            break;

        case SHELL_INT16:
            g_list_print_ctx.my_printf->my_printf("%s\tI16\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  (int)(*(int16_t *)s->p_max),
                                                  (int)(*(int16_t *)s->p_min),
                                                  (int)(*(int16_t *)s->p_var));
            break;

        case SHELL_INT32:
            g_list_print_ctx.my_printf->my_printf("%s\tI32\t(%ld)\t(%ld)\t%ld\r\n",
                                                  s->p_name,
                                                  (long)(*(int32_t *)s->p_max),
                                                  (long)(*(int32_t *)s->p_min),
                                                  (long)(*(int32_t *)s->p_var));
            break;

        case SHELL_FP32:
            g_list_print_ctx.my_printf->my_printf("%s\tFP32\t(%f)\t(%f)\t%f\r\n",
                                                  s->p_name,
                                                  (double)(*(float *)s->p_max),
                                                  (double)(*(float *)s->p_min),
                                                  (double)(*(float *)s->p_var));
            break;
        default:
            break;
        }

        g_list_print_ctx.cur = s->p_next;
        print_flag = 1u;
        return 1;
    }

    g_list_print_ctx.my_printf->my_printf("-----------------------------------------\r\n");
    print_flag = 0u;
    return 1;
}

static void list_print_task(void)
{
    if (g_list_print_ctx.active != 0u)
    {
        list_print_step();
    }
}

REG_TASK_MS(10, list_print_task)

#pragma GCC diagnostic pop

#endif /* SHELL_STRING_PARSE */

/* Context used when a remote peer requests a full shell item enumeration. */
static shell_report_ctx_t shell_report_ctx = {0};

static void shell_data_num_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint32_t shell_data_num = shell_count_get();

    /* Ignore replies. A fresh host request always restarts the report cursor. */
    if (p_pack->is_ack != 0u)
    {
        return;
    }
    section_packform_t pack_ret = {0};
    pack_ret.src = p_pack->dst;
    pack_ret.d_src = p_pack->d_dst;
    pack_ret.dst = p_pack->src;
    pack_ret.d_dst = p_pack->d_src;
    pack_ret.cmd_set = CMD_SET_SHELL_DATA_NUM;
    pack_ret.cmd_word = CMD_WORD_SHELL_DATA_NUM;
    pack_ret.is_ack = 1u;
    pack_ret.len = sizeof(uint32_t);
    pack_ret.p_data = (uint8_t *)&shell_data_num;

    /* Save the routing information so the follow-up report task can stream data. */
    shell_report_ctx.active = 1u;
    shell_report_ctx.my_printf = my_printf;
    shell_report_ctx.p_shell = shell_first_get();
    shell_report_ctx.src = pack_ret.src;
    shell_report_ctx.d_src = pack_ret.d_src;
    shell_report_ctx.dst = pack_ret.dst;
    shell_report_ctx.d_dst = pack_ret.d_dst;

    comm_send_data(&pack_ret, my_printf);
}

REG_COMM(CMD_SET_SHELL_DATA_NUM, CMD_WORD_SHELL_DATA_NUM, shell_data_num_act)

static void shell_data_report_act(void)
{
    if (shell_report_ctx.active != 0u)
    {
        if (shell_report_ctx.p_shell == NULL)
        {
            /* End the report session when every shell item has been emitted. */
            shell_report_ctx.active = 0u;
        }
        else
        {
            section_packform_t packform = {0};

            /* Convert the current shell entry into one compact report frame. */
            shell_report_list_t shell_report_list;
            shell_report_list.name_len = shell_report_ctx.p_shell->p_name_size;
            shell_report_list.type = shell_report_ctx.p_shell->type;
            shell_report_list.data = *(uint32_t *)shell_report_ctx.p_shell->p_var;
            shell_report_list.data_max = *(uint32_t *)shell_report_ctx.p_shell->p_max;
            shell_report_list.data_min = *(uint32_t *)shell_report_ctx.p_shell->p_min;
            memcpy(shell_report_list.name, shell_report_ctx.p_shell->p_name, shell_report_ctx.p_shell->p_name_size);
            shell_report_list.auto_report = (shell_report_ctx.p_shell->status & (1u << 2)) ? 1u : 0u;

            packform.src = shell_report_ctx.src;
            packform.d_src = shell_report_ctx.d_src;
            packform.dst = shell_report_ctx.dst;
            packform.d_dst = shell_report_ctx.d_dst;
            packform.cmd_set = CMD_SET_SHELL_REPORT_LIST;
            packform.cmd_word = CMD_WORD_SHELL_REPORT_LIST;
            packform.is_ack = 0u;
            packform.len = sizeof(shell_report_list_t) - SHELL_STR_SIZE_MAX + shell_report_ctx.p_shell->p_name_size;
            packform.p_data = (uint8_t *)&shell_report_list;
            comm_send_data(&packform, shell_report_ctx.my_printf);
            shell_report_ctx.p_shell = shell_report_ctx.p_shell->p_next;
        }
    }
}

REG_TASK_MS(50, shell_data_report_act)



static void shell_read_data_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    shell_read_data_t *p_shell_read_data;

    if (p_pack == NULL)
    {
        return;
    }
    p_shell_read_data = (shell_read_data_t *)p_pack->p_data;
    /* Validate the variable-length payload before dereferencing its name field. */
    if (p_pack->len != sizeof(shell_read_data_t) - SHELL_STR_SIZE_MAX + p_shell_read_data->name_len)
    {
        return;
    }
    section_shell_t *p = shell_find(p_shell_read_data->name, p_shell_read_data->name_len);
    if (p != NULL)
    {
        if (p->func != NULL)
        {
            /* Let the owner refresh its value before it is reported. */
            p->func(my_printf);
        }
        shell_read_data_ret_t shell_read_data_ret = {0};
        shell_read_data_ret.name_len = p->p_name_size;
        shell_read_data_ret.type = p->type;
        shell_read_data_ret.data = *(uint32_t *)p->p_var;
        memcpy(shell_read_data_ret.name, p->p_name, p->p_name_size);

        section_packform_t packform = {0};
        packform.src = p_pack->dst;
        packform.d_src = p_pack->d_dst;
        packform.dst = p_pack->src;
        packform.d_dst = p_pack->d_src;
        packform.cmd_set = CMD_SET_SHELL_READ_DATA;
        packform.cmd_word = CMD_WORD_SHELL_READ_DATA;
        packform.is_ack = 1u;
        packform.len = sizeof(shell_read_data_ret_t) - SHELL_STR_SIZE_MAX + p->p_name_size;
        packform.p_data = (uint8_t *)&shell_read_data_ret;

        comm_send_data(&packform, my_printf);
    }
}

REG_COMM(CMD_SET_SHELL_READ_DATA, CMD_WORD_SHELL_READ_DATA, shell_read_data_act)

static void shell_write_data_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    shell_write_data_t *p_shell_write_data;
    section_shell_t *p;

    if (p_pack == NULL)
    {
        return;
    }
    p_shell_write_data = (shell_write_data_t *)p_pack->p_data;
    p = shell_find(p_shell_write_data->name, p_shell_write_data->name_len);
    if (p != NULL)
    {
        /* Remote write shares the same data model as the local shell entry. */
        switch (p->type)
        {
        case SHELL_CMD:
            /* Commands are not writable via the generic write-data path. */
            break;
        case SHELL_UINT8:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(uint8_t));
            uint8_t val = *(uint8_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint8_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint8_t));
            /* Re-apply range clamp after the new remote limits are installed. */
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_INT8:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(int8_t));
            int8_t val = *(int8_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int8_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int8_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_UINT16:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(uint16_t));
            uint16_t val = *(uint16_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint16_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint16_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_INT16:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(int16_t));
            int16_t val = *(int16_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int16_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int16_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_UINT32:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(uint32_t));
            uint32_t val = *(uint32_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint32_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint32_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_INT32:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(int32_t));
            int32_t val = *(int32_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int32_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int32_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_FP32:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(float));
            float val = *(float *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(float));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(float));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        }
        shell_write_data_ret_t shell_write_data_ret = {0};
        shell_write_data_ret.data = *(uint32_t *)p->p_var;
        shell_write_data_ret.data_max = *(uint32_t *)p->p_max;
        shell_write_data_ret.data_min = *(uint32_t *)p->p_min;
        memcpy(shell_write_data_ret.name, p->p_name, p->p_name_size);
        shell_write_data_ret.name_len = p->p_name_size;
        shell_write_data_ret.type = p->type;

        section_packform_t packform = {0};
        packform.src = p_pack->dst;
        packform.d_src = p_pack->d_dst;
        packform.dst = p_pack->src;
        packform.d_dst = p_pack->d_src;
        packform.cmd_set = CMD_SET_SHELL_WRITE_DATA;
        packform.cmd_word = CMD_WORD_SHELL_WRITE_DATA;
        packform.is_ack = 1u;
        packform.len = sizeof(shell_write_data_ret_t) - SHELL_STR_SIZE_MAX + p->p_name_size;
        packform.p_data = (uint8_t *)&shell_write_data_ret;

        comm_send_data(&packform, my_printf);

        if (p->func)
            /* Notify the owner after a successful remote update. */
            p->func(my_printf);
    }
}

REG_COMM(CMD_SET_SHELL_WRITE_DATA, CMD_WORD_SHELL_WRITE_DATA, shell_write_data_act)

static void shell_wave_param_enable_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    shell_wave_enable_param_t *p_shell_wave_enable_param;

    if (p_pack == NULL)
    {
        return;
    }
    p_shell_wave_enable_param = (shell_wave_enable_param_t *)p_pack->p_data;
    /* Validate variable-length payload before touching the embedded name field. */
    if (p_pack->len != sizeof(shell_wave_enable_param_t) - SHELL_STR_SIZE_MAX + p_shell_wave_enable_param->name_len)
    {
        return;
    }
    section_shell_t *p = shell_find(p_shell_wave_enable_param->name, p_shell_wave_enable_param->name_len);

    shell_wave_enable_param_ack_t shell_wave_enable_param_ack;

    if (p != NULL)
    {
        shell_wave_enable_param_ack.ok = 1u;
        /* Bit2 is reserved for wave auto-report selection. */
        if (p_shell_wave_enable_param->auto_report != 0u)
        {
            p->status |= 1u << 2;
        }
        else
        {
            p->status &= ~(1u << 2);
        }
    }

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_SHELL_WAVE_ENABLE_PARAM;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_ENABLE_PARAM;
    packform.src = p_pack->dst;
    packform.dst = p_pack->src;
    packform.is_ack = 1u;
    packform.len = sizeof(shell_wave_enable_param_ack_t);
    packform.p_data = (uint8_t *)&shell_wave_enable_param_ack;
    comm_send_data(&packform, my_printf);
}

REG_COMM(CMD_SET_SHELL_WAVE_ENABLE_PARAM, CMD_WORD_SHELL_WAVE_ENABLE_PARAM, shell_wave_param_enable_act)

/* Global enable for the wave streaming state machine. */
static uint8_t shell_wave_report_flg = 0u;
/* Delay between two wave frames, expressed in shell task ticks. */
static uint32_t shell_wave_report_period = 300u;
/* Down-counter used by the WAIT state. */
static uint32_t shell_wave_report_dn_cnt = 0u;
/* Cached output path used to send wave data back to the requester. */
static section_link_tx_func_t *p_shell_wave_report_printf;
/* Cached source address for wave packets. */
static uint8_t shell_wave_src = 0u;
/* Cached destination address for wave packets. */
static uint8_t shell_wave_dst = 0u;

static void shell_wave_start_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->len != sizeof(shell_wave_start_t))
    {
        return;
    }
    shell_wave_start_t *p_shell_wave_start = (shell_wave_start_t *)p_pack->p_data;
    shell_wave_report_flg = p_shell_wave_start->start_report;

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_SHELL_WAVE_START;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_START;
    packform.src = p_pack->dst;
    packform.dst = p_pack->src;
    packform.is_ack = 1u;
    packform.len = 0u;
    packform.p_data = NULL;
    comm_send_data(&packform, my_printf);
    /* Cache the response route so the periodic task can keep streaming later. */
    p_shell_wave_report_printf = my_printf;
    shell_wave_src = packform.src;
    shell_wave_dst = packform.dst;
}
REG_COMM(CMD_SET_SHELL_WAVE_START, CMD_WORD_SHELL_WAVE_START, shell_wave_start_act)

static void shell_wave_period_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->len != sizeof(shell_wave_period_t))
    {
        return;
    }
    shell_wave_period_t *p_shell_wave_period = (shell_wave_period_t *)p_pack->p_data;
    shell_wave_report_period = p_shell_wave_period->reprot_period;

    shell_wave_period_ack_t shell_wave_period_ack = {.reprot_period = shell_wave_report_period};

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_SHELL_WAVE_PERIOD;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_PERIOD;
    packform.src = p_pack->dst;
    packform.dst = p_pack->src;
    packform.is_ack = 1u;
    packform.len = sizeof(shell_wave_period_ack_t);
    packform.p_data = (uint8_t *)&shell_wave_period_ack;
    comm_send_data(&packform, my_printf);
}
REG_COMM(CMD_SET_SHELL_WAVE_PERIOD, CMD_WORD_SHELL_WAVE_PERIOD, shell_wave_period_act)

static void shell_wave_param_act(shell_wave_param_t *p, DEC_MY_PRINTF)
{
    section_packform_t packform = {0};

    /* Send one wave sample or one frame marker packet. */
    packform.cmd_set = CMD_SET_SHELL_WAVE_PARAM;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_PARAM;
    packform.src = shell_wave_src;
    packform.dst = shell_wave_dst;
    packform.len = sizeof(shell_wave_param_t) - SHELL_STR_SIZE_MAX + p->name_len;
    packform.p_data = (uint8_t *)p;

    comm_send_data(&packform, my_printf);
}

typedef enum
{
    /* Idle until a remote peer enables wave streaming. */
    SHELL_WAVE_FSM_IDLE,
    /* Emit the start marker of one wave frame. */
    SHELL_WAVE_FSM_START,
    /* Walk through all auto-report shell variables. */
    SHELL_WAVE_FSM_DATA,
    /* Emit the end marker of one wave frame. */
    SHELL_WAVE_FSM_END,
    /* Wait the requested inter-frame gap before the next round. */
    SHELL_WAVE_FSM_WAIT,
} SHELL_WAVE_FSM_E;

/* Current state of the shell wave report finite-state machine. */
static SHELL_WAVE_FSM_E shell_wave_fsm = 0;

static void shell_wave_report_task(void)
{
    /* delay_cnt throttles packet rate inside the DATA state. */
    static uint8_t delay_cnt = 0u;
    shell_wave_param_t shell_wave_param = {0};
    /* Cursor used to resume variable scanning across task ticks. */
    static section_shell_t *p = NULL;
    switch (shell_wave_fsm)
    {
    case SHELL_WAVE_FSM_IDLE:
        if (shell_wave_report_flg != 0u)
        {
            shell_wave_fsm = SHELL_WAVE_FSM_START;
        }
        break;
    case SHELL_WAVE_FSM_START:
        /* 0x55555555 marks the beginning of one streamed frame. */
        shell_wave_param.data = 0x55555555;
        shell_wave_param_act(&shell_wave_param, p_shell_wave_report_printf);
        p = shell_first_get();
        shell_wave_fsm = SHELL_WAVE_FSM_DATA;
        delay_cnt = 10u;
        break;
    case SHELL_WAVE_FSM_DATA:
        if (delay_cnt != 0u)
        {
            delay_cnt--;
            break;
        }
        else
        {
            delay_cnt = 10u;
        }
        while (p != NULL)
        {
            if (p->status & (1u << 2))
            {
                /* Bit2-selected shell variables are streamed one by one. */
                shell_wave_param.data = (uint32_t)*(uint32_t *)p->p_var;
                shell_wave_param.name_len = p->p_name_size;
                shell_wave_param.type = p->type;
                memcpy((uint8_t *)shell_wave_param.name, (uint8_t *)p->p_name, shell_wave_param.name_len);
                p = p->p_next;
                shell_wave_param_act(&shell_wave_param, p_shell_wave_report_printf);
                break;
            }
            p = p->p_next;
        }
        if (p == NULL)
        {
            shell_wave_fsm = SHELL_WAVE_FSM_END;
        }
        break;
    case SHELL_WAVE_FSM_END:
        /* 0xAAAAAAAA marks the end of one streamed frame. */
        shell_wave_param.data = 0xAAAAAAAA;
        shell_wave_param_act(&shell_wave_param, p_shell_wave_report_printf);
        shell_wave_fsm = SHELL_WAVE_FSM_WAIT;
        shell_wave_report_dn_cnt = shell_wave_report_period;
        break;
    case SHELL_WAVE_FSM_WAIT:
        DN_CNT(shell_wave_report_dn_cnt);
        if (shell_wave_report_dn_cnt == 0u)
        {
            shell_wave_fsm = SHELL_WAVE_FSM_START;
        }
        if (shell_wave_report_flg == 0u)
        {
            shell_wave_fsm = SHELL_WAVE_FSM_IDLE;
        }
        break;
    }
}

REG_TASK_MS(1, shell_wave_report_task)

