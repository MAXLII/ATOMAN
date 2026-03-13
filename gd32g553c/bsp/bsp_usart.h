#ifndef __BSP_USART_H
#define __BSP_USART_H

#include <stdint.h>

#define USART0_TX_PORT GPIOA
#define USART0_TX_PIN GPIO_PIN_9

#define USART0_RX_PORT GPIOA
#define USART0_RX_PIN GPIO_PIN_10

void usart0_dma_tx_init(void);

#endif

