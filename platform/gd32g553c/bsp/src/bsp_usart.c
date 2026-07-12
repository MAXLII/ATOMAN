#include "bsp_usart.h"
#include "gd32g5x3.h"
#include "section.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BSP_USART_DBG_UNIT (USART0)
#define BSP_USART_DBG_RCU (RCU_USART0)
#define BSP_USART_DBG_CLK_IDX (IDX_USART0)
#define BSP_USART_DBG_TX_GPIO_RCU (RCU_GPIOA)
#define BSP_USART_DBG_RX_GPIO_RCU (RCU_GPIOA)
#define BSP_USART_DBG_TX_PORT (GPIOA)
#define BSP_USART_DBG_TX_PIN (GPIO_PIN_9)
#define BSP_USART_DBG_RX_PORT (GPIOA)
#define BSP_USART_DBG_RX_PIN (GPIO_PIN_10)
#define BSP_USART_DBG_GPIO_AF (GPIO_AF_7)
#define BSP_USART_DBG_BAUDRATE (921600UL)
#define BSP_USART_DBG_RX_DMA_CH (DMA_CH0)
#define BSP_USART_DBG_TX_DMA_CH (DMA_CH1)
#define BSP_USART_DBG_RX_DMA_REQUEST (DMA_REQUEST_USART0_RX)
#define BSP_USART_DBG_TX_DMA_REQUEST (DMA_REQUEST_USART0_TX)

#define BSP_USART_ISO_UNIT (USART2)
#define BSP_USART_ISO_RCU (RCU_USART2)
#define BSP_USART_ISO_CLK_IDX (IDX_USART2)
#define BSP_USART_ISO_TX_GPIO_RCU (RCU_GPIOB)
#define BSP_USART_ISO_RX_GPIO_RCU (RCU_GPIOB)
#define BSP_USART_ISO_TX_PORT (GPIOB)
#define BSP_USART_ISO_TX_PIN (GPIO_PIN_10)
#define BSP_USART_ISO_RX_PORT (GPIOB)
#define BSP_USART_ISO_RX_PIN (GPIO_PIN_11)
#define BSP_USART_ISO_GPIO_AF (GPIO_AF_7)
#define BSP_USART_ISO_BAUDRATE (921600UL)
#define BSP_USART_ISO_RX_DMA_CH (DMA_CH2)
#define BSP_USART_ISO_TX_DMA_CH (DMA_CH3)
#define BSP_USART_ISO_RX_DMA_REQUEST (DMA_REQUEST_USART2_RX)
#define BSP_USART_ISO_TX_DMA_REQUEST (DMA_REQUEST_USART2_TX)

#define BSP_USART_RX_DMA_BUF_SIZE (512U)
#define BSP_USART_TX_RING_BUF_SIZE (1024U)
#define BSP_USART_TX_DMA_TIMEOUT (0x00FFFFFFUL)
#define BSP_USART_DBG_PRINTF_BUF_SIZE (256U)
#define BSP_USART_ISO_PRINTF_BUF_SIZE (256U)

typedef struct
{
    uint8_t rx_dma_buf[BSP_USART_RX_DMA_BUF_SIZE];
    uint16_t rx_read_idx;
    uint8_t tx_ring_buf[BSP_USART_TX_RING_BUF_SIZE];
    volatile uint16_t tx_ring_head;
    volatile uint16_t tx_ring_tail;
    volatile uint16_t tx_dma_len;
    volatile uint8_t tx_dma_busy;
} bsp_usart_port_ctx_t;

typedef struct
{
    uint32_t unit;
    rcu_periph_enum usart_rcu;
    usart_idx_enum clk_idx;
    rcu_periph_enum tx_gpio_rcu;
    rcu_periph_enum rx_gpio_rcu;
    uint32_t tx_port;
    uint32_t tx_pin;
    uint32_t rx_port;
    uint32_t rx_pin;
    uint32_t gpio_af;
    uint32_t baudrate;
    dma_channel_enum rx_dma_ch;
    dma_channel_enum tx_dma_ch;
    uint32_t rx_dma_request;
    uint32_t tx_dma_request;
    uint16_t rx_dma_buf_size;
    uint16_t tx_ring_buf_size;
    uint32_t tx_dma_timeout;
} bsp_usart_port_cfg_t;

