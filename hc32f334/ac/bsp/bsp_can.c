#include "bsp_can.h"
#include "section.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define BSP_CAN_UNIT                         (CM_MCAN2)
#define BSP_CAN_CLK_UNIT                     (CLK_MCAN2)
#define BSP_CAN_PERIPH_CLK                   (FCG1_PERIPH_MCAN2)
#define BSP_CAN_CLK_SRC                      (CLK_MCANCLK_SYSCLK_DIV3)

#define BSP_CAN_STD_FILTER_NUM               (0U)
#define BSP_CAN_EXT_FILTER_NUM               (0U)
#define BSP_CAN_RX_FIFO0_NUM                 (8U)
#define BSP_CAN_RX_FIFO0_WATERMARK           (1U)
#define BSP_CAN_RX_FIFO0_DATA_FIELD_SIZE     (MCAN_DATA_SIZE_8BYTE)
#define BSP_CAN_RX_FIFO1_NUM                 (0U)
#define BSP_CAN_RX_FIFO1_DATA_FIELD_SIZE     (MCAN_DATA_SIZE_8BYTE)
#define BSP_CAN_RX_BUF_NUM                   (0U)
#define BSP_CAN_RX_BUF_DATA_FIELD_SIZE       (MCAN_DATA_SIZE_8BYTE)
#define BSP_CAN_TX_BUF_NUM                   (4U)
#define BSP_CAN_TX_FIFO_NUM                  (4U)
#define BSP_CAN_TX_DATA_FIELD_SIZE           (MCAN_DATA_SIZE_8BYTE)
#define BSP_CAN_TX_EVT_NUM                   (4U)

#define BSP_CAN_TX_STD_ID                    (0x101UL)
#define BSP_CAN_TX_FRAME_SIZE                (8U)
#if (BSP_CAN_TEST_ENABLE == 1U)
#define BSP_CAN_TEST_RX_BUF_SIZE             (256U)
#endif

static volatile uint32_t bsp_can_rx_count = 0UL;
static volatile uint32_t bsp_can_tx_count = 0UL;
static volatile uint32_t bsp_can_rx_lost_count = 0UL;
static volatile uint32_t bsp_can_rx_overflow_count = 0UL;
static volatile uint32_t bsp_can_tx_overflow_count = 0UL;
static volatile uint32_t bsp_can_bus_off_count = 0UL;

static volatile uint8_t s_bsp_can_init_done = 0U;
static uint8_t s_bsp_can_rx_buf[BSP_CAN_RX_BUF_SIZE];
static volatile uint16_t s_bsp_can_rx_write_idx = 0U;
static volatile uint16_t s_bsp_can_rx_read_idx = 0U;
static uint8_t s_bsp_can_tx_dma_ring[BSP_CAN_TX_DMA_RING_SIZE];
static volatile uint16_t s_bsp_can_tx_write_idx = 0U;
static volatile uint16_t s_bsp_can_tx_read_idx = 0U;
static volatile uint8_t s_bsp_can_tx_servicing = 0U;

#if (BSP_CAN_TEST_ENABLE == 1U)
static uint8_t s_bsp_can_test_rx_buf[BSP_CAN_TEST_RX_BUF_SIZE];
static volatile uint16_t s_bsp_can_test_rx_write_idx = 0U;
#endif

static uint32_t bsp_can_dlc_to_len(uint32_t u32Dlc)
{
    if (u32Dlc <= MCAN_DLC8)
    {
        return u32Dlc;
    }

    return 8U;
}

static uint32_t bsp_can_enter_critical(void)
{
    uint32_t u32Primask;

    u32Primask = __get_PRIMASK();
    __disable_irq();
    return u32Primask;
}

static void bsp_can_exit_critical(uint32_t u32Primask)
{
    if (0U == u32Primask)
    {
        __enable_irq();
    }
}

