// SPDX-License-Identifier: MIT
/**
 * @file    frame_parameter_service.c
 * @brief   C28x-safe FRAME parameter protocol service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose registered Shell parameters to FRAME list, read, and write commands
 *          - Stream selected parameter values through the FRAME wave protocol
 *          - Serialize every payload through explicit eight-bit little-endian offsets
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - All handlers execute in the cooperative SECTION task context
 *          - Native C28x structures are never treated as physical wire layouts
 *
 * @author  Max.Li
 * @date    2026-09-04
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "comm.h"
#include "f28p55_wire.h"
#include "shell_service.h"

#include <string.h>

#define FRAME_PARAM_COUNT_PAYLOAD_SIZE (4u)
#define FRAME_PARAM_LIST_FIXED_SIZE (15u)
#define FRAME_PARAM_READ_REQUEST_FIXED_SIZE (1u)
#define FRAME_PARAM_READ_ACK_FIXED_SIZE (6u)
#define FRAME_PARAM_WRITE_REQUEST_FIXED_SIZE (13u)
#define FRAME_PARAM_WRITE_ACK_FIXED_SIZE (14u)
#define FRAME_PARAM_WAVE_SELECT_FIXED_SIZE (2u)
#define FRAME_PARAM_WAVE_SELECT_ACK_SIZE (1u)
#define FRAME_PARAM_WAVE_START_SIZE (1u)
#define FRAME_PARAM_WAVE_PERIOD_SIZE (4u)
#define FRAME_PARAM_WAVE_ITEM_FIXED_SIZE (6u)
#define FRAME_PARAM_WAVE_PERIOD_MIN_MS (200u)
#define FRAME_PARAM_WAVE_PERIOD_MAX_MS (60000u)
#define FRAME_PARAM_FP32_EXPONENT_MASK (0x7F800000u)

typedef struct
{
    uint8_t active;                         /* Nonzero while list entries are being reported. */
    section_link_tx_func_t *p_output;       /* Link that owns the current list request. */
    section_item_t *p_item;                 /* Next Shell registration to report. */
    uint8_t source;                         /* Device source address for report frames. */
    uint8_t dynamic_source;                 /* Device dynamic source address. */
    uint8_t destination;                    /* FRAME host destination address. */
    uint8_t dynamic_destination;            /* FRAME host dynamic destination address. */
} frame_parameter_list_context_t;

typedef enum
{
    FRAME_PARAM_WAVE_IDLE = 0,
    FRAME_PARAM_WAVE_START,
    FRAME_PARAM_WAVE_DATA,
    FRAME_PARAM_WAVE_END,
    FRAME_PARAM_WAVE_WAIT
} frame_parameter_wave_state_t;

static frame_parameter_list_context_t s_list_context = {0}; /* Active list-report session. */
static uint8_t s_wave_enabled = 0u; /* Continuous wave-report enable. */
static uint32_t s_wave_period_ms = 300u; /* Delay between complete wave frames. */
static uint32_t s_wave_wait_ms = 0u; /* Remaining delay before the next wave frame. */
static frame_parameter_wave_state_t s_wave_state = FRAME_PARAM_WAVE_IDLE; /* Wave FSM state. */
static section_item_t *s_wave_item = NULL; /* Next registered parameter considered for reporting. */
static section_link_tx_func_t *s_wave_output = NULL; /* Link used by asynchronous wave reports. */
static uint8_t s_wave_source = 0u; /* Device address used by wave reports. */
static uint8_t s_wave_dynamic_source = 0u; /* Device dynamic address used by wave reports. */
static uint8_t s_wave_destination = 0u; /* Host address used by wave reports. */
static uint8_t s_wave_dynamic_destination = 0u; /* Host dynamic address used by wave reports. */

static uint8_t frame_parameter_name_copy(wire_octet_t *p_destination,
                                         const char *p_name,
                                         uint32_t name_length)
{
    uint16_t index; /* Character copied into the logical wire-octet buffer. */

    if ((p_destination == NULL) || (p_name == NULL) ||
        (name_length > SHELL_STR_SIZE_MAX))
    {
        return 0u;
    }

    for (index = 0u; index < (uint16_t)name_length; index++)
    {
        p_destination[index] = wire_octet_get((uint16_t)p_name[index]);
    }
    return (uint8_t)name_length;
}

