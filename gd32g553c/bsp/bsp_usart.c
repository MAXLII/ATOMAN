#include "bsp_usart.h"
#include "section.h"
#include "gd32g5x3.h"
#include "stdio.h"
#include "stdint.h"
#include <stdarg.h>
#include <string.h>

EXT_LINK(USART0_LINK);

/* Set to 1 if you need "last bit already out on wire" indication by USART TC. */
#ifndef USART0_DMA_TX_ENABLE_TC_FINISH
#define USART0_DMA_TX_ENABLE_TC_FINISH 0
#endif

static uint8_t s_usart0_tx_ring[USART0_DMA_TX_RING_SIZE];
static volatile uint16_t s_usart0_tx_head = 0U;
static volatile uint16_t s_usart0_tx_tail = 0U;
static volatile uint16_t s_usart0_tx_dma_len = 0U;
static volatile uint8_t s_usart0_tx_dma_busy = 0U;
#if USART0_DMA_TX_ENABLE_TC_FINISH
static volatile uint8_t s_usart0_tx_wait_tc = 0U;
#endif

static inline uint32_t usart0_dma_tx_lock(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void usart0_dma_tx_unlock(uint32_t primask)
{
    if ((primask & 0x1U) == 0U)
    {
        __enable_irq();
    }
}

static inline uint16_t usart0_ring_count_unsafe(void)
{
    if (s_usart0_tx_head >= s_usart0_tx_tail)
    {
        return (uint16_t)(s_usart0_tx_head - s_usart0_tx_tail);
    }
    return (uint16_t)(USART0_DMA_TX_RING_SIZE - (s_usart0_tx_tail - s_usart0_tx_head));
}

static inline uint16_t usart0_ring_free_unsafe(void)
{
    /* Keep one byte empty to distinguish full vs empty. */
    return (uint16_t)((USART0_DMA_TX_RING_SIZE - 1U) - usart0_ring_count_unsafe());
}

static void usart0_dma_tx_kick(void)
{
    uint16_t contig_len = 0U;

    if (s_usart0_tx_dma_busy != 0U)
    {
        return;
    }

    if (s_usart0_tx_head == s_usart0_tx_tail)
    {
        return;
    }

    if (s_usart0_tx_head > s_usart0_tx_tail)
    {
        contig_len = (uint16_t)(s_usart0_tx_head - s_usart0_tx_tail);
    }
    else
    {
        contig_len = (uint16_t)(USART0_DMA_TX_RING_SIZE - s_usart0_tx_tail);
    }

    if (contig_len == 0U)
    {
        return;
    }

    dma_channel_disable(DMA0, DMA_CH1);
    dma_flag_clear(DMA0, DMA_CH1, DMA_FLAG_G);
    dma_memory_address_config(DMA0, DMA_CH1, (uint32_t)&s_usart0_tx_ring[s_usart0_tx_tail]);
    dma_transfer_number_config(DMA0, DMA_CH1, contig_len);
    s_usart0_tx_dma_len = contig_len;
    s_usart0_tx_dma_busy = 1U;
#if USART0_DMA_TX_ENABLE_TC_FINISH
    s_usart0_tx_wait_tc = 0U;
    usart_interrupt_disable(USART0, USART_INT_TC);
#endif
    dma_channel_enable(DMA0, DMA_CH1);
}

void usart0_dma_tx_init(void)
{
    dma_parameter_struct dma_parameter;

    s_usart0_tx_head = 0U;
    s_usart0_tx_tail = 0U;
    s_usart0_tx_dma_len = 0U;
    s_usart0_tx_dma_busy = 0U;
#if USART0_DMA_TX_ENABLE_TC_FINISH
    s_usart0_tx_wait_tc = 0U;
#endif

    dma_deinit(DMA0, DMA_CH1);
    dma_parameter.periph_addr = (uint32_t)(uint32_t *)&USART_TDATA(USART0);
    dma_parameter.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_parameter.memory_addr = 0U;
    dma_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_parameter.number = 0U;
    dma_parameter.priority = DMA_PRIORITY_MEDIUM;
    dma_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_parameter.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_parameter.request = DMA_REQUEST_USART0_TX; /* MUXID = 25 */
    dma_init(DMA0, DMA_CH1, &dma_parameter);

    dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_G);
    dma_interrupt_enable(DMA0, DMA_CH1, DMA_INT_FTF);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
    nvic_irq_enable(DMA0_Channel1_IRQn, 1U, 0U);
#if USART0_DMA_TX_ENABLE_TC_FINISH
    nvic_irq_enable(USART0_IRQn, 2U, 0U);
    usart_interrupt_disable(USART0, USART_INT_TC);
#endif
}

int usart0_tx_by_dma(const uint8_t *ptr, uint16_t len)
{
    uint32_t primask = 0U;
    uint16_t free_len = 0U;
    uint16_t first_copy = 0U;
    uint16_t second_copy = 0U;

    if ((ptr == NULL) || (len == 0U))
    {
        return USART0_TX_DMA_ERR_PARAM;
    }

    primask = usart0_dma_tx_lock();

    free_len = usart0_ring_free_unsafe();
    if (len > free_len)
    {
        usart0_dma_tx_unlock(primask);
        return USART0_TX_DMA_ERR_FULL;
    }

    first_copy = (uint16_t)(USART0_DMA_TX_RING_SIZE - s_usart0_tx_head);
    if (first_copy > len)
    {
        first_copy = len;
    }
    second_copy = (uint16_t)(len - first_copy);

    memcpy(&s_usart0_tx_ring[s_usart0_tx_head], ptr, first_copy);
    if (second_copy > 0U)
    {
        memcpy(&s_usart0_tx_ring[0], ptr + first_copy, second_copy);
    }

    s_usart0_tx_head = (uint16_t)((s_usart0_tx_head + len) % USART0_DMA_TX_RING_SIZE);

    usart0_dma_tx_kick();
    usart0_dma_tx_unlock(primask);

    return USART0_TX_DMA_OK;
}

void usart0_tx_by_dma_cb(char *ptr, int len)
{
    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }
    (void)usart0_tx_by_dma((const uint8_t *)ptr, (uint16_t)len);
}

void DMA0_Channel1_IRQHandler(void)
{
    if (dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INT_FLAG_FTF) != RESET)
    {
        dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_G);
        dma_channel_disable(DMA0, DMA_CH1);

        s_usart0_tx_tail = (uint16_t)((s_usart0_tx_tail + s_usart0_tx_dma_len) % USART0_DMA_TX_RING_SIZE);
        s_usart0_tx_dma_len = 0U;
        s_usart0_tx_dma_busy = 0U;

        if (s_usart0_tx_head != s_usart0_tx_tail)
        {
            usart0_dma_tx_kick();
        }
#if USART0_DMA_TX_ENABLE_TC_FINISH
        else
        {
            s_usart0_tx_wait_tc = 1U;
            usart_flag_clear(USART0, USART_FLAG_TC);
            usart_interrupt_enable(USART0, USART_INT_TC);
        }
#endif
    }
}

#if USART0_DMA_TX_ENABLE_TC_FINISH
void USART0_IRQHandler(void)
{
    if ((s_usart0_tx_wait_tc != 0U) &&
        (usart_interrupt_flag_get(USART0, USART_INT_FLAG_TC) != RESET))
    {
        usart_interrupt_flag_clear(USART0, USART_INT_FLAG_TC);
        usart_interrupt_disable(USART0, USART_INT_TC);
        s_usart0_tx_wait_tc = 0U;
        /* Optional hook point: RS485 DE low, tx-done callback, etc. */
    }
}
#endif

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
    usart0_dma_tx_init();
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
