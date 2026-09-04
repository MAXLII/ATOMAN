// SPDX-License-Identifier: MIT
/**
 * @file    frame_scope_service.c
 * @brief   C28x-safe FRAME Scope protocol service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Serve every Scope binary command from 0x18 through 0x1F
 *          - Serialize requests and replies at explicit logical-octet offsets
 *          - Preserve deferred list reporting and capture-generation tracking
 *
 *          Design beliefs:
 *          - Entity: one service maps registered Scope objects to FRAME wire payloads
 *          - Prior: request lengths, identifiers, modes, and indexes are untrusted until validated
 *          - Time: list replies advance one item per task tick and capture tags delimit generations
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Native C28x structures are never used as physical wire layouts
 *          - Scope capture state remains owned by scope.c and scope_core.c
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "comm.h"
#include "f280049c_wire.h"
#include "scope_service.h"

#include <stddef.h>

#if (SCOPE_ENABLE == 1u)

#define FRAME_SCOPE_NAME_LENGTH_MAX (64u)
#define FRAME_SCOPE_VARIABLE_COUNT_MAX (10u)
#define FRAME_SCOPE_INVALID_ID (0xFFu)

#define FRAME_SCOPE_LIST_QUERY_SIZE (1u)
#define FRAME_SCOPE_LIST_ACK_FIXED_SIZE (4u)
#define FRAME_SCOPE_LIST_ACK_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_LIST_ACK_IS_LAST_OFFSET (1u)
#define FRAME_SCOPE_LIST_ACK_NAME_LENGTH_OFFSET (2u)
#define FRAME_SCOPE_LIST_ACK_NAME_OFFSET (4u)

#define FRAME_SCOPE_INFO_QUERY_SIZE (4u)
#define FRAME_SCOPE_INFO_QUERY_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_INFO_ACK_SIZE (36u)
#define FRAME_SCOPE_INFO_ACK_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_INFO_ACK_STATUS_OFFSET (1u)
#define FRAME_SCOPE_INFO_ACK_STATE_OFFSET (2u)
#define FRAME_SCOPE_INFO_ACK_DATA_READY_OFFSET (3u)
#define FRAME_SCOPE_INFO_ACK_VARIABLE_COUNT_OFFSET (4u)
#define FRAME_SCOPE_INFO_ACK_SAMPLE_COUNT_OFFSET (8u)
#define FRAME_SCOPE_INFO_ACK_WRITE_INDEX_OFFSET (12u)
#define FRAME_SCOPE_INFO_ACK_TRIGGER_INDEX_OFFSET (16u)
#define FRAME_SCOPE_INFO_ACK_TRIGGER_POST_COUNT_OFFSET (20u)
#define FRAME_SCOPE_INFO_ACK_TRIGGER_DISPLAY_INDEX_OFFSET (24u)
#define FRAME_SCOPE_INFO_ACK_SAMPLE_PERIOD_US_OFFSET (28u)
#define FRAME_SCOPE_INFO_ACK_CAPTURE_TAG_OFFSET (32u)

#define FRAME_SCOPE_VARIABLE_QUERY_SIZE (4u)
#define FRAME_SCOPE_VARIABLE_QUERY_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_VARIABLE_QUERY_INDEX_OFFSET (1u)
#define FRAME_SCOPE_VARIABLE_ACK_FIXED_SIZE (8u)
#define FRAME_SCOPE_VARIABLE_ACK_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_VARIABLE_ACK_STATUS_OFFSET (1u)
#define FRAME_SCOPE_VARIABLE_ACK_INDEX_OFFSET (2u)
#define FRAME_SCOPE_VARIABLE_ACK_IS_LAST_OFFSET (3u)
#define FRAME_SCOPE_VARIABLE_ACK_NAME_LENGTH_OFFSET (4u)
#define FRAME_SCOPE_VARIABLE_ACK_NAME_OFFSET (8u)

#define FRAME_SCOPE_CONTROL_QUERY_SIZE (4u)
#define FRAME_SCOPE_CONTROL_QUERY_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_CONTROL_ACK_SIZE (8u)
#define FRAME_SCOPE_CONTROL_ACK_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_CONTROL_ACK_STATUS_OFFSET (1u)
#define FRAME_SCOPE_CONTROL_ACK_STATE_OFFSET (2u)
#define FRAME_SCOPE_CONTROL_ACK_DATA_READY_OFFSET (3u)
#define FRAME_SCOPE_CONTROL_ACK_CAPTURE_TAG_OFFSET (4u)

#define FRAME_SCOPE_SAMPLE_QUERY_SIZE (12u)
#define FRAME_SCOPE_SAMPLE_QUERY_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_SAMPLE_QUERY_READ_MODE_OFFSET (1u)
#define FRAME_SCOPE_SAMPLE_QUERY_INDEX_OFFSET (4u)
#define FRAME_SCOPE_SAMPLE_QUERY_CAPTURE_TAG_OFFSET (8u)
#define FRAME_SCOPE_SAMPLE_ACK_FIXED_SIZE (16u)
#define FRAME_SCOPE_SAMPLE_ACK_SCOPE_ID_OFFSET (0u)
#define FRAME_SCOPE_SAMPLE_ACK_STATUS_OFFSET (1u)
#define FRAME_SCOPE_SAMPLE_ACK_READ_MODE_OFFSET (2u)
#define FRAME_SCOPE_SAMPLE_ACK_VARIABLE_COUNT_OFFSET (3u)
#define FRAME_SCOPE_SAMPLE_ACK_INDEX_OFFSET (4u)
#define FRAME_SCOPE_SAMPLE_ACK_CAPTURE_TAG_OFFSET (8u)
#define FRAME_SCOPE_SAMPLE_ACK_IS_LAST_OFFSET (12u)
#define FRAME_SCOPE_SAMPLE_ACK_VALUES_OFFSET (16u)
#define FRAME_SCOPE_FP32_WIRE_SIZE (4u)
#define FRAME_SCOPE_SAMPLE_ACK_MAX_SIZE \
    (FRAME_SCOPE_SAMPLE_ACK_FIXED_SIZE + (FRAME_SCOPE_VARIABLE_COUNT_MAX * FRAME_SCOPE_FP32_WIRE_SIZE))

typedef struct
{
    section_item_t *p_item;           /* Next registered Scope reported to the requester. */
    section_link_tx_func_t *p_output; /* Link retained for deferred list replies. */
    uint8_t active;                   /* One while a list response is in progress. */
    uint8_t source;                   /* Device source address for list replies. */
    uint8_t dynamic_source;           /* Device dynamic source address for list replies. */
    uint8_t destination;              /* FRAME host destination address for list replies. */
    uint8_t dynamic_destination;      /* FRAME host dynamic destination for list replies. */
} frame_scope_list_context_t;

