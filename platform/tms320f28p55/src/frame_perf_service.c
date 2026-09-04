// SPDX-License-Identifier: MIT
/**
 * @file    frame_perf_service.c
 * @brief   C28x-safe FRAME Perf protocol service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Serve every binary Perf command from 0x20 through 0x2E
 *          - Serialize protocol fields at explicit logical-octet offsets
 *          - Stream dictionary and sample records with bounded task work
 *
 *          Design beliefs:
 *          - Entity: one transfer context owns one dictionary or sample stream
 *          - Prior: request lengths, filters, and dictionary versions are untrusted until validated
 *          - Time: each service task emits at most one dictionary or sample item and every stream ends explicitly
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Native C28x structures are never used as physical wire layouts
 *          - Perf measurements and record ownership remain in perf.c and perf_core.c
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
#include "perf_service.h"

#include <stddef.h>

#if (PERF_ENABLE == 1u)

#define FRAME_PERF_PROTOCOL_VERSION (0x0001u)
#define FRAME_PERF_INFO_FLAGS (0x03u)
#define FRAME_PERF_TASK_PERIOD_MS (10u)

#define FRAME_PERF_INFO_ACK_SIZE (20u)
#define FRAME_PERF_INFO_PROTOCOL_VERSION_OFFSET (0u)
#define FRAME_PERF_INFO_RECORD_COUNT_OFFSET (2u)
#define FRAME_PERF_INFO_UNIT_US_OFFSET (4u)
#define FRAME_PERF_INFO_COUNT_PER_TICK_OFFSET (8u)
#define FRAME_PERF_INFO_CPU_WINDOW_MS_OFFSET (12u)
#define FRAME_PERF_INFO_FLAGS_OFFSET (16u)

#define FRAME_PERF_SUMMARY_ACK_SIZE (16u)
#define FRAME_PERF_SUMMARY_TASK_LOAD_OFFSET (0u)
#define FRAME_PERF_SUMMARY_TASK_PEAK_OFFSET (4u)
#define FRAME_PERF_SUMMARY_INTERRUPT_LOAD_OFFSET (8u)
#define FRAME_PERF_SUMMARY_INTERRUPT_PEAK_OFFSET (12u)

#define FRAME_PERF_RESET_ACK_SIZE (4u)
#define FRAME_PERF_RESET_ACK_SUCCESS_OFFSET (0u)

#define FRAME_PERF_DICT_QUERY_SIZE (8u)
#define FRAME_PERF_DICT_QUERY_FILTER_OFFSET (0u)
#define FRAME_PERF_DICT_QUERY_VERSION_OFFSET (4u)
#define FRAME_PERF_DICT_ACK_SIZE (16u)
#define FRAME_PERF_DICT_ACK_ACCEPTED_OFFSET (0u)
#define FRAME_PERF_DICT_ACK_FILTER_OFFSET (1u)
#define FRAME_PERF_DICT_ACK_RECORD_COUNT_OFFSET (2u)
#define FRAME_PERF_DICT_ACK_SEQUENCE_OFFSET (4u)
#define FRAME_PERF_DICT_ACK_VERSION_OFFSET (8u)
#define FRAME_PERF_DICT_ACK_REJECT_REASON_OFFSET (12u)
#define FRAME_PERF_DICT_ITEM_FIXED_SIZE (12u)
#define FRAME_PERF_DICT_ITEM_SEQUENCE_OFFSET (0u)
#define FRAME_PERF_DICT_ITEM_INDEX_OFFSET (4u)
#define FRAME_PERF_DICT_ITEM_RECORD_COUNT_OFFSET (6u)
#define FRAME_PERF_DICT_ITEM_RECORD_ID_OFFSET (8u)
#define FRAME_PERF_DICT_ITEM_RECORD_TYPE_OFFSET (10u)
#define FRAME_PERF_DICT_ITEM_NAME_LENGTH_OFFSET (11u)
#define FRAME_PERF_DICT_ITEM_NAME_OFFSET (12u)
#define FRAME_PERF_DICT_NAME_LENGTH_MAX \
    (PERF_OPT_MAX_PAYLOAD_SIZE - FRAME_PERF_DICT_ITEM_FIXED_SIZE)
#define FRAME_PERF_DICT_END_SIZE (12u)
#define FRAME_PERF_DICT_END_SEQUENCE_OFFSET (0u)
#define FRAME_PERF_DICT_END_RECORD_COUNT_OFFSET (4u)
#define FRAME_PERF_DICT_END_STATUS_OFFSET (6u)
#define FRAME_PERF_DICT_END_RESERVED_OFFSET (7u)
#define FRAME_PERF_DICT_END_VERSION_OFFSET (8u)

#define FRAME_PERF_SAMPLE_QUERY_SIZE (8u)
#define FRAME_PERF_SAMPLE_QUERY_FILTER_OFFSET (0u)
#define FRAME_PERF_SAMPLE_QUERY_FLAGS_OFFSET (1u)
#define FRAME_PERF_SAMPLE_QUERY_VERSION_OFFSET (4u)
#define FRAME_PERF_SAMPLE_ACK_SIZE (16u)
#define FRAME_PERF_SAMPLE_ACK_ACCEPTED_OFFSET (0u)
#define FRAME_PERF_SAMPLE_ACK_FILTER_OFFSET (1u)
#define FRAME_PERF_SAMPLE_ACK_RECORD_COUNT_OFFSET (2u)
#define FRAME_PERF_SAMPLE_ACK_SEQUENCE_OFFSET (4u)
#define FRAME_PERF_SAMPLE_ACK_VERSION_OFFSET (8u)
#define FRAME_PERF_SAMPLE_ACK_REJECT_REASON_OFFSET (12u)
#define FRAME_PERF_SAMPLE_BATCH_HEADER_SIZE (8u)
#define FRAME_PERF_SAMPLE_BATCH_SEQUENCE_OFFSET (0u)
#define FRAME_PERF_SAMPLE_BATCH_RECORD_COUNT_OFFSET (4u)
#define FRAME_PERF_SAMPLE_BATCH_ITEM_COUNT_OFFSET (6u)
#define FRAME_PERF_SAMPLE_ITEM_OFFSET (8u)
#define FRAME_PERF_SAMPLE_ITEM_RECORD_ID_OFFSET (0u)
#define FRAME_PERF_SAMPLE_ITEM_TIME_US_OFFSET (2u)
#define FRAME_PERF_SAMPLE_ITEM_MAX_TIME_US_OFFSET (6u)
#define FRAME_PERF_SAMPLE_TASK_PERIOD_US_OFFSET (10u)
#define FRAME_PERF_SAMPLE_TASK_LOAD_OFFSET (14u)
#define FRAME_PERF_SAMPLE_TASK_PEAK_OFFSET (18u)
#define FRAME_PERF_SAMPLE_TASK_ITEM_SIZE (22u)
#define FRAME_PERF_SAMPLE_INTERRUPT_LOAD_OFFSET (10u)
#define FRAME_PERF_SAMPLE_INTERRUPT_PEAK_OFFSET (14u)
#define FRAME_PERF_SAMPLE_INTERRUPT_ITEM_SIZE (18u)
#define FRAME_PERF_SAMPLE_CODE_ITEM_SIZE (10u)
#define FRAME_PERF_SAMPLE_END_SIZE (8u)
#define FRAME_PERF_SAMPLE_END_SEQUENCE_OFFSET (0u)
#define FRAME_PERF_SAMPLE_END_RECORD_COUNT_OFFSET (4u)
#define FRAME_PERF_SAMPLE_END_STATUS_OFFSET (6u)
#define FRAME_PERF_SAMPLE_END_RESERVED_OFFSET (7u)

#define FRAME_PERF_CONTROL_QUERY_SIZE (1u)
#define FRAME_PERF_CONTROL_ENABLE_OFFSET (0u)
#define FRAME_PERF_CONTROL_ACK_SIZE (1u)
#define FRAME_PERF_CONTROL_ACK_SUCCESS_OFFSET (0u)

typedef struct
{
    section_item_t *p_item;           /* Next Section Perf record considered for the active stream. */
    section_link_tx_func_t *p_output; /* Stable link callback retained for asynchronous reports. */
    perf_opt_pull_type_t pull_type;   /* Dictionary or sample stream currently in progress. */
    uint32_t sequence;                /* Nonzero generation identifier for the active stream. */
    uint32_t dict_version;            /* Dictionary version captured when the stream starts. */
    uint16_t record_count;            /* Number of valid records matching the selected filter. */
    uint16_t record_index;            /* Number of dictionary or sample items already emitted. */
    uint8_t active;                   /* One while an asynchronous stream owns this context. */
    uint8_t pending_end;              /* One when the next task must emit the terminal report. */
    uint8_t end_status;               /* PERF_OPT_END_* status emitted by the terminal report. */
    uint8_t type_filter;              /* PERF_OPT_TYPE_* filter captured from the request. */
    uint8_t source;                   /* Device source address for asynchronous reports. */
    uint8_t dynamic_source;           /* Device dynamic source address for asynchronous reports. */
    uint8_t destination;              /* FRAME host destination address for asynchronous reports. */
    uint8_t dynamic_destination;      /* FRAME host dynamic destination for asynchronous reports. */
    wire_octet_t payload[PERF_OPT_MAX_PAYLOAD_SIZE]; /* Reused bounded wire payload storage. */
} frame_perf_context_t;

