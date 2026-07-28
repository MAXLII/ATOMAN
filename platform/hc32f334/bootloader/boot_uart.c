// SPDX-License-Identifier: MIT
/**
 * @file    boot_uart.c
 * @brief   Minimal HC32F334 bootloader USART2 implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure PC10 TX and PC4 RX for USART2
 *          - Buffer USART2 receive bytes with circular DMA for the FRAME parser
 *          - Send FRAME responses from the foreground communication path
 *          - Expose hardware-complete TX status before reset or jump
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - DMA channel 0 is dedicated to USART2 RX; W25Q uses channels 4 and 5
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
#define HC32_BOOT_UART_UNIT CM_USART2
#define HC32_BOOT_UART_TX_PORT GPIO_PORT_C
#define HC32_BOOT_UART_TX_PIN GPIO_PIN_10
#define HC32_BOOT_UART_RX_PORT GPIO_PORT_C
#define HC32_BOOT_UART_RX_PIN GPIO_PIN_04
#define HC32_BOOT_UART_TX_FUNC GPIO_FUNC_36
#define HC32_BOOT_UART_RX_FUNC GPIO_FUNC_37
#define HC32_BOOT_UART_TIMEOUT 1000000UL
#define HC32_BOOT_UART_RX_BUFFER_SIZE 1023U
#define HC32_BOOT_UART_RX_DMA_CH DMA_CH0
#define HC32_BOOT_UART_RX_DMA_MX_CH DMA_MX_CH0
#define HC32_BOOT_UART_RX_DMA_TRIGGER AOS_DMA_0
#define HC32_BOOT_UART_RX_DMA_TC_FLAG DMA_FLAG_TC_CH0
#define HC32_BOOT_UART_RX_DMA_RESTART_THRESHOLD 2048UL
#define HC32_BOOT_UART_CR2_RESET 0x0600UL
#ifndef HC32_BOOT_UART_BRR
#define HC32_BOOT_UART_BRR 0x017CUL
#endif

static uint8_t s_rx_buffer[HC32_BOOT_UART_RX_BUFFER_SIZE] = {0};
static uint16_t s_rx_tail = 0U;

static void hc32_boot_uart_rx_dma_init(void)
{
    stc_dma_init_t dma_init;
    stc_dma_repeat_init_t repeat_init;

    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS | FCG0_PERIPH_DMA, ENABLE);
    DMA_Cmd(CM_DMA, ENABLE);
    DMA_MxChCmd(CM_DMA, HC32_BOOT_UART_RX_DMA_MX_CH, DISABLE);
    (void)DMA_ChCmd(CM_DMA, HC32_BOOT_UART_RX_DMA_CH, DISABLE);
    DMA_ClearTransCompleteStatus(CM_DMA, HC32_BOOT_UART_RX_DMA_TC_FLAG);

    (void)DMA_StructInit(&dma_init);
    dma_init.u32IntEn = DMA_INT_DISABLE;
    dma_init.u32SrcAddr = (uint32_t)&HC32_BOOT_UART_UNIT->RDR;
    dma_init.u32DestAddr = (uint32_t)&s_rx_buffer[0];
    dma_init.u32DataWidth = DMA_DATAWIDTH_8BIT;
    dma_init.u32BlockSize = 1UL;
    dma_init.u32TransCount = UINT16_MAX;
    dma_init.u32SrcAddrInc = DMA_SRC_ADDR_FIX;
    dma_init.u32DestAddrInc = DMA_DEST_ADDR_INC;
    (void)DMA_Init(CM_DMA, HC32_BOOT_UART_RX_DMA_CH, &dma_init);

    (void)DMA_RepeatStructInit(&repeat_init);
    repeat_init.u32Mode = DMA_RPT_DEST;
    repeat_init.u32DestCount = HC32_BOOT_UART_RX_BUFFER_SIZE;
    repeat_init.u32SrcCount = 0UL;
    (void)DMA_RepeatInit(CM_DMA, HC32_BOOT_UART_RX_DMA_CH, &repeat_init);

    AOS_SetTriggerEventSrc(HC32_BOOT_UART_RX_DMA_TRIGGER, EVT_SRC_USART2_RI);
    DMA_MxChCmd(CM_DMA, HC32_BOOT_UART_RX_DMA_MX_CH, ENABLE);
    (void)DMA_ChCmd(CM_DMA, HC32_BOOT_UART_RX_DMA_CH, ENABLE);
    s_rx_tail = 0U;
}

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
    GPIO_SetFunc(HC32_BOOT_UART_TX_PORT, HC32_BOOT_UART_TX_PIN, HC32_BOOT_UART_TX_FUNC);
    GPIO_SetFunc(HC32_BOOT_UART_RX_PORT, HC32_BOOT_UART_RX_PIN, HC32_BOOT_UART_RX_FUNC);
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
    hc32_boot_uart_rx_dma_init();
    return LL_OK;
}

uint8_t hc32_boot_uart_rx_get_byte(uint8_t *p_data)
{
    uint32_t write_address;
    uint16_t write_index;

    if (NULL == p_data)
    {
        return 0U;
    }
    write_address = DMA_GetDestAddr(CM_DMA, HC32_BOOT_UART_RX_DMA_CH);
    if ((write_address < (uint32_t)&s_rx_buffer[0]) ||
        (write_address > ((uint32_t)&s_rx_buffer[HC32_BOOT_UART_RX_BUFFER_SIZE - 1U] + 1UL)))
    {
        return 0U;
    }
    write_index = (uint16_t)(write_address - (uint32_t)&s_rx_buffer[0]);
    if (write_index == HC32_BOOT_UART_RX_BUFFER_SIZE)
    {
        write_index = 0U;
    }
    if (s_rx_tail == write_index)
    {
        if ((DMA_GetTransCount(CM_DMA, HC32_BOOT_UART_RX_DMA_CH) <
             HC32_BOOT_UART_RX_DMA_RESTART_THRESHOLD) ||
            (SET == DMA_GetTransCompleteStatus(CM_DMA, HC32_BOOT_UART_RX_DMA_TC_FLAG)))
        {
            hc32_boot_uart_rx_dma_init();
        }
        return 0U;
    }
    *p_data = s_rx_buffer[s_rx_tail];
    s_rx_tail++;
    if (s_rx_tail == HC32_BOOT_UART_RX_BUFFER_SIZE)
    {
        s_rx_tail = 0U;
    }
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
