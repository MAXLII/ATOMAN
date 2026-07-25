// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.c
 * @brief   Zynq-7020 PS UART1 and PL UART DMA implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure the COM6 PS UART1 link at 921600 8N1
 *          - Configure the COM7 PL UART and autonomous DDR ring DMA
 *          - Publish and consume ring counters with explicit memory barriers
 *          - Record and clear PL error interrupts without protocol work in ISR
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The reserved PL DMA window is mapped normal non-cacheable
 *          - Hardware access is isolated in the Zynq BSP
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_usart.h"

#include "bsp_interrupt.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xuartps.h"
#include "xuartps_hw.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define BSP_USART_BAUD_RATE 921600U   /* PS UART1 baud rate used by the COM5 CH340 link. */
#define BSP_USART_PRINTF_SIZE 512U    /* 单次格式化输出的固定缓冲区容量。 */

#define BSP_USART_PL_BASE 0x40600000U
#define BSP_USART_PL_IRQ_ID 61U
#define BSP_USART_PL_CLOCK_HZ 50000000ULL
#define BSP_USART_PL_OVERSAMPLE 16ULL
#define BSP_USART_PL_RX_RING_BASE 0x1FF00000U
#define BSP_USART_PL_TX_RING_BASE 0x1FF10000U
#define BSP_USART_PL_RING_SIZE 65536U
#define BSP_USART_PL_DMA_WINDOW_BASE 0x1FF00000U
#define BSP_USART_PL_EXPECTED_VERSION 0x00010000U
#define BSP_USART_PL_ERROR_IRQ_MASK 0x0000003FU

#define BSP_USART_PL_REG_CONTROL 0x00U
#define BSP_USART_PL_REG_STATUS 0x04U
#define BSP_USART_PL_REG_CONFIG 0x08U
#define BSP_USART_PL_REG_BAUD_INCREMENT 0x0CU
#define BSP_USART_PL_REG_RX_BASE 0x10U
#define BSP_USART_PL_REG_RX_SIZE 0x14U
#define BSP_USART_PL_REG_RX_PRODUCED 0x18U
#define BSP_USART_PL_REG_RX_CONSUMED 0x1CU
#define BSP_USART_PL_REG_TX_BASE 0x20U
#define BSP_USART_PL_REG_TX_SIZE 0x24U
#define BSP_USART_PL_REG_TX_PRODUCED 0x28U
#define BSP_USART_PL_REG_TX_CONSUMED 0x2CU
#define BSP_USART_PL_REG_IRQ_STATUS 0x30U
#define BSP_USART_PL_REG_IRQ_ENABLE 0x34U
#define BSP_USART_PL_REG_UART_ERRORS 0x38U
#define BSP_USART_PL_REG_DMA_ERRORS 0x3CU
#define BSP_USART_PL_REG_DMA_STOP 0x40U
#define BSP_USART_PL_REG_VERSION 0x44U

#define BSP_USART_PL_CONTROL_ENABLE 0x00000001U
#define BSP_USART_PL_CONTROL_SOFT_RESET 0x00000002U
#define BSP_USART_PL_CONTROL_LOOPBACK 0x00000004U
#define BSP_USART_PL_STATUS_ENABLED 0x00000001U
#define BSP_USART_PL_STATUS_RX_HALTED 0x00000200U
#define BSP_USART_PL_STATUS_TX_HALTED 0x00000400U

static XUartPs s_uart_instance;       /* PS UART1 驱动实例。 */
static uint8_t s_uart_initialized = 0U; /* UART 初始化完成标志。 */
static uint8_t s_pl_uart_initialized = 0U; /* PL UART DMA initialization state. */
static uint32_t s_pl_rx_consumed = 0U; /* PS-owned RX ring consumer counter. */
static uint32_t s_pl_tx_produced = 0U; /* PS-owned TX ring producer counter. */
static volatile uint32_t s_pl_error_irq_count = 0U; /* Serviced PL error IRQ count. */
static volatile uint32_t s_pl_error_irq_latched = 0U; /* OR of serviced error causes. */
static bsp_usart_pl_config_t s_pl_config = {
    .baud_rate = BSP_USART_BAUD_RATE,
    .data_bits = 8U,
    .parity = BSP_USART_PL_PARITY_NONE,
    .stop_bits = 1U,
    .internal_loopback = 0U,
}; /* Active PL UART configuration used by reset recovery. */