static uint32_t frame_parameter_scalar_bits_get(const section_shell_t *p_parameter,
                                                const void *p_value)
{
    uint32_t bits = 0u; /* Canonical 32-bit FRAME scalar representation. */

    if ((p_parameter == NULL) || (p_value == NULL))
    {
        return 0u;
    }

    switch ((SHELL_TYPE_E)p_parameter->type)
    {
    case SHELL_INT8:
    case SHELL_UINT8:
        bits = (uint32_t)(*(const uint8_t *)p_value & 0x00FFu);
        break;
    case SHELL_INT16:
    case SHELL_UINT16:
        bits = (uint32_t)(*(const uint16_t *)p_value);
        break;
    case SHELL_INT32:
    case SHELL_UINT32:
        bits = *(const uint32_t *)p_value;
        break;
    case SHELL_FP32:
        (void)memcpy(&bits, p_value, sizeof(bits));
        break;
    case SHELL_CMD:
    default:
        break;
    }
    return bits;
}

static uint8_t frame_parameter_fp32_bits_are_finite(uint32_t bits)
{
    return ((bits & FRAME_PARAM_FP32_EXPONENT_MASK) != FRAME_PARAM_FP32_EXPONENT_MASK) ? 1u : 0u;
}

static void frame_parameter_callback_run(section_shell_t *p_parameter,
                                         const section_link_tx_func_t *p_output)
{
    shell_core_io_t shell_io = {0}; /* Callback-local adapter for the Shell output interface. */

    if ((p_parameter == NULL) || (p_parameter->func == NULL))
    {
        return;
    }
    if (p_output == NULL)
    {
        p_parameter->func(NULL);
        return;
    }

    shell_io.my_printf = p_output->my_printf;
    shell_io.tx_by_dma = p_output->tx_by_dma;
    p_parameter->func(&shell_io);
}

static int16_t frame_parameter_i8_from_bits(uint32_t bits)
{
    uint16_t low_octet = (uint16_t)(bits & 0x00FFu); /* Encoded two's-complement octet. */

    if ((low_octet & 0x0080u) != 0u)
    {
        low_octet |= 0xFF00u;
    }
    return (int16_t)low_octet;
}

