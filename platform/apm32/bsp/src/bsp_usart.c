// SPDX-License-Identifier: MIT
/**
 * @file    bsp_usart.c
 * @brief   APM32 USART BSP module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize USART1 and UART4 GPIO, clocks, and DMA channels
 *          - Receive bytes through circular DMA buffers
 *          - Transmit bytes through software ring buffers and normal-mode DMA
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - RX and TX paths are protected for task/ISR concurrency with PRIMASK
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_usart.h"

#include "apm32f402_403_dma.h"
#include "apm32f402_403_gpio.h"
#include "apm32f402_403_rcm.h"
#include "apm32f402_403_usart.h"
#include "section.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BSP_USART_DBG_UNIT (UART4)
#define BSP_USART_DBG_CLK_PERIPH (RCM_APB1_PERIPH_UART4)
#define BSP_USART_DBG_CLK_APB1 (1U)
#define BSP_USART_DBG_TX_GPIO_PERIPH (RCM_APB2_PERIPH_GPIOC)
#define BSP_USART_DBG_RX_GPIO_PERIPH (RCM_APB2_PERIPH_GPIOC)
#define BSP_USART_DBG_TX_PORT (GPIOC)
#define BSP_USART_DBG_TX_PIN (GPIO_PIN_10)
#define BSP_USART_DBG_RX_PORT (GPIOC)
#define BSP_USART_DBG_RX_PIN (GPIO_PIN_11)
#define BSP_USART_DBG_BAUDRATE (115200UL)
#define BSP_USART_DBG_RX_DMA_CH (DMA2_Channel3)
#define BSP_USART_DBG_TX_DMA_CH (DMA2_Channel5)
#define BSP_USART_DBG_RX_DMA_FLAG_G (DMA2_FLAG_GINT3)
#define BSP_USART_DBG_TX_DMA_FLAG_G (DMA2_FLAG_GINT5)
#define BSP_USART_DBG_TX_DMA_FLAG_TC (DMA2_FLAG_TC5)

#define BSP_USART_ISO_UNIT (USART1)
#define BSP_USART_ISO_CLK_PERIPH (RCM_APB2_PERIPH_USART1)
#define BSP_USART_ISO_CLK_APB1 (0U)
#define BSP_USART_ISO_TX_GPIO_PERIPH (RCM_APB2_PERIPH_GPIOB)
#define BSP_USART_ISO_RX_GPIO_PERIPH (RCM_APB2_PERIPH_GPIOB)
#define BSP_USART_ISO_TX_PORT (GPIOB)
#define BSP_USART_ISO_TX_PIN (GPIO_PIN_6)
#define BSP_USART_ISO_RX_PORT (GPIOB)
#define BSP_USART_ISO_RX_PIN (GPIO_PIN_7)
#define BSP_USART_ISO_BAUDRATE (38400UL)
#define BSP_USART_ISO_RX_DMA_CH (DMA1_Channel5)
#define BSP_USART_ISO_TX_DMA_CH (DMA1_Channel4)
#define BSP_USART_ISO_RX_DMA_FLAG_G (DMA1_FLAG_GINT5)
#define BSP_USART_ISO_TX_DMA_FLAG_G (DMA1_FLAG_GINT4)
#define BSP_USART_ISO_TX_DMA_FLAG_TC (DMA1_FLAG_TC4)

#define BSP_USART_RX_DMA_BUF_SIZE (512U)
#define BSP_USART_TX_RING_BUF_SIZE (1024U)
#define BSP_USART_TX_DMA_TIMEOUT (0x00FFFFFFUL)
#define BSP_USART_DBG_PRINTF_BUF_SIZE (256U)
#define BSP_USART_ISO_PRINTF_BUF_SIZE (256U)

typedef struct
{
    uint8_t rx_dma_buf[BSP_USART_RX_DMA_BUF_SIZE];
    uint16_t rx_read_idx;
    uint8_t tx_ring_buf[BSP_USART_TX_RING_BUF_SIZE];
    volatile uint16_t tx_ring_head;
    volatile uint16_t tx_ring_tail;
    volatile uint16_t tx_dma_len;
    volatile uint8_t tx_dma_busy;
} bsp_usart_port_ctx_t;

typedef struct
{
    USART_T *unit;
    uint32_t usart_clk_periph;
    uint8_t usart_clk_apb1;
    uint32_t tx_gpio_periph;
    uint32_t rx_gpio_periph;
    GPIO_T *tx_port;
    uint16_t tx_pin;
    GPIO_T *rx_port;
    uint16_t rx_pin;
    uint32_t baudrate;
    DMA_Channel_T *rx_dma_ch;
    DMA_Channel_T *tx_dma_ch;
    uint32_t rx_dma_flag_g;
    uint32_t tx_dma_flag_g;
    uint32_t tx_dma_flag_tc;
    uint16_t rx_dma_buf_size;
    uint16_t tx_ring_buf_size;
    uint32_t tx_dma_timeout;
    uint8_t remap_usart1;
} bsp_usart_port_cfg_t;

static bsp_usart_port_ctx_t s_bsp_usart_dbg_ctx = {0};
static bsp_usart_port_ctx_t s_bsp_usart_iso_ctx = {0};

static const bsp_usart_port_cfg_t s_bsp_usart_dbg_cfg = {
    .unit = BSP_USART_DBG_UNIT,
    .usart_clk_periph = BSP_USART_DBG_CLK_PERIPH,
    .usart_clk_apb1 = BSP_USART_DBG_CLK_APB1,
    .tx_gpio_periph = BSP_USART_DBG_TX_GPIO_PERIPH,
    .rx_gpio_periph = BSP_USART_DBG_RX_GPIO_PERIPH,
    .tx_port = BSP_USART_DBG_TX_PORT,
    .tx_pin = BSP_USART_DBG_TX_PIN,
    .rx_port = BSP_USART_DBG_RX_PORT,
    .rx_pin = BSP_USART_DBG_RX_PIN,
    .baudrate = BSP_USART_DBG_BAUDRATE,
    .rx_dma_ch = BSP_USART_DBG_RX_DMA_CH,
    .tx_dma_ch = BSP_USART_DBG_TX_DMA_CH,
    .rx_dma_flag_g = BSP_USART_DBG_RX_DMA_FLAG_G,
    .tx_dma_flag_g = BSP_USART_DBG_TX_DMA_FLAG_G,
    .tx_dma_flag_tc = BSP_USART_DBG_TX_DMA_FLAG_TC,
    .rx_dma_buf_size = BSP_USART_RX_DMA_BUF_SIZE,
    .tx_ring_buf_size = BSP_USART_TX_RING_BUF_SIZE,
    .tx_dma_timeout = BSP_USART_TX_DMA_TIMEOUT,
    .remap_usart1 = 0U,
};

static const bsp_usart_port_cfg_t s_bsp_usart_iso_cfg = {
    .unit = BSP_USART_ISO_UNIT,
    .usart_clk_periph = BSP_USART_ISO_CLK_PERIPH,
    .usart_clk_apb1 = BSP_USART_ISO_CLK_APB1,
    .tx_gpio_periph = BSP_USART_ISO_TX_GPIO_PERIPH,
    .rx_gpio_periph = BSP_USART_ISO_RX_GPIO_PERIPH,
    .tx_port = BSP_USART_ISO_TX_PORT,
    .tx_pin = BSP_USART_ISO_TX_PIN,
    .rx_port = BSP_USART_ISO_RX_PORT,
    .rx_pin = BSP_USART_ISO_RX_PIN,
    .baudrate = BSP_USART_ISO_BAUDRATE,
    .rx_dma_ch = BSP_USART_ISO_RX_DMA_CH,
    .tx_dma_ch = BSP_USART_ISO_TX_DMA_CH,
    .rx_dma_flag_g = BSP_USART_ISO_RX_DMA_FLAG_G,
    .tx_dma_flag_g = BSP_USART_ISO_TX_DMA_FLAG_G,
    .tx_dma_flag_tc = BSP_USART_ISO_TX_DMA_FLAG_TC,
    .rx_dma_buf_size = BSP_USART_RX_DMA_BUF_SIZE,
    .tx_ring_buf_size = BSP_USART_TX_RING_BUF_SIZE,
    .tx_dma_timeout = BSP_USART_TX_DMA_TIMEOUT,
    .remap_usart1 = 1U,
};

static uint32_t bsp_usart_irq_lock(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

static void bsp_usart_irq_unlock(uint32_t primask)
{
    if ((primask & 0x1U) == 0U)
    {
        __enable_irq();
    }
}

static uint16_t bsp_usart_tx_ring_used(const bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    uint16_t head = ctx->tx_ring_head;
    uint16_t tail = ctx->tx_ring_tail;

    if (head >= tail)
    {
        return (uint16_t)(head - tail);
    }

    return (uint16_t)(cfg->tx_ring_buf_size - tail + head);
}

static uint16_t bsp_usart_tx_ring_free(const bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    return (uint16_t)(cfg->tx_ring_buf_size - bsp_usart_tx_ring_used(ctx, cfg) - 1U);
}

static void bsp_usart_dma_config(DMA_Channel_T *channel,
                                 uint32_t periph_addr,
                                 uint32_t memory_addr,
                                 uint16_t data_count,
                                 DMA_DIR_T direction,
                                 DMA_LOOP_MODE_T loop_mode)
{
    DMA_Config_T dma_cfg;

    DMA_Disable(channel);
    DMA_Reset(channel);
    DMA_ConfigStructInit(&dma_cfg);
    dma_cfg.peripheralBaseAddr = periph_addr;
    dma_cfg.memoryBaseAddr = memory_addr;
    dma_cfg.dir = direction;
    dma_cfg.bufferSize = data_count;
    dma_cfg.peripheralInc = DMA_PERIPHERAL_INC_DISABLE;
    dma_cfg.memoryInc = DMA_MEMORY_INC_ENABLE;
    dma_cfg.peripheralDataSize = DMA_PERIPHERAL_DATA_SIZE_BYTE;
    dma_cfg.memoryDataSize = DMA_MEMORY_DATA_SIZE_BYTE;
    dma_cfg.loopMode = loop_mode;
    dma_cfg.priority = DMA_PRIORITY_MEDIUM;
    dma_cfg.M2M = DMA_M2MEN_DISABLE;
    DMA_Config(channel, &dma_cfg);
}

static void bsp_usart_rx_dma_init(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    bsp_usart_dma_config(cfg->rx_dma_ch,
                         (uint32_t)&cfg->unit->DATA,
                         (uint32_t)ctx->rx_dma_buf,
                         cfg->rx_dma_buf_size,
                         DMA_DIR_PERIPHERAL_SRC,
                         DMA_MODE_CIRCULAR);
    DMA_ClearStatusFlag(cfg->rx_dma_flag_g);
    DMA_Enable(cfg->rx_dma_ch);
}

static void bsp_usart_tx_dma_init(const bsp_usart_port_cfg_t *cfg)
{
    bsp_usart_dma_config(cfg->tx_dma_ch,
                         (uint32_t)&cfg->unit->DATA,
                         0U,
                         0U,
                         DMA_DIR_PERIPHERAL_DST,
                         DMA_MODE_NORMAL);
    DMA_ClearStatusFlag(cfg->tx_dma_flag_g);
}

static uint16_t bsp_usart_rx_dma_get_write_idx(const bsp_usart_port_cfg_t *cfg)
{
    uint16_t write_idx = (uint16_t)(cfg->rx_dma_buf_size - DMA_ReadDataNumber(cfg->rx_dma_ch));

    if (write_idx >= cfg->rx_dma_buf_size)
    {
        write_idx = 0U;
    }
    return write_idx;
}

static uint8_t bsp_usart_rx_has_error(const bsp_usart_port_cfg_t *cfg)
{
    return (uint8_t)((USART_ReadStatusFlag(cfg->unit, USART_FLAG_OVRE) == SET) ||
                     (USART_ReadStatusFlag(cfg->unit, USART_FLAG_NE) == SET) ||
                     (USART_ReadStatusFlag(cfg->unit, USART_FLAG_FE) == SET) ||
                     (USART_ReadStatusFlag(cfg->unit, USART_FLAG_PE) == SET));
}

static void bsp_usart_rx_recover_if_error(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    volatile uint32_t dummy;

    if (bsp_usart_rx_has_error(cfg) == 0U)
    {
        return;
    }

    DMA_Disable(cfg->rx_dma_ch);
    dummy = cfg->unit->DATA;
    (void)dummy;

    USART_ClearStatusFlag(cfg->unit, USART_FLAG_OVRE);
    USART_ClearStatusFlag(cfg->unit, USART_FLAG_NE);
    USART_ClearStatusFlag(cfg->unit, USART_FLAG_FE);
    USART_ClearStatusFlag(cfg->unit, USART_FLAG_PE);

    memset(ctx->rx_dma_buf, 0, cfg->rx_dma_buf_size);
    ctx->rx_read_idx = 0U;
    bsp_usart_rx_dma_init(ctx, cfg);
}

static uint16_t bsp_usart_tx_ring_read(bsp_usart_port_ctx_t *ctx,
                                       const bsp_usart_port_cfg_t *cfg,
                                       uint8_t **pp_data)
{
    uint16_t read_len;

    if ((ctx == NULL) || (cfg == NULL) || (pp_data == NULL) || (ctx->tx_ring_head == ctx->tx_ring_tail))
    {
        return 0U;
    }

    if (ctx->tx_ring_head > ctx->tx_ring_tail)
    {
        read_len = (uint16_t)(ctx->tx_ring_head - ctx->tx_ring_tail);
    }
    else
    {
        read_len = (uint16_t)(cfg->tx_ring_buf_size - ctx->tx_ring_tail);
    }

    *pp_data = &ctx->tx_ring_buf[ctx->tx_ring_tail];
    return read_len;
}

static void bsp_usart_tx_dma_start(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    uint8_t *p_data = NULL;
    uint16_t len;

    if ((ctx == NULL) || (cfg == NULL) || (ctx->tx_dma_busy != 0U))
    {
        return;
    }

    len = bsp_usart_tx_ring_read(ctx, cfg, &p_data);
    if ((len == 0U) || (p_data == NULL))
    {
        return;
    }

    DMA_Disable(cfg->tx_dma_ch);
    DMA_ClearStatusFlag(cfg->tx_dma_flag_g);
    cfg->tx_dma_ch->CHMADDR = (uint32_t)p_data;
    DMA_ConfigDataNumber(cfg->tx_dma_ch, len);
    USART_ClearStatusFlag(cfg->unit, USART_FLAG_TXC);

    ctx->tx_dma_len = len;
    ctx->tx_dma_busy = 1U;
    DMA_Enable(cfg->tx_dma_ch);
}

static void bsp_usart_tx_dma_service(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    if ((ctx == NULL) || (cfg == NULL))
    {
        return;
    }

    if (ctx->tx_dma_busy != 0U)
    {
        if ((DMA_ReadStatusFlag((DMA_FLAG_T)cfg->tx_dma_flag_tc) == RESET) ||
            (USART_ReadStatusFlag(cfg->unit, USART_FLAG_TXC) == RESET))
        {
            return;
        }

        DMA_ClearStatusFlag(cfg->tx_dma_flag_g);
        DMA_Disable(cfg->tx_dma_ch);
        ctx->tx_ring_tail = (uint16_t)((ctx->tx_ring_tail + ctx->tx_dma_len) % cfg->tx_ring_buf_size);
        ctx->tx_dma_len = 0U;
        ctx->tx_dma_busy = 0U;
    }

    bsp_usart_tx_dma_start(ctx, cfg);
}

static uint16_t bsp_usart_tx_ring_write(bsp_usart_port_ctx_t *ctx,
                                        const bsp_usart_port_cfg_t *cfg,
                                        const uint8_t *data,
                                        uint16_t len)
{
    uint16_t offset = 0U;

    if ((ctx == NULL) || (cfg == NULL) || (data == NULL))
    {
        return 0U;
    }

    while (offset < len)
    {
        uint16_t free_len = bsp_usart_tx_ring_free(ctx, cfg);
        uint16_t to_end = (uint16_t)(cfg->tx_ring_buf_size - ctx->tx_ring_head);
        uint16_t write_len = (uint16_t)(len - offset);

        if (free_len == 0U)
        {
            break;
        }
        if (write_len > free_len)
        {
            write_len = free_len;
        }
        if (write_len > to_end)
        {
            write_len = to_end;
        }

        memcpy(&ctx->tx_ring_buf[ctx->tx_ring_head], &data[offset], write_len);
        ctx->tx_ring_head = (uint16_t)((ctx->tx_ring_head + write_len) % cfg->tx_ring_buf_size);
        offset = (uint16_t)(offset + write_len);
    }

    return offset;
}

static uint16_t bsp_usart_write(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg, const uint8_t *data, uint16_t len)
{
    uint32_t primask;
    uint32_t timeout;

    if ((ctx == NULL) || (cfg == NULL) || (data == NULL) || (len == 0U))
    {
        return 0U;
    }

    if (len >= cfg->tx_ring_buf_size)
    {
        return 0U;
    }

    timeout = cfg->tx_dma_timeout;
    while (1)
    {
        uint16_t free_len;
        uint16_t written;

        bsp_usart_tx_dma_service(ctx, cfg);

        primask = bsp_usart_irq_lock();
        free_len = bsp_usart_tx_ring_free(ctx, cfg);
        if (len <= free_len)
        {
            written = bsp_usart_tx_ring_write(ctx, cfg, data, len);
            bsp_usart_tx_dma_start(ctx, cfg);
            bsp_usart_irq_unlock(primask);
            return written;
        }

        bsp_usart_tx_dma_start(ctx, cfg);
        bsp_usart_irq_unlock(primask);

        if (timeout == 0U)
        {
            break;
        }
        timeout--;
    }

    return 0U;
}

static int32_t bsp_usart_read_byte(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg, uint8_t *data)
{
    uint16_t write_idx;

    if ((ctx == NULL) || (cfg == NULL) || (data == NULL))
    {
        return -1;
    }

    bsp_usart_rx_recover_if_error(ctx, cfg);
    write_idx = bsp_usart_rx_dma_get_write_idx(cfg);

    if (ctx->rx_read_idx != write_idx)
    {
        *data = ctx->rx_dma_buf[ctx->rx_read_idx];
        ctx->rx_read_idx++;
        if (ctx->rx_read_idx >= cfg->rx_dma_buf_size)
        {
            ctx->rx_read_idx = 0U;
        }
        return 0;
    }

    return -2;
}

static void bsp_usart_port_gpio_init(const bsp_usart_port_cfg_t *cfg)
{
    GPIO_Config_T gpio_cfg = {0U};

    RCM_EnableAPB2PeriphClock(cfg->tx_gpio_periph);
    RCM_EnableAPB2PeriphClock(cfg->rx_gpio_periph);
    RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_AFIO);

    if (cfg->remap_usart1 != 0U)
    {
        GPIO_ConfigPinRemap(GPIO_REMAP_USART1);
    }

    gpio_cfg.mode = GPIO_MODE_AF_PP;
    gpio_cfg.pin = cfg->tx_pin;
    gpio_cfg.speed = GPIO_SPEED_50MHz;
    GPIO_Config(cfg->tx_port, &gpio_cfg);

    gpio_cfg.mode = GPIO_MODE_IN_PU;
    gpio_cfg.pin = cfg->rx_pin;
    gpio_cfg.speed = GPIO_SPEED_50MHz;
    GPIO_Config(cfg->rx_port, &gpio_cfg);
}

static void bsp_usart_port_init(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    USART_Config_T usart_cfg;

    if ((ctx == NULL) || (cfg == NULL))
    {
        return;
    }

    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1 | RCM_AHB_PERIPH_DMA2);
    if (cfg->usart_clk_apb1 != 0U)
    {
        RCM_EnableAPB1PeriphClock(cfg->usart_clk_periph);
    }
    else
    {
        RCM_EnableAPB2PeriphClock(cfg->usart_clk_periph);
    }

    bsp_usart_port_gpio_init(cfg);

    USART_Reset(cfg->unit);
    USART_ConfigStructInit(&usart_cfg);
    usart_cfg.baudRate = cfg->baudrate;
    usart_cfg.hardwareFlow = USART_HARDWARE_FLOW_NONE;
    usart_cfg.mode = USART_MODE_TX_RX;
    usart_cfg.parity = USART_PARITY_NONE;
    usart_cfg.stopBits = USART_STOP_BIT_1;
    usart_cfg.wordLength = USART_WORD_LEN_8B;
    USART_Config(cfg->unit, &usart_cfg);
    USART_EnableDMA(cfg->unit, USART_DMA_TX_RX);
    USART_Enable(cfg->unit);

    ctx->rx_read_idx = 0U;
    ctx->tx_ring_head = 0U;
    ctx->tx_ring_tail = 0U;
    ctx->tx_dma_len = 0U;
    ctx->tx_dma_busy = 0U;
    memset(ctx->rx_dma_buf, 0, cfg->rx_dma_buf_size);
    memset(ctx->tx_ring_buf, 0, cfg->tx_ring_buf_size);

    bsp_usart_rx_dma_init(ctx, cfg);
    bsp_usart_tx_dma_init(cfg);
}

static void bsp_usart_dbg_init(void)
{
    bsp_usart_port_init(&s_bsp_usart_dbg_ctx, &s_bsp_usart_dbg_cfg);
}

REG_INIT(0, bsp_usart_dbg_init)

static void bsp_usart_iso_init(void)
{
    bsp_usart_port_init(&s_bsp_usart_iso_ctx, &s_bsp_usart_iso_cfg);
}

REG_INIT(0, bsp_usart_iso_init)

static void bsp_usart_dbg_tx_service_task(void)
{
    bsp_usart_tx_dma_service(&s_bsp_usart_dbg_ctx, &s_bsp_usart_dbg_cfg);
}

REG_TASK(100, bsp_usart_dbg_tx_service_task)

static void bsp_usart_iso_tx_service_task(void)
{
    bsp_usart_tx_dma_service(&s_bsp_usart_iso_ctx, &s_bsp_usart_iso_cfg);
}

REG_TASK(100, bsp_usart_iso_tx_service_task)

void bsp_usart_dbg_tx(char *ptr, int len)
{
    uint16_t req_len;

    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    req_len = ((uint32_t)len > 0xFFFFUL) ? 0xFFFFU : (uint16_t)len;
    (void)bsp_usart_write(&s_bsp_usart_dbg_ctx, &s_bsp_usart_dbg_cfg, (const uint8_t *)ptr, req_len);
}

void bsp_usart_dbg_printf(const char *__format, ...)
{
    va_list args;
    int len;
    char buf[BSP_USART_DBG_PRINTF_BUF_SIZE];

    if (__format == NULL)
    {
        return;
    }

    va_start(args, __format);
    len = vsnprintf(buf, sizeof(buf), __format, args);
    va_end(args);

    if (len <= 0)
    {
        return;
    }
    if (len > (int)(sizeof(buf) - 1U))
    {
        len = (int)(sizeof(buf) - 1U);
    }

    bsp_usart_dbg_tx(buf, len);
}

uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data)
{
    return (bsp_usart_read_byte(&s_bsp_usart_dbg_ctx, &s_bsp_usart_dbg_cfg, p_data) == 0) ? 1U : 0U;
}

void bsp_usart_iso_tx(char *ptr, int len)
{
    uint16_t req_len;

    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    req_len = ((uint32_t)len > 0xFFFFUL) ? 0xFFFFU : (uint16_t)len;
    (void)bsp_usart_write(&s_bsp_usart_iso_ctx, &s_bsp_usart_iso_cfg, (const uint8_t *)ptr, req_len);
}

void bsp_usart_iso_printf(const char *__format, ...)
{
    va_list args;
    int len;
    char buf[BSP_USART_ISO_PRINTF_BUF_SIZE];

    if (__format == NULL)
    {
        return;
    }

    va_start(args, __format);
    len = vsnprintf(buf, sizeof(buf), __format, args);
    va_end(args);

    if (len <= 0)
    {
        return;
    }
    if (len > (int)(sizeof(buf) - 1U))
    {
        len = (int)(sizeof(buf) - 1U);
    }

    bsp_usart_iso_tx(buf, len);
}

uint8_t bsp_usart_iso_rx_get_byte(uint8_t *p_data)
{
    return (bsp_usart_read_byte(&s_bsp_usart_iso_ctx, &s_bsp_usart_iso_cfg, p_data) == 0) ? 1U : 0U;
}

int bsp_usart_dbg_tx_dma(const uint8_t *p_data, uint32_t len)
{
    if ((p_data == NULL) || (len > 0xFFFFUL))
    {
        return -1;
    }

    bsp_usart_dbg_tx((char *)p_data, (int)len);
    return 0;
}
