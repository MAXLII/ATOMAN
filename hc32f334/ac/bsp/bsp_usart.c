#include "bsp_usart.h"
#include "section.h"
#include <stdarg.h>
#include <stdio.h>

#define BSP_USART1_UNIT (CM_USART1)
#define BSP_USART1_FCG (FCG3_PERIPH_USART1)

#define BSP_USART1_TX_PORT (GPIO_PORT_A)
#define BSP_USART1_TX_PIN (GPIO_PIN_06)
#define BSP_USART1_RX_PORT (GPIO_PORT_A)
#define BSP_USART1_RX_PIN (GPIO_PIN_07)

#define BSP_USART1_BAUDRATE (115200UL)
#define BSP_USART1_CLK_DIV (USART_CLK_DIV4)

#define BSP_USART1_RX_DMA_UNIT (CM_DMA)
#define BSP_USART1_RX_DMA_CH (DMA_CH2)
#define BSP_USART1_RX_DMA_MX_CH (DMA_MX_CH2)
#define BSP_USART1_RX_DMA_TRIG_SEL (AOS_DMA_2)
#define BSP_USART1_RX_DMA_TRIG_SRC (EVT_SRC_USART1_RI)
#define BSP_USART1_RX_DMA_BUF_SIZE (512U)

#define BSP_USART1_TX_DMA_UNIT (CM_DMA)
#define BSP_USART1_TX_DMA_CH (DMA_CH3)
#define BSP_USART1_TX_DMA_MX_CH (DMA_MX_CH3)
#define BSP_USART1_TX_DMA_TC_FLAG (DMA_FLAG_TC_CH3)
#define BSP_USART1_TX_DMA_CHSTAT_BUSY_MASK (DMA_CHSTAT_CHACT_3)
#define BSP_USART1_TX_DMA_TRIG_SEL (AOS_DMA_3)
#define BSP_USART1_TX_DMA_TRIG_SRC (EVT_SRC_USART1_TI)
#define BSP_USART1_TX_RING_BUF_SIZE (512U)
#define BSP_USART1_TX_DMA_BUF_SIZE (128U)
#define BSP_USART1_TX_DMA_TIMEOUT (0x00FFFFFFUL)

#define BSP_USART2_UNIT (CM_USART2)
#define BSP_USART2_FCG (FCG3_PERIPH_USART2)

#define BSP_USART2_TX_PORT (GPIO_PORT_B)
#define BSP_USART2_TX_PIN (GPIO_PIN_04)
#define BSP_USART2_RX_PORT (GPIO_PORT_B)
#define BSP_USART2_RX_PIN (GPIO_PIN_05)
#define BSP_USART2_PORT_FUNC (GPIO_FUNC_36)

#define BSP_USART2_BAUDRATE (115200UL)
#define BSP_USART2_CLK_DIV (USART_CLK_DIV4)

#define BSP_USART2_RX_DMA_UNIT (CM_DMA)
#define BSP_USART2_RX_DMA_CH (DMA_CH0)
#define BSP_USART2_RX_DMA_MX_CH (DMA_MX_CH0)
#define BSP_USART2_RX_DMA_TRIG_SRC (EVT_SRC_USART2_RI)
#define BSP_USART2_RX_DMA_BUF_SIZE (512U)

#define BSP_USART2_TX_DMA_UNIT (CM_DMA)
#define BSP_USART2_TX_DMA_CH (DMA_CH1)
#define BSP_USART2_TX_DMA_MX_CH (DMA_MX_CH1)
#define BSP_USART2_TX_DMA_TC_FLAG (DMA_FLAG_TC_CH1)
#define BSP_USART2_TX_DMA_CHSTAT_BUSY_MASK (DMA_CHSTAT_CHACT_1)
#define BSP_USART2_TX_DMA_TRIG_SRC (EVT_SRC_USART2_TI)
#define BSP_USART2_TX_RING_BUF_SIZE (512U)
#define BSP_USART2_TX_DMA_BUF_SIZE (128U)
#define BSP_USART2_TX_DMA_TIMEOUT (0x00FFFFFFUL)
#define BSP_USART2_TX_STRESS_PKT_SIZE (48U)
#define BSP_USART_DBG_PRINTF_BUF_SIZE (256U)
#define BSP_USART_ISO_PRINTF_BUF_SIZE (256U)
#define BSP_USART2_DMA_TEST_ENABLE (0U)
#if (BSP_USART2_DMA_TEST_ENABLE == 1U)
#define BSP_USART_TEST_LONG_PRINTF_PERIOD (31UL)
#endif