static frame_scope_list_context_t frame_scope_list_context = {0}; /* Deferred Scope-list session. */

/**
 * @brief Validate one fixed-layout Scope request before accessing its payload.
 * @param[in] p_request Received FRAME metadata and logical-octet payload.
 * @param[in] expected_length Minimum payload length needed by the fixed offsets.
 * @return 1 when the request can be decoded, otherwise 0.
 */
static uint8_t frame_scope_request_is_valid(const section_packform_t *p_request,
                                            uint16_t expected_length)
{
    if ((p_request == NULL) ||       /* A request object is required. */
        (p_request->is_ack != 0u) || /* ACK frames must not recursively trigger handlers. */
        (p_request->p_data == NULL) || /* Every current Scope request has a payload. */
        (p_request->len < expected_length)) /* Ignore compatible fields appended by newer hosts. */
    {
        return 0u;
    }
    return 1u;
}

/**
 * @brief Copy one bounded C string into a logical-octet payload.
 * @param[out] p_destination First payload element available for the encoded name.
 * @param[in] p_name Scope or variable name stored by the registration layer.
 * @return Number of logical octets copied.
 */
static uint8_t frame_scope_name_copy(wire_octet_t *p_destination,
                                     const char *p_name)
{
    uint16_t index = 0u; /* Current character converted to one logical wire octet. */

    if ((p_destination == NULL) || /* The caller must provide bounded payload storage. */
        (p_name == NULL))          /* A missing name is encoded as an empty string. */
    {
        return 0u;
    }

    while ((index < FRAME_SCOPE_NAME_LENGTH_MAX) && /* Bound the response payload. */
           (p_name[index] != '\0'))                 /* Stop at the native string terminator. */
    {
        p_destination[index] = wire_octet_get((uint16_t)p_name[index]);
        index++;
    }
    return (uint8_t)index;
}

/**
 * @brief Resolve a protocol Scope id through the Section registration list.
 * @param[in] scope_id Identifier assigned by scope_init().
 * @return Matching registration, or NULL when the id is not available.
 */
static scope_registration_t *frame_scope_find_by_id(uint8_t scope_id)
{
    section_item_t *p_item = p_scope_first; /* Current Scope registration wrapper. */

    while (p_item != NULL)
    {
        scope_registration_t *p_registration =
            (scope_registration_t *)p_item->p_obj; /* Scope metadata owned by scope.c. */

        if ((p_registration != NULL) &&        /* Ignore an invalid Section object. */
            (p_registration->p_scope != NULL) && /* Only initialized Scope objects can serve data. */
            (p_registration->scope_id == scope_id)) /* Match the stable protocol identifier. */
        {
            return p_registration;
        }
        p_item = p_item->p_next;
    }
    return NULL;
}

/**
 * @brief Advance the nonzero capture generation counter.
 * @param[in,out] p_registration Scope metadata whose capture generation changed.
 */
static void frame_scope_capture_tag_increment(scope_registration_t *p_registration)
{
    if (p_registration == NULL)
    {
        return;
    }

    p_registration->capture_tag++;
    if (p_registration->capture_tag == 0u)
    {
        p_registration->capture_tag++;
    }
}

/**
 * @brief Calculate the trigger marker position shown in logical sample order.
 * @param[in] p_scope Scope capture object.
 * @return Trigger display index, or 0 when the capture geometry is invalid.
 */
static uint32_t frame_scope_trigger_display_index_get(const scope_t *p_scope)
{
    if ((p_scope == NULL) ||                          /* Scope metadata is required. */
        (p_scope->buffer_size == 0u) ||               /* Modulo operations need a nonzero size. */
        (p_scope->trigger_post_cnt >= p_scope->buffer_size)) /* Post samples must fit the capture. */
    {
        return 0u;
    }
    return p_scope->buffer_size - p_scope->trigger_post_cnt - 1u;
}

