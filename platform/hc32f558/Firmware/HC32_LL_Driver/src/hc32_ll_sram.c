/**
 *******************************************************************************
 * @file  hc32_ll_sram.c
 * @brief This file provides firmware functions to manage the SRAM.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2026-04-16       CDT             First version
   2026-02-02       CDT             API reimplemented: SRAM_SetEccMode(), for easier to maintain
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
#include "hc32_ll_sram.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_SRAM SRAM
 * @brief SRAM Driver Library
 * @{
 */

#if (LL_SRAM_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup SRAM_Local_Macros SRAM Local Macros
 * @{
 */

/**
 * @defgroup SRAM_Configuration_Bits_Mask SRAM Configuration Bits Mask
 * @{
 */
#define SRAM_CYCLE_MASK                 (0x00000007UL)
#define SRAM_EI_BIT_POS_MAX             (38U)

#define SRAM_ECC_MD_MASK                (0x3UL)
/**
 * @}
 */

/**
 * @defgroup SRAM_Ecc_Mode_Pos_Start SRAM ecc mode position start
 * @{
 */
#define SRAMC_CKCR_ECCMOD_POS_START     SRAMC_CKCR_CACHE_ECCMOD_POS
/**
 * @}
 */

/**
 * @defgroup SRAM_Exception_Type_Mask SRAM exception type mask
 * @{
 */
#define SRAM_EXP_TYPE_MASK              (SRAMC_CKCR_CACHE_ECCOAD | SRAMC_CKCR_SRAM1_ECCOAD | SRAMC_CKCR_SRAMH_ECCOAD)
/**
 * @}
 */

/**
 * @defgroup SRAM_Check_Parameters_Validity SRAM check parameters validity
 * @{
 */
#define IS_SRAM_BIT_MASK(x, mask)       (((x) != 0U) && (((x) | (mask)) == (mask)))
#define IS_SRAM_1BIT_MASK(x)            (((x) != 0U) && (((x) & ((x) - 1U)) == 0U))

/* Parameter valid check for SRAM wait cycle */
#define IS_SRAM_WAIT_CYCLE(x)           ((x) <= SRAM_WAIT_CYCLE1)

/* Parameter valid check for SRAM selection */
#define IS_SRAM_SEL(x)                  IS_SRAM_BIT_MASK(x, SRAM_SRAM_ALL)

/* Parameter valid check for SRAM ECC SRAM */
#define IS_SRAM_ECC_SRAM(x)             IS_SRAM_BIT_MASK(x, SRAM_ECC_SRAM_ALL)
#define IS_SRAM_ECC_39EIBIT_SRAM(x)     IS_SRAM_BIT_MASK(x, SRAM_ECC_39EIBIT_ALL)

/* Parameter valid check for SRAM ECC SRAM */
#define IS_SRAM_CHECK_SRAM(x)           IS_SRAM_BIT_MASK(x, SRAM_CHECK_SRAM_ALL)

/* Parameter valid check for SRAM flag */
#define IS_SRAM_FLAG(x)                 IS_SRAM_BIT_MASK(x, SRAM_FLAG_ALL)

/* Check SRAM  WTPR register lock status. */
#define IS_SRAM_WTPR_UNLOCK()           (CM_SRAMC->WTPR == SRAM_REG_UNLOCK_KEY)

/* Check SRAM CKPR register lock status. */
#define IS_SRAM_CKPR_UNLOCK()           (CM_SRAMC->CKPR == SRAM_REG_UNLOCK_KEY)

/* Parameter valid check for SRAM exception type mode */
#define IS_SRAM_EXP_TYPE(x)                                                    \
(   ((x) == SRAM_EXP_TYPE_NMI)                  ||                             \
    ((x) == SRAM_EXP_TYPE_RST))

/* Parameter valid check for SRAM ECC mode */
#define IS_SRAM_ECC_MD(x)               ((x) <= SRAM_ECC_MD3)

/* Parameter valid check for SRAM error inject bit */
#define IS_SRAM_EI_BIT_POS(x)           ((x) <= SRAM_EI_BIT_POS_MAX)
/* Parameter valid check for SRAM error inject bit */
#define IS_SRAM_CACHE_EI_BIT_POS(x)     ((x) <= 136U)
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

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @defgroup SRAM_Global_Functions SRAM Global Functions
 * @{
 */

/**
 * @brief  Initializes SRAM.
 * @param  None
 * @retval None
 */
void SRAM_Init(void)
{
    WRITE_REG32(CM_SRAMC->CKSR, SRAM_FLAG_ALL);
}

/**
 * @brief  De-initializes SRAM. RESET the registers of SRAM.
 * @param  None
 * @retval None
 * @note   Call SRAM_REG_Unlock to unlock registers WTCR and CKCR first.
 */
void SRAM_DeInit(void)
{
    /* Call SRAM_REG_Unlock to unlock register WTCR and CKCR. */
    DDL_ASSERT(IS_SRAM_CKPR_UNLOCK());
    DDL_ASSERT(IS_SRAM_WTPR_UNLOCK());

    WRITE_REG32(CM_SRAMC->WTCR, 0UL);
    WRITE_REG32(CM_SRAMC->CKCR, 0UL);
    SET_REG32_BIT(CM_SRAMC->CKSR, SRAM_FLAG_ALL);
    CLR_REG32(CM_SRAMC->SRAM1_EIEN);
    CLR_REG32(CM_SRAMC->SRAMH_EIEN);
    CLR_REG32(CM_SRAMC->CACHE_EIEN);

    CLR_REG32(CM_SRAMC->SRAM1_EIBIT0);
    CLR_REG32(CM_SRAMC->SRAM1_EIBIT1);
    CLR_REG32(CM_SRAMC->SRAMH_EIBIT0);
    CLR_REG32(CM_SRAMC->SRAMH_EIBIT1);

    CLR_REG32(CM_SRAMC->CACHE_EIBIT0);
    CLR_REG32(CM_SRAMC->CACHE_EIBIT1);
    CLR_REG32(CM_SRAMC->CACHE_EIBIT2);
    CLR_REG32(CM_SRAMC->CACHE_EIBIT3);
    CLR_REG32(CM_SRAMC->CACHE_EIBIT4);
}

/**
 * @brief  Specifies access wait cycle for SRAM.
 * @param  [in]  u32SramSel             The SRAM selection.
 *                                      This parameter can be values of @ref SRAM_Sel
 * @param  [in]  u32WriteCycle          The write access wait cycle for the specified SRAM
 *                                      This parameter can be a value of @ref SRAM_Access_Wait_Cycle
 * @param  [in]  u32ReadCycle           The read access wait cycle for the specified SRAM.
 *                                      This parameter can be a value of @ref SRAM_Access_Wait_Cycle
 * @retval None
 * @note   Call SRAM_REG_Unlock to unlock register WTCR first.
 */
void SRAM_SetWaitCycle(uint32_t u32SramSel, uint32_t u32WriteCycle, uint32_t u32ReadCycle)
{
    uint8_t i = 0U;
    uint8_t u8OfsWt;
    uint8_t u8OfsRd;

    DDL_ASSERT(IS_SRAM_SEL(u32SramSel));
    DDL_ASSERT(IS_SRAM_WAIT_CYCLE(u32WriteCycle));
    DDL_ASSERT(IS_SRAM_WAIT_CYCLE(u32ReadCycle));
    DDL_ASSERT(IS_SRAM_WTPR_UNLOCK());

    while (u32SramSel != 0UL) {
        if ((u32SramSel & 0x1UL) != 0UL) {
            u8OfsRd = i * 8U;
            u8OfsWt = u8OfsRd + 4U;
            MODIFY_REG32(CM_SRAMC->WTCR,
                         ((SRAM_CYCLE_MASK << u8OfsWt) | (SRAM_CYCLE_MASK << u8OfsRd)),
                         ((u32WriteCycle << u8OfsWt) | (u32ReadCycle << u8OfsRd)));
        }
        u32SramSel >>= 1U;
        i++;
    }
}

/**
 * @brief  Specifies ECC mode.
 * @param  [in]  u32EccSram             The ECC SRAM.
 *                                      This parameter can be any combination of @ref SRAM_ECC_SRAM
 * @param  [in]  u32EccMode             The ECC mode.
 *                                      This parameter can be a value of @ref SRAM_ECC_Mode
 * @retval None
 * @note   Call SRAM_REG_Unlock to unlock register CKCR first.
 *         The sram of u32EccMode should be the same with the sram of u32EccSram.
 */
void SRAM_SetEccMode(uint32_t u32EccSram, uint32_t u32EccMode)
{
    uint8_t u8Offset = 0U;
    uint32_t u32EccCfgVal  = 0UL;
    uint32_t u32EccCfgMask = 0UL;
    DDL_ASSERT(IS_SRAM_ECC_SRAM(u32EccSram));
    DDL_ASSERT(IS_SRAM_ECC_MD(u32EccMode));
    DDL_ASSERT(IS_SRAM_CKPR_UNLOCK());

    while (u32EccSram != 0UL) {
        if (1UL == (u32EccSram & 0x01UL)) {
            u32EccCfgVal  |= u32EccMode << (SRAMC_CKCR_ECCMOD_POS_START + u8Offset * 2U);
            u32EccCfgMask |= SRAM_ECC_MD_MASK << (SRAMC_CKCR_ECCMOD_POS_START + u8Offset * 2U);
        }
        u8Offset++;
        u32EccSram >>= 1UL;
    }
    MODIFY_REG32(CM_SRAMC->CKCR, u32EccCfgMask, u32EccCfgVal);
}

/**
 * @brief  Specifies the exception type while the chosen sram check error occurred.
 * @param  [in] u32CheckSram            The check SRAM.
 *                                      This parameter can be any combination of @ref SRAM_Check_SRAM
 * @param  [in] u32ExceptionType        The operation after check error occurred.
 *                                      This parameter can be a value of @ref SRAM_Exception_Type
 * @retval None
 * @note   Call SRAM_REG_Unlock to unlock register CKCR first.
 */
void SRAM_SetExceptionType(uint32_t u32CheckSram, uint32_t u32ExceptionType)
{
    DDL_ASSERT(IS_SRAM_CHECK_SRAM(u32CheckSram));
    DDL_ASSERT(IS_SRAM_EXP_TYPE(u32ExceptionType));
    DDL_ASSERT(IS_SRAM_CKPR_UNLOCK());

    if (SRAM_EXP_TYPE_RST == u32ExceptionType) {
        SET_REG32_BIT(CM_SRAMC->CKCR, u32CheckSram);
    } else {
        CLR_REG32_BIT(CM_SRAMC->CKCR, u32CheckSram);
    }
}

/**
 * @brief  Get the status of the specified flag of SRAM.
 * @param  [in]  u32Flag                The flag of SRAM.
 *                                      This parameter can be a value of @ref SRAM_Err_Status_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t SRAM_GetStatus(uint32_t u32Flag)
{
    en_flag_status_t enStatus = RESET;

    DDL_ASSERT(IS_SRAM_FLAG(u32Flag));
    if (READ_REG32_BIT(CM_SRAMC->CKSR, u32Flag) != 0U) {
        enStatus = SET;
    }

    return enStatus;
}

/**
 * @brief  Clear the status of the specified flag of SRAM.
 * @param  [in]  u32Flag                The flag of SRAM.
 *                                      This parameter can be values of @ref SRAM_Err_Status_Flag
 * @retval None
 */
void SRAM_ClearStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_SRAM_FLAG(u32Flag));
    WRITE_REG32(CM_SRAMC->CKSR, u32Flag);
}

