/**
 *******************************************************************************
 * @file  hc32_hal_smbus.h
 * @brief This file contains the functions prototypes of the SMBus protocol
 *        driver library.
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
#ifndef __HC32_HAL_SMBUS_H__
#define __HC32_HAL_SMBUS_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"
#include "hc32_hal_def.h"
#include "hc32_config_stack.h"

#if defined (HC32F558)
#include "hc32f5xx.h"
#include "hc32f5xx_conf.h"
#else
#error "The currently selected chip does not support SMBus."
#endif


/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup HAL_SMBus
 * @{
 */

#if (HAL_SMBUS_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/**
 * @defgroup SMBUS_Global_Types SMBUS Global Types
 * @{
 */

/**
 * @brief SMBUS initialize structure
 */
typedef struct {
    uint32_t u32Baudrate;                   /*!< I2C baudrate, 100 000 ~ 1 000 000 */

    en_functional_state_t enAnalogFilter;   /*!< Specifies if Analog Filter is enable or not.
                                                 This parameter can be a value of @ref en_functional_state_t
                                                 enumeration. */

    uint32_t u32OwnAddr1;                   /*!< Specifies the first device own address.
                                                 This parameter can be a 7-bit or 10-bit address. */

    uint32_t u32AddrMode;                   /*!< Specifies if 7-bit or 10-bit addressing mode for master is selected.
                                                 This parameter can be a value of @ref I2C_Addr_Config */

    uint32_t u32DualAddrMode;               /*!< Specifies if dual addressing mode is selected.
                                                 This parameter can be a value of @ref HAL_SMBUS_Dual_Addr_Mode */

    uint32_t u32OwnAddr2;                   /*!< Specifies the second device own address if dual addressing mode is
                                                selected. This parameter can be a 7-bit address. */

    uint32_t u32OwnAddr2Masks;              /*!< Specifies the acknowledge mask address second device own address
                                                 if dual addressing mode is selected */

    en_functional_state_t enGeneralCall;    /*!< Specifies if general call mode is selected.
                                                 This parameter can be a value of @ref en_functional_state_t. */

    en_functional_state_t enPacketErrorCheck;   /*!< Specifies if Packet Error Check mode is selected.
                                            This parameter can be a value of @ref en_functional_state_t */

    uint32_t u32PeriphMode;                 /*!< Specifies which mode of Peripheral is selected.
                                                 This parameter can be a value I2C_SMBUS_MATCH_HOST or I2C_SMBUS_MATCH_DEFAULT */

    uint8_t u32SMBusTimeout;                /*!< Specifies the SMBus timeout value, it means u32SMBusTimeout * 0.1mS. */
} stc_hal_smbus_init_t;

/**
 * @brief SMBUS HAL level handler structure
 */
typedef struct {
    CM_I2C_TypeDef      *I2Cx;           /*!< SMBUS registers base address */
    stc_hal_smbus_init_t stcInit;        /*!< SMBUS communication parameters */
    uint8_t             *pBuf;           /*!< Pointer to SMBUS transfer buffer */
    uint16_t            u16XferSize;     /*!< SMBUS total transfer size */
    uint16_t            u16DevAddr;      /*!< SMBUS protocol device address */
    uint32_t            u32XferOption;   /*!< SMBUS transfer options @ref HAL_SMBUS_Xfer_Option */
    __IO uint32_t       u32State;        /*!< SMBUS communication state @ref HAL_SMBUS_State */
    __IO uint16_t       u16DataIndex;    /*!< SMBUS transfer counter temp */
    __IO uint32_t       u32ComState;     /*!< I2C communication state @ref SMBUS_Com_State_define */
    __IO uint8_t        u8Lock;          /*!< SMBUS locking object @ref HC32_Series_Hal_Lock_Status_Define */
    __IO uint32_t       u32ErrorCode;    /*!< SMBUS Error code */
    uint8_t             u8Crc8;          /*!< SMBUS PEC data temp */
    uint8_t             u8Crc8Rev;       /*!< SMBUS PEC data received */
    uint32_t            u32TimeoutCnt;   /*!< SMBUS Timeout counter temp */
} stc_hal_smbus_handle_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

#ifdef SMBUS_ARP
/* Devices that support ARP support PEC function by default */
#define SMBUS_PEC
#endif

/*  SMBus frame stucture define  */
/* | Stage 1          | Stage 2      | Stage 3             | Stage 4     | */
/* | Start or Restart | ADDR + TX/RX | DATA or NULL or PEC | Stop        | */

/* Stage 1 */
#define SMBUS_FRAME_START           (0x01UL)
#define SMBUS_FRAME_RESTART         (0x00UL)
/* Stage 3 */
#define SMBUS_FRAME_PEC             (1UL << 26U) /* This Frame need calculate the PEC and tx or rx the PEC */
/* Stage 4 */
#define SMBUS_FRAME_STOP            (0x08UL)

/**
 * @defgroup HAL_SMBUS_Xfer_Option SMBus Transfer Frame Structure definition
 * @{
 */
#define HAL_SMBUS_FIRST_LAST_FRAME  (SMBUS_FRAME_START | SMBUS_FRAME_STOP)  /* S + Addr + R/W + Data + P */
#define HAL_SMBUS_FIRST_NEXT_FRAME  (SMBUS_FRAME_START)                     /* S + Addr + R/W + Data */
#define HAL_SMBUS_NEXT_LAST_FRAME   (SMBUS_FRAME_STOP)                      /* Sr + Addr + R/W + Data + P */
#define HAL_SMBUS_NEXT_FRAME        (0x00UL)                                /* Sr + Addr + R/W + Data */
/**
  * @}
  */

/**
 * @defgroup HAL_SMBUS_State HAL SMBus State definition
  * @{
  */
#define HAL_SMBUS_STAT_RST              (0x00000000UL)          /*!< SMBUS not yet initialized or disabled */
#define HAL_SMBUS_STAT_RDY              (0x00000001UL)          /*!< SMBUS initialized and ready for use */
#define HAL_SMBUS_STAT_BUSY             (0x00000002UL)          /*!< SMBUS internal process is ongoing */
#define HAL_SMBUS_STAT_MASTER_BUSY_TX   (0x00000012UL)          /*!< Master Data Transmission process is ongoing */
#define HAL_SMBUS_STAT_MASTER_BUSY_RX   (0x00000022UL)          /*!< Master Data Reception process is ongoing */
#define HAL_SMBUS_STAT_SLAVE_BUSY_TX    (0x00000032UL)          /*!< Slave Data Transmission process is ongoing */
#define HAL_SMBUS_STAT_SLAVE_BUSY_RX    (0x00000042UL)          /*!< Slave Data Reception process is ongoing */
#define HAL_SMBUS_STAT_TIMEOUT          (0x00000003UL)          /*!< Timeout state */
#define HAL_SMBUS_STAT_ERR              (0x00000004UL)          /*!< Reception process is ongoing */
#define HAL_SMBUS_STAT_LISTEN           (0x00000008UL)          /*!< Address Listen Mode is ongoing */
/**
  * @}
  */

/**
 * @defgroup HAL_SMBUS_Error_Code HAL SMBUS Error Code definition
  * @{
  */
#define HAL_SMBUS_ERR_NONE              (0x00000000UL)          /*!< No error */
#define HAL_SMBUS_ERR_ARLO              (0x00000002UL)          /*!< ARLO error */
#define HAL_SMBUS_ERR_ACKF              (0x00000004UL)          /*!< ACKF error */
#define HAL_SMBUS_ERR_TIMEOUT           (0x00000010UL)          /*!< Timeout error */
#define HAL_SMBUS_ERR_ALERT             (0x00000040UL)          /*!< Alert error */
#define HAL_SMBUS_ERR_PECERR            (0x00000080UL)          /*!< PEC error */
/**
  * @}
  */

/**
 * @defgroup HAL_SMBUS_Dual_Addr_Mode HAL SMBus Dual Addressing Mode
  * @{
  */
#define HAL_SMBUS_DUAL_ADDR_DISABLE     (0UL)
#define HAL_SMBUS_DUAL_ADDR_ENABLE      (1UL)
/**
  * @}
  */

/**
 * @defgroup HAL_SMBUS_Slave_Dir HAL SMBus Slave Transfer Direction
  * @{
  */
#define HAL_SMBUS_SLAVE_DIR_TX          (1U)
#define HAL_SMBUS_SLAVE_DIR_RX          (0U)
/**
  * @}
  */

/* Define for printf */
#if (HAL_PRINT_ENABLE == DDL_ON)
#include <stdio.h>
#define HAL_Printf                      (void)printf
#else
#define HAL_Printf(...)
#endif

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
int32_t HAL_SMBUS_Init(stc_hal_smbus_handle_t *pHalSmbus);
int32_t HAL_SMBUS_DeInit(stc_hal_smbus_handle_t *pHalSmbus);

int32_t HAL_SMBUS_MasterTransmitIT(stc_hal_smbus_handle_t *pHalSmbus, uint16_t u16DevAddr, uint8_t *pData, uint16_t u16Size, uint32_t u32XferOption);
int32_t HAL_SMBUS_MasterReceiveIT(stc_hal_smbus_handle_t *pHalSmbus, uint16_t u16DevAddr, uint8_t *pData, uint16_t u16Size, uint32_t u32XferOption);
int32_t HAL_SMBUS_SlaveTransmitIT(stc_hal_smbus_handle_t *pHalSmbus, uint8_t *pData, uint16_t u16Size, uint32_t u32XferOption);
int32_t HAL_SMBUS_SlaveReceiveIT(stc_hal_smbus_handle_t *pHalSmbus, uint8_t *pData, uint16_t u16Size, uint32_t u32XferOption);

int32_t HAL_SMBUS_EnableAlertIT(stc_hal_smbus_handle_t *pHalSmbus);
int32_t HAL_SMBUS_DisableAlertIT(stc_hal_smbus_handle_t *pHalSmbus);
int32_t HAL_SMBUS_EnableListenIT(stc_hal_smbus_handle_t *pHalSmbus);
int32_t HAL_SMBUS_DisableListenIT(stc_hal_smbus_handle_t *pHalSmbus);

void I2C_EE_IrqHandler(stc_hal_smbus_handle_t *pHalSmbus);
void I2C_TxComplete_IrqHandler(stc_hal_smbus_handle_t *pHalSmbus);
void I2C_TxEmpty_IrqHandler(stc_hal_smbus_handle_t *pHalSmbus);
void I2C_RxFull_IrqHandler(stc_hal_smbus_handle_t *pHalSmbus);

/* weak functions */
void HAL_SMBUS_MasterTxCpltCallback(stc_hal_smbus_handle_t *pHalSmbus);
void HAL_SMBUS_MasterRxCpltCallback(stc_hal_smbus_handle_t *pHalSmbus);
void HAL_SMBUS_SlaveTxCpltCallback(stc_hal_smbus_handle_t *pHalSmbus);
void HAL_SMBUS_SlaveRxCpltCallback(stc_hal_smbus_handle_t *pHalSmbus);
void HAL_SMBUS_AddrCallback(stc_hal_smbus_handle_t *pHalSmbus, uint8_t u8XferDir, uint16_t u16AddrMatchCode);
void HAL_SMBUS_ListenCpltCallback(stc_hal_smbus_handle_t *pHalSmbus);
void HAL_SMBUS_ErrorCallback(stc_hal_smbus_handle_t *pHalSmbus);

void HAL_SMBUS_MspInit(stc_hal_smbus_handle_t *pHalSmbus);
void HAL_SMBUS_MspDeInit(stc_hal_smbus_handle_t *pHalSmbus);

#endif /* HAL_SMBUS_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_HAL_SMBUS_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
