#include "bsp_usart.h"
#include "gd32g5x3.h"
#include "section.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef BSP_USART_DBG_RX_RING_SIZE
#define BSP_USART_DBG_RX_RING_SIZE 512U
#endif

#ifndef BSP_USART_DBG_TX_RING_SIZE
#define BSP_USART_DBG_TX_RING_SIZE 512U
#endif

#ifndef BSP_USART_DBG_ENABLE_TC_FINISH
#define BSP_USART_DBG_ENABLE_TC_FINISH 0U
#endif

typedef struct
{
    uint8_t ring[BSP_USART_DBG_TX_RING_SIZE];
    uint16_t ring_size;
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t dma_len;
    volatile uint8_t dma_busy;
    volatile uint8_t wait_tc;
} bsp_usart_dbg_dma_tx_t;

static bsp_usart_dbg_dma_tx_t s_usart_dbg_tx = {
    .ring = {0},
    .ring_size = (uint16_t)BSP_USART_DBG_TX_RING_SIZE,
    .head = 0U,
    .tail = 0U,
    .dma_len = 0U,
    .dma_busy = 0U,
    .wait_tc = 0U,
};

static uint8_t s_usart_dbg_rx_ring[BSP_USART_DBG_RX_RING_SIZE] = {0};
static volatile uint16_t s_usart_dbg_rx_pos = 0U;

static uint32_t bsp_usart_irq_lock(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    return primask;
}

static void bsp_usart_irq_unlock(uint32_t primask)
{
    if ((primask & 0x1U) == 0U)
    {
        __enable_irq();
    }
}

static uint16_t bsp_usart_tx_ring_count_unsafe(const bsp_usart_dbg_dma_tx_t *p_tx)
{
    if ((p_tx == NULL) || (p_tx->ring_size == 0U))
    {
        return 0U;
    }

    if (p_tx->head >= p_tx->tail)
    {
        return (uint16_t)(p_tx->head - p_tx->tail);
    }

    return (uint16_t)(p_tx->ring_size - (p_tx->tail - p_tx->head));
}

static uint16_t bsp_usart_tx_ring_free_unsafe(const bsp_usart_dbg_dma_tx_t *p_tx)
{
    if ((p_tx == NULL) || (p_tx->ring_size <= 1U))
    {
        return 0U;
    }

    return (uint16_t)((p_tx->ring_size - 1U) - bsp_usart_tx_ring_count_unsafe(p_tx));
}

static void bsp_usart_dbg_tx_dma_kick(void)
{
    uint16_t contig_len = 0U;

    if ((s_usart_dbg_tx.dma_busy != 0U) || (s_usart_dbg_tx.head == s_usart_dbg_tx.tail))
    {
        return;
    }

    if (s_usart_dbg_tx.head > s_usart_dbg_tx.tail)
    {
        contig_len = (uint16_t)(s_usart_dbg_tx.head - s_usart_dbg_tx.tail);
    }
    else
    {
        contig_len = (uint16_t)(s_usart_dbg_tx.ring_size - s_usart_dbg_tx.tail);
    }

    if (contig_len == 0U)
    {
        return;
    }

    dma_channel_disable(DMA0, DMA_CH1);
    dma_flag_clear(DMA0, DMA_CH1, DMA_FLAG_G);
    dma_memory_address_config(DMA0, DMA_CH1, (uint32_t)&s_usart_dbg_tx.ring[s_usart_dbg_tx.tail]);
    dma_transfer_number_config(DMA0, DMA_CH1, contig_len);

    s_usart_dbg_tx.dma_len = contig_len;
    s_usart_dbg_tx.dma_busy = 1U;
    s_usart_dbg_tx.wait_tc = 0U;

    if (BSP_USART_DBG_ENABLE_TC_FINISH != 0U)
    {
        usart_interrupt_disable(USART0, USART_INT_TC);
    }

    dma_channel_enable(DMA0, DMA_CH1);
}