static uint8_t frame_parameter_scalar_set(section_shell_t *p_parameter,
                                          uint32_t value_bits,
                                          uint32_t maximum_bits,
                                          uint32_t minimum_bits)
{
    if ((p_parameter == NULL) || (p_parameter->p_var == NULL) ||
        (p_parameter->p_max == NULL) || (p_parameter->p_min == NULL))
    {
        return 0u;
    }

    switch ((SHELL_TYPE_E)p_parameter->type)
    {
    case SHELL_UINT8:
    {
        uint16_t value = (uint16_t)(value_bits & 0x00FFu); /* Requested unsigned 8-bit value. */
        uint16_t maximum = (uint16_t)(maximum_bits & 0x00FFu); /* Requested upper limit. */
        uint16_t minimum = (uint16_t)(minimum_bits & 0x00FFu); /* Requested lower limit. */

        if (minimum > maximum)
        {
            return 0u;
        }
        value = (value > maximum) ? maximum : value;
        value = (value < minimum) ? minimum : value;
        *(uint8_t *)p_parameter->p_var = (uint8_t)value;
        *(uint8_t *)p_parameter->p_max = (uint8_t)maximum;
        *(uint8_t *)p_parameter->p_min = (uint8_t)minimum;
        break;
    }
    case SHELL_INT8:
    {
        int16_t value = frame_parameter_i8_from_bits(value_bits); /* Requested signed 8-bit value. */
        int16_t maximum = frame_parameter_i8_from_bits(maximum_bits); /* Requested upper limit. */
        int16_t minimum = frame_parameter_i8_from_bits(minimum_bits); /* Requested lower limit. */

        if (minimum > maximum)
        {
            return 0u;
        }
        value = (value > maximum) ? maximum : value;
        value = (value < minimum) ? minimum : value;
        *(int8_t *)p_parameter->p_var = (int8_t)value;
        *(int8_t *)p_parameter->p_max = (int8_t)maximum;
        *(int8_t *)p_parameter->p_min = (int8_t)minimum;
        break;
    }
    case SHELL_UINT16:
    {
        uint16_t value = (uint16_t)value_bits; /* Requested unsigned 16-bit value. */
        uint16_t maximum = (uint16_t)maximum_bits; /* Requested upper limit. */
        uint16_t minimum = (uint16_t)minimum_bits; /* Requested lower limit. */

        if (minimum > maximum)
        {
            return 0u;
        }
        value = (value > maximum) ? maximum : value;
        value = (value < minimum) ? minimum : value;
        *(uint16_t *)p_parameter->p_var = value;
        *(uint16_t *)p_parameter->p_max = maximum;
        *(uint16_t *)p_parameter->p_min = minimum;
        break;
    }
    case SHELL_INT16:
    {
        int16_t value = (int16_t)value_bits; /* Requested signed 16-bit value. */
        int16_t maximum = (int16_t)maximum_bits; /* Requested upper limit. */
        int16_t minimum = (int16_t)minimum_bits; /* Requested lower limit. */

        if (minimum > maximum)
        {
            return 0u;
        }
        value = (value > maximum) ? maximum : value;
        value = (value < minimum) ? minimum : value;
        *(int16_t *)p_parameter->p_var = value;
        *(int16_t *)p_parameter->p_max = maximum;
        *(int16_t *)p_parameter->p_min = minimum;
        break;
    }
    case SHELL_UINT32:
    {
        uint32_t value = value_bits; /* Requested unsigned 32-bit value. */

        if (minimum_bits > maximum_bits)
        {
            return 0u;
        }
        value = (value > maximum_bits) ? maximum_bits : value;
        value = (value < minimum_bits) ? minimum_bits : value;
        *(uint32_t *)p_parameter->p_var = value;
        *(uint32_t *)p_parameter->p_max = maximum_bits;
        *(uint32_t *)p_parameter->p_min = minimum_bits;
        break;
    }
    case SHELL_INT32:
    {
        int32_t value = (int32_t)value_bits; /* Requested signed 32-bit value. */
        int32_t maximum = (int32_t)maximum_bits; /* Requested upper limit. */
        int32_t minimum = (int32_t)minimum_bits; /* Requested lower limit. */

        if (minimum > maximum)
        {
            return 0u;
        }
        value = (value > maximum) ? maximum : value;
        value = (value < minimum) ? minimum : value;
        *(int32_t *)p_parameter->p_var = value;
        *(int32_t *)p_parameter->p_max = maximum;
        *(int32_t *)p_parameter->p_min = minimum;
        break;
    }
    case SHELL_FP32:
    {
        float value;   /* Requested floating-point value. */
        float maximum; /* Requested floating-point upper limit. */
        float minimum; /* Requested floating-point lower limit. */

        if ((frame_parameter_fp32_bits_are_finite(value_bits) == 0u) ||
            (frame_parameter_fp32_bits_are_finite(maximum_bits) == 0u) ||
            (frame_parameter_fp32_bits_are_finite(minimum_bits) == 0u))
        {
            return 0u;
        }
        (void)memcpy(&value, &value_bits, sizeof(value));
        (void)memcpy(&maximum, &maximum_bits, sizeof(maximum));
        (void)memcpy(&minimum, &minimum_bits, sizeof(minimum));
        if (minimum > maximum)
        {
            return 0u;
        }
        value = (value > maximum) ? maximum : value;
        value = (value < minimum) ? minimum : value;
        *(float *)p_parameter->p_var = value;
        *(float *)p_parameter->p_max = maximum;
        *(float *)p_parameter->p_min = minimum;
        break;
    }
    case SHELL_CMD:
    default:
        return 0u;
    }
    return 1u;
}

