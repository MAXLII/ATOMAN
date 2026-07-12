/**
 *******************************************************************************
 * @file  hc32_hal_smbus.c
 * @brief This file provides firmware functions to manage the SMBus protocol.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2026-04-16       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2026, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_hal_smbus.h"
#include "hc32_ll_utility.h"
#include "hc32_ll_i2c.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup HAL_SMBus SMBus
 * @brief SMBUS Driver Library
 * @{
 */

#if (HAL_SMBUS_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup HAL_SMBus_Local_Macros HAL SMBus Local Macros
 * @{
 */

#define MAX_NBYTE_SIZE                  (255U)

/* SMBUS slave transfer interrupt */
#define SMBUS_INT_SLAVE_TX              (I2C_INT_TX_CPLT | I2C_INT_NACK)
#define SMBUS_INT_SLAVE_RX              (I2C_INT_RX_FULL | I2C_INT_NACK)

#define HAL_SMBUS_STAT_MASTER_SLAVE_MASK (0x000000F7UL)

/* SMBUS address */
#define SMBUS_ADDR_GENERAL_CALL         (0x00UL)
#define SMBUS_ADDR_DEFAULT              (0xC2UL >> 1U)
#define SMBUS_ADDR_HOST                 (0x10UL >> 1U)
#define SMBUS_ADDR_ARA                  (0x18UL >> 1U)

/* Register mask */
#define I2C_CR1_RW_MASK                 (0x2EUL)
#define I2C_CR2_RW_MASK                 (0x00F04206UL)

/**
 * @defgroup SMBUS_Com_State_define SMBus Transfer Communication State definition
 * @{
 */

#define SMBUS_I2C_STAT_IDLE             (0x00000001UL)   /*!< SMBUS Idle */
#define SMBUS_I2C_STAT_START            (0x00000002UL)   /*!< SMBUS Start finished */
#define SMBUS_I2C_STAT_ADDR_TX          (0x00000003UL)   /*!< SMBUS device address and RW bit send finished */
#define SMBUS_I2C_STAT_TX               (0x00000004UL)   /*!< SMBUS data TX stage */
#define SMBUS_I2C_STAT_ADDR_RX          (0x00000005UL)   /*!< SMBUS device address and RW bit send finished */
#define SMBUS_I2C_STAT_RX               (0x00000006UL)   /*!< SMBUS data RX stage */
#define SMBUS_I2C_STAT_RESTART          (0x00000007UL)   /*!< SMBUS ReStart finished */
#define SMBUS_I2C_STAT_STOP             (0x00000009UL)
/**
  * @}
  */

#if (USE_RTOS == 1U)
#error " USE_RTOS should be 0 in the current HAL release "
#else
#define __HAL_LOCK(__HANDLE__)                                           \
                                do{                                        \
                                    if((__HANDLE__)->u8Lock == HAL_LOCKED)   \
                                    {                                      \
                                       return HAL_BUSY;                    \
                                    }                                      \
                                    else                                   \
                                    {                                      \
                                       (__HANDLE__)->u8Lock = HAL_LOCKED;    \
                                    }                                      \
                                  }while (0U)

#define __HAL_UNLOCK(__HANDLE__)                                          \
                                  do{                                       \
                                      (__HANDLE__)->u8Lock = HAL_UNLOCKED;    \
                                    }while (0U)
#endif /* USE_RTOS */

/**
 * @defgroup SMBUS_Check_Parameters_Validity SMBUS Check Parameters Validity
 * @{
 */

#define IS_SMBUS_XFER_OPT(x)                                                   \
(   (((x) & HAL_SMBUS_FIRST_LAST_FRAME) == HAL_SMBUS_FIRST_LAST_FRAME)      || \
    (((x) & HAL_SMBUS_FIRST_LAST_FRAME) == HAL_SMBUS_FIRST_NEXT_FRAME)      || \
    (((x) & HAL_SMBUS_FIRST_LAST_FRAME) == HAL_SMBUS_NEXT_LAST_FRAME)       || \
    (((x) & HAL_SMBUS_FIRST_LAST_FRAME) == HAL_SMBUS_NEXT_FRAME))

#define IS_SMBUS_PERIPH_MD(x)                                                  \
(   ((x) == I2C_SMBUS_MATCH_HOST)                   ||                         \
    ((x) == I2C_SMBUS_MATCH_DEFAULT))

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

static int32_t I2C_BaudInit(CM_I2C_TypeDef *I2Cx, uint32_t u32Baudrate);
static en_flag_status_t I2C_IntStdGet(CM_I2C_TypeDef *I2Cx, uint32_t u32IntType);
static uint32_t I2C_GetAddr(CM_I2C_TypeDef *I2Cx);

static void MasterTansCpltCallback(stc_hal_smbus_handle_t *pHalSmbus);
static void MasterDataFinish(stc_hal_smbus_handle_t *pHalSmbus);
static void MasterStopHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void MasterStartHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void MasterReceiveDataHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void MasterTxEmptyHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void MasterTxCompleteHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void SlaveNackHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void SlaveAddrHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void SlaveTxCompleteHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void SlaveReceiveDataHandler(stc_hal_smbus_handle_t *pHalSmbus);
static void SlaveStopHandler(stc_hal_smbus_handle_t *pHalSmbus);

static int32_t SMBUSMasterISR(stc_hal_smbus_handle_t *pHalSmbus);
static int32_t SMBUSSlaveISR(stc_hal_smbus_handle_t *pHalSmbus);

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/

/**
  * @brief  I2C Event interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
void I2C_EE_IrqHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return;
    }

    if ((SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_START)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_START))) {
        if (((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_MASTER_BUSY_TX) ||
            (pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_MASTER_BUSY_RX) {
            (void)SMBUSMasterISR(pHalSmbus);
        }
    }

    if ((SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_STOP)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_STOP))) {
        if (SMBUS_I2C_STAT_STOP == pHalSmbus->u32ComState) {
            (void)SMBUSMasterISR(pHalSmbus);
        } else {
            (void)SMBUSSlaveISR(pHalSmbus);
        }
    }

    if ((SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_MATCH_ADDR0)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0))) {
        (void)SMBUSSlaveISR(pHalSmbus);
    }

    if ((SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_NACKF)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_NACK))) {
        if (((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_MASTER_BUSY_TX) ||
            (pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_MASTER_BUSY_RX) {
            (void)SMBUSMasterISR(pHalSmbus);
        } else {
            (void)SMBUSSlaveISR(pHalSmbus);
        }
    }

    if (SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_GENERAL_CALL | I2C_FLAG_SMBUS_DEFAULT_MATCH |
                             I2C_FLAG_SMBUS_HOST_MATCH | I2C_FLAG_SMBUS_ALERT_MATCH)) {
        (void)SMBUSSlaveISR(pHalSmbus);
    }

    if ((SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_TMOUTF)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_TMOUTIE))) {
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_TMOUTF);
        if (pHalSmbus->u32TimeoutCnt++ == pHalSmbus->stcInit.u32SMBusTimeout) {
            /* SMBus Timeout */
            pHalSmbus->u32TimeoutCnt = 0U;
            pHalSmbus->u32ErrorCode |= HAL_SMBUS_ERR_TIMEOUT;
            if (((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_SLAVE_BUSY_TX) ||
                (pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_SLAVE_BUSY_RX) {
                pHalSmbus->u32State = HAL_SMBUS_STAT_LISTEN;
            }

            /* Call the Error callback to inform upper layer */
            HAL_SMBUS_ErrorCallback(pHalSmbus);
        }
    }

    if ((SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_ARBITRATE_FAIL)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_ARBITRATE_FAIL))) {
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_ARBITRATE_FAIL);
        pHalSmbus->u32ErrorCode |= HAL_SMBUS_ERR_ARLO;
        if (((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_SLAVE_BUSY_TX) ||
            (pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_SLAVE_BUSY_RX) {
            pHalSmbus->u32State = HAL_SMBUS_STAT_LISTEN;
        }
        /* Call the Error callback to inform upper layer */
        HAL_SMBUS_ErrorCallback(pHalSmbus);
    }

#ifdef SMBUS_ALERT
    if ((SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_SMBALERT)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_SMBALERT_PIN))) {
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_SMBALERT);
        pHalSmbus->u32ErrorCode |= HAL_SMBUS_ERR_ALERT;
        /* Call the Error callback to inform upper layer */
        HAL_SMBUS_ErrorCallback(pHalSmbus);
    }
