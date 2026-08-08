// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.c
 * @brief   GD32E507Z-EVAL USART BSP implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure USART0 on PA9 and PA10 at 115200 baud
 *          - Serialize debug messages through a software transmit ring buffer
 *          - Buffer received bytes from the USART0 interrupt
 *          - Adapt formatted output to the Section communication link
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Ring-buffer access is protected by short interrupt-mask regions
 *          - Polling hardware transmit is isolated in one registered task
 *          - Hardware access uses the GD32E50x standard peripheral library
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

#include "bsp_usart.h"

#include "gd32e50x.h"
#include "section.h"

#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#define BSP_USART_PRINT_BUFFER_SIZE 256u /* Maximum formatted debug message including terminator. */
#define BSP_USART_TX_RING_SIZE 1024u      /* Serialized pending debug bytes; one slot remains unused. */
#define BSP_USART_RX_RING_SIZE 256u       /* Interrupt-fed pending receive bytes; one slot remains unused. */

static uint8_t s_bsp_usart_tx_ring[BSP_USART_TX_RING_SIZE]; /* USART0 software transmit queue. */
static volatile uint16_t s_bsp_usart_tx_head = 0u;          /* Next queue position written by producers. */
static volatile uint16_t s_bsp_usart_tx_tail = 0u;          /* Next queue position consumed by the task. */
static uint8_t s_bsp_usart_rx_ring[BSP_USART_RX_RING_SIZE]; /* USART0 interrupt receive queue. */
static volatile uint16_t s_bsp_usart_rx_head = 0u;          /* Next queue position written by the ISR. */
static volatile uint16_t s_bsp_usart_rx_tail = 0u;          /* Next queue position consumed by the link task. */

volatile uint32_t g_bsp_usart_dbg_tx_drop_count = 0u;
volatile uint32_t g_bsp_usart_dbg_rx_drop_count = 0u;
volatile uint32_t g_bsp_usart_dbg_rx_error_count = 0u;

static uint32_t bsp_usart_irq_lock(void)
{
    const uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

static void bsp_usart_irq_unlock(uint32_t primask)
{
    if ((primask & 1u) == 0u)
    {
        __enable_irq();
    }
}

static uint16_t bsp_usart_tx_ring_free(void)
{
    const uint16_t head = s_bsp_usart_tx_head;
    const uint16_t tail = s_bsp_usart_tx_tail;

    if (head >= tail)
    {
        return (uint16_t)(BSP_USART_TX_RING_SIZE - (uint32_t)(head - tail) - 1u);
    }

    return (uint16_t)((uint32_t)tail - (uint32_t)head - 1u);
}

static int bsp_usart_dbg_enqueue(const uint8_t *p_data, uint16_t length)
{
    uint32_t primask = 0u; /* Interrupt state saved while the complete message is queued. */
    uint16_t index = 0u;   /* Current byte in the caller-owned message. */

    if ((p_data == NULL) || (length == 0u))
    {
        return 0;
    }

    primask = bsp_usart_irq_lock();
    if (bsp_usart_tx_ring_free() < length)
    {
        g_bsp_usart_dbg_tx_drop_count++;
        bsp_usart_irq_unlock(primask);
        return 0;
    }

    for (index = 0u; index < length; index++)
    {
        s_bsp_usart_tx_ring[s_bsp_usart_tx_head] = p_data[index];
        s_bsp_usart_tx_head = (uint16_t)(((uint32_t)s_bsp_usart_tx_head + 1u) % BSP_USART_TX_RING_SIZE);
    }
    bsp_usart_irq_unlock(primask);

    return (int)length;
}

static void bsp_usart_dbg_tx_service_task(void)
{
    uint8_t data = 0u;   /* Byte removed atomically from the software queue. */
    uint32_t primask = 0u; /* Interrupt state used while updating the queue tail. */

    for (;;)
    {
        primask = bsp_usart_irq_lock();
        if (s_bsp_usart_tx_tail == s_bsp_usart_tx_head)
        {
            bsp_usart_irq_unlock(primask);
            break;
        }

        data = s_bsp_usart_tx_ring[s_bsp_usart_tx_tail];
        s_bsp_usart_tx_tail = (uint16_t)(((uint32_t)s_bsp_usart_tx_tail + 1u) % BSP_USART_TX_RING_SIZE);
        bsp_usart_irq_unlock(primask);

        while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET)
        {
        }
        usart_data_transmit(USART0, (uint16_t)data);
    }
}

