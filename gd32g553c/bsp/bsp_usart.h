#ifndef __BSP_USART_H
#define __BSP_USART_H

#include <stdint.h>

#define USART0_GPIO_RCU RCU_GPIOC

#define USART0_TX_PORT GPIOC
#define USART0_TX_PIN GPIO_PIN_4

#define USART0_RX_PORT GPIOC
#define USART0_RX_PIN GPIO_PIN_5

int bsp_usart_dbg_tx_dma(const uint8_t *p_data, uint32_t len);
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data);
void bsp_usart_dbg_printf(const char *__format, ...);

#endif