/**
 * @brief  Enable or disable error injection.
 * @param  [in]  u32EccSram             The SRAM selection.
 *                                      This parameter can be any combination of @ref SRAM_ECC_SRAM
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SRAM_ErrorInjectCmd(uint32_t u32EccSram, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_SRAM_ECC_SRAM(u32EccSram));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if ((u32EccSram & SRAM_ECC_SRAMH) != 0U) {
        WRITE_REG32(CM_SRAMC->SRAMH_EIEN, enNewState);
    }
    if ((u32EccSram & SRAM_ECC_SRAM1) != 0U) {
        WRITE_REG32(CM_SRAMC->SRAM1_EIEN, enNewState);
    }
    if ((u32EccSram & SRAM_ECC_CACHE_RAM) != 0U) {
        WRITE_REG32(CM_SRAMC->CACHE_EIEN, enNewState);
    }
}

/**
 * @brief  Enable or disable error injection bit of SRAM_ECC_SRAM.
 * @param  [in]  u32EccSram             The SRAM selection.
 *                                      This parameter can be any combination of @ref SRAM_ECC_SRAM
 * @param  [in]  u16BitPos              The position of the bit to be enabled or disabled error injection.
 *                                      This parameter can be a number between 0 and 38.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SRAM_ErrorInjectBitCmd(uint32_t u32EccSram, uint16_t u16BitPos, en_functional_state_t enNewState)
{
    uint8_t u8RegIndex = (uint8_t)(u16BitPos / 32U);
    uint8_t u8BitPos   = (uint8_t)(u16BitPos % 32U);
    __IO uint32_t *SRAM1_EIBITx = &CM_SRAMC->SRAM1_EIBIT0;
    __IO uint32_t *SRAMH_EIBITx = &CM_SRAMC->SRAMH_EIBIT0;

    DDL_ASSERT(IS_SRAM_ECC_39EIBIT_SRAM(u32EccSram));
    DDL_ASSERT(IS_SRAM_EI_BIT_POS(u16BitPos));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        if ((u32EccSram & SRAM_ECC_SRAMH) != 0U) {
            SET_REG32_BIT(SRAMH_EIBITx[u8RegIndex], (1UL << u8BitPos));
        }
        if ((u32EccSram & SRAM_ECC_SRAM1) != 0U) {
            SET_REG32_BIT(SRAM1_EIBITx[u8RegIndex], (1UL << u8BitPos));
        }
    } else {
        if ((u32EccSram & SRAM_ECC_SRAMH) != 0U) {
            CLR_REG32_BIT(SRAMH_EIBITx[u8RegIndex], (1UL << u8BitPos));
        }
        if ((u32EccSram & SRAM_ECC_SRAM1) != 0U) {
            CLR_REG32_BIT(SRAM1_EIBITx[u8RegIndex], (1UL << u8BitPos));
        }
    }
}

/**
 * @brief  Enable or disable error injection bit of CACHE RAM.
 * @param  [in]  u16BitPos              The position of the bit to be enabled or disabled error injection.
 *                                      This parameter can be a number between 0 and 136.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SRAM_Cache_ErrorInjectBitCmd(uint16_t u16BitPos, en_functional_state_t enNewState)
{
    uint8_t u8RegIndex = (uint8_t)(u16BitPos / 32U);
    uint8_t u8BitPos   = (uint8_t)(u16BitPos % 32U);
    __IO uint32_t *CACHE_EIBITx = &CM_SRAMC->CACHE_EIBIT0;

    DDL_ASSERT(IS_SRAM_CACHE_EI_BIT_POS(u16BitPos));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(CACHE_EIBITx[u8RegIndex], (1UL << u8BitPos));
    } else {
        CLR_REG32_BIT(CACHE_EIBITx[u8RegIndex], (1UL << u8BitPos));
    }
}

/**
 * @brief  Get access address when 1bit or 2bit ECC error occurs in SRAM_ECC_SRAM.
 * @param  [in]  u32EccSram             The SRAM selection.
 *                                      This parameter can be a value of @ref SRAM_ECC_SRAM
 * @retval An uint32_t type value of access address. If 'u32EccSram' is not equal to the value upon,
 *         it will return 0xFFFFFFFFUL.
 */
uint32_t SRAM_GetEccErrorAddr(uint32_t u32EccSram)
{
    uint32_t u32RetAddr = 0xFFFFFFFFUL;

    DDL_ASSERT(IS_SRAM_ECC_SRAM(u32EccSram) && IS_SRAM_1BIT_MASK(u32EccSram));

    if (u32EccSram == SRAM_ECC_SRAM1) {
        u32RetAddr = READ_REG32(CM_SRAMC->SRAM1_ECCERRADDR);
    } else if (u32EccSram == SRAM_ECC_CACHE_RAM) {
        u32RetAddr = READ_REG32(CM_SRAMC->CACHE_ECCERRADDR);
    } else if (u32EccSram == SRAM_ECC_SRAMH) {
        u32RetAddr = READ_REG32(CM_SRAMC->SRAMH_ECCERRADDR);
    } else {
        /* rsvd */
    }

    return u32RetAddr;
}

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

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