static frame_perf_context_t frame_perf_context = {0}; /* Single bounded Perf transfer context. */
static uint32_t frame_perf_sequence = 0u; /* Last sequence allocated to an accepted transfer. */

/**
 * @brief Check that a frame is a new request rather than an acknowledgement.
 * @param[in] p_request Received FRAME metadata.
 * @return 1 for a command request, otherwise 0.
 */
static uint8_t frame_perf_request_is_command(const section_packform_t *p_request)
{
    if ((p_request == NULL) ||       /* A request object is required. */
        (p_request->is_ack != 0u))   /* ACK frames must not recursively trigger handlers. */
    {
        return 0u;
    }
    return 1u;
}

/**
 * @brief Validate the minimum payload length required by a fixed-offset request.
 * @param[in] p_request Received FRAME metadata and logical-octet payload.
 * @param[in] minimum_length Published prefix length required by the decoder.
 * @return 1 when every required offset can be read, otherwise 0.
 */
static uint8_t frame_perf_request_payload_is_valid(const section_packform_t *p_request,
                                                   uint16_t minimum_length)
{
    if ((frame_perf_request_is_command(p_request) == 0u) || /* Reject missing and ACK frames. */
        (p_request->p_data == NULL) ||                       /* Required bytes need readable storage. */
        (p_request->len < minimum_length))                   /* Tail extensions are accepted safely. */
    {
        return 0u;
    }
    return 1u;
}

/**
 * @brief Send one direct response using the route carried by its request.
 * @param[in] p_request Request whose source and destination are reversed for the response.
 * @param[in] p_output Link callback supplied by the Comm dispatcher.
 * @param[in] command_word Perf command word echoed by the direct acknowledgement.
 * @param[in] p_payload Serialized logical-octet payload, or NULL for an empty response.
 * @param[in] payload_length Number of logical wire octets in the response.
 */
