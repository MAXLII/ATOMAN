#ifndef __USART_H
#define __USART_H

#include <stdint.h>

/* tx ring buffer size for USART0 DMA non-blocking transmit */
#ifndef USART0_DMA_TX_RING_SIZE
#define USART0_DMA_TX_RING_SIZE 1024U
#endif

enum
{
    USART0_TX_DMA_OK = 0,
    USART0_TX_DMA_ERR_PARAM = -1,
    USART0_TX_DMA_ERR_FULL = -2,
};

int usart0_tx_by_dma(const uint8_t *ptr, uint16_t len);
void usart0_tx_by_dma_cb(char *ptr, int len);

typedef enum
{
    USART0_LINK,
    USART1_LINK,
    USART2_LINK,
    USART3_LINK,
    USART4_LINK,
} USART_LINK_E;

#endif
