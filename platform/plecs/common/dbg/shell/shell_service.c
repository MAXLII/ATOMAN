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
 * @date    2026-08-02
 * @version 2.0.0
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
#include "plecs.h"

#include <math.h>
#include <string.h>

#if SHELL_STRING_ENABLE == 1u

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
void shell_status_run(void)
{
    const section_item_t *p_list = p_shell_first;

    /* Periodically service status-triggered shell items. */
    for (const section_item_t *p_item = p_list; p_item != NULL; p_item = p_item->p_next)
    {
        section_shell_t *p = (section_shell_t *)p_item->p_obj;
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
                shell_item_print(p, (section_link_tx_func_t *)p->my_printf);
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

static void list_print_item(section_shell_t *p_item, DEC_MY_PRINTF)
{
    if ((p_item == NULL) ||
        (my_printf == NULL) ||
        (my_printf->my_printf == NULL))
    {
        return;
    }

    switch (p_item->type)
    {
    case SHELL_CMD:
        my_printf->my_printf("%s\tCMD\r\n", p_item->p_name);
        break;
    case SHELL_UINT8:
        my_printf->my_printf("%s\tU8\t(%u)\t(%u)\t%u\r\n",
                             p_item->p_name,
                             (unsigned)(*(uint8_t *)p_item->p_max),
                             (unsigned)(*(uint8_t *)p_item->p_min),
                             (unsigned)(*(uint8_t *)p_item->p_var));
        break;
    case SHELL_UINT16:
        my_printf->my_printf("%s\tU16\t(%u)\t(%u)\t%u\r\n",
                             p_item->p_name,
                             (unsigned)(*(uint16_t *)p_item->p_max),
                             (unsigned)(*(uint16_t *)p_item->p_min),
                             (unsigned)(*(uint16_t *)p_item->p_var));
        break;
    case SHELL_UINT32:
        my_printf->my_printf("%s\tU32\t(%lu)\t(%lu)\t%lu\r\n",
                             p_item->p_name,
                             (unsigned long)(*(uint32_t *)p_item->p_max),
                             (unsigned long)(*(uint32_t *)p_item->p_min),
                             (unsigned long)(*(uint32_t *)p_item->p_var));
        break;
    case SHELL_INT8:
        my_printf->my_printf("%s\tI8\t(%d)\t(%d)\t%d\r\n",
                             p_item->p_name,
                             (int)(*(int8_t *)p_item->p_max),
                             (int)(*(int8_t *)p_item->p_min),
                             (int)(*(int8_t *)p_item->p_var));
        break;
    case SHELL_INT16:
        my_printf->my_printf("%s\tI16\t(%d)\t(%d)\t%d\r\n",
                             p_item->p_name,
                             (int)(*(int16_t *)p_item->p_max),
                             (int)(*(int16_t *)p_item->p_min),
                             (int)(*(int16_t *)p_item->p_var));
        break;
    case SHELL_INT32:
        my_printf->my_printf("%s\tI32\t(%ld)\t(%ld)\t%ld\r\n",
                             p_item->p_name,
                             (long)(*(int32_t *)p_item->p_max),
                             (long)(*(int32_t *)p_item->p_min),
                             (long)(*(int32_t *)p_item->p_var));
        break;
    case SHELL_FP32:
        my_printf->my_printf("%s\tFP32\t(%f)\t(%f)\t%f\r\n",
                             p_item->p_name,
                             (double)(*(float *)p_item->p_max),
                             (double)(*(float *)p_item->p_min),
                             (double)(*(float *)p_item->p_var));
        break;
    default:
        break;
    }
}

void list_print_start(DEC_MY_PRINTF)
{
    const section_item_t *p_item = p_shell_first;

    if ((my_printf == NULL) ||
        (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("\r\n==================== SHELL COMMANDS AND VARIABLES ====================\r\n");
    while (p_item != NULL)
    {
        list_print_item((section_shell_t *)p_item->p_obj, my_printf);
        my_printf->my_printf("-----------------------------------------\r\n");
        p_item = p_item->p_next;
    }
}

REG_SHELL_CMD(list, list_print_start)

#pragma GCC diagnostic pop

#endif /* SHELL_STRING_ENABLE */

#define SHELL_REPORT_LIST_FIXED_SIZE (sizeof(shell_report_list_t) - SHELL_STR_SIZE_MAX)

static uint16_t shell_data_report_encode(const section_shell_t *p_shell,
                                         uint8_t *p_payload,
                                         uint16_t payload_capacity)
{
    shell_report_list_t shell_report_list = {0};
    uint16_t record_size = 0u;

    if ((p_shell == NULL) ||
        (p_payload == NULL) ||
        (p_shell->p_name == NULL) ||
        (p_shell->p_name_size > SHELL_STR_SIZE_MAX))
    {
        return 0u;
    }

    record_size = (uint16_t)(SHELL_REPORT_LIST_FIXED_SIZE + p_shell->p_name_size);
    if (record_size > payload_capacity)
    {
        return 0u;
    }

    shell_report_list.name_len = (uint8_t)p_shell->p_name_size;
    shell_report_list.type = (uint8_t)p_shell->type;
    if ((p_shell->type != SHELL_CMD) &&
        (p_shell->p_var != NULL) &&
        (p_shell->p_max != NULL) &&
        (p_shell->p_min != NULL))
    {
        shell_report_list.data = *(uint32_t *)p_shell->p_var;
        shell_report_list.data_max = *(uint32_t *)p_shell->p_max;
        shell_report_list.data_min = *(uint32_t *)p_shell->p_min;
    }
    memcpy(shell_report_list.name, p_shell->p_name, p_shell->p_name_size);
    shell_report_list.auto_report = (p_shell->status & (1u << 2)) ? 1u : 0u;

    (void)memcpy(p_payload, &shell_report_list, record_size);
    return record_size;
}

static void shell_data_batch_send(section_packform_t *p_request,
                                  uint32_t total_count,
                                  uint32_t first_index,
                                  uint16_t item_count,
                                  uint8_t *p_payload,
                                  uint16_t payload_len,
                                  DEC_MY_PRINTF)
{
    shell_report_list_batch_header_t header = {0};
    section_packform_t packform = {0};

    if ((p_request == NULL) ||
        (p_payload == NULL) ||
        (payload_len < sizeof(header)))
    {
        return;
    }

    header.total_count = total_count;
    header.first_index = first_index;
    header.item_count = item_count;
    (void)memcpy(p_payload, &header, sizeof(header));

    packform.src = p_request->dst;
    packform.d_src = p_request->d_dst;
    packform.dst = p_request->src;
    packform.d_dst = p_request->d_src;
    packform.cmd_set = CMD_SET_SHELL_REPORT_LIST_BATCH;
    packform.cmd_word = CMD_WORD_SHELL_REPORT_LIST_BATCH;
    packform.is_ack = 0u;
    packform.len = payload_len;
    packform.p_data = p_payload;
    comm_send_data(&packform, my_printf);
}

static void shell_data_report_batches_send(section_packform_t *p_request,
                                           uint32_t total_count,
                                           DEC_MY_PRINTF)
{
    static uint8_t payload[COMM_MAX_PAYLOAD_SIZE] = {0};
    section_item_t *p_item = p_shell_first;
    uint32_t next_index = 0u;

    while (p_item != NULL)
    {
        const uint32_t first_index = next_index;
        uint16_t payload_len = (uint16_t)sizeof(shell_report_list_batch_header_t);
        uint16_t item_count = 0u;

        while (p_item != NULL)
        {
            const section_shell_t *p_shell = (const section_shell_t *)p_item->p_obj;
            uint16_t record_size = 0u;

            if (p_shell != NULL)
            {
                record_size = shell_data_report_encode(
                    p_shell,
                    &payload[payload_len],
                    (uint16_t)(COMM_MAX_PAYLOAD_SIZE - payload_len));
            }
            if (record_size == 0u)
            {
                if (item_count != 0u)
                {
                    break;
                }
                p_item = p_item->p_next;
                ++next_index;
                continue;
            }

            payload_len = (uint16_t)(payload_len + record_size);
            ++item_count;
            ++next_index;
            p_item = p_item->p_next;
        }

        if (item_count != 0u)
        {
            shell_data_batch_send(p_request,
                                  total_count,
                                  first_index,
                                  item_count,
                                  payload,
                                  payload_len,
                                  my_printf);
        }
    }
}

static void shell_data_num_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint32_t shell_data_num = shell_count_get();

    if ((p_pack == NULL) ||
        (p_pack->is_ack != 0u))
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
    comm_send_data(&pack_ret, my_printf);

    shell_data_report_batches_send(p_pack, shell_data_num, my_printf);
}

REG_COMM(CMD_SET_SHELL_DATA_NUM, CMD_WORD_SHELL_DATA_NUM, shell_data_num_act)

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
    if ((p != NULL) &&
        (p->type != SHELL_CMD) &&
        (p->p_var != NULL))
    {
        shell_read_data_ret_t shell_read_data_ret = {0};
        shell_read_data_ret.name_len = (uint8_t)p->p_name_size;
        shell_read_data_ret.type = (uint8_t)p->type;
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
        packform.len = (uint16_t)(sizeof(shell_read_data_ret_t) - SHELL_STR_SIZE_MAX + p->p_name_size);
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
    if ((p_pack->len < (sizeof(shell_write_data_t) - SHELL_STR_SIZE_MAX)) ||
        (p_shell_write_data->name_len > SHELL_STR_SIZE_MAX) ||
        (p_pack->len != (sizeof(shell_write_data_t) - SHELL_STR_SIZE_MAX + p_shell_write_data->name_len)))
    {
        return;
    }
    p = shell_find(p_shell_write_data->name, p_shell_write_data->name_len);
    if ((p != NULL) &&
        (p->type == SHELL_CMD) &&
        (p->func != NULL))
    {
        shell_write_data_ret_t shell_write_data_ret = {0};
        section_packform_t packform = {0};

        /*
         * FRAME represents command execution with the existing write-data
         * request and zeroed value fields.  A binary transport has no Shell
         * text stream, so execute without a print interface and return the
         * normal write ACK shape expected by FRAME.
         */
        p->func(NULL);

        shell_write_data_ret.name_len = (uint8_t)p->p_name_size;
        shell_write_data_ret.type = (uint8_t)p->type;
        memcpy(shell_write_data_ret.name, p->p_name, p->p_name_size);

        packform.src = p_pack->dst;
        packform.d_src = p_pack->d_dst;
        packform.dst = p_pack->src;
        packform.d_dst = p_pack->d_src;
        packform.cmd_set = CMD_SET_SHELL_WRITE_DATA;
        packform.cmd_word = CMD_WORD_SHELL_WRITE_DATA;
        packform.is_ack = 1u;
        packform.len = (uint16_t)(sizeof(shell_write_data_ret_t) - SHELL_STR_SIZE_MAX + p->p_name_size);
        packform.p_data = (uint8_t *)&shell_write_data_ret;

        comm_send_data(&packform, my_printf);
    }
    else if ((p != NULL) &&
        (p->type != SHELL_CMD) &&
        (p->p_var != NULL) &&
        (p->p_max != NULL) &&
        (p->p_min != NULL) &&
        ((p->status & SHELL_STA_READ_ONLY) == 0u))
    {
        /* Remote write shares the same data model as the local shell entry. */
        switch (p->type)
        {
        case SHELL_CMD:
            /* Commands are not writable via the generic write-data path. */
            break;
        case SHELL_UINT8:
        {
            uint8_t val = 0u;
            memcpy(&val, (uint8_t *)&p_shell_write_data->data, sizeof(val));
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint8_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint8_t));
            /* Re-apply range clamp after the new remote limits are installed. */
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));

            break;
        }
        case SHELL_INT8:
        {
            int8_t val = 0;
            memcpy(&val, (uint8_t *)&p_shell_write_data->data, sizeof(val));
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int8_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int8_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));

            break;
        }
        case SHELL_UINT16:
        {
            uint16_t val = 0u;
            memcpy(&val, (uint8_t *)&p_shell_write_data->data, sizeof(val));
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint16_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint16_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));

            break;
        }
        case SHELL_INT16:
        {
            int16_t val = 0;
            memcpy(&val, (uint8_t *)&p_shell_write_data->data, sizeof(val));
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int16_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int16_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));

            break;
        }
        case SHELL_UINT32:
        {
            uint32_t val = 0u;
            memcpy(&val, (uint8_t *)&p_shell_write_data->data, sizeof(val));
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint32_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint32_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));

            break;
        }
        case SHELL_INT32:
        {
            int32_t val = 0;
            memcpy(&val, (uint8_t *)&p_shell_write_data->data, sizeof(val));
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int32_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int32_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));

            break;
        }
        case SHELL_FP32:
        {
            float val = 0.0f;
            memcpy(&val, (uint8_t *)&p_shell_write_data->data, sizeof(val));
            if (!isfinite(val))
            {
                return;
            }
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(float));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(float));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));

            break;
        }
        }
        shell_write_data_ret_t shell_write_data_ret = {0};
        shell_write_data_ret.data = *(uint32_t *)p->p_var;
        shell_write_data_ret.data_max = *(uint32_t *)p->p_max;
        shell_write_data_ret.data_min = *(uint32_t *)p->p_min;
        memcpy(shell_write_data_ret.name, p->p_name, p->p_name_size);
        shell_write_data_ret.name_len = (uint8_t)p->p_name_size;
        shell_write_data_ret.type = (uint8_t)p->type;

        section_packform_t packform = {0};
        packform.src = p_pack->dst;
        packform.d_src = p_pack->d_dst;
        packform.dst = p_pack->src;
        packform.d_dst = p_pack->d_src;
        packform.cmd_set = CMD_SET_SHELL_WRITE_DATA;
        packform.cmd_word = CMD_WORD_SHELL_WRITE_DATA;
        packform.is_ack = 1u;
        packform.len = (uint16_t)(sizeof(shell_write_data_ret_t) - SHELL_STR_SIZE_MAX + p->p_name_size);
        packform.p_data = (uint8_t *)&shell_write_data_ret;

        comm_send_data(&packform, my_printf);

        if (p->func)
            /* Notify the owner after a successful remote update. */
            p->func((shell_core_io_t *)my_printf);
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
    section_shell_t *p = shell_find(p_shell_wave_enable_param->name,
                                    p_shell_wave_enable_param->name_len);

    shell_wave_enable_param_ack_t shell_wave_enable_param_ack = {0};

    if ((p != NULL) &&
        (p->type != SHELL_CMD) &&
        (p->p_var != NULL))
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
    shell_wave_report_dn_cnt = 0u;

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