/* USART RX/TX use DMA plus polling service tasks; no USART/DMA NVIC interrupt is enabled here. */
typedef struct
{
    uint8_t rx_dma_buf[BSP_USART1_RX_DMA_BUF_SIZE];    /* RX circular DMA buffer. */
    uint16_t rx_read_idx;                              /* Next byte index consumed by software. */
    uint8_t tx_ring_buf[BSP_USART1_TX_RING_BUF_SIZE];  /* TX software ring buffer. */
    uint8_t tx_dma_buf[BSP_USART1_TX_DMA_BUF_SIZE];    /* Linear staging buffer for one TX DMA burst. */
    volatile uint16_t tx_ring_head;                    /* Next TX ring write index. */
    volatile uint16_t tx_ring_tail;                    /* Next TX ring read index. */
    volatile uint16_t tx_dma_len;                      /* Current TX DMA transfer length. */
    volatile uint8_t tx_dma_busy;                      /* TX DMA ownership flag set while a burst is active. */
} bsp_usart_port_ctx_t;

static bsp_usart_port_ctx_t s_bsp_usart1_ctx = {0};
static bsp_usart_port_ctx_t s_bsp_usart2_ctx = {0};

static uint16_t bsp_usart1_write(const uint8_t *data, uint16_t len);
static uint16_t bsp_usart2_write(const uint8_t *data, uint16_t len);
static int32_t bsp_usart1_read_byte(uint8_t *data);
static int32_t bsp_usart2_read_byte(uint8_t *data);

static uint8_t bsp_usart_tx_dma_chstat_is_idle(CM_DMA_TypeDef *dma, uint32_t chstat_busy_mask)
{
    return ((dma->CHSTAT & chstat_busy_mask) == 0UL) ? 1U : 0U;
}

/* Static hardware description for one USART port. The generic helpers use this
 * table plus bsp_usart_port_ctx_t so USART1 and USART2 keep the same logic. */
typedef struct
{
    CM_USART_TypeDef *unit;           /* USART peripheral instance. */
    uint32_t fcg;                     /* Peripheral clock gate mask. */
    uint8_t tx_port;                  /* TX GPIO port. */
    uint32_t tx_pin;                  /* TX GPIO pin. */
    uint8_t rx_port;                  /* RX GPIO port. */
    uint32_t rx_pin;                  /* RX GPIO pin. */
    uint16_t tx_func;                 /* TX pin alternate function. */
    uint16_t rx_func;                 /* RX pin alternate function. */
    uint32_t baudrate;                /* USART baud rate. */
    uint32_t clk_div;                 /* USART clock divider. */
    CM_DMA_TypeDef *rx_dma_unit;      /* RX DMA controller instance. */
    uint8_t rx_dma_ch;                /* RX DMA channel. */
    uint8_t rx_dma_mx_ch;             /* RX DMA mux channel. */
    uint32_t rx_dma_trig_sel;         /* RX DMA trigger select register value. */
    en_event_src_t rx_dma_trig_src;   /* RX DMA trigger event source. */
    uint16_t rx_dma_buf_size;         /* RX DMA circular buffer size. */
    CM_DMA_TypeDef *tx_dma_unit;      /* TX DMA controller instance. */
    uint8_t tx_dma_ch;                /* TX DMA channel. */
    uint8_t tx_dma_mx_ch;             /* TX DMA mux channel. */
    uint32_t tx_dma_tc_flag;          /* TX DMA transfer-complete flag. */
    uint32_t tx_dma_chstat_busy_mask; /* TX DMA channel busy status mask. */
    uint32_t tx_dma_trig_sel;         /* TX DMA trigger select register value. */
    en_event_src_t tx_dma_trig_src;   /* TX DMA trigger event source. */
    uint16_t tx_ring_buf_size;        /* TX software ring buffer size. */
    uint16_t tx_dma_buf_size;         /* TX DMA staging buffer size. */
    uint32_t tx_dma_timeout;          /* Maximum wait time for TX enqueue. */
} bsp_usart_port_cfg_t;

static const bsp_usart_port_cfg_t s_bsp_usart1_cfg = {
    .unit = BSP_USART1_UNIT,
    .fcg = BSP_USART1_FCG,
    .tx_port = BSP_USART1_TX_PORT,
    .tx_pin = BSP_USART1_TX_PIN,
    .rx_port = BSP_USART1_RX_PORT,
    .rx_pin = BSP_USART1_RX_PIN,
    .tx_func = GPIO_FUNC_36,
    .rx_func = GPIO_FUNC_37,
    .baudrate = BSP_USART1_BAUDRATE,
    .clk_div = BSP_USART1_CLK_DIV,
    .rx_dma_unit = BSP_USART1_RX_DMA_UNIT,
    .rx_dma_ch = BSP_USART1_RX_DMA_CH,
    .rx_dma_mx_ch = BSP_USART1_RX_DMA_MX_CH,
    .rx_dma_trig_sel = BSP_USART1_RX_DMA_TRIG_SEL,
    .rx_dma_trig_src = BSP_USART1_RX_DMA_TRIG_SRC,
    .rx_dma_buf_size = BSP_USART1_RX_DMA_BUF_SIZE,
    .tx_dma_unit = BSP_USART1_TX_DMA_UNIT,
    .tx_dma_ch = BSP_USART1_TX_DMA_CH,
    .tx_dma_mx_ch = BSP_USART1_TX_DMA_MX_CH,
    .tx_dma_tc_flag = BSP_USART1_TX_DMA_TC_FLAG,
    .tx_dma_chstat_busy_mask = BSP_USART1_TX_DMA_CHSTAT_BUSY_MASK,
    .tx_dma_trig_sel = BSP_USART1_TX_DMA_TRIG_SEL,
    .tx_dma_trig_src = BSP_USART1_TX_DMA_TRIG_SRC,
    .tx_ring_buf_size = BSP_USART1_TX_RING_BUF_SIZE,
    .tx_dma_buf_size = BSP_USART1_TX_DMA_BUF_SIZE,
    .tx_dma_timeout = BSP_USART1_TX_DMA_TIMEOUT,
};

