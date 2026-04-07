#include "comm_link.h"
#include "usart.h"
#include "comm_addr.h"
#include "section.h"
#include "comm.h"
#include "shell.h"
#include "gd32g5x3.h"
#include <stdarg.h>

#ifndef AC_USART0_DMA_TX_RING_SIZE
#define AC_USART0_DMA_TX_RING_SIZE 1024U
#endif

#ifndef AC_USART0_DMA_TX_ENABLE_TC_FINISH
#define AC_USART0_DMA_TX_ENABLE_TC_FINISH 0U
#endif

#ifndef AC_USART0_COMM_PAYLOAD_SIZE
#define AC_USART0_COMM_PAYLOAD_SIZE 128U
#endif

static uint8_t s_usart0_tx_ring[AC_USART0_DMA_TX_RING_SIZE] = {0};

static const usart_dma_tx_port_t s_usart0_dma_port = {
    .dma_periph = DMA0,
    .dma_channel = DMA_CH1,
    .usart_periph = USART0,
    .tc_finish_enable = AC_USART0_DMA_TX_ENABLE_TC_FINISH,
};

static usart_dma_tx_t s_usart0_dma_tx = {
    .p_ring = s_usart0_tx_ring,
    .ring_size = (uint16_t)sizeof(s_usart0_tx_ring),
    .head = 0U,
    .tail = 0U,
    .dma_len = 0U,
    .dma_busy = 0U,
    .wait_tc = 0U,
    .p_port = &s_usart0_dma_port,
};

static void usart0_tx_by_dma_cb(char *ptr, int len)
{
    usart_dma_tx_write_cb(&s_usart0_dma_tx, ptr, len);
}

static void usart0_printf(const char *__format, ...)
{
    va_list args;

    va_start(args, __format);
    usart_link_vprintf(USART0_LINK, __format, args);
    va_end(args);
}

static section_link_tx_func_t s_usart0_tx_func = {
    .my_printf = usart0_printf,
    .tx_by_dma = usart0_tx_by_dma_cb,
};

static shell_ctx_t s_usart0_shell_ctx = {0};

DECLARE_COMM_CTX(s_usart0_comm_ctx, AC_USART0_COMM_PAYLOAD_SIZE, LOCAL_ADDR, USART0_LINK);

static const section_link_handler_item_t s_usart0_handler_arr[] = {
    {.func = shell_run, .ctx = (void *)&s_usart0_shell_ctx},
    {.func = comm_run, .ctx = (void *)&s_usart0_comm_ctx},
};

REG_LINK(USART0_LINK,
         s_usart0_tx_func,
         (uint32_t *)&DMA_CH0CNT(DMA0),
         s_usart0_handler_arr,
         sizeof(s_usart0_handler_arr) / sizeof(s_usart0_handler_arr[0]));

void DMA0_Channel1_IRQHandler(void)
{
    usart_dma_tx_dma_irq_handler(&s_usart0_dma_tx);
}

#if AC_USART0_DMA_TX_ENABLE_TC_FINISH
void USART0_IRQHandler(void)
{
    usart_dma_tx_tc_irq_handler(&s_usart0_dma_tx);
}
#endif
