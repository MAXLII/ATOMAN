// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope_service.h
 * @brief Portable real-time scope control service and platform binding.
 * @details Part of base. Defines the public COMM contract and bounded messages.
 *          Entity: the service owns protocol semantics; the port owns hardware.
 *          Prior: platform callbacks protect task/ISR access without owning scheduling.
 *          Time: SECTION owns control tasks; an existing control ISR dispatches sampling.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#ifndef REALTIME_SCOPE_SERVICE_H
#define REALTIME_SCOPE_SERVICE_H
#include "realtime_scope.h"
#include "comm.h"

#ifndef REALTIME_SCOPE_ENABLE
#define REALTIME_SCOPE_ENABLE 0
#endif
#define REALTIME_SCOPE_CMD_SET       0x01
#define REALTIME_SCOPE_CMD_WORD      0x3A
#define REALTIME_SCOPE_CONTROL_BYTES 68u
#define REALTIME_SCOPE_REQUEST_DEPTH 4u

typedef struct realtime_scope_control
{
    section_packform_t packet;
    section_link_tx_func_t *p_output;
    uint8_t data[REALTIME_SCOPE_CONTROL_BYTES];
} realtime_scope_control_t;

typedef struct realtime_scope_port
{
    uint32_t byte_budget;
    void (*p_lock)(void);
    void (*p_unlock)(void);
    /* Retire only the data session; the control interrupt continues running. */
    void (*p_stop)(void);
    bool (*p_data_connected)(void);
    uint32_t (*p_data_session)(void);
    uint32_t (*p_link_fault)(void);
    void (*p_identity)(uint8_t hash[32]);
    const volatile void *(*p_resolve)(uint32_t address, uint8_t size);
} realtime_scope_port_t;

/** Bind once from initialization/task context. The service owns its sampler and request queue. */
bool realtime_scope_service_bind(const realtime_scope_port_t *p_port, uint8_t *p_queue, uint32_t capacity);
/** Existing control-loop initialization supplies its actual rate, never a host command.
 *  Zero means no sampling source. May only change while acquisition is stopped. */
bool realtime_scope_service_source_set(uint32_t frequency);
/** @return Declared sampling-source frequency in Hz; 0 until bound by the application. */
uint32_t realtime_scope_service_source_frequency_get(void);
/** ISR entry. The application/demo owns REG_INTERRUPT registration and its priority. */
void realtime_scope_service_sample(void);
/** Task-context transport callback: copy complete EA frames, never split a frame.
 *  The BSP owns aggregation, partial socket writes and connection lifetime. */
uint32_t realtime_scope_service_read(uint8_t *p_data, uint32_t capacity);
#endif