/**
 * @brief Find the first physical sample represented by logical index 0.
 * @param[in] p_scope Scope capture object.
 * @param[in] read_mode Normal completed-capture or force-live read mode.
 * @return Physical circular-buffer index.
 */
static uint32_t frame_scope_logical_start_index_get(const scope_t *p_scope,
                                                    uint8_t read_mode)
{
    if ((p_scope == NULL) ||            /* Scope metadata is required. */
        (p_scope->buffer_size == 0u))    /* Modulo operations need a nonzero size. */
    {
        return 0u;
    }

    if ((read_mode == (uint8_t)SCOPE_READ_MODE_FORCE) && /* Force mode may inspect a live ring. */
        (p_scope->state == SCOPE_STATE_RUNNING) &&       /* The ring is currently advancing. */
        (p_scope->in_trigger == 0u))                     /* Trigger ordering has not been fixed yet. */
    {
        return p_scope->write_index % p_scope->buffer_size;
    }

    return (p_scope->trigger_index + p_scope->trigger_post_cnt + 1u) %
           p_scope->buffer_size;
}

/**
 * @brief Convert one logical Scope sample index into the column-major ring index.
 * @param[in] p_scope Scope capture object.
 * @param[in] read_mode Normal completed-capture or force-live read mode.
 * @param[in] logical_index Sample index used by the FRAME client.
 * @return Physical circular-buffer index.
 */
static uint32_t frame_scope_physical_index_get(const scope_t *p_scope,
                                               uint8_t read_mode,
                                               uint32_t logical_index)
{
    uint32_t start_index = 0u; /* Physical index represented by logical sample 0. */

    if ((p_scope == NULL) ||         /* Scope metadata is required. */
        (p_scope->buffer_size == 0u)) /* Modulo operations need a nonzero size. */
    {
        return 0u;
    }

    start_index = frame_scope_logical_start_index_get(p_scope, read_mode);
    return (start_index + logical_index) % p_scope->buffer_size;
}

/**
 * @brief Send a direct ACK using the route of its request.
 * @param[in] p_request Request whose source and destination are reversed.
 * @param[in] p_output Link used for the response.
 * @param[in] command_word Scope command word being acknowledged.
 * @param[in,out] p_payload Explicitly serialized logical-octet payload.
 * @param[in] payload_length Number of logical octets in the response.
 */
static void frame_scope_reply(const section_packform_t *p_request,
                              section_link_tx_func_t *p_output,
                              uint8_t command_word,
                              wire_octet_t *p_payload,
                              uint16_t payload_length)
{
    section_packform_t reply = {0}; /* FRAME response metadata and logical payload. */

    if ((p_request == NULL) || /* A route cannot be derived without request metadata. */
        (p_output == NULL))    /* No transport is available for the response. */
    {
        return;
    }

    reply.src = p_request->dst;
    reply.d_src = p_request->d_dst;
    reply.dst = p_request->src;
    reply.d_dst = p_request->d_src;
    reply.cmd_set = CMD_SET_SCOPE;
    reply.cmd_word = command_word;
    reply.is_ack = 1u;
    reply.len = payload_length;
    reply.p_data = p_payload;
    comm_send_data(&reply, p_output);
}

/**
 * @brief Send one deferred list ACK through the retained request route.
 * @param[in,out] p_payload Explicitly serialized list item.
 * @param[in] payload_length Number of logical octets in the list item.
 */
static void frame_scope_list_reply(wire_octet_t *p_payload,
                                   uint16_t payload_length)
{
    section_packform_t reply = {0}; /* Deferred FRAME list response. */

    if (frame_scope_list_context.p_output == NULL)
    {
        return;
    }

    reply.src = frame_scope_list_context.source;
    reply.d_src = frame_scope_list_context.dynamic_source;
    reply.dst = frame_scope_list_context.destination;
    reply.d_dst = frame_scope_list_context.dynamic_destination;
    reply.cmd_set = CMD_SET_SCOPE;
    reply.cmd_word = CMD_WORD_SCOPE_LIST_QUERY;
    reply.is_ack = 1u;
    reply.len = payload_length;
    reply.p_data = p_payload;
    comm_send_data(&reply, frame_scope_list_context.p_output);
}

/**
 * @brief Detect completed triggered captures and advance their capture tags.
 */
static void frame_scope_state_poll(void)
{
    section_item_t *p_item = p_scope_first; /* Current registered Scope wrapper. */

    while (p_item != NULL)
    {
        scope_registration_t *p_registration =
            (scope_registration_t *)p_item->p_obj; /* Current Scope service metadata. */

        if ((p_registration != NULL) &&      /* Ignore an invalid Section object. */
            (p_registration->p_scope != NULL)) /* Only initialized Scope objects have state. */
        {
            scope_t *p_scope = p_registration->p_scope; /* Capture state observed this tick. */

            if ((p_registration->last_state == SCOPE_STATE_TRIGGERED) && /* Capture was finishing. */
                (p_scope->state == SCOPE_STATE_IDLE)) /* Core completed the post-trigger samples. */
            {
                p_registration->data_ready = 1u;
                frame_scope_capture_tag_increment(p_registration);
            }
            p_registration->last_state = p_scope->state;
        }
        p_item = p_item->p_next;
    }
}