static uint32_t pl_register_read(uint32_t offset)
{
    return Xil_In32((UINTPTR)(BSP_USART_PL_BASE + offset));
}

static void pl_register_write(uint32_t offset, uint32_t value)
{
    Xil_Out32((UINTPTR)(BSP_USART_PL_BASE + offset), value);
}

static void pl_memory_barrier(void)
{
    __asm__ volatile("dmb sy" ::: "memory");
}

static uint32_t pl_baud_increment_get(uint32_t baud_rate)
{
    uint64_t numerator = (uint64_t)baud_rate *
                         BSP_USART_PL_OVERSAMPLE *
                         0x100000000ULL;

    return (uint32_t)((numerator + (BSP_USART_PL_CLOCK_HZ / 2ULL)) /
                      BSP_USART_PL_CLOCK_HZ);
}

static void pl_uart_error_interrupt_handler(void *callback_ref)
{
    uint32_t pending = 0U; /* Error causes active at ISR entry. */

    (void)callback_ref;
    pending = pl_register_read(BSP_USART_PL_REG_IRQ_STATUS) &
              BSP_USART_PL_ERROR_IRQ_MASK;
    if (pending != 0U)
    {
        s_pl_error_irq_count++;
        s_pl_error_irq_latched |= pending;
        pl_register_write(BSP_USART_PL_REG_IRQ_STATUS, pending);
    }
}

int32_t bsp_usart_init(void)
{
    XUartPs_Config *config = NULL; /* PS UART1 的硬件配置描述符。 */
    int32_t status = XST_FAILURE;  /* Xilinx 驱动初始化结果。 */

    config = XUartPs_LookupConfig(XPAR_XUARTPS_0_DEVICE_ID);
    if (config == NULL)
    {
        return XST_FAILURE;
    }

    status = XUartPs_CfgInitialize(&s_uart_instance, config, config->BaseAddress);
    if (status != XST_SUCCESS)
    {
        return status;
    }

    XUartPs_SetOperMode(&s_uart_instance, XUARTPS_OPER_MODE_NORMAL); /* 采用普通全双工 UART 模式。 */
    status = XUartPs_SetBaudRate(&s_uart_instance, BSP_USART_BAUD_RATE);
    if (status != XST_SUCCESS)
    {
        return status;
    }

    s_uart_initialized = 1U;
    return XST_SUCCESS;
}

void bsp_usart_dbg_tx(char *data, int length)
{
    int index = 0; /* 当前待发送字节索引。 */

    if ((data == NULL) || /* 调用方未提供有效发送缓冲区。 */
        (length <= 0))    /* 当前请求不包含可发送字节。 */
    {
        return;
    }

    for (index = 0; index < length; ++index)
    {
        XUartPs_SendByte(XPAR_XUARTPS_0_BASEADDR, (uint8_t)data[index]); /* 等待 FIFO 空间并发送当前字节。 */
    }
}

void bsp_usart_dbg_printf(const char *format, ...)
{
    va_list args;                         /* 调用方传入的格式化参数列表。 */
    char buffer[BSP_USART_PRINTF_SIZE];   /* 固定容量的格式化发送缓冲区。 */
    int length = 0;                       /* 实际生成并允许发送的字符数量。 */

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (length <= 0)
    {
        return;
    }

    if ((uint32_t)length >= (uint32_t)sizeof(buffer))
    {
        length = (int)sizeof(buffer) - 1;
    }

    bsp_usart_dbg_tx(buffer, length);
}

uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *data)
{
    if ((data == NULL) ||                 /* 调用方未提供接收目标地址。 */
        (s_uart_initialized == 0U))       /* UART 尚未完成初始化。 */
    {
        return 0U;
    }

    if (XUartPs_IsReceiveData(XPAR_XUARTPS_0_BASEADDR) == 0U)
    {
        return 0U;
    }

    *data = XUartPs_RecvByte(XPAR_XUARTPS_0_BASEADDR);
    return 1U;
}

