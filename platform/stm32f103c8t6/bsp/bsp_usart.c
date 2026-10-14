// SPDX-License-Identifier: MIT
/**
 * @file bsp_usart.c
 * @brief PA9 TX / PA10 RX USART1 driver.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Initialize 115200 8N1 without USART1 remapping
 *          - Queue whole TX messages for DMA1 channel 4 and receive bytes in the USART1 ISR
 *          - Count overflow and hardware errors instead of silently overwriting queued bytes
 *          Design notes:
 *          - C11 compatible; no dynamic allocation in platform drivers
 *          - Hardware access belongs to the BSP; framework bindings belong to src
 * @author Max.Li
 * @date 2026-10-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */
#include "bsp_usart.h"
#include "stm32f10x.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#define TX_SIZE 1024u            /* Bounded TX storage, one slot unused. */
#define RX_SIZE 512u             /* Bounded RX storage, one slot unused. */
static uint8_t tx_ring[TX_SIZE]; /* Task producers; DMA consumer. */
static uint8_t rx_ring[RX_SIZE]; /* USART ISR producer; Link task consumer. */
static volatile uint16_t tx_head                 = 0u; /* Next queued transmit position. */
static volatile uint16_t tx_tail                 = 0u; /* DMA-owned first transmit position. */
static volatile uint16_t tx_inflight             = 0u; /* Current contiguous DMA chunk length. */
static volatile uint16_t rx_head                 = 0u; /* Next receive slot written by ISR. */
static volatile uint16_t rx_tail                 = 0u; /* Next receive slot consumed by task. */
volatile uint32_t g_bsp_usart_dbg_tx_drop_count  = 0u; /* Rejected messages. */
volatile uint32_t g_bsp_usart_dbg_rx_drop_count  = 0u; /* Queue-full byte drops. */
volatile uint32_t g_bsp_usart_dbg_rx_error_count = 0u; /* Hardware error events. */

