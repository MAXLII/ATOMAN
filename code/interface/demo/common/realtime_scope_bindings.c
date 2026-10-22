// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope_bindings.c
 * @brief Demo composition for a fixed control interrupt and Ethernet upload stream.
 * @details Part of base. Entity: software storage and BSP callback bindings.
 *          Prior: existing timer/network initialization, without scope-specific peripherals.
 *          Time: sampling is registered by demo_realtime_scope; background work by DBG.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "realtime_scope_service.h"
#include "bsp_system.h"
#include "bsp_tcp_stream.h"
#include "bsp_timer.h"
static uint8_t sample_queue[2500];
static uint32_t saved_interrupts;
static void lock(void)
{
    saved_interrupts = bsp_system_critical_enter();
}
static void unlock(void)
{
    bsp_system_critical_exit(saved_interrupts);
}
static void bind_service(void)
{
    const realtime_scope_port_t binding = {.byte_budget      = 1000000u,
                                           .p_lock           = lock,
                                           .p_unlock         = unlock,
                                           .p_stop           = bsp_tcp_stream_reset,
                                           .p_data_connected = bsp_tcp_stream_connected,
                                           .p_data_session   = bsp_tcp_stream_session,
                                           .p_link_fault     = bsp_tcp_stream_error,
                                           .p_identity       = bsp_system_firmware_identity,
                                           .p_resolve        = bsp_system_sample_address};

    if (    bsp_tcp_stream_configure(9001u, realtime_scope_service_read)
         && realtime_scope_service_bind(&binding, sample_queue, sizeof(sample_queue)))
    {
        uint32_t period = bsp_timer_jitter_period_ticks_get();

        if (period != 0u)
            (void)realtime_scope_service_source_set(bsp_timer_jitter_clock_hz_get() / period);
    }
}
REG_INIT(21, bind_service)