static void frame_perf_reply(const section_packform_t *p_request,
                             section_link_tx_func_t *p_output,
                             uint8_t command_word,
                             wire_octet_t *p_payload,
                             uint16_t payload_length)
{
    section_packform_t response = {0}; /* Direct FRAME acknowledgement metadata. */

    if (p_request == NULL)
    {
        return;
    }

    response.src = p_request->dst;
    response.d_src = p_request->d_dst;
    response.dst = p_request->src;
    response.d_dst = p_request->d_src;
    response.cmd_set = PERF_OPT_CMD_SET;
    response.cmd_word = command_word;
    response.is_ack = 1u;
    response.len = payload_length;
    response.p_data = p_payload;
    comm_send_data(&response, p_output);
}

/**
 * @brief Send one asynchronous report on the route captured at transfer start.
 * @param[in] command_word Perf dictionary, sample, or terminal report command.
 * @param[in] p_payload Serialized logical-octet payload.
 * @param[in] payload_length Number of logical wire octets in the report.
 */
static void frame_perf_report(uint8_t command_word,
                              wire_octet_t *p_payload,
                              uint16_t payload_length)
{
    section_packform_t report = {0}; /* Asynchronous FRAME report metadata. */

    if (frame_perf_context.p_output == NULL)
    {
        return;
    }

    report.src = frame_perf_context.source;
    report.d_src = frame_perf_context.dynamic_source;
    report.dst = frame_perf_context.destination;
    report.d_dst = frame_perf_context.dynamic_destination;
    report.cmd_set = PERF_OPT_CMD_SET;
    report.cmd_word = command_word;
    report.is_ack = 0u;
    report.len = payload_length;
    report.p_data = p_payload;
    comm_send_data(&report, frame_perf_context.p_output);
}

/**
 * @brief Convert one internal Perf record type into its FRAME protocol value.
 * @param[in] record_type SECTION_PERF_RECORD_* value owned by Perf core.
 * @return PERF_OPT_TYPE_* value, or 0 when the record type is unsupported.
 */
static uint8_t frame_perf_record_type_get(uint8_t record_type)
{
    uint8_t protocol_type = 0u; /* FRAME record type selected from the internal type. */

    switch (record_type)
    {
    case SECTION_PERF_RECORD_TASK:
        protocol_type = PERF_OPT_TYPE_TASK;
        break;

    case SECTION_PERF_RECORD_INTERRUPT:
        protocol_type = PERF_OPT_TYPE_INTERRUPT;
        break;

    case SECTION_PERF_RECORD_CODE:
        protocol_type = PERF_OPT_TYPE_CODE;
        break;

    default:
        break;
    }
    return protocol_type;
}

/**
 * @brief Check whether a protocol record type belongs to one requested filter.
 * @param[in] protocol_type PERF_OPT_TYPE_TASK, INTERRUPT, or CODE.
 * @param[in] type_filter PERF_OPT_TYPE_ALL or one concrete record type.
 * @return 1 when the record is valid and selected, otherwise 0.
 */
static uint8_t frame_perf_record_matches(uint8_t protocol_type,
                                         uint8_t type_filter)
{
    if ((protocol_type < PERF_OPT_TYPE_TASK) || /* Type 0 is the wildcard only, never a record. */
        (protocol_type > PERF_OPT_TYPE_CODE))   /* Values above CODE are not published record types. */
    {
        return 0u;
    }

    if (type_filter == PERF_OPT_TYPE_ALL)
    {
        return 1u;
    }
    return (protocol_type == type_filter) ? 1u : 0u;
}

/**
 * @brief Count valid Perf records selected by a protocol filter.
 * @param[in] type_filter PERF_OPT_TYPE_ALL, TASK, INTERRUPT, or CODE.
 * @return Saturated number of matching records.
 */
static uint16_t frame_perf_record_count(uint8_t type_filter)
{
    section_item_t *p_item = p_perf_first; /* Current Section wrapper in the Perf record list. */
    uint16_t count = 0u; /* Number of valid matching records found so far. */

    while (p_item != NULL)
    {
        section_perf_record_t *p_record =
            perf_record_from_item(p_item); /* Perf record recovered through the shared adapter. */

        if (p_record != NULL)
        {
            uint8_t protocol_type =
                frame_perf_record_type_get(p_record->record_type); /* FRAME-facing record type. */

            if ((frame_perf_record_matches(protocol_type, type_filter) == 1u) && /* Filter accepts it. */
                (count != UINT16_MAX))                                            /* Count cannot wrap. */
            {
                count++;
            }
        }
        p_item = p_item->p_next;
    }
    return count;
}

/**
 * @brief Find the next valid Perf record selected by a protocol filter.
 * @param[in] p_start First Section wrapper to inspect.
 * @param[in] type_filter PERF_OPT_TYPE_ALL, TASK, INTERRUPT, or CODE.
 * @return Matching Section wrapper, or NULL at the end of the list.
 */
static section_item_t *frame_perf_record_find_next(section_item_t *p_start,
                                                   uint8_t type_filter)
{
    section_item_t *p_item = p_start; /* Current Section wrapper inspected for a match. */

    while (p_item != NULL)
    {
        section_perf_record_t *p_record =
            perf_record_from_item(p_item); /* Perf record recovered through the shared adapter. */

        if (p_record != NULL)
        {
            uint8_t protocol_type =
                frame_perf_record_type_get(p_record->record_type); /* FRAME-facing record type. */

            if (frame_perf_record_matches(protocol_type, type_filter) == 1u)
            {
                return p_item;
            }
        }
        p_item = p_item->p_next;
    }
    return NULL;
}

/**
 * @brief Copy one bounded record name into a logical-octet payload.
 * @param[out] p_destination First payload element available for the name.
 * @param[in] p_name Native record name owned by the Perf registration.
 * @return Number of logical octets copied.
 */