static const bsp_usart_port_cfg_t s_bsp_usart2_cfg = {
    .unit = BSP_USART2_UNIT,
    .fcg = BSP_USART2_FCG,
    .tx_port = BSP_USART2_TX_PORT,
    .tx_pin = BSP_USART2_TX_PIN,
    .rx_port = BSP_USART2_RX_PORT,
    .rx_pin = BSP_USART2_RX_PIN,
    .tx_func = GPIO_FUNC_36,
    .rx_func = GPIO_FUNC_37,
    .baudrate = BSP_USART2_BAUDRATE,
    .clk_div = BSP_USART2_CLK_DIV,
    .rx_dma_unit = BSP_USART2_RX_DMA_UNIT,
    .rx_dma_ch = BSP_USART2_RX_DMA_CH,
    .rx_dma_mx_ch = BSP_USART2_RX_DMA_MX_CH,
    .rx_dma_trig_sel = AOS_DMA_0,
    .rx_dma_trig_src = BSP_USART2_RX_DMA_TRIG_SRC,
    .rx_dma_buf_size = BSP_USART2_RX_DMA_BUF_SIZE,
    .tx_dma_unit = BSP_USART2_TX_DMA_UNIT,
    .tx_dma_ch = BSP_USART2_TX_DMA_CH,
    .tx_dma_mx_ch = BSP_USART2_TX_DMA_MX_CH,
    .tx_dma_tc_flag = BSP_USART2_TX_DMA_TC_FLAG,
    .tx_dma_chstat_busy_mask = BSP_USART2_TX_DMA_CHSTAT_BUSY_MASK,
    .tx_dma_trig_sel = AOS_DMA_1,
    .tx_dma_trig_src = BSP_USART2_TX_DMA_TRIG_SRC,
    .tx_ring_buf_size = BSP_USART2_TX_RING_BUF_SIZE,
    .tx_dma_buf_size = BSP_USART2_TX_DMA_BUF_SIZE,
    .tx_dma_timeout = BSP_USART2_TX_DMA_TIMEOUT,
};

#if (BSP_USART2_DMA_TEST_ENABLE == 1U)
static char bsp_usart_hex_digit(uint8_t u8Val)
{
    if (u8Val < 10U)
    {
        return (char)('0' + (char)u8Val);
    }

    return (char)('A' + (char)(u8Val - 10U));
}

static uint16_t bsp_usart_append_hex32(uint8_t *pu8Buf, uint16_t u16Idx, uint32_t u32Val)
{
    int32_t i;

    for (i = 7; i >= 0; i--)
    {
        pu8Buf[u16Idx++] = (uint8_t)bsp_usart_hex_digit((uint8_t)((u32Val >> (uint32_t)(i * 4)) & 0x0FU));
    }

    return u16Idx;
}

typedef struct
{
    uint32_t req_bytes;      /* Total bytes requested by the test workload. */
    uint32_t sent_bytes;     /* Total bytes accepted into the TX path. */
    uint32_t drop_bytes;     /* Bytes rejected after the enqueue timeout expires. */
    uint32_t tx_frames;      /* Short TX frame count. */
    uint32_t printf_frames;  /* printf frame count, including long printf frames. */
    uint32_t printf_trunc;   /* printf frames truncated by the fixed printf buffer. */
} bsp_usart_test_port_stats_t;

static bsp_usart_test_port_stats_t s_bsp_usart_dbg_test_stats = {0};
static bsp_usart_test_port_stats_t s_bsp_usart_iso_test_stats = {0};

static void bsp_usart_test_record_tx(bsp_usart_test_port_stats_t *stats, uint16_t req_len, uint16_t sent_len)
{
    if (NULL == stats)
    {
        return;
    }

    stats->req_bytes += req_len;
    stats->sent_bytes += sent_len;
    if (sent_len < req_len)
    {
        stats->drop_bytes += (uint32_t)(req_len - sent_len);
    }
}