#endif
}

/**
  * @brief  I2C transfer complete interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
void I2C_TxComplete_IrqHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return;
    }

    if (((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_MASTER_BUSY_TX) ||
        ((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_MASTER_BUSY_RX)) {
        (void)SMBUSMasterISR(pHalSmbus);
    } else {
        (void)SMBUSSlaveISR(pHalSmbus);
    }
}

/**
  * @brief  I2C transfer empty interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
void I2C_TxEmpty_IrqHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return;
    }

    if ((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_MASTER_BUSY_TX) {
        (void)SMBUSMasterISR(pHalSmbus);
    }
}

/**
  * @brief  I2C receive full interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
void I2C_RxFull_IrqHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return;
    }

    if ((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_MASTER_BUSY_RX) {
        (void)SMBUSMasterISR(pHalSmbus);
    } else if ((pHalSmbus->u32State & HAL_SMBUS_STAT_MASTER_SLAVE_MASK) == HAL_SMBUS_STAT_SLAVE_BUSY_RX) {
        (void)SMBUSSlaveISR(pHalSmbus);
    }
}

/**
 * @defgroup HAL_SMBus_Global_Functions HAL SMBus Global Functions
 * @{
 */

/**
  * @brief  Initialize the SMBUS according to the specified parameters
  *         in the stc_hal_smbus_init_t and initialize the associated handle.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_Init(stc_hal_smbus_handle_t *pHalSmbus)
{
    /* Check the SMBUS handle allocation */
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return HAL_ERR;
    }

    DDL_ASSERT(IS_SMBUS_PERIPH_MD(pHalSmbus->stcInit.u32PeriphMode));

    if (pHalSmbus->u32State == HAL_SMBUS_STAT_RST) {
        /* Allocate lock resource and initialize it */
        pHalSmbus->u8Lock = HAL_UNLOCKED;

        /* Init the low level hardware : GPIO, CLOCK, NVIC */
        HAL_SMBUS_MspInit(pHalSmbus);
    }

    pHalSmbus->u32State = HAL_SMBUS_STAT_BUSY;

    /* Disable the selected SMBUS peripheral */
    I2C_Cmd(pHalSmbus->I2Cx, DISABLE);
    if (LL_OK != I2C_BaudInit(pHalSmbus->I2Cx, pHalSmbus->stcInit.u32Baudrate)) {
        return HAL_ERR;
    }

    I2C_AnalogFilterCmd(pHalSmbus->I2Cx, pHalSmbus->stcInit.enAnalogFilter);

    I2C_SlaveAddrCmd(pHalSmbus->I2Cx, I2C_ADDR0, DISABLE);
    if (0UL != pHalSmbus->stcInit.u32OwnAddr1) {
        I2C_SlaveAddrConfig(pHalSmbus->I2Cx, I2C_ADDR0, pHalSmbus->stcInit.u32AddrMode, pHalSmbus->stcInit.u32OwnAddr1);
        I2C_SlaveAddrCmd(pHalSmbus->I2Cx, I2C_ADDR0, ENABLE);
    }

    if ((HAL_SMBUS_DUAL_ADDR_ENABLE == pHalSmbus->stcInit.u32DualAddrMode) && (0UL != pHalSmbus->stcInit.u32OwnAddr2)) {
        I2C_SlaveAddrConfig(pHalSmbus->I2Cx, I2C_ADDR1, pHalSmbus->stcInit.u32AddrMode, pHalSmbus->stcInit.u32OwnAddr2);
        I2C_SlaveAddrMaskConfig(pHalSmbus->I2Cx, I2C_ADDR1, pHalSmbus->stcInit.u32AddrMode, pHalSmbus->stcInit.u32OwnAddr2Masks);
        I2C_SlaveMaskAddrCmd(pHalSmbus->I2Cx, I2C_ADDR1, ENABLE);
        I2C_SlaveAddrCmd(pHalSmbus->I2Cx, I2C_ADDR1, ENABLE);
    }

    /* General call function config and interrupt enable */
    I2C_GeneralCallCmd(pHalSmbus->I2Cx, pHalSmbus->stcInit.enGeneralCall);
    if (ENABLE == pHalSmbus->stcInit.enGeneralCall) {
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_GENERAL_CALL, ENABLE);
    }

    /* SMBUS address config and interrupt enable */
    if (0UL != pHalSmbus->stcInit.u32PeriphMode) {
        I2C_SmbusMatchAddrCmd(pHalSmbus->I2Cx, pHalSmbus->stcInit.u32PeriphMode, ENABLE);
        if (0UL != (pHalSmbus->stcInit.u32PeriphMode & I2C_SMBUS_MATCH_DEFAULT)) {
            I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_SMBUS_DEFAULT_MATCH, ENABLE);
        }
        if (0UL != (pHalSmbus->stcInit.u32PeriphMode & I2C_SMBUS_MATCH_HOST)) {
            I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_SMBUS_HOST_MATCH, ENABLE);
        }
        I2C_SmbusCmd(pHalSmbus->I2Cx, ENABLE);
    }

    /* Timeout */
    if (pHalSmbus->stcInit.u32SMBusTimeout != 0U) {
        pHalSmbus->u32TimeoutCnt = 0U;
        I2C_TimeoutClkDiv(pHalSmbus->I2Cx, I2C_TIMEOUT_CLK_DIV4);
        I2C_SCLTimeoutSumCmd(pHalSmbus->I2Cx, DISABLE);

        I2C_SCLTimeoutAConfig(pHalSmbus->I2Cx, (uint16_t)(I2C_SRC_CLK / (1UL << I2C_TIMEOUT_CLK_DIV4) / 10000UL)); /* Count for 0.1mS */
        I2C_SCLLowTimeoutCmd(pHalSmbus->I2Cx, ENABLE);
        I2C_SCLTimeoutCmd(pHalSmbus->I2Cx, ENABLE);
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TMOUTIE, ENABLE);
    } else {
        I2C_SCLTimeoutAConfig(pHalSmbus->I2Cx, 0U);
        I2C_SCLLowTimeoutCmd(pHalSmbus->I2Cx, DISABLE);
        I2C_SCLTimeoutCmd(pHalSmbus->I2Cx, DISABLE);
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TMOUTIE, DISABLE);
    }

    /* Enable arbitration interrupt */
    I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_ARBITRATE_FAIL, ENABLE);
    /* Enable PEC calculate */
    I2C_SmbusNBConfig(pHalSmbus->I2Cx, I2C_SMBUS_PEC, ENABLE);
    I2C_MasterReadAddrIntCmd(pHalSmbus->I2Cx, ENABLE);

