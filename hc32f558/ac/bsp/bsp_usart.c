// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.c
 * @brief   HC32F558 USART DMA BSP module.
 * @details
 *          This file is part of the HC32F558 project.
 *
 *          Module responsibilities:
 *          - Configure USART1 GPIO alternate functions on PB6/PB7
 *          - Initialize USART1 with DMA1 channel based TX/RX paths
 *          - Provide debug printf, DMA transmit, and DMA ring-buffer receive helpers
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - RX DMA runs continuously; TX DMA is started per transfer
 *          - Hardware access is abstracted through the HC32 LL driver
 *
 * @author  Max.Li
 * @date    2026-06-06
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_usart.h"

#include <stdarg.h>
#include <stdio.h>

#define BSP_USART_UNIT                  (CM_USART1)
#define BSP_USART_FCG                   (FCG3_PERIPH_USART1)

#define BSP_USART_RX_PORT               (GPIO_PORT_B)
#define BSP_USART_RX_PIN                (GPIO_PIN_06)
#define BSP_USART_RX_FUNC               (GPIO_FUNC_33)

#define BSP_USART_TX_PORT               (GPIO_PORT_B)
#define BSP_USART_TX_PIN                (GPIO_PIN_07)
#define BSP_USART_TX_FUNC               (GPIO_FUNC_32)

#define BSP_USART_BAUDRATE              (115200UL)
#define BSP_USART_PRINTF_BUF_SIZE       (256U)

#define BSP_USART_DMA_UNIT              (CM_DMA1)
#define BSP_USART_DMA_FCG               (FCG0_PERIPH_DMA1)
#define BSP_USART_RX_DMA_CH             (DMA_CH0)
#define BSP_USART_RX_DMA_TRIG_SEL       (AOS_DMA1_0)
#define BSP_USART_RX_DMA_TRIG_EVT       (EVT_SRC_USART1_RI)
#define BSP_USART_TX_DMA_CH             (DMA_CH1)
#define BSP_USART_TX_DMA_TRIG_SEL       (AOS_DMA1_1)
#define BSP_USART_TX_DMA_TRIG_EVT       (EVT_SRC_USART1_TI)

#define BSP_USART_RX_BUF_SIZE           (256U)
#define BSP_USART_RX_HALF_SIZE          (BSP_USART_RX_BUF_SIZE / 2U)
#define BSP_USART_DMA_BLOCK_SIZE        (1UL)
#define BSP_USART_TX_DMA_MAX_LEN        (0xFFFFUL)

static uint8_t s_usart_rx_buf[BSP_USART_RX_BUF_SIZE];
static stc_dma_llp_descriptor_t s_usart_rx_llp[2];
static uint16_t s_usart_rx_read_index;
static uint16_t s_usart_rx_write_index;

static void bsp_usart_gpio_init(void)
{
    LL_PERIPH_WE(LL_PERIPH_GPIO);
    GPIO_SetFunc(BSP_USART_RX_PORT, BSP_USART_RX_PIN, BSP_USART_RX_FUNC);
    GPIO_SetFunc(BSP_USART_TX_PORT, BSP_USART_TX_PIN, BSP_USART_TX_FUNC);
    LL_PERIPH_WP(LL_PERIPH_GPIO);
}

static void bsp_usart_clear_rx_error(void)
{
    uint16_t dummy;
    const uint32_t err_flags = USART_FLAG_OVERRUN | USART_FLAG_FRAME_ERR | USART_FLAG_PARITY_ERR;

    if (RESET != USART_GetStatus(BSP_USART_UNIT, err_flags)) {
        dummy = USART_ReadData(BSP_USART_UNIT);
        (void)dummy;
        USART_ClearStatus(BSP_USART_UNIT, err_flags);
    }
}