static void bsp_usart_test_record_printf_trunc(bsp_usart_test_port_stats_t *stats)
{
    if (NULL != stats)
    {
        stats->printf_trunc++;
    }
}
#endif

static uint16_t bsp_usart_tx_ring_used(const bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    uint16_t u16Head;
    uint16_t u16Tail;

    u16Head = ctx->tx_ring_head;
    u16Tail = ctx->tx_ring_tail;
    if (u16Head >= u16Tail)
    {
        return (uint16_t)(u16Head - u16Tail);
    }

    return (uint16_t)(cfg->tx_ring_buf_size - u16Tail + u16Head);
}

static uint16_t bsp_usart_tx_ring_free(const bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    return (uint16_t)(cfg->tx_ring_buf_size - bsp_usart_tx_ring_used(ctx, cfg) - 1U);
}

static void bsp_usart_rx_dma_init(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    stc_dma_init_t stcDmaInit;
    stc_dma_repeat_init_t stcDmaRepeatInit;

    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS | FCG0_PERIPH_DMA, ENABLE);

    /* RX runs as a circular DMA stream. Software only advances rx_read_idx;
     * the DMA destination address is used as the producer write index. */
    DMA_Cmd(cfg->rx_dma_unit, ENABLE);
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn = DMA_INT_DISABLE;
    stcDmaInit.u32SrcAddr = (uint32_t)(&cfg->unit->RDR);
    stcDmaInit.u32DestAddr = (uint32_t)(&ctx->rx_dma_buf[0]);
    stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32BlockSize = 1U;
    stcDmaInit.u32TransCount = 0xFFFFU;
    stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_FIX;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_INC;
    (void)DMA_Init(cfg->rx_dma_unit, cfg->rx_dma_ch, &stcDmaInit);

    (void)DMA_RepeatStructInit(&stcDmaRepeatInit);
    stcDmaRepeatInit.u32Mode = DMA_RPT_DEST;
    stcDmaRepeatInit.u32DestCount = cfg->rx_dma_buf_size;
    stcDmaRepeatInit.u32SrcCount = 0U;
    (void)DMA_RepeatInit(cfg->rx_dma_unit, cfg->rx_dma_ch, &stcDmaRepeatInit);

    AOS_SetTriggerEventSrc(cfg->rx_dma_trig_sel, cfg->rx_dma_trig_src);
    DMA_MxChCmd(cfg->rx_dma_unit, cfg->rx_dma_mx_ch, ENABLE);
    (void)DMA_ChCmd(cfg->rx_dma_unit, cfg->rx_dma_ch, ENABLE);
}

static uint16_t bsp_usart_rx_dma_get_write_idx(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    uint32_t u32Addr;

    u32Addr = DMA_GetDestAddr(cfg->rx_dma_unit, cfg->rx_dma_ch);
    /* Guard against an invalid DMA pointer while the channel is being reset or
     * if hardware reports an address outside the configured circular buffer. */
    if ((u32Addr < (uint32_t)(&ctx->rx_dma_buf[0])) ||
        (u32Addr > (uint32_t)(&ctx->rx_dma_buf[cfg->rx_dma_buf_size - 1U]) + 1UL))
    {
        return 0U;
    }

    return (uint16_t)(u32Addr - (uint32_t)(&ctx->rx_dma_buf[0]));
}

static void bsp_usart_tx_dma_init(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    stc_dma_init_t stcDmaInit;

    /* TX DMA is armed on demand by bsp_usart_tx_dma_start(). The initial count
     * is a placeholder until bytes are copied from the software ring. */
    DMA_Cmd(cfg->tx_dma_unit, ENABLE);
    (void)DMA_StructInit(&stcDmaInit);
    stcDmaInit.u32IntEn = DMA_INT_DISABLE;
    stcDmaInit.u32SrcAddr = (uint32_t)(&ctx->tx_dma_buf[0]);
    stcDmaInit.u32DestAddr = (uint32_t)(&cfg->unit->TDR);
    stcDmaInit.u32DataWidth = DMA_DATAWIDTH_8BIT;
    stcDmaInit.u32BlockSize = 1U;
    stcDmaInit.u32TransCount = 1U;
    stcDmaInit.u32SrcAddrInc = DMA_SRC_ADDR_INC;
    stcDmaInit.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    (void)DMA_Init(cfg->tx_dma_unit, cfg->tx_dma_ch, &stcDmaInit);

    AOS_SetTriggerEventSrc(cfg->tx_dma_trig_sel, cfg->tx_dma_trig_src);
    DMA_MxChCmd(cfg->tx_dma_unit, cfg->tx_dma_mx_ch, ENABLE);
}

