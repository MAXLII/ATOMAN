// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope_bindings.c
 * @brief Bind the portable SECTION scope service to ESP32-C5 BSP capabilities.
 * @details Part of base. Entity: callback bindings, no sampler or transport state.
 *          Prior: BSP and SECTION initialization precede control commands.
 *          Time: initialization/task context only; sampling belongs to REG_INTERRUPT.
 *          The binding supplies the initialized board timer rate through source_set().
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "realtime_scope_service.h"
#include "realtime_scope_bindings.h"
#include "bsp_system.h"
#include "bsp_timer.h"
#include "bsp_wifi.h"
#include "bsp_wifi_channels.h"
#include <stddef.h>

static uint32_t saved_interrupts;
static void lock(void)
{
    saved_interrupts = bsp_system_critical_enter();
}
static void unlock(void)
{
    bsp_system_critical_exit(saved_interrupts);
}
static void stop(void)
{
    bsp_system_runtime_enter();
    bsp_wifi_channel_reset(BSP_WIFI_CHANNEL_UPLOAD);
    bsp_system_runtime_exit();
}
static bool connected(void)
{
    bsp_wifi_channel_status_t status;
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &status);
    return status.connected;
}
static uint32_t session(void)
{
    bsp_wifi_channel_status_t status;
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &status);
    return status.session;
}
static uint32_t fault(void)
{
    bsp_wifi_channel_status_t status;
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &status);
    return status.error;
}
bool realtime_scope_bindings_bind(uint8_t *p_queue, uint32_t capacity)
{
    if (    p_queue == NULL
         || capacity == 0u)
        return false;
    const realtime_scope_port_t binding = {.byte_budget      = BASE_SCOPE_BYTE_BUDGET,
                                           .p_lock           = lock,
                                           .p_unlock         = unlock,
                                           .p_stop           = stop,
                                           .p_data_connected = connected,
                                           .p_data_session   = session,
                                           .p_link_fault     = fault,
                                           .p_identity       = bsp_system_firmware_identity,
                                           .p_resolve        = bsp_system_sample_address};

    if (bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_UPLOAD, 9001u, realtime_scope_service_read))
    {
        bool bound = realtime_scope_service_bind(&binding, p_queue, capacity);

        if (bound)
            (void)realtime_scope_service_source_set(bsp_timer_frequency_get());
        return bound;
    }
    return false;
}
