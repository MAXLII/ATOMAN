#ifndef __BSP_USART_H
#define __BSP_USART_H

#include <stdint.h>

void bsp_usart_dbg_printf(const char *__format, ...);
void bsp_usart_dbg_tx(char *ptr, int len);
int bsp_usart_dbg_tx_dma(const uint8_t *p_data, uint32_t len);
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data);

void bsp_usart_iso_printf(const char *__format, ...);
void bsp_usart_iso_tx(char *ptr, int len);
uint8_t bsp_usart_iso_rx_get_byte(uint8_t *p_data);

#endif