static bsp_usart_port_ctx_t s_bsp_usart_dbg_ctx = {0};
static bsp_usart_port_ctx_t s_bsp_usart_iso_ctx = {0};

static const bsp_usart_port_cfg_t s_bsp_usart_dbg_cfg = {
    .unit = BSP_USART_DBG_UNIT,
    .usart_rcu = BSP_USART_DBG_RCU,
    .clk_idx = BSP_USART_DBG_CLK_IDX,
    .tx_gpio_rcu = BSP_USART_DBG_TX_GPIO_RCU,
    .rx_gpio_rcu = BSP_USART_DBG_RX_GPIO_RCU,
    .tx_port = BSP_USART_DBG_TX_PORT,
    .tx_pin = BSP_USART_DBG_TX_PIN,
    .rx_port = BSP_USART_DBG_RX_PORT,
    .rx_pin = BSP_USART_DBG_RX_PIN,
    .gpio_af = BSP_USART_DBG_GPIO_AF,
    .baudrate = BSP_USART_DBG_BAUDRATE,
    .rx_dma_ch = BSP_USART_DBG_RX_DMA_CH,
    .tx_dma_ch = BSP_USART_DBG_TX_DMA_CH,
    .rx_dma_request = BSP_USART_DBG_RX_DMA_REQUEST,
    .tx_dma_request = BSP_USART_DBG_TX_DMA_REQUEST,
    .rx_dma_buf_size = BSP_USART_RX_DMA_BUF_SIZE,
    .tx_ring_buf_size = BSP_USART_TX_RING_BUF_SIZE,
    .tx_dma_timeout = BSP_USART_TX_DMA_TIMEOUT,
};

static const bsp_usart_port_cfg_t s_bsp_usart_iso_cfg = {
    .unit = BSP_USART_ISO_UNIT,
    .usart_rcu = BSP_USART_ISO_RCU,
    .clk_idx = BSP_USART_ISO_CLK_IDX,
    .tx_gpio_rcu = BSP_USART_ISO_TX_GPIO_RCU,
    .rx_gpio_rcu = BSP_USART_ISO_RX_GPIO_RCU,
    .tx_port = BSP_USART_ISO_TX_PORT,
    .tx_pin = BSP_USART_ISO_TX_PIN,
    .rx_port = BSP_USART_ISO_RX_PORT,
    .rx_pin = BSP_USART_ISO_RX_PIN,
    .gpio_af = BSP_USART_ISO_GPIO_AF,
    .baudrate = BSP_USART_ISO_BAUDRATE,
    .rx_dma_ch = BSP_USART_ISO_RX_DMA_CH,
    .tx_dma_ch = BSP_USART_ISO_TX_DMA_CH,
    .rx_dma_request = BSP_USART_ISO_RX_DMA_REQUEST,
    .tx_dma_request = BSP_USART_ISO_TX_DMA_REQUEST,
    .rx_dma_buf_size = BSP_USART_RX_DMA_BUF_SIZE,
    .tx_ring_buf_size = BSP_USART_TX_RING_BUF_SIZE,
    .tx_dma_timeout = BSP_USART_TX_DMA_TIMEOUT,
};

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

static uint16_t bsp_usart_tx_ring_used(const bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    uint16_t head = ctx->tx_ring_head;
    uint16_t tail = ctx->tx_ring_tail;

    if (head >= tail)
    {
        return (uint16_t)(head - tail);
    }

    return (uint16_t)(cfg->tx_ring_buf_size - tail + head);
}

static uint16_t bsp_usart_tx_ring_free(const bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    return (uint16_t)(cfg->tx_ring_buf_size - bsp_usart_tx_ring_used(ctx, cfg) - 1U);
}

