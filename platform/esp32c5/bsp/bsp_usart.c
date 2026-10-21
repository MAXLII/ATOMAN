// SPDX-License-Identifier: MIT
/**
 * @file bsp_usart.c
 * @brief Extracted USB Serial/JTAG driver handover and bounded raw I/O.
 * @details Part of base. SDK ring buffers belong to this BSP after init.
 *          Initialize on the same CPU core as SDK USB initialization; no ISR API.
 *          No logging/stdio writer may share this binary transport after handover.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "bsp_usart.h"
#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include "driver/usb_serial_jtag.h"
#include "driver/usb_serial_jtag_vfs.h"
#include "freertos/FreeRTOS.h"

static bool initialized; /**< Initialization is serialized by the owner task. */

int32_t bsp_usart_init(void)
{
    usb_serial_jtag_driver_config_t config = {
        .tx_buffer_size = 2048u, /* Hold several maximum-size E8 frames. */
        .rx_buffer_size = 2048u, /* Absorb USB packet bursts between Link polls. */
    };
    esp_err_t result = ESP_OK; /* SDK driver handover result. */

    if (initialized == true)
    {
        return ESP_OK;
    }

    if (usb_serial_jtag_is_driver_installed() == true)
    {
        result = usb_serial_jtag_wait_tx_done(pdMS_TO_TICKS(20u));

        if (    (result != ESP_OK)           /* Propagate actual driver failure. */
             && (result != ESP_ERR_TIMEOUT)) /* A disconnected PC may leave boot output pending. */
        {
            return result;
        }
        usb_serial_jtag_vfs_use_nonblocking();
        result = usb_serial_jtag_driver_uninstall();

        if (result != ESP_OK)
        {
            return result;
        }
    }
    result = usb_serial_jtag_driver_install(&config);

    if (result == ESP_OK)
    {
        initialized = true;
        /* Leave VFS detached so ROM/stdio traffic cannot enter FRAME packets. */
    }
    return result;
}

int32_t bsp_usart_read(uint8_t *p_data, uint32_t capacity)
{
    if (    (initialized == false) /* Require a live SDK driver. */
         || (p_data == NULL)       /* Reject an absent destination. */
         || (capacity == 0u)       /* Read must have room for at least 1 byte. */
         || (capacity > BSP_USART_TRANSFER_MAX)) /* Bound one cooperative call. */
    {
        return -1;
    }
    return usb_serial_jtag_read_bytes(p_data, capacity, 0u);
}

int32_t bsp_usart_write(const uint8_t *p_data, uint32_t length)
{
    if (    (initialized == false) /* Require a live SDK driver. */
         || (p_data == NULL)       /* Reject an absent source. */
         || (length == 0u)         /* No zero-length USB transaction is exposed. */
         || (length > BSP_USART_TRANSFER_MAX)) /* Bound work and caller ownership. */
    {
        return -1;
    }
    return usb_serial_jtag_write_bytes(p_data, length, pdMS_TO_TICKS(20u));
}

uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data)
{
    return bsp_usart_read(p_data, 1u) == 1 ? 1u : 0u;
}
