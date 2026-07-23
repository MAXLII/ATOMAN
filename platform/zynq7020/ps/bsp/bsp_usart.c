// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.c
 * @brief   Zynq-7020 PS UART1 communication implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure the board CH340 link through PS UART1 at 921600 8N1
 *          - Poll received bytes for the shared section communication link
 *          - Serialize blocking transmit and bounded printf traffic
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The first platform version uses polling instead of DMA
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

static XUartPs s_uart_instance;       /* PS UART1 驱动实例。 */
static uint8_t s_uart_initialized = 0U; /* UART 初始化完成标志。 */

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
