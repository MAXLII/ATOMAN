#include "usart.h"
#include "section.h"
#include <stdarg.h>
#include "gd32g5x3.h"
#include "comm.h"
#include "shell.h"
#include <stdio.h>
#include <string.h>

/* Set to 1 if you need "last bit already out on wire" indication by USART TC. */
#ifndef USART0_DMA_TX_ENABLE_TC_FINISH
#define USART0_DMA_TX_ENABLE_TC_FINISH 0
#endif

static uint8_t s_usart0_tx_ring[USART0_DMA_TX_RING_SIZE];
static volatile uint16_t s_usart0_tx_head = 0U;
static volatile uint16_t s_usart0_tx_tail = 0U;
static volatile uint16_t s_usart0_tx_dma_len = 0U;
static volatile uint8_t s_usart0_tx_dma_busy = 0U;
#if USART0_DMA_TX_ENABLE_TC_FINISH
static volatile uint8_t s_usart0_tx_wait_tc = 0U;
#endif

USART_LINK_E usart_link = USART0_LINK;

static inline uint32_t usart0_dma_tx_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void usart0_dma_tx_unlock(uint32_t primask)
{
    if ((primask & 0x1U) == 0U)
    {
        __enable_irq();
    }
}

static inline uint16_t usart0_ring_count_unsafe(void)
{
    if (s_usart0_tx_head >= s_usart0_tx_tail)
    {
        return (uint16_t)(s_usart0_tx_head - s_usart0_tx_tail);
    }
    return (uint16_t)(USART0_DMA_TX_RING_SIZE - (s_usart0_tx_tail - s_usart0_tx_head));
}

static inline uint16_t usart0_ring_free_unsafe(void)
{
    /* Keep one byte empty to distinguish full vs empty. */
    return (uint16_t)((USART0_DMA_TX_RING_SIZE - 1U) - usart0_ring_count_unsafe());
}

static void usart0_dma_tx_kick(void)
{
    uint16_t contig_len = 0U;

    if (s_usart0_tx_dma_busy != 0U)
    {
        return;
    }

    if (s_usart0_tx_head == s_usart0_tx_tail)
    {
        return;
    }

    if (s_usart0_tx_head > s_usart0_tx_tail)
    {
        contig_len = (uint16_t)(s_usart0_tx_head - s_usart0_tx_tail);
    }
    else
    {
        contig_len = (uint16_t)(USART0_DMA_TX_RING_SIZE - s_usart0_tx_tail);
    }

    if (contig_len == 0U)
    {
        return;
    }

    dma_channel_disable(DMA0, DMA_CH1);
    dma_flag_clear(DMA0, DMA_CH1, DMA_FLAG_G);
    dma_memory_address_config(DMA0, DMA_CH1, (uint32_t)&s_usart0_tx_ring[s_usart0_tx_tail]);
    dma_transfer_number_config(DMA0, DMA_CH1, contig_len);
    s_usart0_tx_dma_len = contig_len;
    s_usart0_tx_dma_busy = 1U;
#if USART0_DMA_TX_ENABLE_TC_FINISH
    s_usart0_tx_wait_tc = 0U;
    usart_interrupt_disable(USART0, USART_INT_TC);
#endif
    dma_channel_enable(DMA0, DMA_CH1);
}

int usart0_tx_by_dma(const uint8_t *ptr, uint16_t len)
{
    uint32_t primask = 0U;
    uint16_t free_len = 0U;
    uint16_t first_copy = 0U;
    uint16_t second_copy = 0U;

    if ((ptr == NULL) || (len == 0U))
    {
        return USART0_TX_DMA_ERR_PARAM;
    }

    primask = usart0_dma_tx_lock();

    free_len = usart0_ring_free_unsafe();
    if (len > free_len)
    {
        usart0_dma_tx_unlock(primask);
        return USART0_TX_DMA_ERR_FULL;
    }

    first_copy = (uint16_t)(USART0_DMA_TX_RING_SIZE - s_usart0_tx_head);
    if (first_copy > len)
    {
        first_copy = len;
    }
    second_copy = (uint16_t)(len - first_copy);

    memcpy(&s_usart0_tx_ring[s_usart0_tx_head], ptr, first_copy);
    if (second_copy > 0U)
    {
        memcpy(&s_usart0_tx_ring[0], ptr + first_copy, second_copy);
    }

    s_usart0_tx_head = (uint16_t)((s_usart0_tx_head + len) % USART0_DMA_TX_RING_SIZE);

    usart0_dma_tx_kick();
    usart0_dma_tx_unlock(primask);

    return USART0_TX_DMA_OK;
}

void usart0_tx_by_dma_cb(char *ptr, int len)
{
    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }
    (void)usart0_tx_by_dma((const uint8_t *)ptr, (uint16_t)len);
}

void DMA0_Channel1_IRQHandler(void)
{
    if (dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INT_FLAG_FTF) != RESET)
    {
        dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_G);
        dma_channel_disable(DMA0, DMA_CH1);

        s_usart0_tx_tail = (uint16_t)((s_usart0_tx_tail + s_usart0_tx_dma_len) % USART0_DMA_TX_RING_SIZE);
        s_usart0_tx_dma_len = 0U;
        s_usart0_tx_dma_busy = 0U;

        if (s_usart0_tx_head != s_usart0_tx_tail)
        {
            usart0_dma_tx_kick();
        }
#if USART0_DMA_TX_ENABLE_TC_FINISH
        else
        {
            s_usart0_tx_wait_tc = 1U;
            usart_flag_clear(USART0, USART_FLAG_TC);
            usart_interrupt_enable(USART0, USART_INT_TC);
        }
#endif
    }
}

#if USART0_DMA_TX_ENABLE_TC_FINISH
void USART0_IRQHandler(void)
{
    if ((s_usart0_tx_wait_tc != 0U) &&
        (usart_interrupt_flag_get(USART0, USART_INT_FLAG_TC) != RESET))
    {
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_TC);
        usart_interrupt_disable(USART0, USART_INT_TC);
        s_usart0_tx_wait_tc = 0U;
        /* Optional hook point: RS485 DE low, tx-done callback, etc. */
    }
}
#endif

void usart0_printf(const char *__format, ...)
{
    usart_link = USART0_LINK; // 设置当前使用的 USART 链接
    va_list args;
    va_start(args, __format);
    vprintf(__format, args);
    va_end(args);
}

section_link_tx_func_t ut1_printf = {
    .my_printf = usart0_printf,
    .tx_by_dma = usart0_tx_by_dma_cb,
};

/* ===================== ctx（由本文件显式持有） ===================== */
static shell_ctx_t ut0_shell_ctx = {0};

/* COMM ctx：配套 payload buffer（长度可按需调整） */
DECLARE_COMM_CTX(ut0_comm_ctx, 128, 0x02, USART0_LINK);

/* ===================== handler_arr：函数 + ctx ===================== */
static const section_link_handler_item_t ut0_handler_arr[] = {
    {.func = shell_run, .ctx = (void *)&ut0_shell_ctx},
    {.func = comm_run, .ctx = (void *)&ut0_comm_ctx},
};

/* ===================== link 注册 ===================== */
REG_LINK(USART0_LINK,
         ut1_printf,
         (uint32_t *)&DMA_CH0CNT(DMA0),
         ut0_handler_arr,
         sizeof(ut0_handler_arr) / sizeof(ut0_handler_arr[0]));