/**
 * @brief Send at most one item from the active asynchronous Scope list.
 */
static void frame_scope_list_poll(void)
{
    section_item_t *p_item = NULL;                  /* Scope wrapper emitted this tick. */
    scope_registration_t *p_registration = NULL;   /* Metadata represented by the list item. */
    wire_octet_t payload[FRAME_SCOPE_LIST_ACK_FIXED_SIZE +
                         FRAME_SCOPE_NAME_LENGTH_MAX] = {0}; /* Serialized list ACK. */
    uint8_t name_length = 0u;                       /* Encoded name length in logical octets. */
    uint8_t is_last = 0u;                           /* One when this item closes the list. */

    if (frame_scope_list_context.active == 0u)
    {
        return;
    }

    p_item = frame_scope_list_context.p_item;
    if (p_item == NULL)
    {
        frame_scope_list_context.active = 0u;
        return;
    }

    p_registration = (scope_registration_t *)p_item->p_obj;
    frame_scope_list_context.p_item = p_item->p_next;
    if ((p_registration == NULL) ||       /* A corrupt registration cannot be described. */
        (p_registration->p_scope == NULL)) /* Scope state must remain owned by a valid object. */
    {
        frame_scope_list_context.active = 0u;
        return;
    }

    is_last = (frame_scope_list_context.p_item == NULL) ? 1u : 0u;
    name_length = frame_scope_name_copy(&payload[FRAME_SCOPE_LIST_ACK_NAME_OFFSET],
                                        p_registration->p_name);
    payload[FRAME_SCOPE_LIST_ACK_SCOPE_ID_OFFSET] = wire_octet_get(p_registration->scope_id);
    payload[FRAME_SCOPE_LIST_ACK_IS_LAST_OFFSET] = is_last;
    payload[FRAME_SCOPE_LIST_ACK_NAME_LENGTH_OFFSET] = name_length;
    frame_scope_list_reply(payload,
                           (uint16_t)(FRAME_SCOPE_LIST_ACK_FIXED_SIZE + name_length));

    if (is_last == 1u)
    {
        frame_scope_list_context.active = 0u;
    }
}

/**
 * @brief Poll capture completion and deferred Scope list reporting.
 */
static void frame_scope_service_task(void)
{
    frame_scope_state_poll();
    frame_scope_list_poll();
}

/**
 * @brief Handle command 0x18 and begin an asynchronous Scope list reply.
 * @param[in] p_pack Received list query.
 * @param[in] my_printf Link used for direct and deferred replies.
 */
static void frame_scope_list_query_act(section_packform_t *p_pack,
                                       DEC_MY_PRINTF)
{
    wire_octet_t empty_payload[FRAME_SCOPE_LIST_ACK_FIXED_SIZE] = {0}; /* Empty-list terminator. */

    if (frame_scope_request_is_valid(p_pack, FRAME_SCOPE_LIST_QUERY_SIZE) == 0u)
    {
        return;
    }

    if (p_scope_first == NULL)
    {
        empty_payload[FRAME_SCOPE_LIST_ACK_SCOPE_ID_OFFSET] = FRAME_SCOPE_INVALID_ID;
        empty_payload[FRAME_SCOPE_LIST_ACK_IS_LAST_OFFSET] = 1u;
        frame_scope_reply(p_pack,
                          my_printf,
                          CMD_WORD_SCOPE_LIST_QUERY,
                          empty_payload,
                          FRAME_SCOPE_LIST_ACK_FIXED_SIZE);
        return;
    }

    frame_scope_list_context.p_item = p_scope_first;
    frame_scope_list_context.p_output = my_printf;
    frame_scope_list_context.source = p_pack->dst;
    frame_scope_list_context.dynamic_source = p_pack->d_dst;
    frame_scope_list_context.destination = p_pack->src;
    frame_scope_list_context.dynamic_destination = p_pack->d_src;
    frame_scope_list_context.active = 1u;
    frame_scope_list_poll();
}

/**
 * @brief Handle command 0x19 and return one fixed-layout Scope information ACK.
 * @param[in] p_pack Received information query.
 * @param[in] my_printf Link used for the ACK.
 */
