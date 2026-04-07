#ifndef __USART_H
#define __USART_H

#include <stdint.h>
#include <stdarg.h>

typedef enum
{
    USART0_LINK,
    USART1_LINK,
    USART2_LINK,
    USART3_LINK,
    USART4_LINK,
} USART_LINK_E;

typedef enum
{
    USART_TX_DMA_OK = 0,
    USART_TX_DMA_ERR_PARAM = -1,
    USART_TX_DMA_ERR_FULL = -2,
} usart_tx_dma_status_e;

typedef struct
{
    uint32_t dma_periph;
    uint32_t dma_channel;
    uint32_t usart_periph;
    uint8_t tc_finish_enable;
} usart_dma_tx_port_t;

typedef struct
{
    uint8_t *p_ring;
    uint16_t ring_size;
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t dma_len;
    volatile uint8_t dma_busy;
    volatile uint8_t wait_tc;
    const usart_dma_tx_port_t *p_port;
} usart_dma_tx_t;

extern USART_LINK_E usart_link;

void usart_link_vprintf(USART_LINK_E link, const char *__format, va_list args);
void usart_link_printf(USART_LINK_E link, const char *__format, ...);

int usart_dma_tx_write(usart_dma_tx_t *p_tx, const uint8_t *ptr, uint16_t len);
void usart_dma_tx_write_cb(usart_dma_tx_t *p_tx, char *ptr, int len);
void usart_dma_tx_dma_irq_handler(usart_dma_tx_t *p_tx);
void usart_dma_tx_tc_irq_handler(usart_dma_tx_t *p_tx);

#endif