#define SHELL_WAVE_RECORD_FIXED_SIZE (sizeof(shell_wave_param_t) - SHELL_STR_SIZE_MAX)

static uint8_t shell_wave_item_is_selected(const section_shell_t *p_shell)
{
    if ((p_shell == NULL) ||
        ((p_shell->status & (1u << 2)) == 0u) ||
        (p_shell->type == SHELL_CMD) ||
        (p_shell->p_var == NULL) ||
        (p_shell->p_name == NULL) ||
        (p_shell->p_name_size > SHELL_STR_SIZE_MAX))
    {
        return 0u;
    }
    return 1u;
}

static uint32_t shell_wave_value_raw_get(const section_shell_t *p_shell)
{
    uint32_t raw_value = 0u;

    switch (p_shell->type)
    {
    case SHELL_INT8:
    case SHELL_UINT8:
        raw_value = (uint32_t)(*(const uint8_t *)p_shell->p_var);
        break;
    case SHELL_INT16:
    case SHELL_UINT16:
        raw_value = (uint32_t)(*(const uint16_t *)p_shell->p_var);
        break;
    case SHELL_INT32:
    case SHELL_UINT32:
        raw_value = *(const uint32_t *)p_shell->p_var;
        break;
    case SHELL_FP32:
        (void)memcpy(&raw_value, p_shell->p_var, sizeof(raw_value));
        break;
    default:
        break;
    }
    return raw_value;
}

