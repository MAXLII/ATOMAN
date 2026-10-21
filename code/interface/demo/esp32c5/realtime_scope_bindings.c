// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope_bindings.c
 * @brief Bind the portable SECTION scope service to ESP32-C5 BSP capabilities.
 * @details Part of base. Entity: callback bindings, no sampler or transport state.
 *          Prior: BSP and SECTION initialization precede control commands.
 *          Time: initialization/task context only; sampling belongs to REG_INTERRUPT.
 *          The control application supplies its actual rate through source_set().
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "realtime_scope_service.h"
#include "bsp_system.h"
#include "bsp_wifi.h"
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
    bsp_wifi_channel_reset(BSP_WIFI_CHANNEL_UPLOAD);
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
static void bind_service(void)
{
    static bool bound;

    if (bound)
        return;
    const realtime_scope_port_t binding = {.byte_budget      = 2000000u,
                                           .p_lock           = lock,
                                           .p_unlock         = unlock,
                                           .p_stop           = stop,
                                           .p_data_connected = connected,
                                           .p_data_session   = session,
                                           .p_link_fault     = fault,
                                           .p_identity       = bsp_system_firmware_identity,
                                           .p_resolve        = bsp_system_sample_address};

    if (bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_UPLOAD, 9001u, realtime_scope_service_read))
        bound = realtime_scope_service_bind(&binding);
}
REG_INIT(0, bind_service)
REG_TASK(1, bind_service)