static void frame_scope_info_query_act(section_packform_t *p_pack,
                                       DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_SCOPE_INFO_ACK_SIZE] = {0}; /* Serialized information ACK. */
    scope_registration_t *p_registration = NULL;           /* Scope selected by the request. */
    uint8_t scope_id = FRAME_SCOPE_INVALID_ID;              /* Requested protocol identifier. */

    if (frame_scope_request_is_valid(p_pack, FRAME_SCOPE_INFO_QUERY_SIZE) == 0u)
    {
        return;
    }

    scope_id = wire_octet_get(p_pack->p_data[FRAME_SCOPE_INFO_QUERY_SCOPE_ID_OFFSET]);
    p_registration = frame_scope_find_by_id(scope_id);
    payload[FRAME_SCOPE_INFO_ACK_SCOPE_ID_OFFSET] = scope_id;
    if (p_registration == NULL)
    {
        payload[FRAME_SCOPE_INFO_ACK_STATUS_OFFSET] =
            wire_octet_get((uint16_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID);
    }
    else
    {
        const scope_t *p_scope = p_registration->p_scope; /* Snapshot source for this ACK. */

        payload[FRAME_SCOPE_INFO_ACK_STATUS_OFFSET] =
            wire_octet_get((uint16_t)SCOPE_TOOL_STATUS_OK);
        payload[FRAME_SCOPE_INFO_ACK_STATE_OFFSET] = wire_octet_get((uint16_t)p_scope->state);
        payload[FRAME_SCOPE_INFO_ACK_DATA_READY_OFFSET] =
            (p_registration->data_ready != 0u) ? 1u : 0u;
        payload[FRAME_SCOPE_INFO_ACK_VARIABLE_COUNT_OFFSET] = wire_octet_get(p_scope->var_count);
        wire_u32_le_write(&payload[FRAME_SCOPE_INFO_ACK_SAMPLE_COUNT_OFFSET], p_scope->buffer_size);
        wire_u32_le_write(&payload[FRAME_SCOPE_INFO_ACK_WRITE_INDEX_OFFSET], p_scope->write_index);
        wire_u32_le_write(&payload[FRAME_SCOPE_INFO_ACK_TRIGGER_INDEX_OFFSET], p_scope->trigger_index);
        wire_u32_le_write(&payload[FRAME_SCOPE_INFO_ACK_TRIGGER_POST_COUNT_OFFSET],
                          p_scope->trigger_post_cnt);
        wire_u32_le_write(&payload[FRAME_SCOPE_INFO_ACK_TRIGGER_DISPLAY_INDEX_OFFSET],
                          frame_scope_trigger_display_index_get(p_scope));
        wire_u32_le_write(&payload[FRAME_SCOPE_INFO_ACK_SAMPLE_PERIOD_US_OFFSET],
                          p_registration->sample_period_us);
        wire_u32_le_write(&payload[FRAME_SCOPE_INFO_ACK_CAPTURE_TAG_OFFSET],
                          p_registration->capture_tag);
    }

    frame_scope_reply(p_pack,
                      my_printf,
                      CMD_WORD_SCOPE_INFO_QUERY,
                      payload,
                      FRAME_SCOPE_INFO_ACK_SIZE);
}

/**
 * @brief Handle command 0x1A and return one variable-name ACK.
 * @param[in] p_pack Received variable query.
 * @param[in] my_printf Link used for the ACK.
 */
static void frame_scope_variable_query_act(section_packform_t *p_pack,
                                           DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_SCOPE_VARIABLE_ACK_FIXED_SIZE +
                         FRAME_SCOPE_NAME_LENGTH_MAX] = {0}; /* Serialized variable ACK. */
    scope_registration_t *p_registration = NULL;             /* Scope selected by the query. */
    const char *p_name = NULL;                                /* Variable name encoded on success. */
    uint8_t scope_id = FRAME_SCOPE_INVALID_ID;                /* Requested Scope identifier. */
    uint8_t variable_index = FRAME_SCOPE_INVALID_ID;          /* Requested variable index. */
    uint8_t name_length = 0u;                                 /* Encoded variable-name length. */

    if (frame_scope_request_is_valid(p_pack, FRAME_SCOPE_VARIABLE_QUERY_SIZE) == 0u)
    {
        return;
    }

    scope_id = wire_octet_get(p_pack->p_data[FRAME_SCOPE_VARIABLE_QUERY_SCOPE_ID_OFFSET]);
    variable_index = wire_octet_get(p_pack->p_data[FRAME_SCOPE_VARIABLE_QUERY_INDEX_OFFSET]);
    p_registration = frame_scope_find_by_id(scope_id);
    payload[FRAME_SCOPE_VARIABLE_ACK_SCOPE_ID_OFFSET] = scope_id;
    payload[FRAME_SCOPE_VARIABLE_ACK_INDEX_OFFSET] = variable_index;
    if (p_registration == NULL)
    {
        payload[FRAME_SCOPE_VARIABLE_ACK_STATUS_OFFSET] =
            wire_octet_get((uint16_t)SCOPE_TOOL_STATUS_SCOPE_ID_INVALID);
        payload[FRAME_SCOPE_VARIABLE_ACK_IS_LAST_OFFSET] = 1u;
    }
    else if (variable_index >= p_registration->p_scope->var_count)
    {
        payload[FRAME_SCOPE_VARIABLE_ACK_STATUS_OFFSET] =
            wire_octet_get((uint16_t)SCOPE_TOOL_STATUS_VAR_INDEX_INVALID);
        payload[FRAME_SCOPE_VARIABLE_ACK_IS_LAST_OFFSET] = 1u;
    }
    else
    {
        const scope_t *p_scope = p_registration->p_scope; /* Scope that owns the requested variable. */

        if (p_scope->var_names != NULL)
        {
            p_name = p_scope->var_names[variable_index];
        }
        name_length = frame_scope_name_copy(&payload[FRAME_SCOPE_VARIABLE_ACK_NAME_OFFSET],
                                            p_name);
        payload[FRAME_SCOPE_VARIABLE_ACK_STATUS_OFFSET] =
            wire_octet_get((uint16_t)SCOPE_TOOL_STATUS_OK);
        payload[FRAME_SCOPE_VARIABLE_ACK_IS_LAST_OFFSET] =
            (((uint16_t)variable_index + 1u) >= (uint16_t)p_scope->var_count) ? 1u : 0u;
        payload[FRAME_SCOPE_VARIABLE_ACK_NAME_LENGTH_OFFSET] = name_length;
    }

    frame_scope_reply(p_pack,
                      my_printf,
                      CMD_WORD_SCOPE_VAR_QUERY,
                      payload,
                      (uint16_t)(FRAME_SCOPE_VARIABLE_ACK_FIXED_SIZE + name_length));
}

