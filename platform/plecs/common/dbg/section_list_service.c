// SPDX-License-Identifier: MIT
/**
 * @file    section_list_service.c
 * @brief   Runtime section-list debug protocol service.
 * @details
 *          This file is part of the base PLECS platform project.
 *
 *          Module responsibilities:
 *          - Build and validate the directory of debug-visible Windows linker registrations
 *          - Reply to directory queries with list identity, name, and node count
 *          - Reply to node queries with registered business-object addresses for map lookup
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Protocol handlers run outside interrupt context
 *          - Windows linker-section access is isolated to this PLECS service adapter
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "section_list_service.h"

#include "comm.h"
#include "section.h"

#include <stddef.h>
#include <stdint.h>

#define SECTION_LIST_DIRECTORY_REQUEST_SIZE (2u)
#define SECTION_LIST_NODE_REQUEST_SIZE (6u)
#define SECTION_LIST_DIRECTORY_SUCCESS_FIXED_SIZE (13u)
#define SECTION_LIST_DIRECTORY_ERROR_SIZE (6u)
#define SECTION_LIST_NODE_SUCCESS_SIZE (16u)
#define SECTION_LIST_NODE_ERROR_SIZE (12u)

static void section_list_service_u16_write(uint8_t *p_dst, uint16_t value)
{
    p_dst[0] = (uint8_t)(value & 0xFFu);
    p_dst[1] = (uint8_t)((value >> 8u) & 0xFFu);
}

static void section_list_service_u32_write(uint8_t *p_dst, uint32_t value)
{
    p_dst[0] = (uint8_t)(value & 0xFFu);
    p_dst[1] = (uint8_t)((value >> 8u) & 0xFFu);
    p_dst[2] = (uint8_t)((value >> 16u) & 0xFFu);
    p_dst[3] = (uint8_t)((value >> 24u) & 0xFFu);
}

static uint16_t section_list_service_u16_read(const uint8_t *p_src)
{
    return (uint16_t)((uint16_t)p_src[0] | ((uint16_t)p_src[1] << 8u));
}

static uint32_t section_list_service_u32_read(const uint8_t *p_src)
{
    return (uint32_t)p_src[0] |
           ((uint32_t)p_src[1] << 8u) |
           ((uint32_t)p_src[2] << 16u) |
           ((uint32_t)p_src[3] << 24u);
}

static uint8_t section_list_service_name_len(const char *p_name)
{
    uint8_t length = 0u;

    if (p_name == NULL)
    {
        return 0u;
    }

    while ((length < SECTION_LIST_NAME_LEN_MAX) && (p_name[length] != '\0'))
    {
        length++;
    }
    return length;
}

static uint32_t section_list_service_node_count(const section_item_t *p_first)
{
    const section_item_t *p_item = p_first;
    uint32_t count = 0u;

    while ((p_item != NULL) && (count < UINT32_MAX))
    {
        count++;
        p_item = p_item->p_next;
    }
    return count;
}

static section_item_t *section_list_service_node_get(section_item_t *p_first,
                                                      uint32_t node_index)
{
    section_item_t *p_item = p_first;
    uint32_t current_index = 0u;

    while ((p_item != NULL) && (current_index < node_index))
    {
        p_item = p_item->p_next;
        current_index++;
    }
    return p_item;
}

static uint8_t section_list_service_registration_is_valid(
    const section_list_registration_t *p_registration)
{
    uint8_t name_length = 0u;

    if ((p_registration == NULL) ||
        (p_registration->p_name == NULL) ||
        (p_registration->pp_head == NULL))
    {
        return 0u;
    }

    name_length = section_list_service_name_len(p_registration->p_name);
    if ((name_length == 0u) || (p_registration->p_name[name_length] != '\0'))
    {
        return 0u;
    }
    return 1u;
}

static const section_list_registration_t *section_list_service_registration_get(
    uint16_t directory_index)
{
    uint16_t current_index = 0u;
    const reg_section_t *p_section = NULL;
    const reg_section_t *p_section_first = NULL;
    const reg_section_t *p_section_last = NULL;

#if defined(SECTION_SENTINEL_REG_SECTION)
    extern const reg_section_t section_reg_start;
    extern const reg_section_t section_reg_stop;
    p_section_first = &section_reg_start + 1;
    p_section_last = &section_reg_stop;
#else
    p_section_first = (const reg_section_t *)&SECTION_START;
    p_section_last = (const reg_section_t *)&SECTION_STOP;
#endif

    for (p_section = p_section_first;
         p_section < p_section_last;
         p_section++)
    {
        if (p_section->section_type == (uint32_t)SECTION_DBG_LIST)
        {
            const section_item_t *p_item = (const section_item_t *)p_section->p_str;
            const section_list_registration_t *p_registration =
                (p_item == NULL) ? NULL : (const section_list_registration_t *)p_item->p_obj;

            if (section_list_service_registration_is_valid(p_registration) != 0u)
            {
                if (current_index == directory_index)
                {
                    return p_registration;
                }
                if (current_index < UINT16_MAX)
                {
                    current_index++;
                }
            }
        }
    }
    return NULL;
}

static uint16_t section_list_service_registration_count(void)
{
    uint16_t count = 0u;

    while ((count < UINT16_MAX) &&
           (section_list_service_registration_get(count) != NULL))
    {
        count++;
    }
    return count;
}

static const section_list_registration_t *section_list_service_find(uint16_t list_id)
{
    if (list_id == 0u)
    {
        return NULL;
    }
    return section_list_service_registration_get((uint16_t)(list_id - 1u));
}

static void section_list_service_reply(section_packform_t *p_request,
                                       DEC_MY_PRINTF,
                                       uint8_t command_word,
                                       uint8_t *p_payload,
                                       uint16_t payload_length)
{
    section_packform_t reply = {0};

    reply.cmd_set = CMD_SET_SECTION_LIST;
    reply.cmd_word = command_word;
    reply.dst = p_request->src;
    reply.d_dst = p_request->d_src;
    reply.src = p_request->dst;
    reply.d_src = p_request->d_dst;
    reply.is_ack = 1u;
    reply.len = payload_length;
    reply.p_data = p_payload;
    comm_send_data(&reply, my_printf);
}

static void section_list_directory_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t payload[SECTION_LIST_DIRECTORY_SUCCESS_FIXED_SIZE + SECTION_LIST_NAME_LEN_MAX] = {0};
    uint16_t requested_index = 0u;
    uint16_t list_count = section_list_service_registration_count();
    uint8_t status = (uint8_t)SECTION_LIST_STATUS_OK;
    uint16_t payload_length = SECTION_LIST_DIRECTORY_ERROR_SIZE;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if ((p_pack->p_data == NULL) || (p_pack->len < SECTION_LIST_DIRECTORY_REQUEST_SIZE))
    {
        status = (uint8_t)SECTION_LIST_STATUS_INVALID_REQUEST;
    }
    else
    {
        requested_index = section_list_service_u16_read(p_pack->p_data);
        if (requested_index >= list_count)
        {
            status = (uint8_t)SECTION_LIST_STATUS_INDEX_INVALID;
        }
    }

    payload[0] = SECTION_LIST_PROTOCOL_VERSION;
    payload[1] = status;
    section_list_service_u16_write(&payload[2], requested_index);
    section_list_service_u16_write(&payload[4], list_count);

    if (status == (uint8_t)SECTION_LIST_STATUS_OK)
    {
        const section_list_registration_t *p_registration =
            section_list_service_registration_get(requested_index);
        const section_item_t *p_first = *p_registration->pp_head;
        uint8_t name_length = section_list_service_name_len(p_registration->p_name);

        section_list_service_u16_write(&payload[6], (uint16_t)(requested_index + 1u));
        section_list_service_u32_write(&payload[8], section_list_service_node_count(p_first));
        payload[12] = name_length;
        for (uint8_t index = 0u; index < name_length; index++)
        {
            payload[13u + index] = (uint8_t)p_registration->p_name[index];
        }
        payload_length = (uint16_t)(13u + name_length);
    }

    section_list_service_reply(p_pack,
                               my_printf,
                               CMD_WORD_SECTION_LIST_DIRECTORY,
                               payload,
                               payload_length);
}

static void section_list_node_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t payload[SECTION_LIST_NODE_SUCCESS_SIZE] = {0};
    uint16_t list_id = 0u;
    uint32_t node_index = 0u;
    uint32_t node_count = 0u;
    uint8_t status = (uint8_t)SECTION_LIST_STATUS_OK;
    uint16_t payload_length = SECTION_LIST_NODE_SUCCESS_SIZE;
    const section_list_registration_t *p_registration = NULL;

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    if ((p_pack->p_data == NULL) || (p_pack->len < SECTION_LIST_NODE_REQUEST_SIZE))
    {
        status = (uint8_t)SECTION_LIST_STATUS_INVALID_REQUEST;
    }
    else
    {
        list_id = section_list_service_u16_read(p_pack->p_data);
        node_index = section_list_service_u32_read(&p_pack->p_data[2]);
        p_registration = section_list_service_find(list_id);
        if (p_registration == NULL)
        {
            status = (uint8_t)SECTION_LIST_STATUS_LIST_ID_INVALID;
        }
        else
        {
            if (p_registration->pp_head == NULL)
            {
                status = (uint8_t)SECTION_LIST_STATUS_REGISTRATION_INVALID;
            }
            else
            {
                section_item_t *p_first = *p_registration->pp_head;
                section_item_t *p_item = NULL;
                node_count = section_list_service_node_count(p_first);
                p_item = section_list_service_node_get(p_first, node_index);
                if (p_item == NULL)
                {
                    status = (uint8_t)SECTION_LIST_STATUS_NODE_INDEX_INVALID;
                }
                else
                {
                    uint32_t address = (uint32_t)(uintptr_t)p_item->p_obj;
                    if (address == 0u)
                    {
                        status = (uint8_t)SECTION_LIST_STATUS_ADDRESS_UNAVAILABLE;
                    }
                    section_list_service_u32_write(&payload[12], address);
                }
            }
        }
    }

    payload[0] = SECTION_LIST_PROTOCOL_VERSION;
    payload[1] = status;
    section_list_service_u16_write(&payload[2], list_id);
    section_list_service_u32_write(&payload[4], node_index);
    section_list_service_u32_write(&payload[8], node_count);
    if (status != (uint8_t)SECTION_LIST_STATUS_OK)
    {
        payload_length = SECTION_LIST_NODE_ERROR_SIZE;
    }

    section_list_service_reply(p_pack,
                               my_printf,
                               CMD_WORD_SECTION_LIST_NODE,
                               payload,
                               payload_length);
}

REG_COMM(CMD_SET_SECTION_LIST, CMD_WORD_SECTION_LIST_DIRECTORY, section_list_directory_act)
REG_COMM(CMD_SET_SECTION_LIST, CMD_WORD_SECTION_LIST_NODE, section_list_node_act)