static void bsp_usart_rx_dma_init(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    dma_parameter_struct dma_parameter;

    dma_channel_disable(DMA0, cfg->rx_dma_ch);
    dma_deinit(DMA0, cfg->rx_dma_ch);
    dma_parameter.periph_addr = (uint32_t)(uint32_t *)&USART_RDATA(cfg->unit);
    dma_parameter.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_parameter.memory_addr = (uint32_t)ctx->rx_dma_buf;
    dma_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_parameter.number = cfg->rx_dma_buf_size;
    dma_parameter.priority = DMA_PRIORITY_MEDIUM;
    dma_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_parameter.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_parameter.request = cfg->rx_dma_request;
    dma_init(DMA0, cfg->rx_dma_ch, &dma_parameter);
    dma_circulation_enable(DMA0, cfg->rx_dma_ch);
    dma_flag_clear(DMA0, cfg->rx_dma_ch, DMA_FLAG_G);
    dma_channel_enable(DMA0, cfg->rx_dma_ch);
}

static void bsp_usart_tx_dma_init(const bsp_usart_port_cfg_t *cfg)
{
    dma_parameter_struct dma_parameter;

    dma_channel_disable(DMA0, cfg->tx_dma_ch);
    dma_deinit(DMA0, cfg->tx_dma_ch);
    dma_parameter.periph_addr = (uint32_t)(uint32_t *)&USART_TDATA(cfg->unit);
    dma_parameter.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_parameter.memory_addr = 0U;
    dma_parameter.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_parameter.number = 0U;
    dma_parameter.priority = DMA_PRIORITY_MEDIUM;
    dma_parameter.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_parameter.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_parameter.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_parameter.request = cfg->tx_dma_request;
    dma_init(DMA0, cfg->tx_dma_ch, &dma_parameter);
    dma_flag_clear(DMA0, cfg->tx_dma_ch, DMA_FLAG_G);
}

static uint16_t bsp_usart_rx_dma_get_write_idx(const bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    uint16_t write_idx = (uint16_t)(cfg->rx_dma_buf_size - DMA_CHCNT(DMA0, cfg->rx_dma_ch));

    (void)ctx;
    if (write_idx >= cfg->rx_dma_buf_size)
    {
        write_idx = 0U;
    }
    return write_idx;
}

static uint8_t bsp_usart_rx_has_error(const bsp_usart_port_cfg_t *cfg)
{
    return (uint8_t)((usart_flag_get(cfg->unit, USART_FLAG_ORERR) == SET) ||
                     (usart_flag_get(cfg->unit, USART_FLAG_NERR) == SET) ||
                     (usart_flag_get(cfg->unit, USART_FLAG_FERR) == SET) ||
                     (usart_flag_get(cfg->unit, USART_FLAG_PERR) == SET));
}

static void bsp_usart_rx_recover_if_error(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    volatile uint32_t dummy;

    if (bsp_usart_rx_has_error(cfg) == 0U)
    {
        return;
    }

    dma_channel_disable(DMA0, cfg->rx_dma_ch);
    dummy = USART_RDATA(cfg->unit);
    (void)dummy;

    usart_flag_clear(cfg->unit, USART_FLAG_ORERR);
    usart_flag_clear(cfg->unit, USART_FLAG_NERR);
    usart_flag_clear(cfg->unit, USART_FLAG_FERR);
    usart_flag_clear(cfg->unit, USART_FLAG_PERR);

    memset(ctx->rx_dma_buf, 0, cfg->rx_dma_buf_size);
    ctx->rx_read_idx = 0U;
    bsp_usart_rx_dma_init(ctx, cfg);
}