int32_t bsp_usart_pl_configure(const bsp_usart_pl_config_t *config)
{
    uint32_t uart_config = 0U; /* Encoded data, parity, and stop fields. */
    uint32_t control = BSP_USART_PL_CONTROL_ENABLE; /* Final enable command. */

    if ((config == NULL) ||
        (config->baud_rate < 1200U) ||
        (config->baud_rate > 1000000U) ||
        (config->data_bits < 5U) ||
        (config->data_bits > 8U) ||
        ((uint32_t)config->parity > (uint32_t)BSP_USART_PL_PARITY_ODD) ||
        ((config->stop_bits != 1U) && (config->stop_bits != 2U)) ||
        (config->internal_loopback > 1U))
    {
        return XST_INVALID_PARAM;
    }

    pl_register_write(BSP_USART_PL_REG_CONTROL, 0U);
    uart_config = (uint32_t)config->data_bits |
                  ((uint32_t)config->parity << 4U);
    if (config->stop_bits == 2U)
    {
        uart_config |= 0x00000040U;
    }
    if (config->internal_loopback != 0U)
    {
        control |= BSP_USART_PL_CONTROL_LOOPBACK;
    }

    pl_register_write(BSP_USART_PL_REG_CONFIG, uart_config);
    pl_register_write(BSP_USART_PL_REG_BAUD_INCREMENT,
                      pl_baud_increment_get(config->baud_rate));
    pl_register_write(BSP_USART_PL_REG_RX_BASE, BSP_USART_PL_RX_RING_BASE);
    pl_register_write(BSP_USART_PL_REG_RX_SIZE, BSP_USART_PL_RING_SIZE);
    pl_register_write(BSP_USART_PL_REG_TX_BASE, BSP_USART_PL_TX_RING_BASE);
    pl_register_write(BSP_USART_PL_REG_TX_SIZE, BSP_USART_PL_RING_SIZE);
    pl_register_write(BSP_USART_PL_REG_IRQ_STATUS, BSP_USART_PL_ERROR_IRQ_MASK);
    pl_register_write(BSP_USART_PL_REG_IRQ_ENABLE, BSP_USART_PL_ERROR_IRQ_MASK);
    pl_register_write(BSP_USART_PL_REG_CONTROL, control);

    if ((pl_register_read(BSP_USART_PL_REG_STATUS) &
         BSP_USART_PL_STATUS_ENABLED) == 0U)
    {
        return XST_FAILURE;
    }

    s_pl_config = *config;
    return XST_SUCCESS;
}

int32_t bsp_usart_pl_reset(void)
{
    int32_t status = XST_FAILURE; /* Reconfiguration result. */

    pl_register_write(BSP_USART_PL_REG_CONTROL,
                      BSP_USART_PL_CONTROL_SOFT_RESET);
    pl_memory_barrier();
    s_pl_rx_consumed = 0U;
    s_pl_tx_produced = 0U;
    s_pl_error_irq_count = 0U;
    s_pl_error_irq_latched = 0U;
    status = bsp_usart_pl_configure(&s_pl_config);

    return status;
}

int32_t bsp_usart_pl_init(void)
{
    int32_t status = XST_FAILURE; /* BSP or hardware initialization result. */

    Xil_SetTlbAttributes((INTPTR)BSP_USART_PL_DMA_WINDOW_BASE, NORM_NONCACHE);
    pl_memory_barrier();

    if (pl_register_read(BSP_USART_PL_REG_VERSION) !=
        BSP_USART_PL_EXPECTED_VERSION)
    {
        return XST_FAILURE;
    }

    status = bsp_interrupt_connect(BSP_USART_PL_IRQ_ID,
                                   (Xil_ExceptionHandler)pl_uart_error_interrupt_handler,
                                   NULL);
    if (status != XST_SUCCESS)
    {
        return status;
    }

    s_pl_uart_initialized = 1U;
    status = bsp_usart_pl_reset();
    if (status != XST_SUCCESS)
    {
        s_pl_uart_initialized = 0U;
        return status;
    }

    bsp_interrupt_enable(BSP_USART_PL_IRQ_ID);
    return XST_SUCCESS;
}