static uint8_t frame_perf_name_copy(wire_octet_t *p_destination,
                                    const char *p_name)
{
    uint16_t index = 0u; /* Native character currently converted to one wire octet. */

    if ((p_destination == NULL) || /* Bounded output storage is required. */
        (p_name == NULL))          /* A missing name is represented by zero length. */
    {
        return 0u;
    }

    while ((index < FRAME_PERF_DICT_NAME_LENGTH_MAX) && /* Keep the report inside its fixed buffer. */
           (p_name[index] != '\0'))                     /* Stop at the native string terminator. */
    {
        p_destination[index] = wire_octet_get((uint16_t)p_name[index]);
        index++;
    }
    return (uint8_t)index;
}

/**
 * @brief Allocate the next nonzero Perf stream sequence.
 * @return Nonzero sequence number for a newly accepted transfer.
 */
static uint32_t frame_perf_sequence_next(void)
{
    frame_perf_sequence++;
    if (frame_perf_sequence == 0u)
    {
        frame_perf_sequence++;
    }
    return frame_perf_sequence;
}

/**
 * @brief Return the transfer context to its idle state.
 */
static void frame_perf_transfer_clear(void)
{
    frame_perf_context = (frame_perf_context_t){0};
    frame_perf_context.pull_type = PERF_OPT_PULL_IDLE;
}

/**
 * @brief Start one bounded dictionary or sample stream.
 * @param[in] p_request Request that supplies the reply route.
 * @param[in] p_output Stable Comm link callback for deferred reports.
 * @param[in] type_filter PERF_OPT_TYPE_ALL, TASK, INTERRUPT, or CODE.
 * @param[in] pull_type Dictionary or sample transfer kind.
 * @param[out] p_reject_reason PERF_OPT_REJECT_* result when the stream is refused.
 * @return 1 when ownership of the transfer context was acquired, otherwise 0.
 */
static uint8_t frame_perf_transfer_start(const section_packform_t *p_request,
                                         section_link_tx_func_t *p_output,
                                         uint8_t type_filter,
                                         perf_opt_pull_type_t pull_type,
                                         uint8_t *p_reject_reason)
{
    if ((p_request == NULL) ||      /* Route metadata is required. */
        (p_reject_reason == NULL))  /* The caller must receive a deterministic rejection reason. */
    {
        return 0u;
    }

    if (frame_perf_context.active == 1u)
    {
        *p_reject_reason = PERF_OPT_REJECT_BUSY;
        return 0u;
    }

    if (type_filter > PERF_OPT_TYPE_CODE)
    {
        *p_reject_reason = PERF_OPT_REJECT_INVALID_FILTER;
        return 0u;
    }

    if ((p_output == NULL) ||            /* Deferred reports need a persistent link object. */
        (p_output->tx_by_dma == NULL))   /* The link must own or synchronously consume each frame. */
    {
        *p_reject_reason = PERF_OPT_REJECT_NO_BUFFER;
        return 0u;
    }

    if ((pull_type != PERF_OPT_PULL_DICT) &&   /* Only dictionary transfers are supported here. */
        (pull_type != PERF_OPT_PULL_SAMPLE))   /* Only sample transfers are supported here. */
    {
        *p_reject_reason = PERF_OPT_REJECT_UNSUPPORTED;
        return 0u;
    }

    frame_perf_context.p_item = p_perf_first;
    frame_perf_context.p_output = p_output;
    frame_perf_context.pull_type = pull_type;
    frame_perf_context.sequence = frame_perf_sequence_next();
    frame_perf_context.dict_version = perf_dict_version_get();
    frame_perf_context.record_count = frame_perf_record_count(type_filter);
    frame_perf_context.record_index = 0u;
    frame_perf_context.active = 1u;
    frame_perf_context.pending_end = 0u;
    frame_perf_context.end_status = PERF_OPT_END_OK;
    frame_perf_context.type_filter = type_filter;
    frame_perf_context.source = p_request->dst;
    frame_perf_context.dynamic_source = p_request->d_dst;
    frame_perf_context.destination = p_request->src;
    frame_perf_context.dynamic_destination = p_request->d_src;
    *p_reject_reason = PERF_OPT_REJECT_OK;
    return 1u;
}

/**
 * @brief Emit the terminal report for the active stream and release its context.
 * @param[in] status PERF_OPT_END_* completion status.
 */
static void frame_perf_transfer_end(uint8_t status)
{
    if (frame_perf_context.active == 0u)
    {
        return;
    }

    if (frame_perf_context.pull_type == PERF_OPT_PULL_DICT)
    {
        wire_u32_le_write(
            &frame_perf_context.payload[FRAME_PERF_DICT_END_SEQUENCE_OFFSET],
            frame_perf_context.sequence);
        wire_u16_le_write(
            &frame_perf_context.payload[FRAME_PERF_DICT_END_RECORD_COUNT_OFFSET],
            frame_perf_context.record_index);
        frame_perf_context.payload[FRAME_PERF_DICT_END_STATUS_OFFSET] =
            wire_octet_get(status);
        frame_perf_context.payload[FRAME_PERF_DICT_END_RESERVED_OFFSET] = 0u;
        wire_u32_le_write(
            &frame_perf_context.payload[FRAME_PERF_DICT_END_VERSION_OFFSET],
            frame_perf_context.dict_version);
        frame_perf_report(PERF_OPT_CMD_DICT_END,
                          frame_perf_context.payload,
                          FRAME_PERF_DICT_END_SIZE);
    }
    else if (frame_perf_context.pull_type == PERF_OPT_PULL_SAMPLE)
    {
        wire_u32_le_write(
            &frame_perf_context.payload[FRAME_PERF_SAMPLE_END_SEQUENCE_OFFSET],
            frame_perf_context.sequence);
        wire_u16_le_write(
            &frame_perf_context.payload[FRAME_PERF_SAMPLE_END_RECORD_COUNT_OFFSET],
            frame_perf_context.record_index);
        frame_perf_context.payload[FRAME_PERF_SAMPLE_END_STATUS_OFFSET] =
            wire_octet_get(status);
        frame_perf_context.payload[FRAME_PERF_SAMPLE_END_RESERVED_OFFSET] = 0u;
        frame_perf_report(PERF_OPT_CMD_SAMPLE_END,
                          frame_perf_context.payload,
                          FRAME_PERF_SAMPLE_END_SIZE);
    }
    else
    {
        /* An invalid internal state has no protocol-specific terminal layout. */
    }

    frame_perf_transfer_clear();
}

