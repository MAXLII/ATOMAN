#ifndef __BSP_USART_H
#define __BSP_USART_H

#include <stdint.h>

#define USART0_TX_PORT GPIOA
#define USART0_TX_PIN GPIO_PIN_9

#define USART0_RX_PORT GPIOA
#define USART0_RX_PIN GPIO_PIN_10

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

void usart0_dma_tx_init(void);
int usart0_tx_by_dma(const uint8_t *ptr, uint16_t len);

/* Compatibility wrapper for legacy callback type: void (*)(char *ptr, int len) */
void usart0_tx_by_dma_cb(char *ptr, int len);

#endif