int32_t bsp_usart_pl_tx_dma(const uint8_t *data, uint32_t length)
{
    volatile uint8_t *tx_ring =
        (volatile uint8_t *)(UINTPTR)BSP_USART_PL_TX_RING_BASE;
    uint32_t sent = 0U; /* Bytes copied and published to the TX ring. */

    if ((data == NULL) && (length != 0U))
    {
        return XST_INVALID_PARAM;
    }
    if (s_pl_uart_initialized == 0U)
    {
        return XST_FAILURE;
    }

    while (sent < length)
    {
        uint32_t tx_consumed =
            pl_register_read(BSP_USART_PL_REG_TX_CONSUMED);
        uint32_t used = s_pl_tx_produced - tx_consumed;
        uint32_t available = 0U;

        if (used < BSP_USART_PL_RING_SIZE)
        {
            available = BSP_USART_PL_RING_SIZE - used;
        }
        if (available == 0U)
        {
            uint32_t status = pl_register_read(BSP_USART_PL_REG_STATUS);

            if (((status & BSP_USART_PL_STATUS_ENABLED) == 0U) ||
                ((status & BSP_USART_PL_STATUS_TX_HALTED) != 0U))
            {
                return XST_FAILURE;
            }
            continue;
        }

        while ((available != 0U) && (sent < length))
        {
            tx_ring[s_pl_tx_produced & (BSP_USART_PL_RING_SIZE - 1U)] =
                data[sent];
            s_pl_tx_produced++;
            sent++;
            available--;
        }

        pl_memory_barrier();
        pl_register_write(BSP_USART_PL_REG_TX_PRODUCED, s_pl_tx_produced);
    }

    return XST_SUCCESS;
}

void bsp_usart_pl_printf(const char *format, ...)
{
    va_list args;                         /* Caller formatting arguments. */
    char buffer[BSP_USART_PRINTF_SIZE];   /* Bounded PL UART output buffer. */
    int length = 0;                       /* Number of bytes selected to send. */

    if (format == NULL)
    {
        return;
    }

    va_start(args, format);
    length = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    if (length <= 0)
    {
        return;
    }
    if ((uint32_t)length >= (uint32_t)sizeof(buffer))
    {
        length = (int)sizeof(buffer) - 1;
    }

    (void)bsp_usart_pl_tx_dma((const uint8_t *)buffer, (uint32_t)length);
}

uint8_t bsp_usart_pl_rx_get_byte(uint8_t *data)
{
    volatile const uint8_t *rx_ring =
        (volatile const uint8_t *)(UINTPTR)BSP_USART_PL_RX_RING_BASE;
    uint32_t rx_produced = 0U; /* Hardware-owned RX producer snapshot. */

    if ((data == NULL) || (s_pl_uart_initialized == 0U))
    {
        return 0U;
    }

    rx_produced = pl_register_read(BSP_USART_PL_REG_RX_PRODUCED);
    if (rx_produced == s_pl_rx_consumed)
    {
        return 0U;
    }

    pl_memory_barrier();
    *data = rx_ring[s_pl_rx_consumed & (BSP_USART_PL_RING_SIZE - 1U)];
    s_pl_rx_consumed++;
    pl_memory_barrier();
    pl_register_write(BSP_USART_PL_REG_RX_CONSUMED, s_pl_rx_consumed);

    return 1U;
}

void bsp_usart_pl_status_get(bsp_usart_pl_status_t *status)
{
    if (status == NULL)
    {
        return;
    }

    status->version = pl_register_read(BSP_USART_PL_REG_VERSION);
    status->status = pl_register_read(BSP_USART_PL_REG_STATUS);
    status->irq_status = pl_register_read(BSP_USART_PL_REG_IRQ_STATUS);
    status->uart_errors = pl_register_read(BSP_USART_PL_REG_UART_ERRORS);
    status->dma_errors = pl_register_read(BSP_USART_PL_REG_DMA_ERRORS);
    status->dma_stop_reason = pl_register_read(BSP_USART_PL_REG_DMA_STOP);
    status->rx_produced = pl_register_read(BSP_USART_PL_REG_RX_PRODUCED);
    status->rx_consumed = pl_register_read(BSP_USART_PL_REG_RX_CONSUMED);
    status->tx_produced = pl_register_read(BSP_USART_PL_REG_TX_PRODUCED);
    status->tx_consumed = pl_register_read(BSP_USART_PL_REG_TX_CONSUMED);
    status->error_irq_count = s_pl_error_irq_count;
    status->error_irq_latched = s_pl_error_irq_latched;
}

void bsp_usart_pl_error_clear(void)
{
    uint32_t pending = pl_register_read(BSP_USART_PL_REG_IRQ_STATUS);

    pl_register_write(BSP_USART_PL_REG_IRQ_STATUS, pending);
    s_pl_error_irq_count = 0U;
    s_pl_error_irq_latched = 0U;
}