/**
 * @brief Emit at most one dictionary item for the active transfer.
 */
static void frame_perf_dictionary_poll(void)
{
    section_item_t *p_item = NULL; /* Matching Section wrapper selected for this task activation. */
    section_perf_record_t *p_record = NULL; /* Perf record described by the outgoing item. */
    uint8_t name_length = 0u; /* Number of record-name octets appended after the fixed header. */
    uint8_t protocol_type = 0u; /* FRAME record type encoded in the item header. */
    uint16_t payload_length = 0u; /* Total dictionary item wire length. */

    if (frame_perf_context.pending_end == 1u)
    {
        frame_perf_transfer_end(frame_perf_context.end_status);
        return;
    }

    if (perf_dict_version_get() != frame_perf_context.dict_version)
    {
        frame_perf_transfer_end(PERF_OPT_END_INTERNAL_ERROR);
        return;
    }

    if (frame_perf_context.record_index >= frame_perf_context.record_count)
    {
        frame_perf_transfer_end(PERF_OPT_END_OK);
        return;
    }

    p_item = frame_perf_record_find_next(frame_perf_context.p_item,
                                         frame_perf_context.type_filter);
    p_record = perf_record_from_item(p_item);
    if ((p_item == NULL) ||  /* The captured count promised another matching record. */
        (p_record == NULL))  /* The Section wrapper must still resolve through perf.c. */
    {
        frame_perf_transfer_end(PERF_OPT_END_INTERNAL_ERROR);
        return;
    }

    protocol_type = frame_perf_record_type_get(p_record->record_type);
    name_length = frame_perf_name_copy(
        &frame_perf_context.payload[FRAME_PERF_DICT_ITEM_NAME_OFFSET],
        p_record->p_name);

    wire_u32_le_write(
        &frame_perf_context.payload[FRAME_PERF_DICT_ITEM_SEQUENCE_OFFSET],
        frame_perf_context.sequence);
    wire_u16_le_write(
        &frame_perf_context.payload[FRAME_PERF_DICT_ITEM_INDEX_OFFSET],
        frame_perf_context.record_index);
    wire_u16_le_write(
        &frame_perf_context.payload[FRAME_PERF_DICT_ITEM_RECORD_COUNT_OFFSET],
        frame_perf_context.record_count);
    wire_u16_le_write(
        &frame_perf_context.payload[FRAME_PERF_DICT_ITEM_RECORD_ID_OFFSET],
        p_record->record_id);
    frame_perf_context.payload[FRAME_PERF_DICT_ITEM_RECORD_TYPE_OFFSET] =
        wire_octet_get(protocol_type);
    frame_perf_context.payload[FRAME_PERF_DICT_ITEM_NAME_LENGTH_OFFSET] =
        wire_octet_get(name_length);

    payload_length = (uint16_t)(FRAME_PERF_DICT_ITEM_FIXED_SIZE + name_length);
    frame_perf_report(PERF_OPT_CMD_DICT_ITEM_REPORT,
                      frame_perf_context.payload,
                      payload_length);

    frame_perf_context.p_item = p_item->p_next;
    frame_perf_context.record_index++;
    if (frame_perf_context.record_index >= frame_perf_context.record_count)
    {
        frame_perf_context.end_status = PERF_OPT_END_OK;
        frame_perf_context.pending_end = 1u;
    }
}

/**
 * @brief Serialize one Perf sample item at its fixed Python-protocol offsets.
 * @param[in] p_record Perf record whose latest metrics are sampled.
 * @param[out] p_destination First wire octet of the type-specific sample item.
 * @return Serialized item length, or 0 for an invalid record.
 */
static uint16_t frame_perf_sample_item_write(section_perf_record_t *p_record,
                                             wire_octet_t *p_destination)
{
    uint16_t item_size = 0u; /* Type-specific sample item wire length. */

    if ((p_record == NULL) ||      /* A shared Perf record is required. */
        (p_destination == NULL))   /* The caller must provide bounded payload storage. */
    {
        return 0u;
    }

    wire_u16_le_write(
        &p_destination[FRAME_PERF_SAMPLE_ITEM_RECORD_ID_OFFSET],
        p_record->record_id);
    wire_u32_le_write(
        &p_destination[FRAME_PERF_SAMPLE_ITEM_TIME_US_OFFSET],
        perf_count_to_us(p_record->time));
    wire_u32_le_write(
        &p_destination[FRAME_PERF_SAMPLE_ITEM_MAX_TIME_US_OFFSET],
        perf_count_to_us(p_record->max_time));

    switch (p_record->record_type)
    {
    case SECTION_PERF_RECORD_TASK:
        wire_u32_le_write(
            &p_destination[FRAME_PERF_SAMPLE_TASK_PERIOD_US_OFFSET],
            perf_task_period_us_get(p_record));
        wire_f32_le_write(
            &p_destination[FRAME_PERF_SAMPLE_TASK_LOAD_OFFSET],
            p_record->load * 100.0f);
        wire_f32_le_write(
            &p_destination[FRAME_PERF_SAMPLE_TASK_PEAK_OFFSET],
            p_record->load_max * 100.0f);
        item_size = FRAME_PERF_SAMPLE_TASK_ITEM_SIZE;
        break;

    case SECTION_PERF_RECORD_INTERRUPT:
        wire_f32_le_write(
            &p_destination[FRAME_PERF_SAMPLE_INTERRUPT_LOAD_OFFSET],
            p_record->load * 100.0f);
        wire_f32_le_write(
            &p_destination[FRAME_PERF_SAMPLE_INTERRUPT_PEAK_OFFSET],
            p_record->load_max * 100.0f);
        item_size = FRAME_PERF_SAMPLE_INTERRUPT_ITEM_SIZE;
        break;

    case SECTION_PERF_RECORD_CODE:
        item_size = FRAME_PERF_SAMPLE_CODE_ITEM_SIZE;
        break;

    default:
        break;
    }
    return item_size;
}