static void bsp_usart_rx_dma_init(void)
{
    stc_dma_init_t dma_init;
    stc_dma_llp_init_t llp_init;

    (void)DMA_ChCmd(BSP_USART_DMA_UNIT, BSP_USART_RX_DMA_CH, DISABLE);
    (void)DMA_StructInit(&dma_init);
    dma_init.u32IntEn = DMA_INT_DISABLE;
    dma_init.u32SrcAddr = (uint32_t)&BSP_USART_UNIT->RDR;
    dma_init.u32DestAddr = (uint32_t)&s_usart_rx_buf[0];
    dma_init.u32DataWidth = DMA_DATAWIDTH_8BIT;
    dma_init.u32BlockSize = BSP_USART_DMA_BLOCK_SIZE;
    dma_init.u32TransCount = BSP_USART_RX_HALF_SIZE;
    dma_init.u32SrcAddrInc = DMA_SRC_ADDR_FIX;
    dma_init.u32DestAddrInc = DMA_DEST_ADDR_INC;
    (void)DMA_Init(BSP_USART_DMA_UNIT, BSP_USART_RX_DMA_CH, &dma_init);

    (void)DMA_LlpStructInit(&llp_init);
    llp_init.u32State = DMA_LLP_ENABLE;
    llp_init.u32Mode = DMA_LLP_WAIT;
    llp_init.u32Addr = (uint32_t)&s_usart_rx_llp[0];
    (void)DMA_LlpInit(BSP_USART_DMA_UNIT, BSP_USART_RX_DMA_CH, &llp_init);

    s_usart_rx_llp[0].SARx = (uint32_t)&BSP_USART_UNIT->RDR;
    s_usart_rx_llp[0].DARx = (uint32_t)&s_usart_rx_buf[BSP_USART_RX_HALF_SIZE];
    s_usart_rx_llp[0].DTCTLx = ((uint32_t)BSP_USART_RX_HALF_SIZE << DMA_DTCTL_CNT_POS) |
                               (BSP_USART_DMA_BLOCK_SIZE << DMA_DTCTL_BLKSIZE_POS);
    s_usart_rx_llp[0].RPTx = 0UL;
    s_usart_rx_llp[0].SNSEQCTLx = 0UL;
    s_usart_rx_llp[0].DNSEQCTLx = 0UL;
    s_usart_rx_llp[0].LLPx = (uint32_t)&s_usart_rx_llp[1];
    s_usart_rx_llp[0].CHCTLx = DMA_SRC_ADDR_FIX | DMA_DEST_ADDR_INC | DMA_DATAWIDTH_8BIT |
                               DMA_LLP_ENABLE | DMA_LLP_WAIT;

    s_usart_rx_llp[1].SARx = (uint32_t)&BSP_USART_UNIT->RDR;
    s_usart_rx_llp[1].DARx = (uint32_t)&s_usart_rx_buf[0];
    s_usart_rx_llp[1].DTCTLx = s_usart_rx_llp[0].DTCTLx;
    s_usart_rx_llp[1].RPTx = 0UL;
    s_usart_rx_llp[1].SNSEQCTLx = 0UL;
    s_usart_rx_llp[1].DNSEQCTLx = 0UL;
    s_usart_rx_llp[1].LLPx = (uint32_t)&s_usart_rx_llp[0];
    s_usart_rx_llp[1].CHCTLx = s_usart_rx_llp[0].CHCTLx;

    s_usart_rx_read_index = 0U;
    s_usart_rx_write_index = 0U;

    AOS_SetTriggerEventSrc(BSP_USART_RX_DMA_TRIG_SEL, BSP_USART_RX_DMA_TRIG_EVT);
    (void)DMA_ChCmd(BSP_USART_DMA_UNIT, BSP_USART_RX_DMA_CH, ENABLE);
}

static void bsp_usart_tx_dma_init(void)
{
    stc_dma_init_t dma_init;

    (void)DMA_ChCmd(BSP_USART_DMA_UNIT, BSP_USART_TX_DMA_CH, DISABLE);
    (void)DMA_StructInit(&dma_init);
    dma_init.u32IntEn = DMA_INT_DISABLE;
    dma_init.u32SrcAddr = 0UL;
    dma_init.u32DestAddr = (uint32_t)&BSP_USART_UNIT->TDR;
    dma_init.u32DataWidth = DMA_DATAWIDTH_8BIT;
    dma_init.u32BlockSize = BSP_USART_DMA_BLOCK_SIZE;
    dma_init.u32TransCount = 0UL;
    dma_init.u32SrcAddrInc = DMA_SRC_ADDR_INC;
    dma_init.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    (void)DMA_Init(BSP_USART_DMA_UNIT, BSP_USART_TX_DMA_CH, &dma_init);

    AOS_SetTriggerEventSrc(BSP_USART_TX_DMA_TRIG_SEL, BSP_USART_TX_DMA_TRIG_EVT);
}

static void bsp_usart_dma_init(void)
{
    LL_PERIPH_WE(LL_PERIPH_FCG);
    FCG_Fcg0PeriphClockCmd(BSP_USART_DMA_FCG | FCG0_PERIPH_AOS, ENABLE);
    LL_PERIPH_WP(LL_PERIPH_FCG);

    DMA_Cmd(BSP_USART_DMA_UNIT, ENABLE);
    bsp_usart_rx_dma_init();
    bsp_usart_tx_dma_init();
}