static uint16_t bsp_usart_tx_ring_read(bsp_usart_port_ctx_t *ctx,
                                       const bsp_usart_port_cfg_t *cfg,
                                       uint8_t **pp_data)
{
    uint16_t read_len;

    if ((ctx == NULL) || (cfg == NULL) || (pp_data == NULL) || (ctx->tx_ring_head == ctx->tx_ring_tail))
    {
        return 0U;
    }

    if (ctx->tx_ring_head > ctx->tx_ring_tail)
    {
        read_len = (uint16_t)(ctx->tx_ring_head - ctx->tx_ring_tail);
    }
    else
    {
        read_len = (uint16_t)(cfg->tx_ring_buf_size - ctx->tx_ring_tail);
    }

    *pp_data = &ctx->tx_ring_buf[ctx->tx_ring_tail];
    return read_len;
}

static void bsp_usart_tx_dma_start(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    uint8_t *p_data = NULL;
    uint16_t len;

    if ((ctx == NULL) || (cfg == NULL) || (ctx->tx_dma_busy != 0U))
    {
        return;
    }

    len = bsp_usart_tx_ring_read(ctx, cfg, &p_data);
    if ((len == 0U) || (p_data == NULL))
    {
        return;
    }

    dma_channel_disable(DMA0, cfg->tx_dma_ch);
    dma_flag_clear(DMA0, cfg->tx_dma_ch, DMA_FLAG_G);
    dma_memory_address_config(DMA0, cfg->tx_dma_ch, (uint32_t)p_data);
    dma_transfer_number_config(DMA0, cfg->tx_dma_ch, len);
    usart_flag_clear(cfg->unit, USART_FLAG_TC);

    ctx->tx_dma_len = len;
    ctx->tx_dma_busy = 1U;
    dma_channel_enable(DMA0, cfg->tx_dma_ch);
}

static void bsp_usart_tx_dma_service(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    if ((ctx == NULL) || (cfg == NULL))
    {
        return;
    }

    if (ctx->tx_dma_busy != 0U)
    {
        if ((dma_flag_get(DMA0, cfg->tx_dma_ch, DMA_FLAG_FTF) == RESET) ||
            (usart_flag_get(cfg->unit, USART_FLAG_TC) == RESET))
        {
            return;
        }

        dma_flag_clear(DMA0, cfg->tx_dma_ch, DMA_FLAG_G);
        dma_channel_disable(DMA0, cfg->tx_dma_ch);
        ctx->tx_ring_tail = (uint16_t)((ctx->tx_ring_tail + ctx->tx_dma_len) % cfg->tx_ring_buf_size);
        ctx->tx_dma_len = 0U;
        ctx->tx_dma_busy = 0U;
    }

    bsp_usart_tx_dma_start(ctx, cfg);
}

static uint16_t bsp_usart_tx_ring_write(bsp_usart_port_ctx_t *ctx,
                                        const bsp_usart_port_cfg_t *cfg,
                                        const uint8_t *data,
                                        uint16_t len)
{
    uint16_t offset = 0U;

    if ((ctx == NULL) || (cfg == NULL) || (data == NULL))
    {
        return 0U;
    }

    while (offset < len)
    {
        uint16_t free_len = bsp_usart_tx_ring_free(ctx, cfg);
        uint16_t to_end = (uint16_t)(cfg->tx_ring_buf_size - ctx->tx_ring_head);
        uint16_t write_len = (uint16_t)(len - offset);

        if (free_len == 0U)
        {
            break;
        }
        if (write_len > free_len)
        {
            write_len = free_len;
        }
        if (write_len > to_end)
        {
            write_len = to_end;
        }

        memcpy(&ctx->tx_ring_buf[ctx->tx_ring_head], &data[offset], write_len);
        ctx->tx_ring_head = (uint16_t)((ctx->tx_ring_head + write_len) % cfg->tx_ring_buf_size);
        offset = (uint16_t)(offset + write_len);
    }

    return offset;
}

