/**
 *******************************************************************************
 * @file  hc32_ll_sram.h
 * @brief This file contains all the functions prototypes of the SRAM driver
 *        library.
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
#ifndef __HC32_LL_SRAM_H__
#define __HC32_LL_SRAM_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"

#include "hc32f5xx.h"
#include "hc32f5xx_conf.h"
/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup LL_SRAM
 * @{
 */

#if (LL_SRAM_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup SRAM_Global_Macros SRAM Global Macros
 * @{
 */

/**
 * @defgroup SRAM_Sel SRAM Selection
 * @{
 */
#define SRAM_SRAMH              (1UL << 2U)                 /*!< SRAMH: 0x1FFF8000~0x1FFFFFFF, 32KB */
#define SRAM_SRAM1              (1UL << 0U)                 /*!< SRAM1: 0x20000000~0x20017FFF, 96KB */
#define SRAM_SRAM_ALL           (SRAM_SRAMH | SRAM_SRAM1)
/**
 * @}
 */

/**
 * @defgroup SRAM_ECC_SRAM ECC SRAM Definition
 * @{
 */
#define SRAM_ECC_CACHE_RAM      (1UL << 0U)
#define SRAM_ECC_SRAM1          (1UL << 1U)
#define SRAM_ECC_SRAMH          (1UL << 3U)
#define SRAM_ECC_39EIBIT_ALL    (SRAM_ECC_SRAM1 | SRAM_ECC_SRAMH)
#define SRAM_ECC_137EIBIT_ALL   (SRAM_ECC_CACHE_RAM)
#define SRAM_ECC_SRAM_ALL       (SRAM_ECC_39EIBIT_ALL | SRAM_ECC_137EIBIT_ALL)
/**
 * @}
 */

/**
 * @defgroup SRAM_Access_Wait_Cycle SRAM Access Wait Cycle
 * @{
 */
#define SRAM_WAIT_CYCLE0        (0U)                        /*!< Wait 0 CPU cycle. */
#define SRAM_WAIT_CYCLE1        (1U)                        /*!< Wait 1 CPU cycle. */
/**
 * @}
 */

/**
 * @defgroup SRAM_Exception_Type SRAM exception type
 * @note Even-parity check error, ECC check error.
 * @{
 */
#define SRAM_EXP_TYPE_NMI               (0UL)
#define SRAM_EXP_TYPE_RST               (1UL)
/**
 * @}
 */

/**
 * @defgroup SRAM_Check_SRAM SRAM check sram
 * @{
 */
#define SRAM_CHECK_SRAM1                (SRAMC_CKCR_SRAM1_ECCOAD)
#define SRAM_CHECK_SRAMH                (SRAMC_CKCR_SRAMH_ECCOAD)
#define SRAM_CHECK_CACHE_RAM            (SRAMC_CKCR_CACHE_ECCOAD)
#define SRAM_CHECK_SRAM_ALL             (SRAM_CHECK_SRAM1 | SRAM_CHECK_SRAMH | SRAM_CHECK_CACHE_RAM)
/**
 * @}
 */

/**
 * @defgroup SRAM_ECC_Mode SRAM ECC Mode
 * @{
 */
#define SRAM_ECC_MD_INVD        (0UL)                       /*!< The ECC mode is invalid. */
#define SRAM_ECC_MD1            (1UL)                       /*!< When 1-bit error occurres:
                                                                 ECC error corrects.
                                                                 No 1-bit-error status flag setting, no interrupt or reset.
                                                                 When 2-bit error occurres:
                                                                 ECC error detects.
                                                                 2-bit-error status flag sets and interrupt or reset occurres. */
#define SRAM_ECC_MD2            (2UL)                       /*!< When 1-bit error occurres:
                                                                 ECC error corrects.
                                                                 1-bit-error status flag sets, no interrupt or reset.
                                                                 When 2-bit error occurres:
                                                                 ECC error detects.
                                                                 2-bit-error status flag sets and interrupt or reset occurres. */
#define SRAM_ECC_MD3            (3UL)                       /*!< When 1-bit error occurres:
                                                                 ECC error corrects.
                                                                 1-bit-error status flag sets and interrupt or reset occurres.
                                                                 When 2-bit error occurres:
                                                                 ECC error detects.
                                                                 2-bit-error status flag sets and interrupt or reset occurres. */
/**
 * @}
 */

/**
 * @defgroup SRAM_Err_Status_Flag SRAM Error Status Flag
 * @{
 */
#define SRAM_FLAG_SRAM1_1ERR            (SRAMC_CKSR_SRAM1_1ERR)     /*!< SRAM1 ECC 1-bit error. */
#define SRAM_FLAG_SRAM1_2ERR            (SRAMC_CKSR_SRAM1_2ERR)     /*!< SRAM1 ECC 2-bit error. */
#define SRAM_FLAG_SRAMH_1ERR            (SRAMC_CKSR_SRAMH_1ERR)     /*!< SRAMH ECC 1-bit error. */
#define SRAM_FLAG_SRAMH_2ERR            (SRAMC_CKSR_SRAMH_2ERR)     /*!< SRAMH ECC 2-bit error. */
#define SRAM_FLAG_CACHE_1ERR            (SRAMC_CKSR_CACHE_1ERR)     /*!< Cache ECC 1-bit error. */
#define SRAM_FLAG_CACHE_2ERR            (SRAMC_CKSR_CACHE_2ERR)     /*!< Cache ECC 2-bit error. */
#define SRAM_FLAG_ALL                   (0x003FFFCFUL)

/**
 * @}
 */

/**
 * @defgroup SRAM_Reg_Protect_Key SRAM Register Protect Key
 * @{
 */
#define SRAM_REG_LOCK_KEY               (0x76U)
#define SRAM_REG_UNLOCK_KEY             (0x77U)
/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup SRAM_Global_Functions
 * @{
 */

/**
 * @brief  Lock SRAM registers, write protect.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void SRAM_REG_Lock(void)
{
    WRITE_REG32(CM_SRAMC->WTPR, SRAM_REG_LOCK_KEY);
    WRITE_REG32(CM_SRAMC->CKPR, SRAM_REG_LOCK_KEY);
}

/**
 * @brief  Unlock SRAM registers, write enable.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void SRAM_REG_Unlock(void)
{
    WRITE_REG32(CM_SRAMC->WTPR, SRAM_REG_UNLOCK_KEY);
    WRITE_REG32(CM_SRAMC->CKPR, SRAM_REG_UNLOCK_KEY);
}

void SRAM_Init(void);
void SRAM_DeInit(void);

void SRAM_SetWaitCycle(uint32_t u32SramSel, uint32_t u32WriteCycle, uint32_t u32ReadCycle);
void SRAM_SetEccMode(uint32_t u32EccSram, uint32_t u32EccMode);
void SRAM_SetExceptionType(uint32_t u32CheckSram, uint32_t u32ExceptionType);

en_flag_status_t SRAM_GetStatus(uint32_t u32Flag);
void SRAM_ClearStatus(uint32_t u32Flag);

void SRAM_ErrorInjectCmd(uint32_t u32EccSram, en_functional_state_t enNewState);
void SRAM_ErrorInjectBitCmd(uint32_t u32EccSram, uint16_t u16BitPos, en_functional_state_t enNewState);
uint32_t SRAM_GetEccErrorAddr(uint32_t u32EccSram);

void SRAM_Cache_ErrorInjectBitCmd(uint16_t u16BitPos, en_functional_state_t enNewState);

/**
 * @}
 */

#endif /* LL_SRAM_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_SRAM_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