#ifdef SMBUS_ALERT
    /* Enable alert function for host */
    if (0UL != (pHalSmbus->stcInit.u32PeriphMode & I2C_SMBUS_MATCH_HOST)) {
        I2C_SmbusAlertPinCmd(pHalSmbus->I2Cx, ENABLE);
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_SMBALERT_PIN, ENABLE);
    }
#endif

    I2C_Cmd(pHalSmbus->I2Cx, ENABLE);

    pHalSmbus->u32ErrorCode = HAL_SMBUS_ERR_NONE;
    pHalSmbus->u32State = HAL_SMBUS_STAT_RDY;
    pHalSmbus->u32ComState = SMBUS_I2C_STAT_IDLE;

    return HAL_OK;
}


/**
  * @brief  DeInitialize the SMBUS peripheral.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_DeInit(stc_hal_smbus_handle_t *pHalSmbus)
{
    /* Check the SMBUS handle allocation */
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return HAL_ERR;
    }

    pHalSmbus->u32State = HAL_SMBUS_STAT_BUSY;

    /* Disable the SMBUS Peripheral Clock */
    I2C_Cmd(pHalSmbus->I2Cx, DISABLE);

    /* DeInit the low level hardware: GPIO, CLOCK, NVIC */
    HAL_SMBUS_MspDeInit(pHalSmbus);

    pHalSmbus->u32ErrorCode = HAL_SMBUS_ERR_NONE;
    pHalSmbus->u32State = HAL_SMBUS_STAT_RST;

    /* Release Lock */
    __HAL_UNLOCK(pHalSmbus);

    return HAL_OK;
}

 /**
 * @brief  Initialize the SMBUS MSP.
 * @param  [in] pHalSmbus  Pointer to a HAL SMBUS handle structure
 * @retval None
 */
__WEAKDEF void HAL_SMBUS_MspInit(stc_hal_smbus_handle_t *pHalSmbus)
{
    (void)(pHalSmbus);
}