static void frame_parameter_reply(const section_packform_t *p_request,
                                  DEC_MY_PRINTF,
                                  uint8_t command_word,
                                  uint8_t is_ack,
                                  wire_octet_t *p_payload,
                                  uint16_t payload_length)
{
    section_packform_t reply = {0}; /* FRAME response metadata and logical payload. */

    reply.src = p_request->dst;
    reply.d_src = p_request->d_dst;
    reply.dst = p_request->src;
    reply.d_dst = p_request->d_src;
    reply.cmd_set = CMD_SET_SHELL_DATA_NUM;
    reply.cmd_word = command_word;
    reply.is_ack = is_ack;
    reply.len = payload_length;
    reply.p_data = p_payload;
    comm_send_data(&reply, my_printf);
}

static void frame_parameter_count_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_PARAM_COUNT_PAYLOAD_SIZE] = {0}; /* Parameter-count response. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    wire_u32_le_write(payload, shell_count_get());
    s_list_context.active = 1u;
    s_list_context.p_output = my_printf;
    s_list_context.p_item = p_shell_first;
    s_list_context.source = p_pack->dst;
    s_list_context.dynamic_source = p_pack->d_dst;
    s_list_context.destination = p_pack->src;
    s_list_context.dynamic_destination = p_pack->d_src;
    frame_parameter_reply(p_pack,
                          my_printf,
                          CMD_WORD_SHELL_DATA_NUM,
                          1u,
                          payload,
                          FRAME_PARAM_COUNT_PAYLOAD_SIZE);
}

REG_COMM(CMD_SET_SHELL_DATA_NUM, CMD_WORD_SHELL_DATA_NUM, frame_parameter_count_act)

static void frame_parameter_list_task(void)
{
    section_packform_t report = {0}; /* One asynchronous parameter-list report. */
    section_shell_t *p_parameter;    /* Parameter described by this report. */
    wire_octet_t payload[FRAME_PARAM_LIST_FIXED_SIZE + SHELL_STR_SIZE_MAX] = {0}; /* Serialized report. */
    uint8_t name_length; /* Encoded parameter-name length. */

    if (s_list_context.active == 0u)
    {
        return;
    }
    if (s_list_context.p_item == NULL)
    {
        s_list_context.active = 0u;
        return;
    }

    p_parameter = (section_shell_t *)s_list_context.p_item->p_obj;
    s_list_context.p_item = s_list_context.p_item->p_next;
    if (p_parameter == NULL)
    {
        return;
    }

    name_length = frame_parameter_name_copy(&payload[FRAME_PARAM_LIST_FIXED_SIZE],
                                            p_parameter->p_name,
                                            p_parameter->p_name_size);
    payload[0] = name_length;
    payload[1] = wire_octet_get((uint16_t)p_parameter->type);
    wire_u32_le_write(&payload[2], frame_parameter_scalar_bits_get(p_parameter, p_parameter->p_var));
    wire_u32_le_write(&payload[6], frame_parameter_scalar_bits_get(p_parameter, p_parameter->p_max));
    wire_u32_le_write(&payload[10], frame_parameter_scalar_bits_get(p_parameter, p_parameter->p_min));
    payload[14] = ((p_parameter->status & SHELL_STA_AUTO) != 0u) ? 1u : 0u;

    report.src = s_list_context.source;
    report.d_src = s_list_context.dynamic_source;
    report.dst = s_list_context.destination;
    report.d_dst = s_list_context.dynamic_destination;
    report.cmd_set = CMD_SET_SHELL_REPORT_LIST;
    report.cmd_word = CMD_WORD_SHELL_REPORT_LIST;
    report.is_ack = 0u;
    report.len = (uint16_t)(FRAME_PARAM_LIST_FIXED_SIZE + name_length);
    report.p_data = payload;
    comm_send_data(&report, s_list_context.p_output);
}

REG_TASK_MS(10, frame_parameter_list_task)