/**
 * @brief Serialize and send one fixed-layout Scope control ACK.
 * @param[in] p_pack Control request whose route is acknowledged.
 * @param[in] my_printf Link used for the ACK.
 * @param[in] command_word Control command word from 0x1B through 0x1E.
 * @param[in] scope_id Scope identifier copied into the ACK.
 * @param[in] status Result defined by scope_tool_status_e.
 * @param[in] p_registration Matching Scope registration, or NULL for an invalid id.
 */
static void frame_scope_control_reply(section_packform_t *p_pack,
                                      section_link_tx_func_t *my_printf,
                                      uint8_t command_word,
                                      uint8_t scope_id,
                                      scope_tool_status_e status,
                                      const scope_registration_t *p_registration)
{
    wire_octet_t payload[FRAME_SCOPE_CONTROL_ACK_SIZE] = {0}; /* Serialized control ACK. */

    payload[FRAME_SCOPE_CONTROL_ACK_SCOPE_ID_OFFSET] = scope_id;
    payload[FRAME_SCOPE_CONTROL_ACK_STATUS_OFFSET] = wire_octet_get((uint16_t)status);
    if ((p_registration != NULL) &&       /* Invalid ids retain zero state fields. */
        (p_registration->p_scope != NULL)) /* Valid metadata must own a Scope object. */
    {
        payload[FRAME_SCOPE_CONTROL_ACK_STATE_OFFSET] =
            wire_octet_get((uint16_t)p_registration->p_scope->state);
        payload[FRAME_SCOPE_CONTROL_ACK_DATA_READY_OFFSET] =
            (p_registration->data_ready != 0u) ? 1u : 0u;
        wire_u32_le_write(&payload[FRAME_SCOPE_CONTROL_ACK_CAPTURE_TAG_OFFSET],
                          p_registration->capture_tag);
    }

    frame_scope_reply(p_pack,
                      my_printf,
                      command_word,
                      payload,
                      FRAME_SCOPE_CONTROL_ACK_SIZE);
}

/**
 * @brief Handle command 0x1B and start an idle Scope capture.
 * @param[in] p_pack Received start request.
 * @param[in] my_printf Link used for the ACK.
 */
static void frame_scope_start_act(section_packform_t *p_pack,
                                  DEC_MY_PRINTF)
{
    scope_registration_t *p_registration = NULL; /* Scope selected by the request. */
    uint8_t scope_id = FRAME_SCOPE_INVALID_ID;    /* Requested Scope identifier. */
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID; /* Start result. */

    if (frame_scope_request_is_valid(p_pack, FRAME_SCOPE_CONTROL_QUERY_SIZE) == 0u)
    {
        return;
    }

    scope_id = wire_octet_get(p_pack->p_data[FRAME_SCOPE_CONTROL_QUERY_SCOPE_ID_OFFSET]);
    p_registration = frame_scope_find_by_id(scope_id);
    if (p_registration == NULL)
    {
        status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
    }
    else if (p_registration->p_scope->state != SCOPE_STATE_IDLE)
    {
        status = SCOPE_TOOL_STATUS_RUNNING_DENIED;
    }
    else
    {
        p_registration->data_ready = 0u;
        frame_scope_capture_tag_increment(p_registration);
        scope_start(p_registration->p_scope);
        p_registration->last_state = p_registration->p_scope->state;
        status = SCOPE_TOOL_STATUS_OK;
    }

    frame_scope_control_reply(p_pack,
                              my_printf,
                              CMD_WORD_SCOPE_START,
                              scope_id,
                              status,
                              p_registration);
}

/**
 * @brief Handle command 0x1C and trigger a running Scope capture.
 * @param[in] p_pack Received trigger request.
 * @param[in] my_printf Link used for the ACK.
 */
static void frame_scope_trigger_act(section_packform_t *p_pack,
                                    DEC_MY_PRINTF)
{
    scope_registration_t *p_registration = NULL; /* Scope selected by the request. */
    uint8_t scope_id = FRAME_SCOPE_INVALID_ID;    /* Requested Scope identifier. */
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID; /* Trigger result. */

    if (frame_scope_request_is_valid(p_pack, FRAME_SCOPE_CONTROL_QUERY_SIZE) == 0u)
    {
        return;
    }

    scope_id = wire_octet_get(p_pack->p_data[FRAME_SCOPE_CONTROL_QUERY_SCOPE_ID_OFFSET]);
    p_registration = frame_scope_find_by_id(scope_id);
    if (p_registration == NULL)
    {
        status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
    }
    else if (p_registration->p_scope->state != SCOPE_STATE_RUNNING)
    {
        status = SCOPE_TOOL_STATUS_RUNNING_DENIED;
    }
    else
    {
        scope_trigger(p_registration->p_scope);
        status = SCOPE_TOOL_STATUS_OK;
    }

    frame_scope_control_reply(p_pack,
                              my_printf,
                              CMD_WORD_SCOPE_TRIGGER,
                              scope_id,
                              status,
                              p_registration);
}