static uint16_t bsp_usart_tx_ring_read(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg, uint8_t *data, uint16_t len)
{
    uint16_t u16Offset;
    uint16_t u16Read;
    uint16_t u16ToEnd;
    uint16_t i;

    if ((NULL == data) || (0U == len))
    {
        return 0U;
    }

    u16Read = bsp_usart_tx_ring_used(ctx, cfg);
    if (u16Read > len)
    {
        u16Read = len;
    }

    u16Offset = 0U;
    while (u16Offset < u16Read)
    {
        /* Split copies at the physical end of the ring buffer. */
        u16ToEnd = (uint16_t)(cfg->tx_ring_buf_size - ctx->tx_ring_tail);
        len = (uint16_t)(u16Read - u16Offset);
        if (len > u16ToEnd)
        {
            len = u16ToEnd;
        }

        for (i = 0U; i < len; i++)
        {
            data[(uint16_t)(u16Offset + i)] = ctx->tx_ring_buf[(uint16_t)(ctx->tx_ring_tail + i)];
        }

        ctx->tx_ring_tail = (uint16_t)((ctx->tx_ring_tail + len) % cfg->tx_ring_buf_size);
        u16Offset = (uint16_t)(u16Offset + len);
    }

    return u16Read;
}

static int32_t bsp_usart_tx_dma_start(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    uint16_t u16Len;

    /* Move one contiguous chunk out of the ring so DMA never has to handle
     * wrap-around in the software buffer. */
    u16Len = bsp_usart_tx_ring_read(ctx, cfg, ctx->tx_dma_buf, cfg->tx_dma_buf_size);
    if (0U == u16Len)
    {
        return LL_ERR_BUF_EMPTY;
    }

    DMA_ClearTransCompleteStatus(cfg->tx_dma_unit, cfg->tx_dma_tc_flag);
    (void)DMA_SetSrcAddr(cfg->tx_dma_unit, cfg->tx_dma_ch, (uint32_t)(&ctx->tx_dma_buf[0]));
    (void)DMA_SetDestAddr(cfg->tx_dma_unit, cfg->tx_dma_ch, (uint32_t)(&cfg->unit->TDR));
    (void)DMA_SetTransCount(cfg->tx_dma_unit, cfg->tx_dma_ch, u16Len);
    USART_ClearStatus(cfg->unit, USART_FLAG_TX_END);
    (void)DMA_ChCmd(cfg->tx_dma_unit, cfg->tx_dma_ch, ENABLE);
    DMA_MxChSWTrigger(cfg->tx_dma_unit, cfg->tx_dma_mx_ch);

    ctx->tx_dma_len = u16Len;
    ctx->tx_dma_busy = 1U;
    return LL_OK;
}

static void bsp_usart_tx_dma_service(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    if (0U != ctx->tx_dma_busy)
    {
        /* A DMA TC flag alone is not enough here: the USART shift register must
         * also be empty before the staging buffer can be reused safely. */
        if ((0U == bsp_usart_tx_dma_chstat_is_idle(cfg->tx_dma_unit, cfg->tx_dma_chstat_busy_mask)) ||
            (RESET == USART_GetStatus(cfg->unit, USART_FLAG_TX_CPLT)))
        {
            return;
        }

        DMA_ClearTransCompleteStatus(cfg->tx_dma_unit, cfg->tx_dma_tc_flag);
        USART_ClearStatus(cfg->unit, USART_FLAG_TX_END);
        ctx->tx_dma_len = 0U;
        ctx->tx_dma_busy = 0U;
    }

    if ((0U != bsp_usart_tx_dma_chstat_is_idle(cfg->tx_dma_unit, cfg->tx_dma_chstat_busy_mask)) &&
        (SET == USART_GetStatus(cfg->unit, USART_FLAG_TX_CPLT)))
    {
        (void)bsp_usart_tx_dma_start(ctx, cfg);
    }
}

static uint16_t bsp_usart_tx_ring_write(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg, const uint8_t *data, uint16_t len)
{
    uint16_t u16Offset;
    uint16_t u16Free;
    uint16_t u16ToEnd;
    uint16_t u16Write;
    uint16_t i;

    if ((NULL == data) || (0U == len))
    {
        return 0U;
    }

    u16Offset = 0U;
    while (u16Offset < len)
    {
        /* Keep one slot empty so head == tail always means "empty". */
        u16Free = bsp_usart_tx_ring_free(ctx, cfg);
        if (0U == u16Free)
        {
            break;
        }

        u16ToEnd = (uint16_t)(cfg->tx_ring_buf_size - ctx->tx_ring_head);
        u16Write = (uint16_t)(len - u16Offset);
        if (u16Write > u16Free)
        {
            u16Write = u16Free;
        }
        if (u16Write > u16ToEnd)
        {
            u16Write = u16ToEnd;
        }

        for (i = 0U; i < u16Write; i++)
        {
            ctx->tx_ring_buf[(uint16_t)(ctx->tx_ring_head + i)] = data[(uint16_t)(u16Offset + i)];
        }

        ctx->tx_ring_head = (uint16_t)((ctx->tx_ring_head + u16Write) % cfg->tx_ring_buf_size);
        u16Offset = (uint16_t)(u16Offset + u16Write);
    }

    return u16Offset;
}