/**
 * @brief Emit at most one sample item for the active transfer.
 */
static void frame_perf_sample_poll(void)
{
    section_item_t *p_item = NULL; /* Matching Section wrapper selected for this task activation. */
    section_perf_record_t *p_record = NULL; /* Perf record sampled into the outgoing batch. */
    uint16_t item_size = 0u; /* Type-specific item length following the fixed batch header. */
    uint16_t payload_length = 0u; /* Total sample-batch wire length. */

    if (frame_perf_context.pending_end == 1u)
    {
        frame_perf_transfer_end(frame_perf_context.end_status);
        return;
    }

    if (perf_dict_version_get() != frame_perf_context.dict_version)
    {
        frame_perf_transfer_end(PERF_OPT_END_INTERNAL_ERROR);
        return;
    }

    if (frame_perf_context.record_index >= frame_perf_context.record_count)
    {
        frame_perf_transfer_end(PERF_OPT_END_OK);
        return;
    }

    p_item = frame_perf_record_find_next(frame_perf_context.p_item,
                                         frame_perf_context.type_filter);
    p_record = perf_record_from_item(p_item);
    if ((p_item == NULL) ||  /* The captured count promised another matching record. */
        (p_record == NULL))  /* The Section wrapper must still resolve through perf.c. */
    {
        frame_perf_transfer_end(PERF_OPT_END_INTERNAL_ERROR);
        return;
    }

    item_size = frame_perf_sample_item_write(
        p_record,
        &frame_perf_context.payload[FRAME_PERF_SAMPLE_ITEM_OFFSET]);
    if (item_size == 0u)
    {
        frame_perf_transfer_end(PERF_OPT_END_INTERNAL_ERROR);
        return;
    }

    wire_u32_le_write(
        &frame_perf_context.payload[FRAME_PERF_SAMPLE_BATCH_SEQUENCE_OFFSET],
        frame_perf_context.sequence);
    wire_u16_le_write(
        &frame_perf_context.payload[FRAME_PERF_SAMPLE_BATCH_RECORD_COUNT_OFFSET],
        frame_perf_context.record_count);
    wire_u16_le_write(
        &frame_perf_context.payload[FRAME_PERF_SAMPLE_BATCH_ITEM_COUNT_OFFSET],
        1u);

    payload_length = (uint16_t)(FRAME_PERF_SAMPLE_BATCH_HEADER_SIZE + item_size);
    frame_perf_report(PERF_OPT_CMD_SAMPLE_BATCH_REPORT,
                      frame_perf_context.payload,
                      payload_length);

    frame_perf_context.p_item = p_item->p_next;
    frame_perf_context.record_index++;
    if (frame_perf_context.record_index >= frame_perf_context.record_count)
    {
        frame_perf_context.end_status = PERF_OPT_END_OK;
        frame_perf_context.pending_end = 1u;
    }
}

/**
 * @brief Reply with Perf protocol capabilities and timing conversion data.
 * @param[in] p_request Received binary command.
 * @param[in] p_output Link callback used for the direct acknowledgement.
 */
static void frame_perf_info_query_act(section_packform_t *p_request,
                                      section_link_tx_func_t *p_output)
{
    wire_octet_t payload[FRAME_PERF_INFO_ACK_SIZE] = {0}; /* Fixed-offset information response. */

    if (frame_perf_request_is_command(p_request) == 0u)
    {
        return;
    }

    wire_u16_le_write(
        &payload[FRAME_PERF_INFO_PROTOCOL_VERSION_OFFSET],
        FRAME_PERF_PROTOCOL_VERSION);
    wire_u16_le_write(
        &payload[FRAME_PERF_INFO_RECORD_COUNT_OFFSET],
        frame_perf_record_count(PERF_OPT_TYPE_ALL));
    wire_f32_le_write(
        &payload[FRAME_PERF_INFO_UNIT_US_OFFSET],
        perf_count_unit_us_get());
    wire_u32_le_write(
        &payload[FRAME_PERF_INFO_COUNT_PER_TICK_OFFSET],
        perf_cnt_per_sys_tick_get());
    wire_u32_le_write(
        &payload[FRAME_PERF_INFO_CPU_WINDOW_MS_OFFSET],
        PERF_CPU_LOAD_PERIOD_MS);
    payload[FRAME_PERF_INFO_FLAGS_OFFSET] =
        wire_octet_get(FRAME_PERF_INFO_FLAGS);

    frame_perf_reply(p_request,
                     p_output,
                     PERF_OPT_CMD_INFO_QUERY,
                     payload,
                     FRAME_PERF_INFO_ACK_SIZE);
}