/** @brief Start a contiguous TX chunk; caller holds PRIMASK. */
static void tx_kick(void)
{
    uint16_t count = (uint16_t)(((uint32_t)tx_head + TX_SIZE - tx_tail) % TX_SIZE); /* Queued bytes. */
    uint16_t part = (uint16_t)(TX_SIZE - tx_tail); /* Bytes before the ring wraps. */

    if (    (tx_inflight != 0u) /* DMA already owns a chunk. */
         || (count == 0u))      /* There is no work to submit. */
    {
        return;
    }

    if (part > count)
    {
        part = count;
    }
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA_ClearFlag(DMA1_FLAG_GL4);
    DMA1_Channel4->CMAR = (uint32_t)(uintptr_t)&tx_ring[tx_tail];
    DMA_SetCurrDataCounter(DMA1_Channel4, part);
    tx_inflight = part;
    __DMB();
    DMA_Cmd(DMA1_Channel4, ENABLE);
}
void bsp_usart_init(void)
{
    GPIO_InitTypeDef gpio   = {0}; /* USART1 pin modes. */
    USART_InitTypeDef usart = {0}; /* 115200 8N1 bidirectional transport. */
    DMA_InitTypeDef dma     = {0}; /* Memory-incrementing USART1 TX transfer. */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_USART1, DISABLE);
    gpio.GPIO_Pin   = GPIO_Pin_9;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin  = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &gpio);
    USART_StructInit(&usart);
    usart.USART_BaudRate = 115200u;
    USART_Init(USART1, &usart);
    DMA_DeInit(DMA1_Channel4);
    dma.DMA_PeripheralBaseAddr = (uint32_t)(uintptr_t)&USART1->DR;
    dma.DMA_DIR                = DMA_DIR_PeripheralDST;
    dma.DMA_MemoryInc          = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize     = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode               = DMA_Mode_Normal;
    dma.DMA_Priority           = DMA_Priority_Medium;
    dma.DMA_M2M                = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &dma);
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC | DMA_IT_TE, ENABLE);
    NVIC_SetPriority(DMA1_Channel4_IRQn, 3u);
    NVIC_SetPriority(USART1_IRQn, 1u);
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    NVIC_EnableIRQ(USART1_IRQn);
    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);
}
void bsp_usart_dbg_tx(char *p_data, int length)
{
    uint32_t primask    = __get_PRIMASK(); /* Caller interrupt state restored after queue mutation. */
    uint32_t free_count = 0u; /* Slots available for one complete message. */
    int index           = 0;  /* Message offset. */

    if (    (p_data == NULL) /* No caller storage. */
         || (length <= 0))   /* No bytes to queue. */
    {
        return;
    }
    __disable_irq();
    free_count = ((uint32_t)tx_tail + TX_SIZE - tx_head - 1u) % TX_SIZE;

    if ((uint32_t)length > free_count)
    {
        g_bsp_usart_dbg_tx_drop_count++;
        __set_PRIMASK(primask);
        return;
    }

    for (index = 0; index < length; index++)
    {
        tx_ring[tx_head] = (uint8_t)p_data[index];
        tx_head = (uint16_t)(((uint32_t)tx_head + 1u) % TX_SIZE);
    }
    tx_kick();
    __set_PRIMASK(primask);
}
void bsp_usart_dbg_printf(const char *p_format, ...)
{
    char buffer[256] = {0}; /* Per-call buffer safe across task preemption. */
    va_list args;           /* Arguments consumed before asynchronous transport. */
    int length = 0;         /* Formatter's required byte count. */

    if (p_format == NULL)
    {
        return;
    }
    va_start(args, p_format);
    length = vsnprintf(buffer, sizeof(buffer), p_format, args);
    va_end(args);

    if (    (length < 0) /* Formatting failed. */
         || ((size_t)length >= sizeof(buffer))) /* Do not transmit truncated diagnostics. */
    {
        g_bsp_usart_dbg_tx_drop_count++;
        return;
    }
    bsp_usart_dbg_tx(buffer, length);
}
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data)
{
    if (    (p_data == NULL)      /* The caller must provide storage. */
         || (rx_head == rx_tail)) /* No published receive byte. */
    {
        return 0u;
    }
    *p_data = rx_ring[rx_tail];
    __DMB();
    rx_tail = (uint16_t)(((uint32_t)rx_tail + 1u) % RX_SIZE);
    return 1u;
}
void bsp_usart_dbg_irq_handler(void)
{
    uint32_t status = USART1->SR; /* Capture SR before DR clears receive/error conditions. */
    uint16_t next = (uint16_t)(((uint32_t)rx_head + 1u) % RX_SIZE); /* Candidate RX slot. */
    uint8_t byte = (uint8_t)(USART1->DR & 0xFFu); /* SR/DR sequence clears ORE/FE/NE/PE. */

    if ((status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) != 0u)
    {
        g_bsp_usart_dbg_rx_error_count++;
    }

    if ((status & USART_SR_RXNE) == 0u)
    {
        return;
    }

    if (next == rx_tail)
    {
        g_bsp_usart_dbg_rx_drop_count++;
        return;
    }
    rx_ring[rx_head] = byte;
    __DMB();
    rx_head = next;
}
void bsp_usart_dbg_tx_irq_handler(void)
{
    uint32_t primask = __get_PRIMASK(); /* Serialize DMA completion with task TX submissions. */
    __disable_irq();

    if (DMA_GetITStatus(DMA1_IT_TE4) != RESET)
    {
        DMA_Cmd(DMA1_Channel4, DISABLE);
        DMA_ClearITPendingBit(DMA1_IT_GL4);
        tx_tail = (uint16_t)(((uint32_t)tx_tail + tx_inflight) % TX_SIZE);
        tx_inflight = 0u;
        g_bsp_usart_dbg_tx_drop_count++;
        tx_kick();
    }
    else if (DMA_GetITStatus(DMA1_IT_TC4) != RESET)
    {
        DMA_Cmd(DMA1_Channel4, DISABLE);
        DMA_ClearITPendingBit(DMA1_IT_GL4);
        tx_tail = (uint16_t)(((uint32_t)tx_tail + tx_inflight) % TX_SIZE);
        tx_inflight = 0u;
        tx_kick();
    }
    __set_PRIMASK(primask);
}