static uint16_t bsp_can_rx_ring_used(void)
{
    uint16_t u16WriteIdx;
    uint16_t u16ReadIdx;

    u16WriteIdx = s_bsp_can_rx_write_idx;
    u16ReadIdx = s_bsp_can_rx_read_idx;
    if (u16WriteIdx >= u16ReadIdx)
    {
        return (uint16_t)(u16WriteIdx - u16ReadIdx);
    }

    return (uint16_t)(BSP_CAN_RX_BUF_SIZE - u16ReadIdx + u16WriteIdx);
}

static uint16_t bsp_can_rx_ring_free(void)
{
    return (uint16_t)(BSP_CAN_RX_BUF_SIZE - bsp_can_rx_ring_used() - 1U);
}

static void bsp_can_rx_ring_push_byte(uint8_t u8Data)
{
    uint16_t u16NextIdx;

    u16NextIdx = (uint16_t)(s_bsp_can_rx_write_idx + 1U);
    if (u16NextIdx >= BSP_CAN_RX_BUF_SIZE)
    {
        u16NextIdx = 0U;
    }

    if (u16NextIdx == s_bsp_can_rx_read_idx)
    {
        bsp_can_rx_overflow_count++;
        return;
    }

    s_bsp_can_rx_buf[s_bsp_can_rx_write_idx] = u8Data;
    s_bsp_can_rx_write_idx = u16NextIdx;
}

static void bsp_can_rx_ring_push_frame(const stc_mcan_rx_msg_t *pstcRxMsg)
{
    uint32_t u32Idx;
    uint32_t u32Len;

    if ((NULL == pstcRxMsg) ||
        (pstcRxMsg->RTR != 0U) ||
        ((pstcRxMsg->IDE != MCAN_STD_ID) && (pstcRxMsg->IDE != MCAN_EXT_ID)))
    {
        return;
    }

    u32Len = bsp_can_dlc_to_len(pstcRxMsg->DLC);
    if (bsp_can_rx_ring_free() < u32Len)
    {
        bsp_can_rx_overflow_count++;
        return;
    }

    for (u32Idx = 0U; u32Idx < u32Len; u32Idx++)
    {
        bsp_can_rx_ring_push_byte(pstcRxMsg->au8Data[u32Idx]);
    }

    bsp_can_rx_count++;
}

static uint16_t bsp_can_tx_ring_next(uint16_t u16Idx)
{
    u16Idx++;
    if (u16Idx >= BSP_CAN_TX_DMA_RING_SIZE)
    {
        u16Idx = 0U;
    }

    return u16Idx;
}

static uint8_t bsp_can_tx_ring_is_empty(void)
{
    return (uint8_t)(s_bsp_can_tx_read_idx == s_bsp_can_tx_write_idx);
}

static uint16_t bsp_can_tx_ring_used(void)
{
    uint16_t u16WriteIdx;
    uint16_t u16ReadIdx;

    u16WriteIdx = s_bsp_can_tx_write_idx;
    u16ReadIdx = s_bsp_can_tx_read_idx;
    if (u16WriteIdx >= u16ReadIdx)
    {
        return (uint16_t)(u16WriteIdx - u16ReadIdx);
    }

    return (uint16_t)(BSP_CAN_TX_DMA_RING_SIZE - u16ReadIdx + u16WriteIdx);
}

static uint16_t bsp_can_tx_ring_free(void)
{
    return (uint16_t)(BSP_CAN_TX_DMA_RING_SIZE - bsp_can_tx_ring_used() - 1U);
}

static uint8_t bsp_can_tx_ring_push_byte(uint8_t u8Data)
{
    uint16_t u16NextIdx;
    uint32_t u32Primask;

    u32Primask = bsp_can_enter_critical();
    u16NextIdx = bsp_can_tx_ring_next(s_bsp_can_tx_write_idx);
    if (u16NextIdx == s_bsp_can_tx_read_idx)
    {
        bsp_can_tx_overflow_count++;
        bsp_can_exit_critical(u32Primask);
        return 0U;
    }

    s_bsp_can_tx_dma_ring[s_bsp_can_tx_write_idx] = u8Data;
    s_bsp_can_tx_write_idx = u16NextIdx;
    bsp_can_exit_critical(u32Primask);

    return 1U;
}

