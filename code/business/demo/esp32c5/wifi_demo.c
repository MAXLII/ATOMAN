// SPDX-License-Identifier: MIT
/**
 * @file wifi_demo.c
 * @brief Demo Wi-Fi parameter commands and diagnostic presentation.
 * @details Entity: command state and text formatting. Prior: Wi-Fi Interface is available.
 *          Time: serialized application command callbacks; no BSP or SDK access.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "wifi_interface.h"
#include "shell.h"
#include <stddef.h>
static uint32_t wifi_enable;
static int32_t wifi_control_error;
static void wifi_init(void)
{
    wifi_enable = wifi_interface_enabled_get();
}
REG_INIT(2, wifi_init)
static void wifi_changed(DEC_MY_PRINTF)
{
    (void)my_printf;
    wifi_control_error = wifi_interface_enable_set(wifi_enable);
}
/** @param my_printf Requesting Link output. */
static void wifi(DEC_MY_PRINTF)
{
    wifi_interface_status_t status = {0}; /* Latest task-owned driver snapshot. */
    uint32_t address               = 0u;  /* Host-independent bytes from the network-order IPv4 value. */
    const uint8_t *p_bytes         = (const uint8_t *)&address; /* Object representation is valid byte access. */
    wifi_interface_status_get(&status);
    address = status.ipv4_be;

    if (    (my_printf != NULL)             /* Require a bound Link. */
         && (my_printf->my_printf != NULL)) /* Text output is optional on generic Links. */
    {
        my_printf->my_printf("wifi mode=%u ready=%lu ip=%u.%u.%u.%u rssi=%ld error=%ld control=%ld\r\n",
                             (unsigned)status.mode,
                             (unsigned long)status.ready,
                             (unsigned)p_bytes[0],
                             (unsigned)p_bytes[1],
                             (unsigned)p_bytes[2],
                             (unsigned)p_bytes[3],
                             (long)status.rssi_dbm,
                             (long)status.error,
                             (long)wifi_control_error);
        my_printf->my_printf("wifi internal_heap=%lu poll_us=%lu write_us=%lu tx_bytes=%lu blocked=%lu\r\n",
                             (unsigned long)status.internal_heap_free,
                             (unsigned long)status.max_poll_gap_us,
                             (unsigned long)status.max_write_us,
                             (unsigned long)status.tx_bytes,
                             (unsigned long)status.backpressure);
        my_printf->my_printf(
            "wifi staging_peak=%lu batches=%lu batch_gap_us=%lu pending_us=%lu partial=%lu heap_min=%lu\r\n",
            (unsigned long)status.tx_high_water,
            (unsigned long)status.tx_batches,
            (unsigned long)status.max_batch_gap_us,
            (unsigned long)status.max_pending_us,
            (unsigned long)status.partial_writes,
            (unsigned long)status.internal_heap_minimum);
    }
}

/** @brief Explicit synchronous radio diagnostic; may delay application work during acquisition. */
static void wifi_rssi(DEC_MY_PRINTF)
{
    (void)wifi_interface_rssi_refresh();
    wifi(my_printf);
}

REG_SHELL_VAR(WIFI_ENABLE, wifi_enable, SHELL_UINT32, 1u, 0u, wifi_changed, SHELL_STA_NULL)
REG_SHELL_CMD(wifi, wifi)
REG_SHELL_CMD(wifi_rssi, wifi_rssi)