/**
  * @brief DeInitialize the SMBUS MSP.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
__WEAKDEF void HAL_SMBUS_MspDeInit(stc_hal_smbus_handle_t *pHalSmbus)
{
    (void)(pHalSmbus);
}

/**
  * @brief  Transmit in master/host SMBUS mode an amount of data with Interrupt.
  * @param [in] pHalSmbus       Pointer to a stc_hal_smbus_handle_t structure that contains
  *                             the configuration information for the specified SMBUS.
  * @param [in] u16DevAddr      Target device address: The device 7 bits address value
  * @param [in] pData           Pointer to data buffer
  * @param [in] u16Size         Amount of data to be sent
  * @param [in] u32XferOption   Options of Transfer, value of @ref HAL_SMBUS_Xfer_Option,
  *                             SMBUS_FRAME_PEC bit is valid for PEC data transmission.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_MasterTransmitIT(stc_hal_smbus_handle_t *pHalSmbus, uint16_t u16DevAddr, uint8_t *pData, uint16_t u16Size, uint32_t u32XferOption)
{
    uint16_t u16XferSizeLimit;
    /* Check the parameters */
    DDL_ASSERT(IS_SMBUS_XFER_OPT(u32XferOption));

    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL) || ((u16Size != 0U) && (pData == NULL))) {
        return HAL_ERR;
    }

    if (pHalSmbus->u32State == HAL_SMBUS_STAT_RDY) {
        /* Process Locked */
        __HAL_LOCK(pHalSmbus);

        if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)) {
            /* Have PEC BYTE */
            u16XferSizeLimit = MAX_NBYTE_SIZE + 1U;
        } else {
            u16XferSizeLimit = MAX_NBYTE_SIZE;
        }

        pHalSmbus->u32State = HAL_SMBUS_STAT_MASTER_BUSY_TX;
        pHalSmbus->u32ErrorCode = HAL_SMBUS_ERR_NONE;
        /* Prepare transfer parameters */
        pHalSmbus->pBuf = pData;
        pHalSmbus->u16DataIndex = 0U;
        if (u16XferSizeLimit < u16Size) {
            pHalSmbus->u16XferSize = u16XferSizeLimit;
        } else {
            pHalSmbus->u16XferSize = u16Size;
        }
        pHalSmbus->u32XferOption = u32XferOption;
        pHalSmbus->u16DevAddr = u16DevAddr;
        pHalSmbus->u32TimeoutCnt = 0U;

        /* NBYTE config */
        I2C_SetNByte(pHalSmbus->I2Cx, (uint8_t)pHalSmbus->u16XferSize);
        I2C_SmbusNBConfig(pHalSmbus->I2Cx, I2C_SMBUS_NBYTE, ENABLE);

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);

        /* Enable START interrupt */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_START, ENABLE);

        if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_START)) {
            /* Generate start signal */
            pHalSmbus->u32ComState = SMBUS_I2C_STAT_START;
            I2C_GenerateStart(pHalSmbus->I2Cx);
        } else {
            /* Generate Restart signal */
            pHalSmbus->u32ComState = SMBUS_I2C_STAT_RESTART;
            I2C_GenerateRestart(pHalSmbus->I2Cx);
        }

        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
  * @brief  Receive in master/host SMBUS mode an amount of data with Interrupt.
  * @param [in] pHalSmbus       Pointer to a stc_hal_smbus_handle_t structure that contains
  *                             the configuration information for the specified SMBUS.
  * @param [in] u16DevAddr      Target device address: The device 7 bits address value
  * @param [in] pData           Pointer to data buffer
  * @param [in] u16Size         Amount of data to be receive
  * @param [in] u32XferOption   Options of Transfer, value of @ref HAL_SMBUS_Xfer_Option,
  *                             SMBUS_FRAME_PEC bit is valid for PEC data transmission.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_MasterReceiveIT(stc_hal_smbus_handle_t *pHalSmbus, uint16_t u16DevAddr, uint8_t *pData, uint16_t u16Size, uint32_t u32XferOption)
{
    uint16_t u16XferSizeLimit;
    /* Check the parameters */
    DDL_ASSERT(IS_SMBUS_XFER_OPT(u32XferOption));

    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL) || ((u16Size != 0U) && (pData == NULL))) {
        return HAL_ERR;
    }

    if (pHalSmbus->u32State == HAL_SMBUS_STAT_RDY) {
        /* Process Locked */
        __HAL_LOCK(pHalSmbus);

        pHalSmbus->u32State = HAL_SMBUS_STAT_MASTER_BUSY_RX;

        pHalSmbus->u32ErrorCode = HAL_SMBUS_ERR_NONE;

        if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)) {
            /* Have PEC BYTE */
            u16XferSizeLimit = MAX_NBYTE_SIZE + 1U;
        } else {
            u16XferSizeLimit = MAX_NBYTE_SIZE;
        }
        /* Prepare transfer parameters */
        pHalSmbus->pBuf = pData;
        pHalSmbus->u16DataIndex = 0U;
        if (u16XferSizeLimit < u16Size) {
            pHalSmbus->u16XferSize = u16XferSizeLimit;
        } else {
            pHalSmbus->u16XferSize = u16Size;
        }
        pHalSmbus->u32XferOption = u32XferOption;
        pHalSmbus->u16DevAddr = u16DevAddr;
        pHalSmbus->u32TimeoutCnt = 0U;

        /* NBYTE config */
        I2C_SetNByte(pHalSmbus->I2Cx, (uint8_t)pHalSmbus->u16XferSize);
        I2C_SmbusNBConfig(pHalSmbus->I2Cx, I2C_SMBUS_NBYTE | I2C_SMBUS_AUTONAK, ENABLE);

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);

        if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_START)) {
            /* Enable START interrupt */
            I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_START, ENABLE);
            /* Generate start signal */
            pHalSmbus->u32ComState = SMBUS_I2C_STAT_START;
            I2C_GenerateStart(pHalSmbus->I2Cx);
        } else {
            /* Generate Restart signal */
            pHalSmbus->u32ComState = SMBUS_I2C_STAT_RESTART;
            I2C_GenerateRestart(pHalSmbus->I2Cx);
        }

        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
  * @brief  Transmit in slave/device SMBUS mode an amount of data in non-blocking mode with Interrupt.
  * @param [in] pHalSmbus       Pointer to a stc_hal_smbus_handle_t structure that contains
  *                             the configuration information for the specified SMBUS.
  * @param [in] pData           Pointer to data buffer
  * @param [in] u16Size         Amount of data to be sent
  * @param [in] u32XferOption   Options of Transfer, value of @ref HAL_SMBUS_Xfer_Option,
  *                             SMBUS_FRAME_PEC bit is valid for PEC data transmission.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_SlaveTransmitIT(stc_hal_smbus_handle_t *pHalSmbus, uint8_t *pData, uint16_t u16Size, uint32_t u32XferOption)
{
    uint8_t u8Data;
    uint16_t u16XferSizeLimit;
    /* Check the parameters */
    DDL_ASSERT(IS_SMBUS_XFER_OPT(u32XferOption));

    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL) || ((u16Size != 0U) && (pData == NULL))) {
        return HAL_ERR;
    }

    if ((pHalSmbus->u32State & HAL_SMBUS_STAT_LISTEN) == HAL_SMBUS_STAT_LISTEN) {
        if ((pData == NULL) || (u16Size == 0U)) {
            return HAL_ERR;
        }

        /* Disable Interrupts, to prevent preemption during treatment in case of multicall */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0 | I2C_INT_STOP | SMBUS_INT_SLAVE_TX, DISABLE);

        /* Process Locked */
        __HAL_LOCK(pHalSmbus);

        if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)) {
            /* Have PEC BYTE */
            u16XferSizeLimit = MAX_NBYTE_SIZE + 1U;
        } else {
            u16XferSizeLimit = MAX_NBYTE_SIZE;
        }
        pHalSmbus->u32State = (HAL_SMBUS_STAT_SLAVE_BUSY_TX | HAL_SMBUS_STAT_LISTEN);
        pHalSmbus->u32ErrorCode = HAL_SMBUS_ERR_NONE;
        /* Prepare transfer parameters */
        pHalSmbus->pBuf = pData;
        if (u16XferSizeLimit < u16Size) {
            pHalSmbus->u16XferSize = u16XferSizeLimit;
        } else {
            pHalSmbus->u16XferSize = u16Size;
        }
        pHalSmbus->u32XferOption = u32XferOption;
        pHalSmbus->u16DataIndex = 0U;

        I2C_AckConfig(pHalSmbus->I2Cx, I2C_ACK);

        /* Clear ADDR flag after prepare the transfer parameters */
        /* This action will generate an acknowledge to the HOST */
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_MATCH_ADDR0);

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);

        /* REnable ADDR interrupt */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0 | I2C_INT_STOP | SMBUS_INT_SLAVE_TX, ENABLE);

        /* Send first data */
        if ((0UL != pHalSmbus->u16XferSize)) {
            u8Data = *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex);
            I2C_WriteData(pHalSmbus->I2Cx, u8Data);
            pHalSmbus->u16DataIndex++;
        }

        return HAL_OK;
    } else {
        return HAL_ERR;
    }
}

/**
  * @brief  Receive in slave/device SMBUS mode an amount of data in non-blocking mode with Interrupt.
  * @param [in] pHalSmbus       Pointer to a stc_hal_smbus_handle_t structure that contains
  *                             the configuration information for the specified SMBUS.
  * @param [in] pData           Pointer to data buffer
  * @param [in] u16Size         Amount of data to be sent
  * @param [in] u32XferOption   Options of Transfer, value of @ref HAL_SMBUS_Xfer_Option,
  *                             SMBUS_FRAME_PEC bit is valid for PEC data transmission.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_SlaveReceiveIT(stc_hal_smbus_handle_t *pHalSmbus, uint8_t *pData, uint16_t u16Size, uint32_t u32XferOption)
{
    uint8_t u8Data;
    uint16_t u16XferSizeLimit;
    /* Check the parameters */
    DDL_ASSERT(IS_SMBUS_XFER_OPT(u32XferOption));

    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL) || ((u16Size != 0U) && (pData == NULL))) {
        return HAL_ERR;
    }

    if ((pHalSmbus->u32State & HAL_SMBUS_STAT_LISTEN) == HAL_SMBUS_STAT_LISTEN) {
        if ((pData == NULL) || (u16Size == 0U)) {
            return HAL_ERR;
        }

        /* Disable Interrupts, to prevent preemption during treatment in case of multi-call */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0, DISABLE);
        /* Process Locked */
        __HAL_LOCK(pHalSmbus);

        pHalSmbus->u32State = (HAL_SMBUS_STAT_SLAVE_BUSY_RX | HAL_SMBUS_STAT_LISTEN);
        pHalSmbus->u32ErrorCode = HAL_SMBUS_ERR_NONE;

        if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)) {
            /* Have PEC BYTE */
            u16XferSizeLimit = MAX_NBYTE_SIZE + 1U;
        } else {
            u16XferSizeLimit = MAX_NBYTE_SIZE;
        }
        /* Prepare transfer parameters */
        pHalSmbus->pBuf = pData;
        if (u16XferSizeLimit < u16Size) {
            pHalSmbus->u16XferSize = u16XferSizeLimit;
        } else {
            pHalSmbus->u16XferSize = u16Size;
        }
        pHalSmbus->u32XferOption = u32XferOption;
        pHalSmbus->u16DataIndex = 0U;

        /* Clear ADDR flag after prepare the transfer parameters */
        /* This action will generate an acknowledge to the HOST */
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_MATCH_ADDR0);

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
        /* Enable ADDR interrupt */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0 | I2C_INT_STOP | SMBUS_INT_SLAVE_RX, ENABLE);

        /* Master should send first data after address matched with interrupt. So should read first data here */
        if ((SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_RX_FULL)) && (0UL < pHalSmbus->u16XferSize)) {
            u8Data = I2C_ReadData(pHalSmbus->I2Cx);
            *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex) = u8Data;
            pHalSmbus->u16DataIndex++;
        }
        return HAL_OK;
    } else {
        return HAL_ERR;
    }
}

