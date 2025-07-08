#include "bsp_usart.h"
#include "section.h"
#include "gd32g5x3.h"
#include "stdio.h"
#include "stdint.h"
#include <stdarg.h>

USART_LINK_E usart_link = USART0_LINK;

void usart0_printf(const char *__format, ...)
{
    usart_link = USART0_LINK; // 设置当前使用的 USART 链接
    va_list args;
    va_start(args, __format);
    vprintf(__format, args);
    va_end(args);
}

void (*my_func_arr[])(char, void (*my_printf)(const char *__format, ...)) = {shell_run};

REG_LINK(ut0,
         128,
         usart0_printf,
         (uint32_t *)&DMA_CH0CNT(DMA0),
         my_func_arr,
         sizeof(my_func_arr) / sizeof(my_func_arr[0]));

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

    usart_baudrate_set(USART0, 1000000);
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
    dma_parameter.memory_addr = (uint32_t)GET_LINK_BUFF(ut0);
    dma_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_parameter.number = GET_LINK_SIZE(ut0);
    dma_parameter.priority = DMA_PRIORITY_MEDIUM;
    dma_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_parameter.request = DMA_REQUEST_USART0_RX;
    dma_init(DMA0, DMA_CH0, &dma_parameter);
    dma_circulation_enable(DMA0, DMA_CH0);
    dma_channel_enable(DMA0, DMA_CH0);
}

REG_INIT(bsp_usart_init)

int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++)
    {
        while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET)
            ;
        usart_data_transmit(USART0, ptr[i]);
    }
    return len;
}