static void frame_parameter_read_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_shell_t *p_parameter; /* Parameter matched by the request name. */
    wire_octet_t payload[FRAME_PARAM_READ_ACK_FIXED_SIZE + SHELL_STR_SIZE_MAX] = {0}; /* Read response. */
    uint8_t name_length; /* Validated request-name length. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u) ||
        (p_pack->p_data == NULL) ||
        (p_pack->len < FRAME_PARAM_READ_REQUEST_FIXED_SIZE))
    {
        return;
    }

    name_length = wire_octet_get(p_pack->p_data[0]);
    if ((name_length == 0u) || (name_length > SHELL_STR_SIZE_MAX) ||
        (p_pack->len != (uint16_t)(FRAME_PARAM_READ_REQUEST_FIXED_SIZE + name_length)))
    {
        return;
    }

    p_parameter = shell_find((const char *)&p_pack->p_data[1], name_length);
    if (p_parameter == NULL)
    {
        return;
    }
    frame_parameter_callback_run(p_parameter, my_printf);

    payload[0] = frame_parameter_name_copy(&payload[FRAME_PARAM_READ_ACK_FIXED_SIZE],
                                           p_parameter->p_name,
                                           p_parameter->p_name_size);
    payload[1] = wire_octet_get((uint16_t)p_parameter->type);
    wire_u32_le_write(&payload[2], frame_parameter_scalar_bits_get(p_parameter, p_parameter->p_var));
    frame_parameter_reply(p_pack,
                          my_printf,
                          CMD_WORD_SHELL_READ_DATA,
                          1u,
                          payload,
                          (uint16_t)(FRAME_PARAM_READ_ACK_FIXED_SIZE + payload[0]));
}

REG_COMM(CMD_SET_SHELL_READ_DATA, CMD_WORD_SHELL_READ_DATA, frame_parameter_read_act)

static void frame_parameter_write_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_shell_t *p_parameter; /* Writable parameter matched by the request name. */
    wire_octet_t payload[FRAME_PARAM_WRITE_ACK_FIXED_SIZE + SHELL_STR_SIZE_MAX] = {0}; /* Write response. */
    uint8_t name_length; /* Validated request-name length. */
    uint32_t value_bits; /* Requested scalar bit pattern. */
    uint32_t maximum_bits; /* Requested upper-limit bit pattern. */
    uint32_t minimum_bits; /* Requested lower-limit bit pattern. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u) ||
        (p_pack->p_data == NULL) ||
        (p_pack->len < FRAME_PARAM_WRITE_REQUEST_FIXED_SIZE))
    {
        return;
    }

    name_length = wire_octet_get(p_pack->p_data[0]);
    if ((name_length == 0u) || (name_length > SHELL_STR_SIZE_MAX) ||
        (p_pack->len != (uint16_t)(FRAME_PARAM_WRITE_REQUEST_FIXED_SIZE + name_length)))
    {
        return;
    }

    p_parameter = shell_find((const char *)&p_pack->p_data[FRAME_PARAM_WRITE_REQUEST_FIXED_SIZE],
                             name_length);
    if ((p_parameter == NULL) || (p_parameter->type == (uint32_t)SHELL_CMD))
    {
        return;
    }

    value_bits = wire_u32_le_read(&p_pack->p_data[1]);
    maximum_bits = wire_u32_le_read(&p_pack->p_data[5]);
    minimum_bits = wire_u32_le_read(&p_pack->p_data[9]);
    if (frame_parameter_scalar_set(p_parameter,
                                   value_bits,
                                   maximum_bits,
                                   minimum_bits) == 0u)
    {
        return;
    }

    payload[0] = frame_parameter_name_copy(&payload[FRAME_PARAM_WRITE_ACK_FIXED_SIZE],
                                           p_parameter->p_name,
                                           p_parameter->p_name_size);
    payload[1] = wire_octet_get((uint16_t)p_parameter->type);
    wire_u32_le_write(&payload[2], frame_parameter_scalar_bits_get(p_parameter, p_parameter->p_var));
    wire_u32_le_write(&payload[6], frame_parameter_scalar_bits_get(p_parameter, p_parameter->p_max));
    wire_u32_le_write(&payload[10], frame_parameter_scalar_bits_get(p_parameter, p_parameter->p_min));
    frame_parameter_reply(p_pack,
                          my_printf,
                          CMD_WORD_SHELL_WRITE_DATA,
                          1u,
                          payload,
                          (uint16_t)(FRAME_PARAM_WRITE_ACK_FIXED_SIZE + payload[0]));

    frame_parameter_callback_run(p_parameter, my_printf);
}

REG_COMM(CMD_SET_SHELL_WRITE_DATA, CMD_WORD_SHELL_WRITE_DATA, frame_parameter_write_act)

static void frame_parameter_wave_select_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_shell_t *p_parameter; /* Parameter selected for continuous reporting. */
    wire_octet_t payload[FRAME_PARAM_WAVE_SELECT_ACK_SIZE] = {0}; /* Selection result. */
    uint8_t name_length; /* Validated request-name length. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u) ||
        (p_pack->p_data == NULL) ||
        (p_pack->len < FRAME_PARAM_WAVE_SELECT_FIXED_SIZE))
    {
        return;
    }

    name_length = wire_octet_get(p_pack->p_data[0]);
    if ((name_length == 0u) || (name_length > SHELL_STR_SIZE_MAX) ||
        (p_pack->len != (uint16_t)(FRAME_PARAM_WAVE_SELECT_FIXED_SIZE + name_length)))
    {
        return;
    }

    p_parameter = shell_find((const char *)&p_pack->p_data[FRAME_PARAM_WAVE_SELECT_FIXED_SIZE],
                             name_length);
    if ((p_parameter != NULL) && (p_parameter->type != (uint32_t)SHELL_CMD))
    {
        if (wire_octet_get(p_pack->p_data[1]) != 0u)
        {
            p_parameter->status |= SHELL_STA_AUTO;
        }
        else
        {
            p_parameter->status &= ~SHELL_STA_AUTO;
        }
        payload[0] = 1u;
    }

    frame_parameter_reply(p_pack,
                          my_printf,
                          CMD_WORD_SHELL_WAVE_ENABLE_PARAM,
                          1u,
                          payload,
                          FRAME_PARAM_WAVE_SELECT_ACK_SIZE);
}

REG_COMM(CMD_SET_SHELL_WAVE_ENABLE_PARAM,
         CMD_WORD_SHELL_WAVE_ENABLE_PARAM,
         frame_parameter_wave_select_act)

static void frame_parameter_wave_start_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if ((p_pack == NULL) || (p_pack->is_ack != 0u) ||
        (p_pack->p_data == NULL) ||
        (p_pack->len != FRAME_PARAM_WAVE_START_SIZE))
    {
        return;
    }

    s_wave_enabled = (wire_octet_get(p_pack->p_data[0]) != 0u) ? 1u : 0u;
    s_wave_output = my_printf;
    s_wave_source = p_pack->dst;
    s_wave_dynamic_source = p_pack->d_dst;
    s_wave_destination = p_pack->src;
    s_wave_dynamic_destination = p_pack->d_src;
    if (s_wave_enabled == 0u)
    {
        s_wave_state = FRAME_PARAM_WAVE_IDLE;
        s_wave_item = NULL;
    }

    frame_parameter_reply(p_pack,
                          my_printf,
                          CMD_WORD_SHELL_WAVE_START,
                          1u,
                          NULL,
                          0u);
}

REG_COMM(CMD_SET_SHELL_WAVE_START,
         CMD_WORD_SHELL_WAVE_START,
         frame_parameter_wave_start_act)

static void frame_parameter_wave_period_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_PARAM_WAVE_PERIOD_SIZE] = {0}; /* Applied period response. */
    uint32_t requested_period; /* Requested interval between parameter-wave frames. */

    if ((p_pack == NULL) || (p_pack->is_ack != 0u) ||
        (p_pack->p_data == NULL) ||
        (p_pack->len != FRAME_PARAM_WAVE_PERIOD_SIZE))
    {
        return;
    }

    requested_period = wire_u32_le_read(p_pack->p_data);
    requested_period = (requested_period < FRAME_PARAM_WAVE_PERIOD_MIN_MS) ?
                           FRAME_PARAM_WAVE_PERIOD_MIN_MS : requested_period;
    requested_period = (requested_period > FRAME_PARAM_WAVE_PERIOD_MAX_MS) ?
                           FRAME_PARAM_WAVE_PERIOD_MAX_MS : requested_period;
    s_wave_period_ms = requested_period;
    wire_u32_le_write(payload, s_wave_period_ms);
    frame_parameter_reply(p_pack,
                          my_printf,
                          CMD_WORD_SHELL_WAVE_PERIOD,
                          1u,
                          payload,
                          FRAME_PARAM_WAVE_PERIOD_SIZE);
}