/**
  * @brief  Enable the Address listen mode with Interrupt.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_EnableListenIT(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return HAL_ERR;
    }

    pHalSmbus->u32State = HAL_SMBUS_STAT_LISTEN;

    /* Enable the Address Match interrupt */
    I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0 | I2C_INT_STOP | I2C_INT_NACK, ENABLE);

    return HAL_OK;
}

/**
  * @brief  Disable the Address listen mode with Interrupt.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_DisableListenIT(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return HAL_ERR;
    }

    /* Disable Address listen mode only if a transfer is not ongoing */
    if (pHalSmbus->u32State == HAL_SMBUS_STAT_LISTEN) {
        pHalSmbus->u32State = HAL_SMBUS_STAT_RDY;

        /* Disable the Address Match interrupt */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0 | I2C_INT_STOP | I2C_INT_NACK, DISABLE);

        return HAL_OK;
    } else {
        return HAL_BUSY;
    }
}

/**
  * @brief  Enable the SMBUS alert mode with Interrupt.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUSx peripheral.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_EnableAlertIT(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return HAL_ERR;
    }

    /* Enable SMBus alert */
    I2C_SmbusMatchAddrCmd(pHalSmbus->I2Cx, I2C_SMBUS_MATCH_ALERT, ENABLE);

    /* Clear ALERT flag */
    I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_SMBUS_ALERT_MATCH);

    /* Enable Alert Interrupt */
    I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_SMBUS_ALERT_MATCH, ENABLE);

    return HAL_OK;
}

/**
  * @brief  Disable the SMBUS alert mode with Interrupt.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUSx peripheral.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
int32_t HAL_SMBUS_DisableAlertIT(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus == NULL) || (pHalSmbus->I2Cx == NULL)) {
        return HAL_ERR;
    }

    /* Disable Alert Interrupt */
    I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_SMBUS_ALERT_MATCH, DISABLE);

    return HAL_OK;
}

/**
  * @brief  Master Tx Transfer completed callback.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
__WEAKDEF void HAL_SMBUS_MasterTxCpltCallback(stc_hal_smbus_handle_t *pHalSmbus)
{
    (void)(pHalSmbus);
}

/**
  * @brief  Master Rx Transfer completed callback.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
__WEAKDEF void HAL_SMBUS_MasterRxCpltCallback(stc_hal_smbus_handle_t *pHalSmbus)
{
    (void)(pHalSmbus);
}

/** @brief  Slave Tx Transfer completed callback.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
__WEAKDEF void HAL_SMBUS_SlaveTxCpltCallback(stc_hal_smbus_handle_t *pHalSmbus)
{
    (void)(pHalSmbus);
}

/**
  * @brief  Slave Rx Transfer completed callback.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
__WEAKDEF void HAL_SMBUS_SlaveRxCpltCallback(stc_hal_smbus_handle_t *pHalSmbus)
{
    (void)(pHalSmbus);
}

/**
  * @brief  Slave Address Match callback.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @param [in] u8XferDir   Master request Transfer Direction @ref HAL_SMBUS_Slave_Dir
  * @param [in] u16AddrMatchCode Address Match Code
  * @retval None
  */
__WEAKDEF void HAL_SMBUS_AddrCallback(stc_hal_smbus_handle_t *pHalSmbus, uint8_t u8XferDir, uint16_t u16AddrMatchCode)
{
    (void)(pHalSmbus);
    (void)(u8XferDir);
    (void)(u16AddrMatchCode);
}

/**
  * @brief  Listen Complete callback.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
__WEAKDEF void HAL_SMBUS_ListenCpltCallback(stc_hal_smbus_handle_t *pHalSmbus)
{
    (void)(pHalSmbus);
}

/**
  * @brief  SMBUS error callback.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
__WEAKDEF void HAL_SMBUS_ErrorCallback(stc_hal_smbus_handle_t *pHalSmbus)
{
    (void)(pHalSmbus);
}

/**
 * @}
 */

/**
 * @addtogroup HAL_SMBus_Private_Functions HAL SMBus Private Functions
 * @{
 */

/**
 * @brief  Initialize the I2C baudrate.
 * @param [in] I2Cx                 Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2C or CM_I2Cx:  I2C instance register base.
 * @param [in] u32Baudrate          The target SMBUS communication baudrate.
 * @retval int32_t
 *         - LL_OK:                 Success
 *         - LL_ERR_TIMEOUT:        Failed
 *         - LL_ERR_INVD_PARAM:     Parameter error
*/
static int32_t I2C_BaudInit(CM_I2C_TypeDef *I2Cx, uint32_t u32Baudrate)
{
    int32_t i32Ret;
    float32_t fErr;
    stc_i2c_init_t stcI2cInit;
    uint32_t I2cSrcClk;
    uint32_t I2cClkDiv;
    uint32_t I2cClkDivReg;

    DDL_ASSERT(u32Baudrate != 0UL);
    I2cSrcClk = I2C_SRC_CLK;
    I2cClkDiv = I2cSrcClk / u32Baudrate / I2C_WIDTH_MAX_IMME;
    for (I2cClkDivReg = I2C_CLK_DIV1; I2cClkDivReg <= I2C_CLK_DIV128; I2cClkDivReg++) {
        if (I2cClkDiv < (1UL << I2cClkDivReg)) {
            break;
        }
    }

    (void)I2C_StructInit(&stcI2cInit);
    stcI2cInit.u32Baudrate = u32Baudrate;
    stcI2cInit.u32SclTime  = (uint32_t)((uint64_t)250UL * ((uint64_t)I2cSrcClk / ((uint64_t)1UL << I2cClkDivReg)) / (uint64_t)(1UL * 1000UL * 1000UL * 1000UL)); /* SCL time is about 250nS in EVB board */
    stcI2cInit.u32ClockDiv = I2cClkDivReg;
    i32Ret = I2C_Init(I2Cx, &stcI2cInit, &fErr);

    if (LL_OK == i32Ret) {
        I2C_BusWaitCmd(I2Cx, ENABLE);
    }

    return i32Ret;
}

/**
 * @brief  I2C interrupt configuration status get.
 * @param [in] I2Cx                 Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2C or CM_I2Cx:  I2C instance register base.
 * @param [in] u32IntType           Specifies the I2C interrupts @ref I2C_Int_Flag
 * @retval An @ref en_flag_status_t enumeration type value
*/
static en_flag_status_t I2C_IntStdGet(CM_I2C_TypeDef *I2Cx, uint32_t u32IntType)
{
    if ((I2Cx->CR2 & u32IntType) != 0UL) {
        return SET;
    } else {
        return RESET;
    }
}