static void bsp_usart_port_init(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg)
{
    stc_gpio_init_t stcGpioInit;
    stc_usart_uart_init_t stcUartInit;

    /* Configure GPIO, USART and DMA in one common path; cfg selects the port. */
    GPIO_REG_Unlock();

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    stcGpioInit.u16PinDrv = PIN_HIGH_DRV;
    stcGpioInit.u16PinOutputType = PIN_OUT_TYPE_CMOS;
    stcGpioInit.u16PullUp = PIN_PU_OFF;
    stcGpioInit.u16PullDown = PIN_PD_OFF;
    stcGpioInit.u16PinState = PIN_STAT_SET;
    (void)GPIO_Init(cfg->tx_port, cfg->tx_pin, &stcGpioInit);

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_IN;
    stcGpioInit.u16PullUp = PIN_PU_ON;
    stcGpioInit.u16PullDown = PIN_PD_OFF;
    (void)GPIO_Init(cfg->rx_port, cfg->rx_pin, &stcGpioInit);

    GPIO_SetFunc(cfg->tx_port, cfg->tx_pin, cfg->tx_func);
    GPIO_SetFunc(cfg->rx_port, cfg->rx_pin, cfg->rx_func);
    GPIO_OutputCmd(cfg->tx_port, cfg->tx_pin, ENABLE);
    GPIO_OutputCmd(cfg->rx_port, cfg->rx_pin, DISABLE);

    GPIO_REG_Lock();

    FCG_Fcg3PeriphClockCmd(cfg->fcg, ENABLE);

    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32ClockDiv = cfg->clk_div;
    stcUartInit.u32Baudrate = cfg->baudrate;
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    stcUartInit.u32HWFlowControl = USART_HW_FLOWCTRL_NONE;

    (void)USART_UART_Init(cfg->unit, &stcUartInit, NULL);
    USART_FuncCmd(cfg->unit, (USART_RX | USART_TX), ENABLE);
    USART_ClearStatus(cfg->unit, USART_FLAG_TX_END);
    ctx->tx_ring_head = 0U;
    ctx->tx_ring_tail = 0U;
    ctx->tx_dma_len = 0U;
    ctx->tx_dma_busy = 0U;
    ctx->rx_read_idx = 0U;
    bsp_usart_rx_dma_init(ctx, cfg);
    bsp_usart_tx_dma_init(ctx, cfg);
    DMA_ClearTransCompleteStatus(cfg->tx_dma_unit, cfg->tx_dma_tc_flag);
}

static uint16_t bsp_usart_write(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg, const uint8_t *data, uint16_t len)
{
    uint16_t u16Offset;
    uint16_t u16Write;
    uint32_t u32Timeout;

    if (NULL == data)
    {
        return 0U;
    }

    u16Offset = 0U;
    u32Timeout = cfg->tx_dma_timeout;
    while (u16Offset < len)
    {
        u16Write = bsp_usart_tx_ring_write(ctx, cfg, &data[u16Offset], (uint16_t)(len - u16Offset));
        if (0U != u16Write)
        {
            u16Offset = (uint16_t)(u16Offset + u16Write);
        }
        else
        {
            /* Backpressure is intentional: drain TX DMA while waiting for ring
             * space. A drop is recorded only if this wait exceeds the timeout. */
            bsp_usart_tx_dma_service(ctx, cfg);
            if (0U == u32Timeout)
            {
                break;
            }
            u32Timeout--;
        }
    }

    return u16Offset;
}

static int32_t bsp_usart_read_byte(bsp_usart_port_ctx_t *ctx, const bsp_usart_port_cfg_t *cfg, uint8_t *data)
{
    uint16_t u16WriteIdx;

    if (NULL == data)
    {
        return LL_ERR_INVD_PARAM;
    }

    /* RX has no copy step. The DMA write index is compared with rx_read_idx and
     * software consumes one byte at a time from the circular DMA buffer. */
    u16WriteIdx = bsp_usart_rx_dma_get_write_idx(ctx, cfg);
    if (u16WriteIdx >= cfg->rx_dma_buf_size)
    {
        u16WriteIdx = 0U;
    }

    if (ctx->rx_read_idx != u16WriteIdx)
    {
        *data = ctx->rx_dma_buf[ctx->rx_read_idx];
        ctx->rx_read_idx++;
        if (ctx->rx_read_idx >= cfg->rx_dma_buf_size)
        {
            ctx->rx_read_idx = 0U;
        }
        return LL_OK;
    }

    return LL_ERR_TIMEOUT;
}