/**
 * @brief Handle command 0x1D and stop a Scope while retaining its capture data.
 * @param[in] p_pack Received stop request.
 * @param[in] my_printf Link used for the ACK.
 */
static void frame_scope_stop_act(section_packform_t *p_pack,
                                 DEC_MY_PRINTF)
{
    scope_registration_t *p_registration = NULL; /* Scope selected by the request. */
    uint8_t scope_id = FRAME_SCOPE_INVALID_ID;    /* Requested Scope identifier. */
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID; /* Stop result. */

    if (frame_scope_request_is_valid(p_pack, FRAME_SCOPE_CONTROL_QUERY_SIZE) == 0u)
    {
        return;
    }

    scope_id = wire_octet_get(p_pack->p_data[FRAME_SCOPE_CONTROL_QUERY_SCOPE_ID_OFFSET]);
    p_registration = frame_scope_find_by_id(scope_id);
    if (p_registration != NULL)
    {
        scope_stop(p_registration->p_scope);
        p_registration->data_ready = 1u;
        p_registration->last_state = p_registration->p_scope->state;
        status = SCOPE_TOOL_STATUS_OK;
    }

    frame_scope_control_reply(p_pack,
                              my_printf,
                              CMD_WORD_SCOPE_STOP,
                              scope_id,
                              status,
                              p_registration);
}

/**
 * @brief Handle command 0x1E and reset Scope state and capture readiness.
 * @param[in] p_pack Received reset request.
 * @param[in] my_printf Link used for the ACK.
 */
static void frame_scope_reset_act(section_packform_t *p_pack,
                                  DEC_MY_PRINTF)
{
    scope_registration_t *p_registration = NULL; /* Scope selected by the request. */
    uint8_t scope_id = FRAME_SCOPE_INVALID_ID;    /* Requested Scope identifier. */
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID; /* Reset result. */

    if (frame_scope_request_is_valid(p_pack, FRAME_SCOPE_CONTROL_QUERY_SIZE) == 0u)
    {
        return;
    }

    scope_id = wire_octet_get(p_pack->p_data[FRAME_SCOPE_CONTROL_QUERY_SCOPE_ID_OFFSET]);
    p_registration = frame_scope_find_by_id(scope_id);
    if (p_registration != NULL)
    {
        scope_reset(p_registration->p_scope);
        p_registration->data_ready = 0u;
        frame_scope_capture_tag_increment(p_registration);
        p_registration->last_state = p_registration->p_scope->state;
        status = SCOPE_TOOL_STATUS_OK;
    }

    frame_scope_control_reply(p_pack,
                              my_printf,
                              CMD_WORD_SCOPE_RESET,
                              scope_id,
                              status,
                              p_registration);
}

/**
 * @brief Handle command 0x1F and return one logical sample across all Scope variables.
 * @param[in] p_pack Received sample query.
 * @param[in] my_printf Link used for the ACK.
 */