static uint16_t shell_wave_record_encode(const section_shell_t *p_shell,
                                         uint8_t *p_payload,
                                         uint16_t payload_capacity)
{
    shell_wave_param_t wave_param = {0};
    uint16_t record_size = 0u;

    if ((shell_wave_item_is_selected(p_shell) == 0u) ||
        (p_payload == NULL))
    {
        return 0u;
    }

    record_size = (uint16_t)(SHELL_WAVE_RECORD_FIXED_SIZE + p_shell->p_name_size);
    if (record_size > payload_capacity)
    {
        return 0u;
    }

    wave_param.name_len = (uint8_t)p_shell->p_name_size;
    wave_param.type = (uint8_t)p_shell->type;
    wave_param.data = shell_wave_value_raw_get(p_shell);
    (void)memcpy(wave_param.name, p_shell->p_name, p_shell->p_name_size);
    (void)memcpy(p_payload, &wave_param, record_size);
    return record_size;
}

static uint32_t shell_wave_selected_count_get(void)
{
    const section_item_t *p_item = p_shell_first;
    uint32_t selected_count = 0u;

    while (p_item != NULL)
    {
        if (shell_wave_item_is_selected((const section_shell_t *)p_item->p_obj) == 1u)
        {
            ++selected_count;
        }
        p_item = p_item->p_next;
    }
    return selected_count;
}

