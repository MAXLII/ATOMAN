#ifndef __BSP_USART_H
#define __BSP_USART_H

#define USART0_TX_PORT GPIOA
#define USART0_TX_PIN GPIO_PIN_9

#define USART0_RX_PORT GPIOA
#define USART0_RX_PIN GPIO_PIN_10

typedef enum{
    USART0_LINK,
    USART1_LINK,
    USART2_LINK,
    USART3_LINK,
    USART4_LINK,
}USART_LINK_E;

void usart0_printf(const char *__format, ...);

#endif