/**
 * @brief Reply with current and peak task and interrupt load percentages.
 * @param[in] p_request Received binary command.
 * @param[in] p_output Link callback used for the direct acknowledgement.
 */
static void frame_perf_summary_query_act(section_packform_t *p_request,
                                         section_link_tx_func_t *p_output)
{
    wire_octet_t payload[FRAME_PERF_SUMMARY_ACK_SIZE] = {0}; /* Fixed-offset load response. */

    if (frame_perf_request_is_command(p_request) == 0u)
    {
        return;
    }

    wire_f32_le_write(
        &payload[FRAME_PERF_SUMMARY_TASK_LOAD_OFFSET],
        perf_task_metric_get() * 100.0f);
    wire_f32_le_write(
        &payload[FRAME_PERF_SUMMARY_TASK_PEAK_OFFSET],
        perf_task_metric_max_get() * 100.0f);
    wire_f32_le_write(
        &payload[FRAME_PERF_SUMMARY_INTERRUPT_LOAD_OFFSET],
        perf_interrupt_metric_get() * 100.0f);
    wire_f32_le_write(
        &payload[FRAME_PERF_SUMMARY_INTERRUPT_PEAK_OFFSET],
        perf_interrupt_metric_max_get() * 100.0f);

    frame_perf_reply(p_request,
                     p_output,
                     PERF_OPT_CMD_SUMMARY_QUERY,
                     payload,
                     FRAME_PERF_SUMMARY_ACK_SIZE);
}

/**
 * @brief Reset all Perf peak metrics and acknowledge the operation.
 * @param[in] p_request Received binary command.
 * @param[in] p_output Link callback used for the direct acknowledgement.
 */
static void frame_perf_reset_peak_act(section_packform_t *p_request,
                                      section_link_tx_func_t *p_output)
{
    wire_octet_t payload[FRAME_PERF_RESET_ACK_SIZE] = {0}; /* Legacy four-octet success response. */

    if (frame_perf_request_is_command(p_request) == 0u)
    {
        return;
    }

    perf_reset_peak_value();
    payload[FRAME_PERF_RESET_ACK_SUCCESS_OFFSET] = 1u;
    frame_perf_reply(p_request,
                     p_output,
                     PERF_OPT_CMD_RESET_PEAK,
                     payload,
                     FRAME_PERF_RESET_ACK_SIZE);
}

/**
 * @brief Accept or reject one asynchronous dictionary transfer request.
 * @param[in] p_request Received binary command.
 * @param[in] p_output Link callback retained by an accepted transfer.
 */
static void frame_perf_dict_query_act(section_packform_t *p_request,
                                      section_link_tx_func_t *p_output)
{
    wire_octet_t payload[FRAME_PERF_DICT_ACK_SIZE] = {0}; /* Fixed-offset dictionary acknowledgement. */
    uint32_t known_dict_version = 0u; /* Host cache version, currently advisory. */
    uint32_t current_dict_version = 0u; /* Device dictionary version returned in every response. */
    uint8_t accepted = 0u; /* One after the transfer context is acquired. */
    uint8_t reject_reason = PERF_OPT_REJECT_INVALID_FILTER; /* Deterministic malformed-request result. */
    uint8_t type_filter = 0xFFu; /* Invalid default used for a short or missing payload. */

    if (frame_perf_request_is_command(p_request) == 0u)
    {
        return;
    }

    current_dict_version = perf_dict_version_get();
    if (frame_perf_request_payload_is_valid(p_request,
                                            FRAME_PERF_DICT_QUERY_SIZE) == 1u)
    {
        type_filter = wire_octet_get(
            p_request->p_data[FRAME_PERF_DICT_QUERY_FILTER_OFFSET]);
        known_dict_version = wire_u32_le_read(
            &p_request->p_data[FRAME_PERF_DICT_QUERY_VERSION_OFFSET]);
        (void)known_dict_version;

        accepted = frame_perf_transfer_start(p_request,
                                             p_output,
                                             type_filter,
                                             PERF_OPT_PULL_DICT,
                                             &reject_reason);
    }

    payload[FRAME_PERF_DICT_ACK_ACCEPTED_OFFSET] = wire_octet_get(accepted);
    payload[FRAME_PERF_DICT_ACK_FILTER_OFFSET] = wire_octet_get(type_filter);
    wire_u32_le_write(
        &payload[FRAME_PERF_DICT_ACK_VERSION_OFFSET],
        current_dict_version);
    payload[FRAME_PERF_DICT_ACK_REJECT_REASON_OFFSET] =
        wire_octet_get(reject_reason);

    if (accepted == 1u)
    {
        wire_u16_le_write(
            &payload[FRAME_PERF_DICT_ACK_RECORD_COUNT_OFFSET],
            frame_perf_context.record_count);
        wire_u32_le_write(
            &payload[FRAME_PERF_DICT_ACK_SEQUENCE_OFFSET],
            frame_perf_context.sequence);
    }

    frame_perf_reply(p_request,
                     p_output,
                     PERF_OPT_CMD_DICT_QUERY,
                     payload,
                     FRAME_PERF_DICT_ACK_SIZE);
}

/**
 * @brief Accept or reject one versioned asynchronous sample transfer request.
 * @param[in] p_request Received binary command.
 * @param[in] p_output Link callback retained by an accepted transfer.
 */