static void shell_wave_batch_send(uint32_t simulation_tick_100us,
                                  uint32_t total_count,
                                  uint32_t first_index,
                                  uint16_t item_count,
                                  uint8_t *p_payload,
                                  uint16_t payload_len,
                                  DEC_MY_PRINTF)
{
    shell_wave_batch_header_t header = {0};
    section_packform_t packform = {0};

    header.simulation_tick_100us = simulation_tick_100us;
    header.total_count = total_count;
    header.first_index = first_index;
    header.item_count = item_count;
    (void)memcpy(p_payload, &header, sizeof(header));

    packform.cmd_set = CMD_SET_SHELL_WAVE_BATCH;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_BATCH;
    packform.src = shell_wave_src;
    packform.dst = shell_wave_dst;
    packform.len = payload_len;
    packform.p_data = p_payload;

    comm_send_data(&packform, my_printf);
}

static void shell_wave_report_task(void)
{
    static uint8_t payload[COMM_MAX_PAYLOAD_SIZE] = {0};
    section_item_t *p_item = NULL;
    uint32_t simulation_tick_100us = 0u;
    uint32_t total_count = 0u;
    uint32_t next_index = 0u;

    if (shell_wave_report_flg == 0u)
    {
        shell_wave_report_dn_cnt = 0u;
        return;
    }

    DN_CNT(shell_wave_report_dn_cnt);
    if (shell_wave_report_dn_cnt != 0u)
    {
        return;
    }

    simulation_tick_100us = __atomic_load_n(&plecs_time_100us, __ATOMIC_RELAXED);
    total_count = shell_wave_selected_count_get();
    p_item = p_shell_first;
    while (p_item != NULL)
    {
        const uint32_t first_index = next_index;
        uint16_t payload_len = (uint16_t)sizeof(shell_wave_batch_header_t);
        uint16_t item_count = 0u;

        while (p_item != NULL)
        {
            const section_shell_t *p_shell = (const section_shell_t *)p_item->p_obj;
            uint16_t record_size = 0u;

            if (shell_wave_item_is_selected(p_shell) == 0u)
            {
                p_item = p_item->p_next;
                continue;
            }

            record_size = shell_wave_record_encode(
                p_shell,
                &payload[payload_len],
                (uint16_t)(COMM_MAX_PAYLOAD_SIZE - payload_len));
            if (record_size == 0u)
            {
                break;
            }

            payload_len = (uint16_t)(payload_len + record_size);
            ++item_count;
            ++next_index;
            p_item = p_item->p_next;
        }

        if (item_count != 0u)
        {
            shell_wave_batch_send(simulation_tick_100us,
                                  total_count,
                                  first_index,
                                  item_count,
                                  payload,
                                  payload_len,
                                  p_shell_wave_report_printf);
        }
    }

    shell_wave_report_dn_cnt = shell_wave_report_period;
}

REG_TASK_MS(1, shell_wave_report_task)