REG_COMM(CMD_SET_SHELL_WAVE_PERIOD,
         CMD_WORD_SHELL_WAVE_PERIOD,
         frame_parameter_wave_period_act)

static void frame_parameter_wave_send(uint32_t value_bits,
                                      const section_shell_t *p_parameter)
{
    section_packform_t report = {0}; /* One asynchronous wave marker or value report. */
    wire_octet_t payload[FRAME_PARAM_WAVE_ITEM_FIXED_SIZE + SHELL_STR_SIZE_MAX] = {0}; /* Serialized report. */
    uint8_t name_length = 0u; /* Encoded name length, zero for frame markers. */

    if (p_parameter != NULL)
    {
        name_length = frame_parameter_name_copy(&payload[FRAME_PARAM_WAVE_ITEM_FIXED_SIZE],
                                                p_parameter->p_name,
                                                p_parameter->p_name_size);
        payload[1] = wire_octet_get((uint16_t)p_parameter->type);
    }
    payload[0] = name_length;
    wire_u32_le_write(&payload[2], value_bits);

    report.src = s_wave_source;
    report.d_src = s_wave_dynamic_source;
    report.dst = s_wave_destination;
    report.d_dst = s_wave_dynamic_destination;
    report.cmd_set = CMD_SET_SHELL_WAVE_PARAM;
    report.cmd_word = CMD_WORD_SHELL_WAVE_PARAM;
    report.is_ack = 0u;
    report.len = (uint16_t)(FRAME_PARAM_WAVE_ITEM_FIXED_SIZE + name_length);
    report.p_data = payload;
    comm_send_data(&report, s_wave_output);
}