static void bsp_usart1_tx_service_task(void)
{
    bsp_usart_tx_dma_service(&s_bsp_usart1_ctx, &s_bsp_usart1_cfg);
}

REG_TASK(100, bsp_usart1_tx_service_task)

static void bsp_usart2_tx_service_task(void)
{
    bsp_usart_tx_dma_service(&s_bsp_usart2_ctx, &s_bsp_usart2_cfg);
}

REG_TASK(100, bsp_usart2_tx_service_task)

static void bsp_usart1_init(void)
{
    bsp_usart_port_init(&s_bsp_usart1_ctx, &s_bsp_usart1_cfg);
}

REG_INIT(0, bsp_usart1_init)

static void bsp_usart2_init(void)
{
    bsp_usart_port_init(&s_bsp_usart2_ctx, &s_bsp_usart2_cfg);
}

REG_INIT(0, bsp_usart2_init)

static uint16_t bsp_usart1_write(const uint8_t *data, uint16_t len)
{
    return bsp_usart_write(&s_bsp_usart1_ctx, &s_bsp_usart1_cfg, data, len);
}

static uint16_t bsp_usart2_write(const uint8_t *data, uint16_t len)
{
    return bsp_usart_write(&s_bsp_usart2_ctx, &s_bsp_usart2_cfg, data, len);
}

void bsp_usart_dbg_tx(char *ptr, int len)
{
    uint16_t u16ReqLen;
    uint16_t u16SentLen;

    if ((NULL == ptr) || (len <= 0))
    {
        return;
    }

    u16ReqLen = ((uint32_t)len > 0xFFFFUL) ? 0xFFFFU : (uint16_t)len;
    u16SentLen = bsp_usart1_write((const uint8_t *)ptr, u16ReqLen);
#if (BSP_USART2_DMA_TEST_ENABLE == 1U)
    bsp_usart_test_record_tx(&s_bsp_usart_dbg_test_stats, u16ReqLen, u16SentLen);
#endif
}

void bsp_usart_dbg_printf(const char *__format, ...)
{
    va_list stcArgs;
    int i32Len;
    char acBuf[BSP_USART_DBG_PRINTF_BUF_SIZE];

    if (NULL == __format)
    {
        return;
    }

    va_start(stcArgs, __format);
    i32Len = vsnprintf(acBuf, sizeof(acBuf), __format, stcArgs);
    va_end(stcArgs);

    if (i32Len <= 0)
    {
        return;
    }

    if (i32Len > (int)(sizeof(acBuf) - 1U))
    {
#if (BSP_USART2_DMA_TEST_ENABLE == 1U)
        bsp_usart_test_record_printf_trunc(&s_bsp_usart_dbg_test_stats);
#endif
        i32Len = (int)(sizeof(acBuf) - 1U);
    }

    bsp_usart_dbg_tx(acBuf, i32Len);
}

uint8_t bsp_usart_dbg_rx_get_byte(uint8_t *p_data)
{
    return (LL_OK == bsp_usart1_read_byte(p_data)) ? 1U : 0U;
}

void bsp_usart_iso_tx(char *ptr, int len)
{
    uint16_t u16ReqLen;
    uint16_t u16SentLen;

    if ((NULL == ptr) || (len <= 0))
    {
        return;
    }

    u16ReqLen = ((uint32_t)len > 0xFFFFUL) ? 0xFFFFU : (uint16_t)len;
    u16SentLen = bsp_usart2_write((const uint8_t *)ptr, u16ReqLen);
#if (BSP_USART2_DMA_TEST_ENABLE == 1U)
    bsp_usart_test_record_tx(&s_bsp_usart_iso_test_stats, u16ReqLen, u16SentLen);
#endif
}

void bsp_usart_iso_printf(const char *__format, ...)
{
    va_list stcArgs;
    int i32Len;
    char acBuf[BSP_USART_ISO_PRINTF_BUF_SIZE];

    if (NULL == __format)
    {
        return;
    }

    va_start(stcArgs, __format);
    i32Len = vsnprintf(acBuf, sizeof(acBuf), __format, stcArgs);
    va_end(stcArgs);

    if (i32Len <= 0)
    {
        return;
    }

    if (i32Len > (int)(sizeof(acBuf) - 1U))
    {
#if (BSP_USART2_DMA_TEST_ENABLE == 1U)
        bsp_usart_test_record_printf_trunc(&s_bsp_usart_iso_test_stats);
#endif
        i32Len = (int)(sizeof(acBuf) - 1U);
    }

    bsp_usart_iso_tx(acBuf, i32Len);
}

uint8_t bsp_usart_iso_rx_get_byte(uint8_t *p_data)
{
    return (LL_OK == bsp_usart2_read_byte(p_data)) ? 1U : 0U;
}

static int32_t bsp_usart1_read_byte(uint8_t *data)
{
    return bsp_usart_read_byte(&s_bsp_usart1_ctx, &s_bsp_usart1_cfg, data);
}