static void bsp_usart_rx_dma_update_write_index(void)
{
    uint32_t dest_addr;
    const uint32_t base_addr = (uint32_t)&s_usart_rx_buf[0];

    dest_addr = DMA_GetDestAddr(BSP_USART_DMA_UNIT, BSP_USART_RX_DMA_CH);
    if ((dest_addr >= base_addr) && (dest_addr < (base_addr + BSP_USART_RX_BUF_SIZE))) {
        s_usart_rx_write_index = (uint16_t)(dest_addr - base_addr);
    } else {
        s_usart_rx_write_index = 0U;
    }
}

void bsp_usart_init(void)
{
    stc_usart_uart_init_t uart_init;

    bsp_usart_gpio_init();

    LL_PERIPH_WE(LL_PERIPH_FCG);
    FCG_Fcg3PeriphClockCmd(BSP_USART_FCG, ENABLE);
    LL_PERIPH_WP(LL_PERIPH_FCG);

    (void)USART_UART_StructInit(&uart_init);
    uart_init.u32Baudrate = BSP_USART_BAUDRATE;
    uart_init.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    uart_init.u32HWFlowControl = USART_HW_FLOWCTRL_NONE;

    (void)USART_UART_Init(BSP_USART_UNIT, &uart_init, NULL);
    USART_ClearStatus(BSP_USART_UNIT, USART_FLAG_TX_END);

    bsp_usart_dma_init();
    USART_FuncCmd(BSP_USART_UNIT, USART_RX, ENABLE);
}

void bsp_usart_dbg_tx(const char *ptr, int len)
{
    if ((NULL == ptr) || (len <= 0)) {
        return;
    }

    (void)bsp_usart_dbg_tx_dma((const uint8_t *)ptr, (uint32_t)len);
}

uint32_t bsp_usart_dbg_tx_dma(const uint8_t *data, uint32_t len)
{
    uint32_t send_len = len;
    const uint32_t tx_complete_flag = DMA_FLAG_TC_CH0 << BSP_USART_TX_DMA_CH;

    if ((NULL == data) || (0UL == len)) {
        return 0UL;
    }
    if (send_len > BSP_USART_TX_DMA_MAX_LEN) {
        send_len = BSP_USART_TX_DMA_MAX_LEN;
    }

    while (RESET == USART_GetStatus(BSP_USART_UNIT, USART_FLAG_TX_CPLT)) {
    }

    (void)DMA_ChCmd(BSP_USART_DMA_UNIT, BSP_USART_TX_DMA_CH, DISABLE);
    DMA_ClearTransCompleteStatus(BSP_USART_DMA_UNIT, tx_complete_flag);
    (void)DMA_SetSrcAddr(BSP_USART_DMA_UNIT, BSP_USART_TX_DMA_CH, (uint32_t)data);
    (void)DMA_SetTransCount(BSP_USART_DMA_UNIT, BSP_USART_TX_DMA_CH, send_len);
    (void)DMA_ChCmd(BSP_USART_DMA_UNIT, BSP_USART_TX_DMA_CH, ENABLE);

    USART_ClearStatus(BSP_USART_UNIT, USART_FLAG_TX_CPLT);
    USART_FuncCmd(BSP_USART_UNIT, USART_TX, DISABLE);
    USART_FuncCmd(BSP_USART_UNIT, USART_TX, ENABLE);

    while (RESET == DMA_GetTransCompleteStatus(BSP_USART_DMA_UNIT, tx_complete_flag)) {
    }
    while (RESET == USART_GetStatus(BSP_USART_UNIT, USART_FLAG_TX_CPLT)) {
    }
    (void)DMA_ChCmd(BSP_USART_DMA_UNIT, BSP_USART_TX_DMA_CH, DISABLE);

    return send_len;
}

void bsp_usart_dbg_printf(const char *format, ...)
{
    va_list args;
    int len;
    char buf[BSP_USART_PRINTF_BUF_SIZE];

    if (NULL == format) {
        return;
    }

    va_start(args, format);
    len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (len <= 0) {
        return;
    }
    if (len > (int)(sizeof(buf) - 1U)) {
        len = (int)(sizeof(buf) - 1U);
    }

    bsp_usart_dbg_tx(buf, len);
}

uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *data)
{
    if (NULL == data) {
        return 0U;
    }

    bsp_usart_clear_rx_error();
    bsp_usart_rx_dma_update_write_index();
    if (s_usart_rx_read_index == s_usart_rx_write_index) {
        return 0U;
    }

    *data = s_usart_rx_buf[s_usart_rx_read_index];
    s_usart_rx_read_index++;
    if (s_usart_rx_read_index >= BSP_USART_RX_BUF_SIZE) {
        s_usart_rx_read_index = 0U;
    }

    return 1U;
}