static uint16_t bsp_usart_write(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg, const uint8_t *data, uint16_t len)
{
    uint32_t primask;
    uint32_t timeout;

    if ((ctx == NULL) || (cfg == NULL) || (data == NULL) || (len == 0U))
    {
        return 0U;
    }

    if (len >= cfg->tx_ring_buf_size)
    {
        return 0U;
    }

    timeout = cfg->tx_dma_timeout;
    while (1)
    {
        uint16_t free_len;
        uint16_t written;

        bsp_usart_tx_dma_service(ctx, cfg);

        primask = bsp_usart_irq_lock();
        free_len = bsp_usart_tx_ring_free(ctx, cfg);
        if (len <= free_len)
        {
            written = bsp_usart_tx_ring_write(ctx, cfg, data, len);
            bsp_usart_tx_dma_start(ctx, cfg);
            bsp_usart_irq_unlock(primask);
            return written;
        }

        bsp_usart_tx_dma_start(ctx, cfg);
        bsp_usart_irq_unlock(primask);

        if (timeout == 0U)
        {
            break;
        }
        timeout--;
    }

    return 0U;
}

static int32_t bsp_usart_read_byte(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg, uint8_t *data)
{
    uint16_t write_idx;

    if ((ctx == NULL) || (cfg == NULL) || (data == NULL))
    {
        return -1;
    }

    bsp_usart_rx_recover_if_error(ctx, cfg);
    write_idx = bsp_usart_rx_dma_get_write_idx(ctx, cfg);

    if (ctx->rx_read_idx != write_idx)
    {
        *data = ctx->rx_dma_buf[ctx->rx_read_idx];
        ctx->rx_read_idx++;
        if (ctx->rx_read_idx >= cfg->rx_dma_buf_size)
        {
            ctx->rx_read_idx = 0U;
        }
        return 0;
    }

    return -2;
}

static void bsp_usart_port_gpio_init(const bsp_usart_port_cfg_t *cfg)
{
    rcu_periph_clock_enable(cfg->tx_gpio_rcu);
    rcu_periph_clock_enable(cfg->rx_gpio_rcu);

    gpio_af_set(cfg->tx_port, cfg->gpio_af, cfg->tx_pin);
    gpio_af_set(cfg->rx_port, cfg->gpio_af, cfg->rx_pin);

    gpio_mode_set(cfg->tx_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, cfg->tx_pin);
    gpio_output_options_set(cfg->tx_port, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, cfg->tx_pin);

    gpio_mode_set(cfg->rx_port, GPIO_MODE_AF, GPIO_PUPD_PULLUP, cfg->rx_pin);
    gpio_output_options_set(cfg->rx_port, GPIO_OTYPE_PP, GPIO_OSPEED_12MHZ, cfg->rx_pin);
}