/**
 * @brief I2C current marching address
 * @param [in] I2Cx                 Pointer to I2C instance register base.
 *                                  This parameter can be a value of the following:
 *         @arg CM_I2C or CM_I2Cx:  I2C instance register base.
 * @retval uint32_t                 The Address
 */
static uint32_t I2C_GetAddr(CM_I2C_TypeDef *I2Cx)
{
    return (READ_REG32(I2Cx->SLVADDR) & I2C_SLVADDR_SLVADR);
}

/**
  * @brief  Master NACK interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void MasterNackHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_NACK);

    /* Check that SMBUS transfer finished */
    if (pHalSmbus->u16DataIndex == pHalSmbus->u16XferSize) {
        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
    } else {
        /* Data transfer not finish, receive NACK, error */
        pHalSmbus->u32ErrorCode |= HAL_SMBUS_ERR_ACKF;

        /* Enter stop stage */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_STOP, ENABLE);
        /* Generate stop condition */
        pHalSmbus->u32ComState = SMBUS_I2C_STAT_STOP;
        I2C_GenerateStop(pHalSmbus->I2Cx);

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);

        /* Call the Error callback to inform upper layer */
        HAL_SMBUS_ErrorCallback(pHalSmbus);
    }
}

static void MasterTansCpltCallback(stc_hal_smbus_handle_t *pHalSmbus)
{
    if (pHalSmbus->u32State == HAL_SMBUS_STAT_MASTER_BUSY_TX) {
        pHalSmbus->u32State = HAL_SMBUS_STAT_RDY;
        pHalSmbus->u32ComState = SMBUS_I2C_STAT_IDLE;
        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
        HAL_SMBUS_MasterTxCpltCallback(pHalSmbus);
    } else if (pHalSmbus->u32State == HAL_SMBUS_STAT_MASTER_BUSY_RX) {
        pHalSmbus->u32State = HAL_SMBUS_STAT_RDY;
        pHalSmbus->u32ComState = SMBUS_I2C_STAT_IDLE;
        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
        HAL_SMBUS_MasterRxCpltCallback(pHalSmbus);
    }
}

/**
  * @brief  Master transfer or receive finish process
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void MasterDataFinish(stc_hal_smbus_handle_t *pHalSmbus)
{
    if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_STOP)) {
        /* Enter stop stage */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_STOP, ENABLE);
        /* Generate stop condition */
        pHalSmbus->u32ComState = SMBUS_I2C_STAT_STOP;
        I2C_GenerateStop(pHalSmbus->I2Cx);
    } else {
        MasterTansCpltCallback(pHalSmbus);
    }
}

/**
  * @brief  Master receive the last data
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void MasterRxLastData(stc_hal_smbus_handle_t *pHalSmbus)
{
    uint8_t u8Data;

    if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)) {
        if (pHalSmbus->stcInit.enPacketErrorCheck == ENABLE) {
            /* Read PEC */
            pHalSmbus->u8Crc8Rev = I2C_ReadData(pHalSmbus->I2Cx);
            if (pHalSmbus->u8Crc8Rev != pHalSmbus->u8Crc8) {
                pHalSmbus->u32ErrorCode |= HAL_SMBUS_ERR_PECERR;
                HAL_Printf("HAL_SMBUS_ErrorCallback\r\n");
                HAL_SMBUS_ErrorCallback(pHalSmbus);
            }
        }
    } else {
        /* Read last normal data */
        u8Data = I2C_ReadData(pHalSmbus->I2Cx);
        *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex) = u8Data;
        pHalSmbus->u16DataIndex++;

    }
}

/**
  * @brief  Master stop interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void MasterStopHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    I2C_SmbusNBConfig(pHalSmbus->I2Cx, I2C_SMBUS_NBYTE | I2C_SMBUS_AUTONAK, DISABLE);
    /* Disable Stop flag interrupt */
    I2C_IntCmd(pHalSmbus->I2Cx,
               I2C_INT_TX_EMPTY | I2C_INT_RX_FULL | I2C_INT_TX_CPLT | I2C_INT_STOP | I2C_INT_NACK | I2C_INT_START,
               DISABLE);

    I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_STOP);

    if (pHalSmbus->u32ErrorCode == HAL_SMBUS_ERR_ACKF) {
        pHalSmbus->u32State = HAL_SMBUS_STAT_RDY;
        pHalSmbus->u32ComState = SMBUS_I2C_STAT_IDLE;

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
    } else {
        MasterTansCpltCallback(pHalSmbus);
    }

    pHalSmbus->u32TimeoutCnt = 0U;
}

/**
  * @brief  Master start interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void MasterStartHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    uint8_t u8AddrTemp = 0U;

    I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_START | I2C_FLAG_CLR_NACK);
    I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_STOP | I2C_INT_NACK, ENABLE);
    /* Enable TEI interrupt which indicate address transfer end */
    I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TX_CPLT, ENABLE);

    if (pHalSmbus->u32State == HAL_SMBUS_STAT_MASTER_BUSY_TX) {
        /* Enter data tx stage */
        pHalSmbus->u32ComState = SMBUS_I2C_STAT_ADDR_TX;
        u8AddrTemp = ((uint8_t)(pHalSmbus->u16DevAddr << 1U) | I2C_DIR_TX);
    } else if (pHalSmbus->u32State == HAL_SMBUS_STAT_MASTER_BUSY_RX) {
        /* Enter data rx stage */
        pHalSmbus->u32ComState = SMBUS_I2C_STAT_ADDR_RX;
        u8AddrTemp = ((uint8_t)(pHalSmbus->u16DevAddr << 1U) | I2C_DIR_RX);
    }
    pHalSmbus->u16DataIndex = 0U;

    I2C_WriteData(pHalSmbus->I2Cx, u8AddrTemp);

}

/**
  * @brief  Master receive full interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void MasterReceiveDataHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    if (pHalSmbus->u16DataIndex == (pHalSmbus->u16XferSize - 1U)) {
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_RX_FULL, DISABLE);
        /* Read last data */
        MasterRxLastData(pHalSmbus);

        MasterDataFinish(pHalSmbus);
    } else {
        *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex) = I2C_ReadData(pHalSmbus->I2Cx);
        pHalSmbus->u16DataIndex++;
        pHalSmbus->u8Crc8 = I2C_GetPECValue(pHalSmbus->I2Cx);
    }
}

/**
  * @brief  Master transfer empty interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void MasterTxEmptyHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    if (pHalSmbus->u16DataIndex < (pHalSmbus->u16XferSize - 1U)) {
        I2C_WriteData(pHalSmbus->I2Cx, *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex));
        pHalSmbus->u16DataIndex++;
    } else if (pHalSmbus->u16DataIndex == (pHalSmbus->u16XferSize - 1U)) {
        /* Last data */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TX_EMPTY, DISABLE);
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TX_CPLT, ENABLE);

        if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)) {
            if (pHalSmbus->stcInit.enPacketErrorCheck == ENABLE) {
                /* Wait TX CPLT */
                while (I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_TX_CPLT) == RESET) {
                    /* Wait */
                }
                pHalSmbus->u8Crc8 = I2C_GetPECValue(pHalSmbus->I2Cx);
                /* Send PEC */
                I2C_WriteData(pHalSmbus->I2Cx, pHalSmbus->u8Crc8);
                pHalSmbus->u16DataIndex++;
            }
        } else {
            I2C_WriteData(pHalSmbus->I2Cx, *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex));
            pHalSmbus->u16DataIndex++;
        }
    } else {
        /* do not have data trasfer */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TX_EMPTY, DISABLE);
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TX_CPLT, ENABLE);
    }
}