static void frame_scope_sample_query_act(section_packform_t *p_pack,
                                         DEC_MY_PRINTF)
{
    wire_octet_t payload[FRAME_SCOPE_SAMPLE_ACK_MAX_SIZE] = {0}; /* Serialized sample ACK. */
    scope_registration_t *p_registration = NULL;                 /* Scope selected by the request. */
    const scope_t *p_scope = NULL;                                /* Capture data source on a valid id. */
    uint32_t sample_index = 0u;                                   /* Requested logical sample index. */
    uint32_t expected_capture_tag = 0u;                            /* Optional generation guard. */
    uint32_t physical_index = 0u;                                 /* Resolved ring-buffer index. */
    uint16_t payload_length = FRAME_SCOPE_SAMPLE_ACK_FIXED_SIZE;  /* ACK length in logical octets. */
    uint8_t scope_id = FRAME_SCOPE_INVALID_ID;                    /* Requested Scope identifier. */
    uint8_t read_mode = (uint8_t)SCOPE_READ_MODE_NORMAL;          /* Requested read consistency mode. */
    uint8_t variable_count = 0u;                                  /* Values appended on success. */
    uint8_t variable_index = 0u;                                  /* Current captured variable. */
    scope_tool_status_e status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID; /* Sample read result. */

    if (frame_scope_request_is_valid(p_pack, FRAME_SCOPE_SAMPLE_QUERY_SIZE) == 0u)
    {
        return;
    }

    scope_id = wire_octet_get(p_pack->p_data[FRAME_SCOPE_SAMPLE_QUERY_SCOPE_ID_OFFSET]);
    read_mode = wire_octet_get(p_pack->p_data[FRAME_SCOPE_SAMPLE_QUERY_READ_MODE_OFFSET]);
    sample_index = wire_u32_le_read(&p_pack->p_data[FRAME_SCOPE_SAMPLE_QUERY_INDEX_OFFSET]);
    expected_capture_tag =
        wire_u32_le_read(&p_pack->p_data[FRAME_SCOPE_SAMPLE_QUERY_CAPTURE_TAG_OFFSET]);
    p_registration = frame_scope_find_by_id(scope_id);

    payload[FRAME_SCOPE_SAMPLE_ACK_SCOPE_ID_OFFSET] = scope_id;
    payload[FRAME_SCOPE_SAMPLE_ACK_READ_MODE_OFFSET] = read_mode;
    wire_u32_le_write(&payload[FRAME_SCOPE_SAMPLE_ACK_INDEX_OFFSET], sample_index);
    if (p_registration == NULL)
    {
        status = SCOPE_TOOL_STATUS_SCOPE_ID_INVALID;
    }
    else
    {
        p_scope = p_registration->p_scope;
        wire_u32_le_write(&payload[FRAME_SCOPE_SAMPLE_ACK_CAPTURE_TAG_OFFSET],
                          p_registration->capture_tag);
        if ((read_mode != (uint8_t)SCOPE_READ_MODE_NORMAL) && /* Only two parser modes are defined. */
            (read_mode != (uint8_t)SCOPE_READ_MODE_FORCE))   /* Unknown modes cannot define ordering. */
        {
            status = SCOPE_TOOL_STATUS_SAMPLE_INDEX_INVALID;
        }
        else if ((read_mode == (uint8_t)SCOPE_READ_MODE_NORMAL) && /* Normal reads require a frozen capture. */
                 (p_scope->state != SCOPE_STATE_IDLE))             /* A running ring is not stable. */
        {
            status = SCOPE_TOOL_STATUS_RUNNING_DENIED;
        }
        else if ((expected_capture_tag != 0u) && /* Zero explicitly disables generation matching. */
                 (expected_capture_tag != p_registration->capture_tag)) /* The capture changed since discovery. */
        {
            status = SCOPE_TOOL_STATUS_CAPTURE_CHANGED;
        }
        else if ((p_registration->data_ready == 0u) && /* No completed capture has been published. */
                 (read_mode != (uint8_t)SCOPE_READ_MODE_FORCE)) /* Force mode intentionally bypasses readiness. */
        {
            status = SCOPE_TOOL_STATUS_DATA_NOT_READY;
        }
        else if ((p_scope->buffer == NULL) ||       /* No sample storage is available. */
                 (p_scope->buffer_size == 0u) ||    /* A zero-length ring cannot be indexed. */
                 (sample_index >= p_scope->buffer_size)) /* Requested sample lies outside the capture. */
        {
            status = SCOPE_TOOL_STATUS_SAMPLE_INDEX_INVALID;
        }
        else
        {
            variable_count = p_scope->var_count;
            if (variable_count > FRAME_SCOPE_VARIABLE_COUNT_MAX)
            {
                variable_count = FRAME_SCOPE_VARIABLE_COUNT_MAX;
            }

            physical_index = frame_scope_physical_index_get(p_scope,
                                                            read_mode,
                                                            sample_index);
            for (variable_index = 0u;
                 variable_index < variable_count;
                 variable_index++)
            {
                uint16_t value_offset =
                    (uint16_t)(FRAME_SCOPE_SAMPLE_ACK_VALUES_OFFSET +
                               ((uint16_t)variable_index * FRAME_SCOPE_FP32_WIRE_SIZE));
                float value =
                    p_scope->buffer[physical_index +
                                    ((uint32_t)variable_index * p_scope->buffer_size)];

                wire_f32_le_write(&payload[value_offset], value);
            }

            payload[FRAME_SCOPE_SAMPLE_ACK_VARIABLE_COUNT_OFFSET] = variable_count;
            payload[FRAME_SCOPE_SAMPLE_ACK_IS_LAST_OFFSET] =
                ((sample_index + 1u) >= p_scope->buffer_size) ? 1u : 0u;
            payload_length =
                (uint16_t)(FRAME_SCOPE_SAMPLE_ACK_FIXED_SIZE +
                           ((uint16_t)variable_count * FRAME_SCOPE_FP32_WIRE_SIZE));
            status = SCOPE_TOOL_STATUS_OK;
        }
    }

    payload[FRAME_SCOPE_SAMPLE_ACK_STATUS_OFFSET] = wire_octet_get((uint16_t)status);
    frame_scope_reply(p_pack,
                      my_printf,
                      CMD_WORD_SCOPE_SAMPLE_QUERY,
                      payload,
                      payload_length);
}

REG_TASK_MS(1, frame_scope_service_task)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_LIST_QUERY, frame_scope_list_query_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_INFO_QUERY, frame_scope_info_query_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_VAR_QUERY, frame_scope_variable_query_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_START, frame_scope_start_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_TRIGGER, frame_scope_trigger_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_STOP, frame_scope_stop_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_RESET, frame_scope_reset_act)
REG_COMM(CMD_SET_SCOPE, CMD_WORD_SCOPE_SAMPLE_QUERY, frame_scope_sample_query_act)

#endif /* SCOPE_ENABLE == 1u */
