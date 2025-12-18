#include "bsp_usart.h"
#include "section.h"
#include "gd32g5x3.h"
#include "stdio.h"
#include "stdint.h"
#include <stdarg.h>

EXT_LINK(USART0_LINK);

void bsp_usart_init(void)
{
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_GPIOA);
    usart_deinit(USART0);

    gpio_af_set(USART0_TX_PORT, GPIO_AF_7, USART0_TX_PIN);
    gpio_af_set(USART0_RX_PORT, GPIO_AF_7, USART0_RX_PIN);

    gpio_mode_set(USART0_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_TX_PIN);
    gpio_output_options_set(USART0_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, USART0_TX_PIN);

    gpio_mode_set(USART0_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_RX_PIN);
    gpio_output_options_set(USART0_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, USART0_RX_PIN);

    usart_baudrate_set(USART0, 115200);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);

    dma_deinit(DMA0, DMA_CH0);

    dma_parameter_struct dma_parameter;
    dma_parameter.periph_addr = (uint32_t)(uint32_t *)&USART_RDATA(USART0);
    dma_parameter.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_parameter.memory_addr = (uint32_t)GET_LINK_BUFF(USART0_LINK);
    dma_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_parameter.number = GET_LINK_SIZE(USART0_LINK);
    dma_parameter.priority = DMA_PRIORITY_MEDIUM;
    dma_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_parameter.request = DMA_REQUEST_USART0_RX;
    dma_init(DMA0, DMA_CH0, &dma_parameter);
    dma_circulation_enable(DMA0, DMA_CH0);
    dma_channel_enable(DMA0, DMA_CH0);
}

REG_INIT(0, bsp_usart_init)

int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        uint32_t timeout = 10000; // 超时计数
        while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET)
        {
            if (--timeout == 0)
                break; // 超时退出，避免死循环
        }
        usart_data_transmit(USART0, ptr[i]);
    }
    return len;
}