/**
  * @brief  Master transfer complete interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void MasterTxCompleteHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TX_CPLT, DISABLE);

    if ((SMBUS_I2C_STAT_ADDR_TX == pHalSmbus->u32ComState) && (pHalSmbus->u16DataIndex == 0U)) {
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_TX_CPLT);
        /* Address send finished, Data TX */
        if (pHalSmbus->u16XferSize == 0UL) {
            /* Have no data transfer, enter stop stage */
            MasterDataFinish(pHalSmbus);
        } else {
            /* If Address send receive ACK */
            if (RESET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_ACKR)) {
                pHalSmbus->u32ComState = SMBUS_I2C_STAT_TX;
                /* Config tx buffer empty interrupt function */
                I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TX_EMPTY, ENABLE);
                /* Transfer first data here */
                I2C_WriteData(pHalSmbus->I2Cx, *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex));
                pHalSmbus->u16DataIndex++;
            }
        }
    } else if ((SMBUS_I2C_STAT_ADDR_RX == pHalSmbus->u32ComState) && (pHalSmbus->u16DataIndex == 0U)) {
        /* Address send finished, Data RX */
        if (pHalSmbus->u16XferSize == 0UL) {
            /* Have no data receive, enter stop stage */
            MasterDataFinish(pHalSmbus);
        } else {
            /* If Address send receive ACK */
            if (RESET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_ACKR)) {
                I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_RX_FULL, ENABLE);
                pHalSmbus->u32ComState = SMBUS_I2C_STAT_RX;
                I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_TX_CPLT);
            }
        }
    } else {
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_TX_CPLT);
        /* Data Tx finish */
        MasterDataFinish(pHalSmbus);
    }
}

/**
  * @brief  Interrupt Sub-Routine which handle the Interrupt Flags Master Mode.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
static int32_t SMBUSMasterISR(stc_hal_smbus_handle_t *pHalSmbus)
{
    uint32_t u32Status;
    /* Process Locked */
    __HAL_LOCK(pHalSmbus);

    u32Status = pHalSmbus->I2Cx->SR;

    if ((0UL != (u32Status & I2C_FLAG_NACKF)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_NACK))) {
        /* Master NACK interrupt */
        HAL_Printf("MasterNackHandler\r\n");
        MasterNackHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_STOP)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_STOP))) {
        /* Master STOP interrupt */
        HAL_Printf("MasterStopHandler\r\n");
        MasterStopHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_START)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_START))) {
        /* Master START interrupt */
        HAL_Printf("MasterStartHandler\r\n");
        MasterStartHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_RX_FULL)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_RX_FULL))) {
        /* Master RXI interrupt */
        HAL_Printf("MasterReceiveDataHandler\r\n");
        MasterReceiveDataHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_TX_EMPTY)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_TX_EMPTY))) {
        /* Master TXI interrupt */
        HAL_Printf("MasterTxEmptyHandler\r\n");
        MasterTxEmptyHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_TX_CPLT)) && (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_TX_CPLT))) {
        /* Master TCI interrupt */
        HAL_Printf("MasterTxCompleteHandler\r\n");
        MasterTxCompleteHandler(pHalSmbus);
    }

    /* Process Unlocked */
    __HAL_UNLOCK(pHalSmbus);
    return HAL_OK;
}

/**
  * @brief  Slave NACK interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void SlaveNackHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_NACK);

    /* Check that SMBUS transfer finished */
    if (pHalSmbus->u16DataIndex == pHalSmbus->u16XferSize) {
        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
    } else {
        /* So reset Slave Busy state */
        pHalSmbus->u32State &= ~((uint32_t)HAL_SMBUS_STAT_SLAVE_BUSY_TX);
        pHalSmbus->u32State &= ~((uint32_t)HAL_SMBUS_STAT_SLAVE_BUSY_RX);

        /* Disable RX/TX Interrupts, keep only ADDR Interrupt */
        I2C_IntCmd(pHalSmbus->I2Cx, SMBUS_INT_SLAVE_TX | SMBUS_INT_SLAVE_RX, DISABLE);

        /* Set ErrorCode corresponding to a Non-Acknowledge */
        pHalSmbus->u32ErrorCode |= HAL_SMBUS_ERR_ACKF;

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);

        /* Call the Error callback to inform upper layer */
        HAL_SMBUS_ErrorCallback(pHalSmbus);
    }
}

/**
  * @brief  Slave address matched interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void SlaveAddrHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    uint8_t u8XferDir;
    uint32_t AddrTmp = I2C_GetAddr(pHalSmbus->I2Cx);

    if (SET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_TRA)) {
        u8XferDir = HAL_SMBUS_SLAVE_DIR_TX;
    } else {
        u8XferDir = HAL_SMBUS_SLAVE_DIR_RX;
    }

    I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0, DISABLE);

    /* Process Unlocked */
    __HAL_UNLOCK(pHalSmbus);

    HAL_SMBUS_AddrCallback(pHalSmbus, u8XferDir, (uint16_t)AddrTmp);

    pHalSmbus->u32TimeoutCnt = 0U;
}

/**
  * @brief  Slave transfer complete interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void SlaveTxCompleteHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_TX_CPLT);

    if (pHalSmbus->u16DataIndex == pHalSmbus->u16XferSize) {
        /* Remove HAL_SMBUS_STAT_SLAVE_BUSY_TX, keep only HAL_SMBUS_STAT_LISTEN */
        I2C_IntCmd(pHalSmbus->I2Cx, I2C_INT_TX_CPLT, DISABLE);
        pHalSmbus->u32State &= ~((uint32_t)HAL_SMBUS_STAT_SLAVE_BUSY_TX);

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
        /* Call the Tx complete callback to inform upper layer of the end of transmit process */
        HAL_SMBUS_SlaveTxCpltCallback(pHalSmbus);
    } else {
        if (RESET == I2C_GetStatus(pHalSmbus->I2Cx, I2C_FLAG_ACKR)) {
            if ((pHalSmbus->u16DataIndex == (pHalSmbus->u16XferSize - 1U)) && (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)))  {
                if (pHalSmbus->stcInit.enPacketErrorCheck == ENABLE) {
                    pHalSmbus->u8Crc8 = I2C_GetPECValue(pHalSmbus->I2Cx);
                    /* last data is PEC */
                    I2C_WriteData(pHalSmbus->I2Cx, pHalSmbus->u8Crc8);
                    pHalSmbus->u16DataIndex++;
                }
            } else {
                I2C_WriteData(pHalSmbus->I2Cx, *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex));
                pHalSmbus->u16DataIndex++;
            }
        }
    }
}

