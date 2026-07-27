// SPDX-License-Identifier: MIT
/**
 * @file    bsp_w25q64.c
 * @brief   HC32F334 W25Q64 SPI flash driver.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure the HC32 SPI peripheral and board-specific flash pins
 *          - Execute JEDEC read, physical read, page program, and sector erase commands
 *          - Convert the W25Q64 WIP bit into an asynchronous device state
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; one foreground owner must serialize transactions
 *          - SPI runs in mode 0 at PCLK1/4, which is 15 MHz for the board clock setup
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_w25q64.h"

#include "hc32_ll.h"

#define BSP_W25Q64_SPI_UNIT              CM_SPI
#define BSP_W25Q64_SCK_PORT              GPIO_PORT_B
#define BSP_W25Q64_SCK_PIN               GPIO_PIN_05
#define BSP_W25Q64_SCK_FUNC              GPIO_FUNC_49
#define BSP_W25Q64_MOSI_PORT             GPIO_PORT_A
#define BSP_W25Q64_MOSI_PIN              GPIO_PIN_00
#define BSP_W25Q64_MOSI_FUNC             GPIO_FUNC_50
#define BSP_W25Q64_MISO_PORT             GPIO_PORT_A
#define BSP_W25Q64_MISO_PIN              GPIO_PIN_01
#define BSP_W25Q64_MISO_FUNC             GPIO_FUNC_51
#define BSP_W25Q64_CS_PORT               GPIO_PORT_A
#define BSP_W25Q64_CS_PIN                GPIO_PIN_06
#define BSP_W25Q64_SPI_TIMEOUT            1000000UL
#ifndef BSP_W25Q64_SPI_MODE
#define BSP_W25Q64_SPI_MODE               SPI_MD_0
#endif
#ifndef BSP_W25Q64_SPI_PRESCALER
#define BSP_W25Q64_SPI_PRESCALER          SPI_BR_CLK_DIV4
#endif
#define BSP_W25Q64_DMA_UNIT               CM_DMA
#define BSP_W25Q64_DMA_RX_CH              DMA_CH4
#define BSP_W25Q64_DMA_RX_MX_CH           DMA_MX_CH4
#define BSP_W25Q64_DMA_RX_TRIGGER         AOS_DMA_4
#define BSP_W25Q64_DMA_RX_TC_FLAG         DMA_FLAG_TC_CH4
#define BSP_W25Q64_DMA_TX_CH              DMA_CH5
#define BSP_W25Q64_DMA_TX_MX_CH           DMA_MX_CH5
#define BSP_W25Q64_DMA_TX_TRIGGER         AOS_DMA_5
#define BSP_W25Q64_DMA_TX_TC_FLAG         DMA_FLAG_TC_CH5
#define BSP_W25Q64_DMA_ERROR_FLAGS        (DMA_FLAG_REQ_ERR_CH4 | DMA_FLAG_REQ_ERR_CH5 | \
                                           DMA_FLAG_TRANS_ERR_CH4 | DMA_FLAG_TRANS_ERR_CH5)
#define BSP_W25Q64_SELF_TEST_WAIT_LIMIT   1000000UL
#define BSP_W25Q64_SELF_TEST_LENGTH       300UL

#define W25Q64_CMD_WRITE_ENABLE           0x06U
#define W25Q64_CMD_READ_STATUS_1          0x05U
#define W25Q64_CMD_READ_DATA              0x03U
#define W25Q64_CMD_PAGE_PROGRAM           0x02U
#define W25Q64_CMD_SECTOR_ERASE_4K        0x20U
#define W25Q64_CMD_READ_JEDEC_ID          0x9FU
#define W25Q64_STATUS_1_WIP               (1U << 0U)
#define W25Q64_STATUS_1_WEL               (1U << 1U)

static uint8_t s_initialized;
static uint8_t s_io_error;
static const uint8_t s_dma_dummy_tx = 0xFFU;
static uint8_t s_dma_dummy_rx;

static void bsp_w25q64_cs_high(void)
{
    GPIO_SetPins(BSP_W25Q64_CS_PORT, BSP_W25Q64_CS_PIN);
}

static void bsp_w25q64_cs_low(void)
{
    GPIO_ResetPins(BSP_W25Q64_CS_PORT, BSP_W25Q64_CS_PIN);
}

static uint8_t bsp_w25q64_range_is_valid(uint32_t address, uint32_t length)
{
    return (uint8_t)((address <= BSP_W25Q64_CAPACITY_BYTES) &&
                     (length <= (BSP_W25Q64_CAPACITY_BYTES - address)));
}

static bsp_w25q64_result_t bsp_w25q64_transmit(const uint8_t *p_data, uint32_t length)
{
    int32_t result;

    result = SPI_Trans(BSP_W25Q64_SPI_UNIT, p_data, length, BSP_W25Q64_SPI_TIMEOUT);
    if (LL_OK != result)
    {
        s_io_error = 1U;
        return BSP_W25Q64_RESULT_IO_ERROR;
    }

    return BSP_W25Q64_RESULT_SUCCESS;
}

static bsp_w25q64_result_t bsp_w25q64_receive(uint8_t *p_data, uint32_t length)
{
    int32_t result;

    result = SPI_Receive(BSP_W25Q64_SPI_UNIT, p_data, length, BSP_W25Q64_SPI_TIMEOUT);
    if (LL_OK != result)
    {
        s_io_error = 1U;
        return BSP_W25Q64_RESULT_IO_ERROR;
    }

    return BSP_W25Q64_RESULT_SUCCESS;
}

static void bsp_w25q64_dma_stop(void)
{
    DMA_MxChCmd(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_TX_MX_CH, DISABLE);
    DMA_MxChCmd(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_RX_MX_CH, DISABLE);
    (void)DMA_ChCmd(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_TX_CH, DISABLE);
    (void)DMA_ChCmd(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_RX_CH, DISABLE);
}

static bsp_w25q64_result_t bsp_w25q64_transfer_dma(const uint8_t *p_tx,
                                                   uint8_t *p_rx,
                                                   uint32_t length)
{
    stc_dma_init_t dma_init;
    uint32_t timeout;
    int32_t result;

    if ((0UL == length) || (length > UINT16_MAX))
    {
        return BSP_W25Q64_RESULT_INVALID_ARGUMENT;
    }

    bsp_w25q64_dma_stop();
    DMA_ClearErrStatus(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_ERROR_FLAGS);
    DMA_ClearTransCompleteStatus(BSP_W25Q64_DMA_UNIT,
                                 BSP_W25Q64_DMA_RX_TC_FLAG | BSP_W25Q64_DMA_TX_TC_FLAG);

    (void)DMA_StructInit(&dma_init);
    dma_init.u32IntEn = DMA_INT_DISABLE;
    dma_init.u32SrcAddr = (uint32_t)&BSP_W25Q64_SPI_UNIT->DR;
    dma_init.u32DestAddr = (uint32_t)((NULL != p_rx) ? p_rx : &s_dma_dummy_rx);
    dma_init.u32DataWidth = DMA_DATAWIDTH_8BIT;
    dma_init.u32BlockSize = 1UL;
    dma_init.u32TransCount = length;
    dma_init.u32SrcAddrInc = DMA_SRC_ADDR_FIX;
    dma_init.u32DestAddrInc = (NULL != p_rx) ? DMA_DEST_ADDR_INC : DMA_DEST_ADDR_FIX;
    result = DMA_Init(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_RX_CH, &dma_init);

    (void)DMA_StructInit(&dma_init);
    dma_init.u32IntEn = DMA_INT_DISABLE;
    dma_init.u32SrcAddr = (uint32_t)((NULL != p_tx) ? p_tx : &s_dma_dummy_tx);
    dma_init.u32DestAddr = (uint32_t)&BSP_W25Q64_SPI_UNIT->DR;
    dma_init.u32DataWidth = DMA_DATAWIDTH_8BIT;
    dma_init.u32BlockSize = 1UL;
    dma_init.u32TransCount = length;
    dma_init.u32SrcAddrInc = (NULL != p_tx) ? DMA_SRC_ADDR_INC : DMA_SRC_ADDR_FIX;
    dma_init.u32DestAddrInc = DMA_DEST_ADDR_FIX;
    if (LL_OK == result)
    {
        result = DMA_Init(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_TX_CH, &dma_init);
    }
    if (LL_OK != result)
    {
        bsp_w25q64_dma_stop();
        s_io_error = 1U;
        return BSP_W25Q64_RESULT_IO_ERROR;
    }

    AOS_SetTriggerEventSrc(BSP_W25Q64_DMA_RX_TRIGGER, EVT_SRC_SPI_SPRI);
    AOS_SetTriggerEventSrc(BSP_W25Q64_DMA_TX_TRIGGER, EVT_SRC_SPI_SPTI);
    (void)DMA_ChCmd(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_RX_CH, ENABLE);
    (void)DMA_ChCmd(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_TX_CH, ENABLE);
    DMA_MxChCmd(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_RX_MX_CH, ENABLE);
    DMA_MxChCmd(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_TX_MX_CH, ENABLE);

    timeout = BSP_W25Q64_SPI_TIMEOUT;
    while ((SET != DMA_GetTransCompleteStatus(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_RX_TC_FLAG)) ||
           (SET != DMA_GetTransCompleteStatus(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_TX_TC_FLAG)))
    {
        if (SET == DMA_GetErrStatus(BSP_W25Q64_DMA_UNIT, BSP_W25Q64_DMA_ERROR_FLAGS))
        {
            bsp_w25q64_dma_stop();
            s_io_error = 1U;
            return BSP_W25Q64_RESULT_IO_ERROR;
        }
        if (0UL == timeout)
        {
            bsp_w25q64_dma_stop();
            s_io_error = 1U;
            return BSP_W25Q64_RESULT_TIMEOUT;
        }
        timeout--;
    }

    timeout = BSP_W25Q64_SPI_TIMEOUT;
    while (RESET == SPI_GetStatus(BSP_W25Q64_SPI_UNIT, SPI_FLAG_IDLE))
    {
        if (0UL == timeout)
        {
            bsp_w25q64_dma_stop();
            s_io_error = 1U;
            return BSP_W25Q64_RESULT_TIMEOUT;
        }
        timeout--;
    }

    bsp_w25q64_dma_stop();
    DMA_ClearTransCompleteStatus(BSP_W25Q64_DMA_UNIT,
                                 BSP_W25Q64_DMA_RX_TC_FLAG | BSP_W25Q64_DMA_TX_TC_FLAG);
    return BSP_W25Q64_RESULT_SUCCESS;
}

static bsp_w25q64_result_t bsp_w25q64_status_read(uint8_t *p_status)
{
    uint8_t command = W25Q64_CMD_READ_STATUS_1;
    bsp_w25q64_result_t result;

    if (NULL == p_status)
    {
        return BSP_W25Q64_RESULT_INVALID_ARGUMENT;
    }

    bsp_w25q64_cs_low();
    result = bsp_w25q64_transmit(&command, 1UL);
    if (BSP_W25Q64_RESULT_SUCCESS == result)
    {
        result = bsp_w25q64_receive(p_status, 1UL);
    }
    bsp_w25q64_cs_high();

    return result;
}

static bsp_w25q64_result_t bsp_w25q64_write_enable(void)
{
    uint8_t command = W25Q64_CMD_WRITE_ENABLE;
    uint8_t status;
    bsp_w25q64_result_t result;

    bsp_w25q64_cs_low();
    result = bsp_w25q64_transmit(&command, 1UL);
    bsp_w25q64_cs_high();
    if (BSP_W25Q64_RESULT_SUCCESS != result)
    {
        return result;
    }

    result = bsp_w25q64_status_read(&status);
    if ((BSP_W25Q64_RESULT_SUCCESS == result) && (0U == (status & W25Q64_STATUS_1_WEL)))
    {
        result = BSP_W25Q64_RESULT_IO_ERROR;
        s_io_error = 1U;
    }

    return result;
}

static bsp_w25q64_result_t bsp_w25q64_wait_ready(void)
{
    uint32_t count;

    for (count = 0UL; count < BSP_W25Q64_SELF_TEST_WAIT_LIMIT; count++)
    {
        const bsp_w25q64_state_t state = bsp_w25q64_state_get();

        if (BSP_W25Q64_STATE_READY == state)
        {
            return BSP_W25Q64_RESULT_SUCCESS;
        }
        if (BSP_W25Q64_STATE_ERROR == state)
        {
            return BSP_W25Q64_RESULT_IO_ERROR;
        }
    }

    return BSP_W25Q64_RESULT_TIMEOUT;
}

bsp_w25q64_result_t bsp_w25q64_init(void)
{
    stc_gpio_init_t gpio_init;
    stc_spi_init_t spi_init;
    uint32_t jedec_id;
    int32_t result;

    s_initialized = 0U;
    s_io_error = 0U;

    GPIO_REG_Unlock();

    (void)GPIO_StructInit(&gpio_init);
    gpio_init.u16PinDir = PIN_DIR_OUT;
    gpio_init.u16PinDrv = PIN_HIGH_DRV;
    gpio_init.u16PinOutputType = PIN_OUT_TYPE_CMOS;
    gpio_init.u16PinState = PIN_STAT_RST;
    (void)GPIO_Init(BSP_W25Q64_SCK_PORT, BSP_W25Q64_SCK_PIN, &gpio_init);
    (void)GPIO_Init(BSP_W25Q64_MOSI_PORT, BSP_W25Q64_MOSI_PIN, &gpio_init);

    (void)GPIO_StructInit(&gpio_init);
    gpio_init.u16PinDir = PIN_DIR_IN;
    gpio_init.u16PullUp = PIN_PU_ON;
    (void)GPIO_Init(BSP_W25Q64_MISO_PORT, BSP_W25Q64_MISO_PIN, &gpio_init);

    (void)GPIO_StructInit(&gpio_init);
    gpio_init.u16PinDir = PIN_DIR_OUT;
    gpio_init.u16PinDrv = PIN_HIGH_DRV;
    gpio_init.u16PinOutputType = PIN_OUT_TYPE_CMOS;
    gpio_init.u16PinState = PIN_STAT_SET;
    (void)GPIO_Init(BSP_W25Q64_CS_PORT, BSP_W25Q64_CS_PIN, &gpio_init);

    GPIO_SetFunc(BSP_W25Q64_SCK_PORT, BSP_W25Q64_SCK_PIN, BSP_W25Q64_SCK_FUNC);
    GPIO_SetFunc(BSP_W25Q64_MOSI_PORT, BSP_W25Q64_MOSI_PIN, BSP_W25Q64_MOSI_FUNC);
    GPIO_SetFunc(BSP_W25Q64_MISO_PORT, BSP_W25Q64_MISO_PIN, BSP_W25Q64_MISO_FUNC);
    GPIO_OutputCmd(BSP_W25Q64_SCK_PORT, BSP_W25Q64_SCK_PIN, ENABLE);
    GPIO_OutputCmd(BSP_W25Q64_MOSI_PORT, BSP_W25Q64_MOSI_PIN, ENABLE);
    GPIO_OutputCmd(BSP_W25Q64_MISO_PORT, BSP_W25Q64_MISO_PIN, DISABLE);
    GPIO_OutputCmd(BSP_W25Q64_CS_PORT, BSP_W25Q64_CS_PIN, ENABLE);
    bsp_w25q64_cs_high();

    GPIO_REG_Lock();

    FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_SPI, ENABLE);
    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_DMA | FCG0_PERIPH_AOS, ENABLE);
    DMA_Cmd(BSP_W25Q64_DMA_UNIT, ENABLE);
    (void)SPI_StructInit(&spi_init);
    spi_init.u32WireMode = SPI_3_WIRE;
    spi_init.u32TransMode = SPI_FULL_DUPLEX;
    spi_init.u32MasterSlave = SPI_MASTER;
    spi_init.u32ModeFaultDetect = SPI_MD_FAULT_DETECT_DISABLE;
    spi_init.u32Parity = SPI_PARITY_INVD;
    spi_init.u32SpiMode = BSP_W25Q64_SPI_MODE;
    spi_init.u32BaudRatePrescaler = BSP_W25Q64_SPI_PRESCALER;
    spi_init.u32DataBits = SPI_DATA_SIZE_8BIT;
    spi_init.u32FirstBit = SPI_FIRST_MSB;
    spi_init.u32FrameLevel = SPI_1_FRAME;

    result = SPI_Init(BSP_W25Q64_SPI_UNIT, &spi_init);
    if (LL_OK != result)
    {
        s_io_error = 1U;
        return BSP_W25Q64_RESULT_IO_ERROR;
    }
    SPI_Cmd(BSP_W25Q64_SPI_UNIT, ENABLE);
    s_initialized = 1U;

    result = (int32_t)bsp_w25q64_read_jedec_id(&jedec_id);
    if (BSP_W25Q64_RESULT_SUCCESS != result)
    {
        return (bsp_w25q64_result_t)result;
    }
    if (BSP_W25Q64_EXPECTED_JEDEC_ID != jedec_id)
    {
        return BSP_W25Q64_RESULT_ID_MISMATCH;
    }

    return BSP_W25Q64_RESULT_SUCCESS;
}

bsp_w25q64_result_t bsp_w25q64_read_jedec_id(uint32_t *p_jedec_id)
{
    uint8_t command = W25Q64_CMD_READ_JEDEC_ID;
    uint8_t id[3] = {0U, 0U, 0U};
    bsp_w25q64_result_t result;

    if ((0U == s_initialized) || (NULL == p_jedec_id))
    {
        return BSP_W25Q64_RESULT_INVALID_ARGUMENT;
    }

    bsp_w25q64_cs_low();
    result = bsp_w25q64_transmit(&command, 1UL);
    if (BSP_W25Q64_RESULT_SUCCESS == result)
    {
        result = bsp_w25q64_receive(id, sizeof(id));
    }
    bsp_w25q64_cs_high();
    if (BSP_W25Q64_RESULT_SUCCESS == result)
    {
        *p_jedec_id = ((uint32_t)id[0] << 16U) | ((uint32_t)id[1] << 8U) | id[2];
    }

    return result;
}

bsp_w25q64_state_t bsp_w25q64_state_get(void)
{
    uint8_t status;

    if ((0U == s_initialized) || (0U != s_io_error))
    {
        return BSP_W25Q64_STATE_ERROR;
    }
    if (BSP_W25Q64_RESULT_SUCCESS != bsp_w25q64_status_read(&status))
    {
        return BSP_W25Q64_STATE_ERROR;
    }

    return (0U != (status & W25Q64_STATUS_1_WIP)) ? BSP_W25Q64_STATE_BUSY : BSP_W25Q64_STATE_READY;
}

bsp_w25q64_result_t bsp_w25q64_read(uint32_t address, uint32_t length, uint8_t *p_data)
{
    uint8_t command[4];
    bsp_w25q64_result_t result;

    if ((0UL != length) && (NULL == p_data))
    {
        return BSP_W25Q64_RESULT_INVALID_ARGUMENT;
    }
    if (0U == bsp_w25q64_range_is_valid(address, length))
    {
        return BSP_W25Q64_RESULT_OUT_OF_RANGE;
    }
    if (0UL == length)
    {
        return BSP_W25Q64_RESULT_SUCCESS;
    }
    if (BSP_W25Q64_STATE_READY != bsp_w25q64_state_get())
    {
        return BSP_W25Q64_RESULT_BUSY;
    }

    command[0] = W25Q64_CMD_READ_DATA;
    command[1] = (uint8_t)(address >> 16U);
    command[2] = (uint8_t)(address >> 8U);
    command[3] = (uint8_t)address;

    bsp_w25q64_cs_low();
    result = bsp_w25q64_transmit(command, sizeof(command));
    if (BSP_W25Q64_RESULT_SUCCESS == result)
    {
        result = bsp_w25q64_transfer_dma(NULL, p_data, length);
    }
    bsp_w25q64_cs_high();

    return result;
}

bsp_w25q64_result_t bsp_w25q64_page_program(uint32_t address,
                                            uint32_t length,
                                            const uint8_t *p_data)
{
    uint8_t command[4];
    bsp_w25q64_result_t result;

    if ((0UL == length) || (NULL == p_data))
    {
        return BSP_W25Q64_RESULT_INVALID_ARGUMENT;
    }
    if ((0U == bsp_w25q64_range_is_valid(address, length)) ||
        (length > BSP_W25Q64_PAGE_SIZE) ||
        (length > (BSP_W25Q64_PAGE_SIZE - (address % BSP_W25Q64_PAGE_SIZE))))
    {
        return BSP_W25Q64_RESULT_OUT_OF_RANGE;
    }
    if (BSP_W25Q64_STATE_READY != bsp_w25q64_state_get())
    {
        return BSP_W25Q64_RESULT_BUSY;
    }

    result = bsp_w25q64_write_enable();
    if (BSP_W25Q64_RESULT_SUCCESS != result)
    {
        return result;
    }

    command[0] = W25Q64_CMD_PAGE_PROGRAM;
    command[1] = (uint8_t)(address >> 16U);
    command[2] = (uint8_t)(address >> 8U);
    command[3] = (uint8_t)address;

    bsp_w25q64_cs_low();
    result = bsp_w25q64_transmit(command, sizeof(command));
    if (BSP_W25Q64_RESULT_SUCCESS == result)
    {
        result = bsp_w25q64_transfer_dma(p_data, NULL, length);
    }
    bsp_w25q64_cs_high();

    return result;
}

bsp_w25q64_result_t bsp_w25q64_sector_erase(uint32_t address)
{
    uint8_t command[4];
    bsp_w25q64_result_t result;

    if ((address >= BSP_W25Q64_CAPACITY_BYTES) || (0UL != (address % BSP_W25Q64_SECTOR_SIZE)))
    {
        return BSP_W25Q64_RESULT_OUT_OF_RANGE;
    }
    if (BSP_W25Q64_STATE_READY != bsp_w25q64_state_get())
    {
        return BSP_W25Q64_RESULT_BUSY;
    }

    result = bsp_w25q64_write_enable();
    if (BSP_W25Q64_RESULT_SUCCESS != result)
    {
        return result;
    }

    command[0] = W25Q64_CMD_SECTOR_ERASE_4K;
    command[1] = (uint8_t)(address >> 16U);
    command[2] = (uint8_t)(address >> 8U);
    command[3] = (uint8_t)address;

    bsp_w25q64_cs_low();
    result = bsp_w25q64_transmit(command, sizeof(command));
    bsp_w25q64_cs_high();

    return result;
}

bsp_w25q64_result_t bsp_w25q64_self_test(void)
{
    uint8_t write_data[BSP_W25Q64_SELF_TEST_LENGTH];
    uint8_t read_data[BSP_W25Q64_SELF_TEST_LENGTH];
    uint32_t offset;
    uint32_t chunk;
    bsp_w25q64_result_t result;

    for (offset = 0UL; offset < BSP_W25Q64_SELF_TEST_LENGTH; offset++)
    {
        write_data[offset] = (uint8_t)((offset * 37UL) ^ (offset >> 1U) ^ 0xA5UL);
        read_data[offset] = 0U;
    }

    result = bsp_w25q64_sector_erase(BSP_W25Q64_SELF_TEST_ADDRESS);
    if (BSP_W25Q64_RESULT_SUCCESS == result)
    {
        result = bsp_w25q64_wait_ready();
    }

    offset = 0UL;
    while ((BSP_W25Q64_RESULT_SUCCESS == result) && (offset < BSP_W25Q64_SELF_TEST_LENGTH))
    {
        chunk = BSP_W25Q64_PAGE_SIZE - ((BSP_W25Q64_SELF_TEST_ADDRESS + 0xF0UL + offset) % BSP_W25Q64_PAGE_SIZE);
        if (chunk > (BSP_W25Q64_SELF_TEST_LENGTH - offset))
        {
            chunk = BSP_W25Q64_SELF_TEST_LENGTH - offset;
        }
        result = bsp_w25q64_page_program(BSP_W25Q64_SELF_TEST_ADDRESS + 0xF0UL + offset,
                                         chunk,
                                         &write_data[offset]);
        if (BSP_W25Q64_RESULT_SUCCESS == result)
        {
            result = bsp_w25q64_wait_ready();
        }
        offset += chunk;
    }

    if (BSP_W25Q64_RESULT_SUCCESS == result)
    {
        result = bsp_w25q64_read(BSP_W25Q64_SELF_TEST_ADDRESS + 0xF0UL,
                                  BSP_W25Q64_SELF_TEST_LENGTH,
                                  read_data);
    }
    if (BSP_W25Q64_RESULT_SUCCESS == result)
    {
        for (offset = 0UL; offset < BSP_W25Q64_SELF_TEST_LENGTH; offset++)
        {
            if (write_data[offset] != read_data[offset])
            {
                result = BSP_W25Q64_RESULT_VERIFY_ERROR;
                break;
            }
        }
    }

    return result;
}