static void frame_parameter_wave_task(void)
{
    switch (s_wave_state)
    {
    case FRAME_PARAM_WAVE_IDLE:
        if (s_wave_enabled != 0u)
        {
            s_wave_state = FRAME_PARAM_WAVE_START;
        }
        break;

    case FRAME_PARAM_WAVE_START:
        frame_parameter_wave_send(0x55555555u, NULL);
        s_wave_item = p_shell_first;
        s_wave_state = FRAME_PARAM_WAVE_DATA;
        break;

    case FRAME_PARAM_WAVE_DATA:
        while (s_wave_item != NULL)
        {
            section_shell_t *p_parameter = (section_shell_t *)s_wave_item->p_obj; /* Candidate wave parameter. */

            s_wave_item = s_wave_item->p_next;
            if ((p_parameter != NULL) &&
                (p_parameter->type != (uint32_t)SHELL_CMD) &&
                ((p_parameter->status & SHELL_STA_AUTO) != 0u))
            {
                frame_parameter_wave_send(
                    frame_parameter_scalar_bits_get(p_parameter, p_parameter->p_var),
                    p_parameter);
                return;
            }
        }
        s_wave_state = FRAME_PARAM_WAVE_END;
        break;

    case FRAME_PARAM_WAVE_END:
        frame_parameter_wave_send(0xAAAAAAAAu, NULL);
        s_wave_wait_ms = s_wave_period_ms;
        s_wave_state = FRAME_PARAM_WAVE_WAIT;
        break;

    case FRAME_PARAM_WAVE_WAIT:
        if (s_wave_enabled == 0u)
        {
            s_wave_state = FRAME_PARAM_WAVE_IDLE;
        }
        else if (s_wave_wait_ms > 0u)
        {
            s_wave_wait_ms--;
        }
        else
        {
            s_wave_state = FRAME_PARAM_WAVE_START;
        }
        break;

    default:
        s_wave_state = FRAME_PARAM_WAVE_IDLE;
        break;
    }
}

REG_TASK_MS(1, frame_parameter_wave_task)