/**
  * @brief  Slave receive full interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void SlaveReceiveDataHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    uint8_t u8Data;

    if (pHalSmbus->u16DataIndex <= (pHalSmbus->u16XferSize - 1U)) {
        /* Read one data */
        u8Data = I2C_ReadData(pHalSmbus->I2Cx);

        if (pHalSmbus->u16DataIndex == (pHalSmbus->u16XferSize - 1U)) {
            /* Last data */
            if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)) {
                if (pHalSmbus->stcInit.enPacketErrorCheck == ENABLE) {
                    pHalSmbus->u8Crc8Rev = u8Data;
                    /* The last data is PEC */
                    if (u8Data != pHalSmbus->u8Crc8) {
                        HAL_Printf("slave pec error\r\n");
                        pHalSmbus->u32ErrorCode = HAL_SMBUS_ERR_PECERR;
                    }
                }
            } else {
                *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex) = u8Data;
                pHalSmbus->u8Crc8 = I2C_GetPECValue(pHalSmbus->I2Cx);
            }
            pHalSmbus->u16DataIndex++;
            /* Last Byte is received, disable Interrupt */
            I2C_IntCmd(pHalSmbus->I2Cx, SMBUS_INT_SLAVE_RX, DISABLE);

            /* Remove HAL_SMBUS_STAT_SLAVE_BUSY_RX, keep only HAL_SMBUS_STAT_LISTEN */
            pHalSmbus->u32State &= ~((uint32_t)HAL_SMBUS_STAT_SLAVE_BUSY_RX);

            /* Process Unlocked */
            __HAL_UNLOCK(pHalSmbus);
            HAL_SMBUS_SlaveRxCpltCallback(pHalSmbus);
        } else {
            *(pHalSmbus->pBuf + pHalSmbus->u16DataIndex) = u8Data;
            pHalSmbus->u16DataIndex++;
            if (0UL != (pHalSmbus->u32XferOption & SMBUS_FRAME_PEC)) {
                pHalSmbus->u8Crc8 = I2C_GetPECValue(pHalSmbus->I2Cx);
            }
        }
    } else {
        /* Last Byte is received, disable Interrupt */
        I2C_IntCmd(pHalSmbus->I2Cx, SMBUS_INT_SLAVE_RX, DISABLE);

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
        HAL_SMBUS_SlaveRxCpltCallback(pHalSmbus);
    }
}

/**
  * @brief  Slave stop interrupt handler
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval None
  */
static void SlaveStopHandler(stc_hal_smbus_handle_t *pHalSmbus)
{
    if ((pHalSmbus->u32State & HAL_SMBUS_STAT_LISTEN) == HAL_SMBUS_STAT_LISTEN) {
        /* Disable RX and TX and ADDR vInterrupts */
        I2C_IntCmd(pHalSmbus->I2Cx, SMBUS_INT_SLAVE_TX | SMBUS_INT_SLAVE_RX | I2C_INT_MATCH_ADDR0 | I2C_INT_STOP | I2C_INT_NACK, DISABLE);
        I2C_AckConfig(pHalSmbus->I2Cx, I2C_ACK);

        /* Clear STOP and ADDR Flag */
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_STOP | I2C_FLAG_MATCH_ADDR0);

        pHalSmbus->u16DataIndex = 0U;
        pHalSmbus->u32State = HAL_SMBUS_STAT_RDY;
        pHalSmbus->u32ComState = SMBUS_I2C_STAT_IDLE;

        /* Process Unlocked */
        __HAL_UNLOCK(pHalSmbus);
        HAL_SMBUS_ListenCpltCallback(pHalSmbus);
    }
    pHalSmbus->u32TimeoutCnt = 0U;
}

/**
  * @brief  Interrupt Sub-Routine which handle the Interrupt Flags Slave Mode.
  * @param [in] pHalSmbus   Pointer to a stc_hal_smbus_handle_t structure that contains
  *                         the configuration information for the specified SMBUS.
  * @retval HAL status @ref HC32_Series_Hal_Status_Define
  */
static int32_t SMBUSSlaveISR(stc_hal_smbus_handle_t *pHalSmbus)
{
    uint32_t u32Status;
    /* Process Locked */
    __HAL_LOCK(pHalSmbus);

    u32Status = pHalSmbus->I2Cx->SR;
    if ((0UL != (u32Status & I2C_FLAG_TX_CPLT)) &&
        (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_TX_CPLT))) {
        HAL_Printf("SlaveTxCompleteHandler\r\n");
        /* Slave transfer complete interrupt */
        SlaveTxCompleteHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_MATCH_ADDR0)) &&
               (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_MATCH_ADDR0))) {
        HAL_Printf("SlaveAddrHandler\r\n");
        /* Slave address matched interrupt */
        SlaveAddrHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_GENERAL_CALL)) &&
               (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_GENERAL_CALL))) {
        HAL_Printf("SlaveAddrHandler_GENERAL_CALL\r\n");
        /* Slave address matched interrupt */
        SlaveAddrHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_SMBUS_DEFAULT_MATCH)) &&
               (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_SMBUS_DEFAULT_MATCH))) {
        HAL_Printf("SlaveAddrHandler_SMBUS_DEFAUL\r\n");
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_SMBUS_DEFAULT_MATCH);
        /* Slave address matched interrupt */
        SlaveAddrHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_SMBUS_HOST_MATCH)) &&
               (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_SMBUS_HOST_MATCH))) {
        HAL_Printf("SlaveAddrHandler_SMBUS_HOST\r\n");
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_SMBUS_HOST_MATCH);
        /* Slave address matched interrupt */
        SlaveAddrHandler(pHalSmbus);
    } else if ((0UL != (u32Status & I2C_FLAG_SMBUS_ALERT_MATCH)) &&
               (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_SMBUS_ALERT_MATCH))) {
        HAL_Printf("SlaveAddrHandler_SMBUS_ALERT\r\n");
        I2C_ClearStatus(pHalSmbus->I2Cx, I2C_FLAG_CLR_SMBUS_ALERT_MATCH);
        /* Slave address matched interrupt */
        SlaveAddrHandler(pHalSmbus);
    }

    u32Status = pHalSmbus->I2Cx->SR;
    if ((0UL != (u32Status & I2C_FLAG_RX_FULL)) &&
        (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_RX_FULL))) {
        HAL_Printf("SlaveReceiveDataHandler\r\n");
        /* Slave receive full interrupt */
        SlaveReceiveDataHandler(pHalSmbus);
    }

    if ((0UL != (u32Status & I2C_FLAG_NACKF)) &&
        (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_NACK))) {
        /* Slave NACK interrupt */
        HAL_Printf("SlaveNackHandler\r\n");
        SlaveNackHandler(pHalSmbus);
    }

    if ((0UL != (u32Status & I2C_FLAG_STOP)) &&
        (SET == I2C_IntStdGet(pHalSmbus->I2Cx, I2C_INT_STOP))) {
        /* Slave STOP interrupt */
        HAL_Printf("SlaveStopHandler\r\n");
        SlaveStopHandler(pHalSmbus);
    }

    /* Process Unlocked */
    __HAL_UNLOCK(pHalSmbus);
    return HAL_OK;
}

/**
 * @}
 */

#endif /* HAL_SMBUS_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