static void bsp_usart_port_init(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    if ((ctx == NULL) || (cfg == NULL))
    {
        return;
    }

    rcu_periph_clock_enable(RCU_DMA0);
    rcu_periph_clock_enable(RCU_DMAMUX);
    rcu_periph_clock_enable(cfg->usart_rcu);
    rcu_usart_clock_config(cfg->clk_idx, RCU_USARTSRC_APB);

    bsp_usart_port_gpio_init(cfg);

    usart_deinit(cfg->unit);
    usart_word_length_set(cfg->unit, USART_WL_8BIT);
    usart_stop_bit_set(cfg->unit, USART_STB_1BIT);
    usart_parity_config(cfg->unit, USART_PM_NONE);
    usart_oversample_config(cfg->unit, USART_OVSMOD_16);
    usart_fifo_disable(cfg->unit);
    usart_dma_receive_config(cfg->unit, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(cfg->unit, USART_TRANSMIT_DMA_ENABLE);
    usart_baudrate_set(cfg->unit, cfg->baudrate);
    usart_enable(cfg->unit);
    usart_receive_config(cfg->unit, USART_RECEIVE_ENABLE);
    usart_transmit_config(cfg->unit, USART_TRANSMIT_ENABLE);

    ctx->rx_read_idx = 0U;
    ctx->tx_ring_head = 0U;
    ctx->tx_ring_tail = 0U;
    ctx->tx_dma_len = 0U;
    ctx->tx_dma_busy = 0U;
    memset(ctx->rx_dma_buf, 0, cfg->rx_dma_buf_size);
    memset(ctx->tx_ring_buf, 0, cfg->tx_ring_buf_size);

    bsp_usart_rx_dma_init(ctx, cfg);
    bsp_usart_tx_dma_init(cfg);
}

static void bsp_usart_dbg_init(void)
{
    bsp_usart_port_init(&s_bsp_usart_dbg_ctx, &s_bsp_usart_dbg_cfg);
}

REG_INIT(0, bsp_usart_dbg_init)

static void bsp_usart_iso_init(void)
{
    bsp_usart_port_init(&s_bsp_usart_iso_ctx, &s_bsp_usart_iso_cfg);
}

REG_INIT(0, bsp_usart_iso_init)

static void bsp_usart_dbg_tx_service_task(void)
{
    bsp_usart_tx_dma_service(&s_bsp_usart_dbg_ctx, &s_bsp_usart_dbg_cfg);
}

REG_TASK(100, bsp_usart_dbg_tx_service_task)

static void bsp_usart_iso_tx_service_task(void)
{
    bsp_usart_tx_dma_service(&s_bsp_usart_iso_ctx, &s_bsp_usart_iso_cfg);
}

REG_TASK(100, bsp_usart_iso_tx_service_task)

void bsp_usart_dbg_tx(char *ptr, int len)
{
    uint16_t req_len;

    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    req_len = ((uint32_t)len > 0xFFFFUL) ? 0xFFFFU : (uint16_t)len;
    (void)bsp_usart_write(&s_bsp_usart_dbg_ctx, &s_bsp_usart_dbg_cfg, (const uint8_t *)ptr, req_len);
}

void bsp_usart_dbg_printf(const char *__format, ...)
{
    va_list args;
    int len;
    char buf[BSP_USART_DBG_PRINTF_BUF_SIZE];

    if (__format == NULL)
    {
        return;
    }

    va_start(args, __format);
    len = vsnprintf(buf, sizeof(buf), __format, args);
    va_end(args);

    if (len <= 0)
    {
        return;
    }
    if (len > (int)(sizeof(buf) - 1U))
    {
        len = (int)(sizeof(buf) - 1U);
    }

    bsp_usart_dbg_tx(buf, len);
}

uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data)
{
    return (bsp_usart_read_byte(&s_bsp_usart_dbg_ctx, &s_bsp_usart_dbg_cfg, p_data) == 0) ? 1U : 0U;
}

void bsp_usart_iso_tx(char *ptr, int len)
{
    uint16_t req_len;

    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    req_len = ((uint32_t)len > 0xFFFFUL) ? 0xFFFFU : (uint16_t)len;
    (void)bsp_usart_write(&s_bsp_usart_iso_ctx, &s_bsp_usart_iso_cfg, (const uint8_t *)ptr, req_len);
}

void bsp_usart_iso_printf(const char *__format, ...)
{
    va_list args;
    int len;
    char buf[BSP_USART_ISO_PRINTF_BUF_SIZE];

    if (__format == NULL)
    {
        return;
    }

    va_start(args, __format);
    len = vsnprintf(buf, sizeof(buf), __format, args);
    va_end(args);

    if (len <= 0)
    {
        return;
    }
    if (len > (int)(sizeof(buf) - 1U))
    {
        len = (int)(sizeof(buf) - 1U);
    }

    bsp_usart_iso_tx(buf, len);
}

uint8_t bsp_usart_iso_rx_get_byte(uint8_t *p_data)
{
    return (bsp_usart_read_byte(&s_bsp_usart_iso_ctx, &s_bsp_usart_iso_cfg, p_data) == 0) ? 1U : 0U;
}

int bsp_usart_dbg_tx_dma(const uint8_t *p_data, uint32_t len)
{
    if ((p_data == NULL) || (len > 0xFFFFUL))
    {
        return -1;
    }

    bsp_usart_dbg_tx((char *)p_data, (int)len);
    return 0;
}
