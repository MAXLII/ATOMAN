#include "usart.h"
#include "gd32g5x3.h"
#include <stdio.h>
#include <string.h>

USART_LINK_E usart_link = USART0_LINK;

static inline uint32_t usart_dma_tx_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void usart_dma_tx_unlock(uint32_t primask)
{
    if ((primask & 0x1U) == 0U)
    {
        __enable_irq();
    }
}

void usart_link_vprintf(USART_LINK_E link, const char *__format, va_list args)
{
    usart_link = link;
    vprintf(__format, args);
}

void usart_link_printf(USART_LINK_E link, const char *__format, ...)
{
    va_list args;

    va_start(args, __format);
    usart_link_vprintf(link, __format, args);
    va_end(args);
}

static inline uint16_t usart_dma_tx_ring_count_unsafe(const usart_dma_tx_t *p_tx)
{
    if ((p_tx == NULL) || (p_tx->p_ring == NULL) || (p_tx->ring_size == 0U))
    {
        return 0U;
    }

    if (p_tx->head >= p_tx->tail)
    {
        return (uint16_t)(p_tx->head - p_tx->tail);
    }

    return (uint16_t)(p_tx->ring_size - (p_tx->tail - p_tx->head));
}

static inline uint16_t usart_dma_tx_ring_free_unsafe(const usart_dma_tx_t *p_tx)
{
    if ((p_tx == NULL) || (p_tx->ring_size <= 1U))
    {
        return 0U;
    }

    return (uint16_t)((p_tx->ring_size - 1U) - usart_dma_tx_ring_count_unsafe(p_tx));
}

static void usart_dma_tx_kick(usart_dma_tx_t *p_tx)
{
    uint16_t contig_len = 0U;

    if ((p_tx == NULL) || (p_tx->p_port == NULL) || (p_tx->p_ring == NULL))
    {
        return;
    }

    if ((p_tx->dma_busy != 0U) || (p_tx->head == p_tx->tail))
    {
        return;
    }

    if (p_tx->head > p_tx->tail)
    {
        contig_len = (uint16_t)(p_tx->head - p_tx->tail);
    }
    else
    {
        contig_len = (uint16_t)(p_tx->ring_size - p_tx->tail);
    }

    if (contig_len == 0U)
    {
        return;
    }

    dma_channel_disable(p_tx->p_port->dma_periph, p_tx->p_port->dma_channel);
    dma_flag_clear(p_tx->p_port->dma_periph, p_tx->p_port->dma_channel, DMA_FLAG_G);
    dma_memory_address_config(p_tx->p_port->dma_periph,
                              p_tx->p_port->dma_channel,
                              (uint32_t)&p_tx->p_ring[p_tx->tail]);
    dma_transfer_number_config(p_tx->p_port->dma_periph,
                               p_tx->p_port->dma_channel,
                               contig_len);
    p_tx->dma_len = contig_len;
    p_tx->dma_busy = 1U;

    if (p_tx->p_port->tc_finish_enable != 0U)
    {
        p_tx->wait_tc = 0U;
        usart_interrupt_disable(p_tx->p_port->usart_periph, USART_INT_TC);
    }

    dma_channel_enable(p_tx->p_port->dma_periph, p_tx->p_port->dma_channel);
}

int usart_dma_tx_write(usart_dma_tx_t *p_tx, const uint8_t *ptr, uint16_t len)
{
    uint32_t primask = 0U;
    uint16_t free_len = 0U;
    uint16_t first_copy = 0U;
    uint16_t second_copy = 0U;

    if ((p_tx == NULL) || (p_tx->p_port == NULL) || (p_tx->p_ring == NULL) ||
        (ptr == NULL) || (len == 0U))
    {
        return USART_TX_DMA_ERR_PARAM;
    }

    primask = usart_dma_tx_lock();

    free_len = usart_dma_tx_ring_free_unsafe(p_tx);
    if (len > free_len)
    {
        usart_dma_tx_unlock(primask);
        return USART_TX_DMA_ERR_FULL;
    }

    first_copy = (uint16_t)(p_tx->ring_size - p_tx->head);
    if (first_copy > len)
    {
        first_copy = len;
    }
    second_copy = (uint16_t)(len - first_copy);

    memcpy(&p_tx->p_ring[p_tx->head], ptr, first_copy);
    if (second_copy > 0U)
    {
        memcpy(&p_tx->p_ring[0], ptr + first_copy, second_copy);
    }

    p_tx->head = (uint16_t)((p_tx->head + len) % p_tx->ring_size);

    usart_dma_tx_kick(p_tx);
    usart_dma_tx_unlock(primask);

    return USART_TX_DMA_OK;
}

void usart_dma_tx_write_cb(usart_dma_tx_t *p_tx, char *ptr, int len)
{
    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    (void)usart_dma_tx_write(p_tx, (const uint8_t *)ptr, (uint16_t)len);
}

void usart_dma_tx_dma_irq_handler(usart_dma_tx_t *p_tx)
{
    if ((p_tx == NULL) || (p_tx->p_port == NULL) || (p_tx->p_ring == NULL))
    {
        return;
    }

    if (dma_interrupt_flag_get(p_tx->p_port->dma_periph,
                               p_tx->p_port->dma_channel,
                               DMA_INT_FLAG_FTF) == RESET)
    {
        return;
    }

    dma_interrupt_flag_clear(p_tx->p_port->dma_periph,
                             p_tx->p_port->dma_channel,
                             DMA_INT_FLAG_G);
    dma_channel_disable(p_tx->p_port->dma_periph, p_tx->p_port->dma_channel);

    p_tx->tail = (uint16_t)((p_tx->tail + p_tx->dma_len) % p_tx->ring_size);
    p_tx->dma_len = 0U;
    p_tx->dma_busy = 0U;

    if (p_tx->head != p_tx->tail)
    {
        usart_dma_tx_kick(p_tx);
        return;
    }

    if (p_tx->p_port->tc_finish_enable != 0U)
    {
        p_tx->wait_tc = 1U;
        usart_flag_clear(p_tx->p_port->usart_periph, USART_FLAG_TC);
        usart_interrupt_enable(p_tx->p_port->usart_periph, USART_INT_TC);
    }
}

void usart_dma_tx_tc_irq_handler(usart_dma_tx_t *p_tx)
{
    if ((p_tx == NULL) || (p_tx->p_port == NULL) ||
        (p_tx->p_port->tc_finish_enable == 0U) || (p_tx->wait_tc == 0U))
    {
        return;
    }

    if (usart_interrupt_flag_get(p_tx->p_port->usart_periph, USART_INT_FLAG_TC) == RESET)
    {
        return;
    }

    usart_interrupt_flag_clear(p_tx->p_port->usart_periph, USART_INT_FLAG_TC);
    usart_interrupt_disable(p_tx->p_port->usart_periph, USART_INT_TC);
    p_tx->wait_tc = 0U;
}