static uint32_t bsp_can_tx_ring_peek_frame(uint8_t *p_frame)
{
    uint16_t u16Idx;
    uint32_t u32Idx;
    uint32_t u32Len;

    if ((NULL == p_frame) || (0U != bsp_can_tx_ring_is_empty()))
    {
        return 0U;
    }

    (void)memset(p_frame, 0, BSP_CAN_TX_FRAME_SIZE);
    u32Len = bsp_can_tx_ring_used();
    if (u32Len > BSP_CAN_TX_FRAME_SIZE)
    {
        u32Len = BSP_CAN_TX_FRAME_SIZE;
    }

    u16Idx = s_bsp_can_tx_read_idx;
    for (u32Idx = 0U; u32Idx < u32Len; u32Idx++)
    {
        p_frame[u32Idx] = s_bsp_can_tx_dma_ring[u16Idx];
        u16Idx = bsp_can_tx_ring_next(u16Idx);
    }

    return u32Len;
}

static void bsp_can_tx_ring_pop(uint32_t u32Len)
{
    uint32_t u32Idx;
    uint32_t u32Primask;

    u32Primask = bsp_can_enter_critical();
    for (u32Idx = 0U; u32Idx < u32Len; u32Idx++)
    {
        if (s_bsp_can_tx_read_idx == s_bsp_can_tx_write_idx)
        {
            break;
        }
        s_bsp_can_tx_read_idx = bsp_can_tx_ring_next(s_bsp_can_tx_read_idx);
    }
    bsp_can_exit_critical(u32Primask);
}

static void bsp_can_comm_clock_config(void)
{
    CLK_SetCANClockSrc(BSP_CAN_CLK_UNIT, BSP_CAN_CLK_SRC);
}

static void bsp_can_pin_config(void)
{
    GPIO_SetFunc(BSP_CAN_TX_PORT, BSP_CAN_TX_PIN, BSP_CAN_TX_PIN_FUNC);
    GPIO_SetFunc(BSP_CAN_RX_PORT, BSP_CAN_RX_PIN, BSP_CAN_RX_PIN_FUNC);
}

static void bsp_can_phy_enable(void)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    stcGpioInit.u16PinState = PIN_STAT_RST;
    GPIO_Init(BSP_CAN_PHY_STBY_PORT, BSP_CAN_PHY_STBY_PIN, &stcGpioInit);
    GPIO_ResetPins(BSP_CAN_PHY_STBY_PORT, BSP_CAN_PHY_STBY_PIN);
    GPIO_OutputCmd(BSP_CAN_PHY_STBY_PORT, BSP_CAN_PHY_STBY_PIN, ENABLE);
}