static int32_t bsp_usart2_read_byte(uint8_t *data)
{
    return bsp_usart_read_byte(&s_bsp_usart2_ctx, &s_bsp_usart2_cfg, data);
}

#if (BSP_USART2_DMA_TEST_ENABLE == 1U)
static uint16_t bsp_usart_test_append_msg(uint8_t *buf, uint16_t idx, const char *tag, uint32_t seq)
{
    while ('\0' != *tag)
    {
        buf[idx++] = (uint8_t)*tag++;
    }

    buf[idx++] = ' ';
    idx = bsp_usart_append_hex32(buf, idx, seq);
    buf[idx++] = '\r';
    buf[idx++] = '\n';

    return idx;
}

static void bsp_usart_tx_test_task(void)
{
    static uint32_t u32TxSeq = 0UL;
    uint32_t u32Seq;
    uint8_t au8Msg[BSP_USART2_TX_STRESS_PKT_SIZE];
    uint16_t u16Idx;

    /* One short frame per port every millisecond keeps steady TX pressure. */
    u32Seq = u32TxSeq++;
    u16Idx = bsp_usart_test_append_msg(au8Msg, 0U, "DBG TX", u32Seq);
    s_bsp_usart_dbg_test_stats.tx_frames++;
    bsp_usart_dbg_tx((char *)au8Msg, (int)u16Idx);

    u16Idx = bsp_usart_test_append_msg(au8Msg, 0U, "ISO TX", u32Seq);
    s_bsp_usart_iso_test_stats.tx_frames++;
    bsp_usart_iso_tx((char *)au8Msg, (int)u16Idx);
}

REG_TASK_MS(1, bsp_usart_tx_test_task)

static void bsp_usart_printf_test_task(void)
{
    static uint32_t u32PrintfSeq = 0UL;
    uint32_t u32Seq;

    /* printf traffic runs slower but periodically sends a long frame to force
     * buffer pressure and exercise truncation accounting. */
    u32Seq = u32PrintfSeq++;
    s_bsp_usart_dbg_test_stats.printf_frames++;
    if ((u32Seq % BSP_USART_TEST_LONG_PRINTF_PERIOD) == 0UL)
    {
        bsp_usart_dbg_printf("DBG PRINTF LONG %08lX 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF\r\n",
                             (unsigned long)u32Seq);
    }
    else
    {
        bsp_usart_dbg_printf("DBG PRINTF %08lX\r\n", (unsigned long)u32Seq);
    }

    s_bsp_usart_iso_test_stats.printf_frames++;
    if ((u32Seq % BSP_USART_TEST_LONG_PRINTF_PERIOD) == 0UL)
    {
        bsp_usart_iso_printf("ISO PRINTF LONG %08lX 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF 0123456789ABCDEF\r\n",
                             (unsigned long)u32Seq);
    }
    else
    {
        bsp_usart_iso_printf("ISO PRINTF %08lX\r\n", (unsigned long)u32Seq);
    }
}

REG_TASK_MS(7, bsp_usart_printf_test_task)

static uint16_t bsp_usart_test_format_result(char *buf,
                                             uint16_t buf_size,
                                             const char *name,
                                             const bsp_usart_test_port_stats_t *stats)
{
    int len;

    len = snprintf(buf,
                   buf_size,
                   "%s TEST req=%lu sent=%lu drop=%lu tx=%lu printf=%lu trunc=%lu\r\n",
                   name,
                   (unsigned long)stats->req_bytes,
                   (unsigned long)stats->sent_bytes,
                   (unsigned long)stats->drop_bytes,
                   (unsigned long)stats->tx_frames,
                   (unsigned long)stats->printf_frames,
                   (unsigned long)stats->printf_trunc);
    if (len <= 0)
    {
        return 0U;
    }
    if (len >= (int)buf_size)
    {
        return (uint16_t)(buf_size - 1U);
    }

    return (uint16_t)len;
}

static void bsp_usart_test_result_task(void)
{
    char acBuf[128];
    uint16_t u16Len;

    /* Publish cumulative counters once per second without using printf wrappers,
     * so the result line itself does not skew printf statistics. */
    u16Len = bsp_usart_test_format_result(acBuf,
                                          (uint16_t)sizeof(acBuf),
                                          "DBG",
                                          &s_bsp_usart_dbg_test_stats);
    (void)bsp_usart1_write((const uint8_t *)acBuf, u16Len);

    u16Len = bsp_usart_test_format_result(acBuf,
                                          (uint16_t)sizeof(acBuf),
                                          "ISO",
                                          &s_bsp_usart_iso_test_stats);
    (void)bsp_usart2_write((const uint8_t *)acBuf, u16Len);
}

REG_TASK_MS(1000, bsp_usart_test_result_task)
#endif