static void bsp_usart_dbg_dma_tx_init(void)
{
    dma_parameter_struct dma_parameter;

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
    dma_parameter.request = DMA_REQUEST_USART0_TX;
    dma_init(DMA0, DMA_CH1, &dma_parameter);

    dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_G);
    dma_interrupt_enable(DMA0, DMA_CH1, DMA_INT_FTF);
    NVIC_ClearPendingIRQ(DMA0_Channel1_IRQn);
    NVIC_SetPriority(DMA0_Channel1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2U, 0U));
    NVIC_EnableIRQ(DMA0_Channel1_IRQn);

    NVIC_ClearPendingIRQ(USART0_IRQn);
    NVIC_SetPriority(USART0_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 2U, 0U));
    NVIC_EnableIRQ(USART0_IRQn);
}

static void bsp_usart_blocking_write(const uint8_t *p_data, uint32_t len)
{
    uint32_t i;

    if ((p_data == NULL) || (len == 0U))
    {
        return;
    }

    for (i = 0U; i < len; ++i)
    {
        uint32_t timeout = 10000U;

        while (usart_flag_get(USART0, USART_FLAG_TBE) == RESET)
        {
            if (--timeout == 0U)
            {
                break;
            }
        }

        usart_data_transmit(USART0, p_data[i]);
    }
}

void bsp_usart_init(void)
{
    dma_parameter_struct dma_parameter;

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(USART0_GPIO_RCU);

    rcu_usart_clock_config(IDX_USART0, RCU_USARTSRC_APB);
    usart_deinit(USART0);

    gpio_af_set(USART0_TX_PORT, GPIO_AF_7, USART0_TX_PIN);
    gpio_af_set(USART0_RX_PORT, GPIO_AF_7, USART0_RX_PIN);

    gpio_mode_set(USART0_TX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_TX_PIN);
    gpio_output_options_set(USART0_TX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, USART0_TX_PIN);

    gpio_mode_set(USART0_RX_PORT, GPIO_MODE_AF, GPIO_PUPD_PULLUP, USART0_RX_PIN);
    gpio_output_options_set(USART0_RX_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, USART0_RX_PIN);

    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_oversample_config(USART0, USART_OVSMOD_16);
    usart_fifo_disable(USART0);
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
    usart_baudrate_set(USART0, 115200U);

    dma_deinit(DMA0, DMA_CH0);
    dma_parameter.periph_addr = (uint32_t)(uint32_t *)&USART_RDATA(USART0);
    dma_parameter.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_parameter.memory_addr = (uint32_t)s_usart_dbg_rx_ring;
    dma_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_parameter.number = BSP_USART_DBG_RX_RING_SIZE;
    dma_parameter.priority = DMA_PRIORITY_MEDIUM;
    dma_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_parameter.request = DMA_REQUEST_USART0_RX;
    dma_init(DMA0, DMA_CH0, &dma_parameter);
    dma_circulation_enable(DMA0, DMA_CH0);

    usart_enable(USART0);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    dma_channel_enable(DMA0, DMA_CH0);

    bsp_usart_dbg_dma_tx_init();
}

REG_INIT(0, bsp_usart_init)

int bsp_usart_dbg_tx_dma(const uint8_t *p_data, uint32_t len)
{
    uint32_t primask = 0U;
    uint16_t free_len = 0U;
    uint16_t write_len = 0U;
    uint16_t first_copy = 0U;
    uint16_t second_copy = 0U;

    if ((p_data == NULL) || (len == 0U) || (len > 0xFFFFU))
    {
        return -1;
    }

    write_len = (uint16_t)len;
    primask = bsp_usart_irq_lock();

    free_len = bsp_usart_tx_ring_free_unsafe(&s_usart_dbg_tx);
    if (write_len > free_len)
    {
        bsp_usart_irq_unlock(primask);
        return -2;
    }

    first_copy = (uint16_t)(s_usart_dbg_tx.ring_size - s_usart_dbg_tx.head);
    if (first_copy > write_len)
    {
        first_copy = write_len;
    }
    second_copy = (uint16_t)(write_len - first_copy);

    memcpy(&s_usart_dbg_tx.ring[s_usart_dbg_tx.head], p_data, first_copy);
    if (second_copy > 0U)
    {
        memcpy(&s_usart_dbg_tx.ring[0], p_data + first_copy, second_copy);
    }

    s_usart_dbg_tx.head = (uint16_t)((s_usart_dbg_tx.head + write_len) % s_usart_dbg_tx.ring_size);
    bsp_usart_dbg_tx_dma_kick();
    bsp_usart_irq_unlock(primask);

    return 0;
}

uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data)
{
    uint16_t write_pos = 0U;
    uint16_t read_pos = 0U;

    if (p_data == NULL)
    {
        return 0U;
    }

    write_pos = (uint16_t)(BSP_USART_DBG_RX_RING_SIZE - DMA_CHCNT(DMA0, DMA_CH0));
    if (write_pos >= BSP_USART_DBG_RX_RING_SIZE)
    {
        write_pos = 0U;
    }

    read_pos = s_usart_dbg_rx_pos;
    if (read_pos == write_pos)
    {
        return 0U;
    }

    *p_data = s_usart_dbg_rx_ring[read_pos];
    read_pos++;
    if (read_pos >= BSP_USART_DBG_RX_RING_SIZE)
    {
        read_pos = 0U;
    }
    s_usart_dbg_rx_pos = read_pos;

    return 1U;
}

void bsp_usart_dbg_printf(const char *__format, ...)
{
    char buf[256];
    va_list args;
    int len;

    va_start(args, __format);
    len = vsnprintf(buf, sizeof(buf), __format, args);
    va_end(args);

    if (len <= 0)
    {
        return;
    }

    if ((uint32_t)len > sizeof(buf))
    {
        len = (int)sizeof(buf);
    }

    (void)bsp_usart_dbg_tx_dma((const uint8_t *)buf, (uint32_t)len);
}

void DMA0_Channel1_IRQHandler(void)
{
    if (dma_interrupt_flag_get(DMA0, DMA_CH1, DMA_INT_FLAG_FTF) == RESET)
    {
        return;
    }

    dma_interrupt_flag_clear(DMA0, DMA_CH1, DMA_INT_FLAG_G);
    dma_channel_disable(DMA0, DMA_CH1);

    s_usart_dbg_tx.tail = (uint16_t)((s_usart_dbg_tx.tail + s_usart_dbg_tx.dma_len) % s_usart_dbg_tx.ring_size);
    s_usart_dbg_tx.dma_len = 0U;
    s_usart_dbg_tx.dma_busy = 0U;

    if (s_usart_dbg_tx.head != s_usart_dbg_tx.tail)
    {
        bsp_usart_dbg_tx_dma_kick();
        return;
    }

    if (BSP_USART_DBG_ENABLE_TC_FINISH != 0U)
    {
        s_usart_dbg_tx.wait_tc = 1U;
        usart_flag_clear(USART0, USART_FLAG_TC);
        usart_interrupt_enable(USART0, USART_INT_TC);
    }
}

void USART0_IRQHandler(void)
{
    if ((BSP_USART_DBG_ENABLE_TC_FINISH == 0U) || (s_usart_dbg_tx.wait_tc == 0U))
    {
        return;
    }

    if (usart_interrupt_flag_get(USART0, USART_INT_FLAG_TC) == RESET)
    {
        return;
    }

    usart_interrupt_flag_clear(USART0, USART_INT_FLAG_TC);
    usart_interrupt_disable(USART0, USART_INT_TC);
    s_usart_dbg_tx.wait_tc = 0U;
}

int _write(int file, char *ptr, int len)
{
    (void)file;
    bsp_usart_blocking_write((const uint8_t *)ptr, (uint32_t)len);
    return len;
}