static int32_t bsp_can_mcan_config(void)
{
    stc_mcan_init_t stcMcanInit;

    (void)MCAN_StructInit(&stcMcanInit);
    stcMcanInit.u32Mode = MCAN_MD_NORMAL;
    stcMcanInit.u32FrameFormat = MCAN_FRAME_CLASSIC;
    stcMcanInit.u32AutoRetx = MCAN_AUTO_RETX_ENABLE;
    stcMcanInit.stcBitTime.u32NominalPrescaler = 2U;
    stcMcanInit.stcBitTime.u32NominalTimeSeg1 = 16U;
    stcMcanInit.stcBitTime.u32NominalTimeSeg2 = 4U;
    stcMcanInit.stcBitTime.u32NominalSyncJumpWidth = 4U;
    stcMcanInit.stcMsgRam.u32AddrOffset = 0U;
    stcMcanInit.stcMsgRam.u32StdFilterNum = BSP_CAN_STD_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32ExtFilterNum = BSP_CAN_EXT_FILTER_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0Num = BSP_CAN_RX_FIFO0_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo0DataSize = BSP_CAN_RX_FIFO0_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxFifo1Num = BSP_CAN_RX_FIFO1_NUM;
    stcMcanInit.stcMsgRam.u32RxFifo1DataSize = BSP_CAN_RX_FIFO1_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32RxBufferNum = BSP_CAN_RX_BUF_NUM;
    stcMcanInit.stcMsgRam.u32RxBufferDataSize = BSP_CAN_RX_BUF_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32TxBufferNum = BSP_CAN_TX_BUF_NUM;
    stcMcanInit.stcMsgRam.u32TxFifoQueueNum = BSP_CAN_TX_FIFO_NUM;
    stcMcanInit.stcMsgRam.u32TxFifoQueueMode = MCAN_TX_FIFO_MD;
    stcMcanInit.stcMsgRam.u32TxDataSize = BSP_CAN_TX_DATA_FIELD_SIZE;
    stcMcanInit.stcMsgRam.u32TxEventNum = BSP_CAN_TX_EVT_NUM;
    stcMcanInit.stcFilter.pstcStdFilterList = NULL;
    stcMcanInit.stcFilter.pstcExtFilterList = NULL;
    stcMcanInit.stcFilter.u32StdFilterConfigNum = BSP_CAN_STD_FILTER_NUM;
    stcMcanInit.stcFilter.u32ExtFilterConfigNum = 0U;

    FCG_Fcg1PeriphClockCmd(BSP_CAN_PERIPH_CLK, ENABLE);
    if (LL_OK != MCAN_Init(BSP_CAN_UNIT, &stcMcanInit))
    {
        return LL_ERR;
    }

    MCAN_SetFifoWatermark(BSP_CAN_UNIT, MCAN_WATERMARK_RX_FIFO0, BSP_CAN_RX_FIFO0_WATERMARK);
    MCAN_RxFifoOperationModeConfig(BSP_CAN_UNIT, MCAN_RX_FIFO0, MCAN_RX_FIFO_BLOCKING);
    MCAN_TimestampCounterConfig(BSP_CAN_UNIT, 1U);
    MCAN_TimestampCounterCmd(BSP_CAN_UNIT, ENABLE);
    MCAN_GlobalFilterConfig(BSP_CAN_UNIT,
                            MCAN_NMF_ACCEPT_IN_RX_FIFO0,
                            MCAN_NMF_ACCEPT_IN_RX_FIFO0,
                            MCAN_REMOTE_FRAME_REJECT,
                            MCAN_REMOTE_FRAME_REJECT);

    return LL_OK;
}

static int32_t bsp_can_send_frame(const uint8_t *p_data, uint32_t u32Len)
{
    stc_mcan_tx_msg_t stcTxMsg;

    if ((NULL == p_data) || (0U == u32Len) || (u32Len > BSP_CAN_TX_FRAME_SIZE) ||
        (0U == s_bsp_can_init_done))
    {
        return LL_ERR;
    }

    (void)memset(&stcTxMsg, 0, sizeof(stcTxMsg));
    stcTxMsg.ID = BSP_CAN_TX_STD_ID;
    stcTxMsg.IDE = MCAN_STD_ID;
    stcTxMsg.DLC = u32Len;
    (void)memcpy(stcTxMsg.au8Data, p_data, u32Len);

    if (LL_OK != MCAN_AddMsgToTxFifoQueue(BSP_CAN_UNIT, &stcTxMsg))
    {
        return LL_ERR;
    }

    bsp_can_tx_count++;
    return LL_OK;
}

static void bsp_can_tx_service(void)
{
    uint8_t au8Frame[BSP_CAN_TX_FRAME_SIZE];
    uint32_t u32Len;

    if ((0U == s_bsp_can_init_done) || (0U != s_bsp_can_tx_servicing))
    {
        return;
    }

    s_bsp_can_tx_servicing = 1U;
    while (0U != (u32Len = bsp_can_tx_ring_peek_frame(au8Frame)))
    {
        if (LL_OK != bsp_can_send_frame(au8Frame, u32Len))
        {
            break;
        }

        bsp_can_tx_ring_pop(u32Len);
    }
    s_bsp_can_tx_servicing = 0U;
}

