// SPDX-License-Identifier: MIT
/**
 * @file    boot_uart.c
 * @brief   Minimal HC32F334 bootloader USART2 implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure PC10 TX and PC4 RX for USART2
 *          - Send FRAME responses and poll received bytes without application buffers
 *          - Expose hardware-complete TX status before reset or jump
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Foreground polling only; no UART ISR ownership
 *          - A bounded wait prevents a failed USART from trapping the bootloader
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "boot_uart.h"

#include "hc32_ll.h"

#include <stddef.h>
#define HC32_BOOT_UART_UNIT       CM_USART2
#define HC32_BOOT_UART_TX_PORT    GPIO_PORT_C
#define HC32_BOOT_UART_TX_PIN     GPIO_PIN_10
#define HC32_BOOT_UART_RX_PORT    GPIO_PORT_C
#define HC32_BOOT_UART_RX_PIN     GPIO_PIN_04
#define HC32_BOOT_UART_FUNC       GPIO_FUNC_36
#define HC32_BOOT_UART_TIMEOUT    1000000UL
#define HC32_BOOT_UART_CR2_RESET  0x0600UL
#ifndef HC32_BOOT_UART_BRR
#define HC32_BOOT_UART_BRR        0x017CUL
#endif

int32_t hc32_boot_uart_init(void)
{
    stc_gpio_init_t gpio_init;
    GPIO_REG_Unlock();
    (void)GPIO_StructInit(&gpio_init);
    gpio_init.u16PinDir = PIN_DIR_OUT;
    gpio_init.u16PinDrv = PIN_HIGH_DRV;
    gpio_init.u16PinOutputType = PIN_OUT_TYPE_CMOS;
    gpio_init.u16PinState = PIN_STAT_SET;
    (void)GPIO_Init(HC32_BOOT_UART_TX_PORT, HC32_BOOT_UART_TX_PIN, &gpio_init);

    (void)GPIO_StructInit(&gpio_init);
    gpio_init.u16PinDir = PIN_DIR_IN;
    gpio_init.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(HC32_BOOT_UART_RX_PORT, HC32_BOOT_UART_RX_PIN, &gpio_init);
    GPIO_SetFunc(HC32_BOOT_UART_TX_PORT, HC32_BOOT_UART_TX_PIN, HC32_BOOT_UART_FUNC);
    GPIO_SetFunc(HC32_BOOT_UART_RX_PORT, HC32_BOOT_UART_RX_PIN, HC32_BOOT_UART_FUNC);
    GPIO_OutputCmd(HC32_BOOT_UART_TX_PORT, HC32_BOOT_UART_TX_PIN, ENABLE);
    GPIO_OutputCmd(HC32_BOOT_UART_RX_PORT, HC32_BOOT_UART_RX_PIN, DISABLE);
    GPIO_REG_Lock();

    FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_USART2, ENABLE);
    WRITE_REG32(HC32_BOOT_UART_UNIT->CR1, USART_OVER_SAMPLE_8BIT | USART_CR1_FBME);
    WRITE_REG32(HC32_BOOT_UART_UNIT->CR2, HC32_BOOT_UART_CR2_RESET);
    WRITE_REG32(HC32_BOOT_UART_UNIT->CR3, USART_HW_FLOWCTRL_NONE);
    WRITE_REG32(HC32_BOOT_UART_UNIT->PR, USART_CLK_DIV4);
    WRITE_REG32(HC32_BOOT_UART_UNIT->BRR, HC32_BOOT_UART_BRR);
    USART_FuncCmd(HC32_BOOT_UART_UNIT, USART_RX | USART_TX, ENABLE);
    return LL_OK;
}

uint8_t hc32_boot_uart_rx_get_byte(uint8_t *p_data)
{
    if ((NULL == p_data) || (SET != USART_GetStatus(HC32_BOOT_UART_UNIT, USART_FLAG_RX_FULL)))
    {
        return 0U;
    }
    *p_data = (uint8_t)USART_ReadData(HC32_BOOT_UART_UNIT);
    return 1U;
}

void hc32_boot_uart_tx(char *p_data, int length)
{
    int index;

    if ((NULL == p_data) || (length <= 0))
    {
        return;
    }
    for (index = 0; index < length; index++)
    {
        uint32_t timeout = HC32_BOOT_UART_TIMEOUT;
        while (SET != USART_GetStatus(HC32_BOOT_UART_UNIT, USART_FLAG_TX_EMPTY))
        {
            if (0UL == timeout)
            {
                return;
            }
            timeout--;
        }
        USART_WriteData(HC32_BOOT_UART_UNIT, (uint16_t)(uint8_t)p_data[index]);
    }
}

void hc32_boot_uart_printf(const char *p_format, ...)
{
    (void)p_format;
}

uint8_t hc32_boot_uart_tx_is_idle(void)
{
    return (SET == USART_GetStatus(HC32_BOOT_UART_UNIT, USART_FLAG_TX_CPLT)) ? 1U : 0U;
}