static void frame_perf_sample_query_act(section_packform_t *p_request,
                                        section_link_tx_func_t *p_output)
{
    wire_octet_t payload[FRAME_PERF_SAMPLE_ACK_SIZE] = {0}; /* Fixed-offset sample acknowledgement. */
    uint32_t query_dict_version = 0u; /* Dictionary version named by the host request. */
    uint32_t current_dict_version = 0u; /* Device dictionary version returned in every response. */
    uint8_t accepted = 0u; /* One after the transfer context is acquired. */
    uint8_t query_flags = 0u; /* Tail-compatible flags reserved by protocol version one. */
    uint8_t reject_reason = PERF_OPT_REJECT_INVALID_FILTER; /* Deterministic malformed-request result. */
    uint8_t type_filter = 0xFFu; /* Invalid default used for a short or missing payload. */

    if (frame_perf_request_is_command(p_request) == 0u)
    {
        return;
    }

    current_dict_version = perf_dict_version_get();
    if (frame_perf_request_payload_is_valid(p_request,
                                            FRAME_PERF_SAMPLE_QUERY_SIZE) == 1u)
    {
        type_filter = wire_octet_get(
            p_request->p_data[FRAME_PERF_SAMPLE_QUERY_FILTER_OFFSET]);
        query_flags = wire_octet_get(
            p_request->p_data[FRAME_PERF_SAMPLE_QUERY_FLAGS_OFFSET]);
        query_dict_version = wire_u32_le_read(
            &p_request->p_data[FRAME_PERF_SAMPLE_QUERY_VERSION_OFFSET]);
        (void)query_flags;

        if (type_filter > PERF_OPT_TYPE_CODE)
        {
            reject_reason = PERF_OPT_REJECT_INVALID_FILTER;
        }
        else if (query_dict_version != current_dict_version)
        {
            reject_reason = PERF_OPT_REJECT_DICT_MISMATCH;
        }
        else
        {
            accepted = frame_perf_transfer_start(p_request,
                                                 p_output,
                                                 type_filter,
                                                 PERF_OPT_PULL_SAMPLE,
                                                 &reject_reason);
        }
    }

    payload[FRAME_PERF_SAMPLE_ACK_ACCEPTED_OFFSET] = wire_octet_get(accepted);
    payload[FRAME_PERF_SAMPLE_ACK_FILTER_OFFSET] = wire_octet_get(type_filter);
    wire_u32_le_write(
        &payload[FRAME_PERF_SAMPLE_ACK_VERSION_OFFSET],
        current_dict_version);
    payload[FRAME_PERF_SAMPLE_ACK_REJECT_REASON_OFFSET] =
        wire_octet_get(reject_reason);

    if (accepted == 1u)
    {
        wire_u16_le_write(
            &payload[FRAME_PERF_SAMPLE_ACK_RECORD_COUNT_OFFSET],
            frame_perf_context.record_count);
        wire_u32_le_write(
            &payload[FRAME_PERF_SAMPLE_ACK_SEQUENCE_OFFSET],
            frame_perf_context.sequence);
    }

    frame_perf_reply(p_request,
                     p_output,
                     PERF_OPT_CMD_SAMPLE_QUERY,
                     payload,
                     FRAME_PERF_SAMPLE_ACK_SIZE);
}

/**
 * @brief Cancel an active transfer when report control is disabled.
 * @param[in] p_request Received binary command.
 * @param[in] p_output Link callback used for a non-broadcast acknowledgement.
 */
static void frame_perf_report_control_act(section_packform_t *p_request,
                                          section_link_tx_func_t *p_output)
{
    wire_octet_t payload[FRAME_PERF_CONTROL_ACK_SIZE] = {0}; /* One-octet control acknowledgement. */
    uint8_t enable = 0u; /* Legacy missing-payload behavior treats the request as disable. */

    if (frame_perf_request_is_command(p_request) == 0u)
    {
        return;
    }

    if (frame_perf_request_payload_is_valid(p_request,
                                            FRAME_PERF_CONTROL_QUERY_SIZE) == 1u)
    {
        enable = wire_octet_get(
            p_request->p_data[FRAME_PERF_CONTROL_ENABLE_OFFSET]);
    }

    if (enable == 0u)
    {
        frame_perf_transfer_clear();
        payload[FRAME_PERF_CONTROL_ACK_SUCCESS_OFFSET] = 1u;
    }

    if ((p_request->dst == 0u) &&     /* Broadcast controls execute locally. */
        (p_request->d_dst == 0u))     /* Broadcast acknowledgements would collide. */
    {
        return;
    }

    frame_perf_reply(p_request,
                     p_output,
                     PERF_OPT_CMD_REPORT_CONTROL,
                     payload,
                     FRAME_PERF_CONTROL_ACK_SIZE);
}

/**
 * @brief Advance one active dictionary or sample stream by bounded work.
 */
static void frame_perf_poll_task(void)
{
    if (frame_perf_context.active == 0u)
    {
        return;
    }

    switch (frame_perf_context.pull_type)
    {
    case PERF_OPT_PULL_DICT:
        frame_perf_dictionary_poll();
        break;

    case PERF_OPT_PULL_SAMPLE:
        frame_perf_sample_poll();
        break;

    default:
        frame_perf_transfer_clear();
        break;
    }
}

/**
 * @brief Initialize the C28x-safe Perf protocol service state.
 */
static void frame_perf_service_init(void)
{
    frame_perf_transfer_clear();
    frame_perf_sequence = 0u;
}

REG_INIT(1, frame_perf_service_init)
REG_TASK_MS(FRAME_PERF_TASK_PERIOD_MS, frame_perf_poll_task)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_INFO_QUERY, frame_perf_info_query_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_SUMMARY_QUERY, frame_perf_summary_query_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_RESET_PEAK, frame_perf_reset_peak_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_DICT_QUERY, frame_perf_dict_query_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_SAMPLE_QUERY, frame_perf_sample_query_act)
REG_COMM(PERF_OPT_CMD_SET, PERF_OPT_CMD_REPORT_CONTROL, frame_perf_report_control_act)

#endif /* PERF_ENABLE == 1u */