static int32_t bsp_can_init(void)
{
    if (0U != s_bsp_can_init_done)
    {
        return LL_OK;
    }

    LL_PERIPH_WE(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_INTC | LL_PERIPH_PWC_CLK_RMU);

    bsp_can_comm_clock_config();
    bsp_can_pin_config();
    bsp_can_phy_enable();
    if (LL_OK != bsp_can_mcan_config())
    {
        LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_INTC | LL_PERIPH_PWC_CLK_RMU);
        return LL_ERR;
    }

    s_bsp_can_rx_write_idx = 0U;
    s_bsp_can_rx_read_idx = 0U;
    s_bsp_can_tx_write_idx = 0U;
    s_bsp_can_tx_read_idx = 0U;
    s_bsp_can_tx_servicing = 0U;
    bsp_can_rx_count = 0UL;
    bsp_can_tx_count = 0UL;
    bsp_can_rx_lost_count = 0UL;
    bsp_can_rx_overflow_count = 0UL;
    bsp_can_tx_overflow_count = 0UL;
    bsp_can_bus_off_count = 0UL;

    MCAN_Start(BSP_CAN_UNIT);
    s_bsp_can_init_done = 1U;

    LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_INTC | LL_PERIPH_PWC_CLK_RMU);
    return LL_OK;
}

static void bsp_can_write(const uint8_t *p_data, uint16_t len)
{
    uint16_t u16Offset;

    if (NULL == p_data)
    {
        return;
    }

    u16Offset = 0U;
    while (u16Offset < len)
    {
        if (0U == bsp_can_tx_ring_free())
        {
            break;
        }

        if (0U == bsp_can_tx_ring_push_byte(p_data[u16Offset]))
        {
            break;
        }

        u16Offset++;
    }

    if (u16Offset < len)
    {
        bsp_can_tx_overflow_count += (uint32_t)(len - u16Offset);
    }
}

void bsp_can_dbg_tx(char *ptr, int len)
{
    uint16_t u16ReqLen;

    if ((NULL == ptr) || (len <= 0))
    {
        return;
    }

    u16ReqLen = ((uint32_t)len > 0xFFFFUL) ? 0xFFFFU : (uint16_t)len;
    bsp_can_write((const uint8_t *)ptr, u16ReqLen);
}

void bsp_can_dbg_printf(const char *__format, ...)
{
    va_list stcArgs;
    int i32Len;
    char acBuf[BSP_CAN_DBG_PRINTF_BUF_SIZE];

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
        i32Len = (int)(sizeof(acBuf) - 1U);
    }

    bsp_can_dbg_tx(acBuf, i32Len);
}

uint8_t bsp_can_dbg_rx_get_byte(uint8_t *p_data)
{
    if ((NULL == p_data) || (s_bsp_can_rx_read_idx == s_bsp_can_rx_write_idx))
    {
        return 0U;
    }

    *p_data = s_bsp_can_rx_buf[s_bsp_can_rx_read_idx];
    s_bsp_can_rx_read_idx++;
    if (s_bsp_can_rx_read_idx >= BSP_CAN_RX_BUF_SIZE)
    {
        s_bsp_can_rx_read_idx = 0U;
    }

    return 1U;
}