REG_TASK_MS(1, bsp_usart_dbg_tx_service_task)

static void bsp_usart_dbg_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USART0);

    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200u);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);
    usart_interrupt_enable(USART0, USART_INT_RBNE);
    nvic_irq_enable(USART0_IRQn, 2u, 0u);
}

REG_INIT(0, bsp_usart_dbg_init)

void bsp_usart_dbg_tx(char *p_data, int length)
{
    uint32_t transfer_length = 0u; /* Length constrained to the software queue API. */

    if ((p_data == NULL) || /* No transmit buffer was provided. */
        (length <= 0))      /* There are no bytes to transmit. */
    {
        return;
    }

    transfer_length = (uint32_t)length;
    if (transfer_length > UINT16_MAX)
    {
        transfer_length = UINT16_MAX;
    }

    (void)bsp_usart_dbg_enqueue((const uint8_t *)(uintptr_t)p_data, (uint16_t)transfer_length);
}

void bsp_usart_dbg_printf(const char *p_format, ...)
{
    char buffer[BSP_USART_PRINT_BUFFER_SIZE] = {0}; /* Stack-owned formatted output buffer. */
    va_list arguments;                              /* Variable arguments associated with p_format. */
    int length = 0;                                 /* Number of formatted characters to transmit. */

    if (p_format == NULL)
    {
        return;
    }

    va_start(arguments, p_format);
    length = vsnprintf(buffer, sizeof(buffer), p_format, arguments);
    va_end(arguments);

    if (length <= 0)
    {
        return;
    }
    if ((size_t)length >= sizeof(buffer))
    {
        length = (int)(sizeof(buffer) - 1u);
    }

    bsp_usart_dbg_tx(buffer, length);
}

int bsp_usart_dbg_tx_dma(const uint8_t *p_data, uint32_t length)
{
    uint32_t transfer_length = length; /* Length constrained to the signed BSP transmit API. */

    if (p_data == NULL)
    {
        return 0;
    }
    if (transfer_length > UINT16_MAX)
    {
        transfer_length = UINT16_MAX;
    }

    return bsp_usart_dbg_enqueue(p_data, (uint16_t)transfer_length);
}

uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data)
{
    uint32_t primask = 0u; /* Interrupt state saved while the receive queue tail is updated. */

    if (p_data == NULL)
    {
        return 0u;
    }

    primask = bsp_usart_irq_lock();
    if (s_bsp_usart_rx_tail == s_bsp_usart_rx_head)
    {
        bsp_usart_irq_unlock(primask);
        return 0u;
    }

    *p_data = s_bsp_usart_rx_ring[s_bsp_usart_rx_tail];
    s_bsp_usart_rx_tail = (uint16_t)(((uint32_t)s_bsp_usart_rx_tail + 1u) % BSP_USART_RX_RING_SIZE);
    bsp_usart_irq_unlock(primask);
    return 1u;
}

void bsp_usart_dbg_irq_handler(void)
{
    uint16_t next_head = 0u; /* Queue position following the byte being received. */
    uint8_t data = 0u;       /* Byte read once to clear the hardware receive condition. */

    if ((usart_flag_get(USART0, USART_FLAG_RBNE) == RESET) &&
        (usart_flag_get(USART0, USART_FLAG_ORERR) == RESET))
    {
        return;
    }

    if (usart_flag_get(USART0, USART_FLAG_ORERR) != RESET)
    {
        g_bsp_usart_dbg_rx_error_count++;
    }

    data = (uint8_t)usart_data_receive(USART0);
    next_head = (uint16_t)(((uint32_t)s_bsp_usart_rx_head + 1u) % BSP_USART_RX_RING_SIZE);
    if (next_head == s_bsp_usart_rx_tail)
    {
        g_bsp_usart_dbg_rx_drop_count++;
        return;
    }

    s_bsp_usart_rx_ring[s_bsp_usart_rx_head] = data;
    s_bsp_usart_rx_head = next_head;
}

void bsp_usart_iso_printf(const char *p_format, ...)
{
    (void)p_format;
}

void bsp_usart_iso_tx(char *p_data, int length)
{
    (void)p_data;
    (void)length;
}

uint8_t bsp_usart_iso_rx_get_byte(uint8_t *p_data)
{
    (void)p_data;
    return 0u;
}
