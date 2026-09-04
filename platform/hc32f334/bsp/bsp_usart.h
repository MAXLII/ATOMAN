#ifndef __BSP_USART_H__
#define __BSP_USART_H__

#include "hc32_ll.h"

#define BSP_COMM_LINK_ENABLE_ISO 1u
#define BSP_COMM_LINK_ENABLE_CAN 1u
#define BSP_COMM_LINK_ENABLE_PL 0u

void bsp_usart_dbg_printf(const char *__format, ...);
void bsp_usart_dbg_tx(char *ptr, int len);
uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data);
void bsp_comm_memory_barrier(void);

void bsp_usart_iso_printf(const char *__format, ...);
void bsp_usart_iso_tx(char *ptr, int len);
uint8_t bsp_usart_iso_rx_get_byte(uint8_t *p_data);
uint8_t bsp_usart_iso_tx_is_idle(void);

#endif