static void bsp_can_rx_poll_task(void)
{
    stc_mcan_rx_msg_t stcRxMsg;

    if (SET == MCAN_GetStatus(BSP_CAN_UNIT, MCAN_FLAG_RX_FIFO0_NEW_MSG))
    {
        MCAN_ClearStatus(BSP_CAN_UNIT, MCAN_FLAG_RX_FIFO0_NEW_MSG);
    }
    if (SET == MCAN_GetStatus(BSP_CAN_UNIT, MCAN_FLAG_RX_FIFO0_WATERMARK))
    {
        MCAN_ClearStatus(BSP_CAN_UNIT, MCAN_FLAG_RX_FIFO0_WATERMARK);
    }
    if (SET == MCAN_GetStatus(BSP_CAN_UNIT, MCAN_FLAG_RX_FIFO0_FULL))
    {
        MCAN_ClearStatus(BSP_CAN_UNIT, MCAN_FLAG_RX_FIFO0_FULL);
    }
    if (SET == MCAN_GetStatus(BSP_CAN_UNIT, MCAN_FLAG_RX_FIFO0_MSG_LOST))
    {
        MCAN_ClearStatus(BSP_CAN_UNIT, MCAN_FLAG_RX_FIFO0_MSG_LOST);
        bsp_can_rx_lost_count++;
    }

    while (LL_OK == MCAN_GetRxMsg(BSP_CAN_UNIT, MCAN_RX_FIFO0, &stcRxMsg))
    {
        bsp_can_rx_ring_push_frame(&stcRxMsg);
    }
}

static void bsp_can_bus_status_task(void)
{
    if (SET == MCAN_GetStatus(BSP_CAN_UNIT, MCAN_FLAG_BUS_OFF))
    {
        MCAN_ClearStatus(BSP_CAN_UNIT, MCAN_FLAG_BUS_OFF);
        bsp_can_bus_off_count++;
        if (SET == MCAN_GetProtocolFlagStatus(BSP_CAN_UNIT, MCAN_PROTOCOL_FLAG_BUS_OFF))
        {
            MCAN_Start(BSP_CAN_UNIT);
        }
    }
}

static void bsp_can_poll_task(void)
{
    bsp_can_rx_poll_task();
    bsp_can_bus_status_task();
    bsp_can_tx_service();
}

REG_TASK_MS(1, bsp_can_poll_task)

#if (BSP_CAN_AUTO_INIT == 1U)
static void bsp_can_init_entry(void)
{
    (void)bsp_can_init();
}

REG_INIT(0, bsp_can_init_entry)
#endif

#if (BSP_CAN_TEST_ENABLE == 1U)
static void bsp_can_test_tx_task(void)
{
    static uint32_t u32Seq = 0UL;
    uint8_t au8TestData[8U];

    au8TestData[0] = 0xA5U;
    au8TestData[1] = 0x5AU;
    au8TestData[2] = (uint8_t)(u32Seq >> 24U);
    au8TestData[3] = (uint8_t)(u32Seq >> 16U);
    au8TestData[4] = (uint8_t)(u32Seq >> 8U);
    au8TestData[5] = (uint8_t)(u32Seq);
    au8TestData[6] = 0x55U;
    au8TestData[7] = 0xAAU;
    u32Seq++;

    bsp_can_dbg_tx((char *)au8TestData, (int)sizeof(au8TestData));
}

REG_TASK_MS(1000, bsp_can_test_tx_task)

static void bsp_can_test_printf_task(void)
{
    static uint32_t u32PrintfSeq = 0UL;

    bsp_can_dbg_printf("CAN %08lX\r\n", (unsigned long)u32PrintfSeq++);
}

REG_TASK_MS(1500, bsp_can_test_printf_task)

static void bsp_can_test_rx_task(void)
{
    uint8_t u8Data;

    while (0U != bsp_can_dbg_rx_get_byte(&u8Data))
    {
        s_bsp_can_test_rx_buf[s_bsp_can_test_rx_write_idx] = u8Data;
        s_bsp_can_test_rx_write_idx++;
        if (s_bsp_can_test_rx_write_idx >= BSP_CAN_TEST_RX_BUF_SIZE)
        {
            s_bsp_can_test_rx_write_idx = 0U;
        }
    }
}

REG_TASK_MS(1, bsp_can_test_rx_task)
#endif
