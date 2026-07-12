/**
 *******************************************************************************
 * @file  hc32_ll_ermu.c
 * @brief This file provides firmware functions to manage the Error Management
 *        Unit(ERMU).
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
#include "hc32_ll_ermu.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_ERMU ERMU
 * @brief Error Management Unit
 * @{
 */

#if (LL_ERMU_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup ERMU_Local_Macros ERMU Local Macros
 * @{
 */

#define ERMU_RMU_TIMEOUT                (100U)

#define ERMU_UNIT_REG(reg_base, unit)   (*(__IO uint32_t *)((uint32_t)(&(reg_base)) + ((unit) * 0x0080UL)))

#define ERMU_WTUNIT_REG(reg_base, unit) (*(__IO uint32_t *)((uint32_t)(&(reg_base)) + ((unit) * 0x0020UL)))

#define ERMU_GROUP_REG(reg_base, group) (*(__IO uint32_t *)((uint32_t)(&(reg_base)) + ((uint32_t)(group) * 0x04UL)))

/**
 * @defgroup ERMU_Check_Parameters_Validity ERMU Check Parameters Validity
 * @{
 */

#define IS_EOUT_UNIT(x)                                                         \
(   ((x) == ERMU_EOUT0)                 ||                                      \
    ((x) == ERMU_EOUT1)                 ||                                      \
    ((x) == ERMU_EOUT2)                 ||                                      \
    ((x) == ERMU_EOUT3))

#define IS_WTMR_UNIT(x)                                                         \
(   ((x) == ERMU_WTMR0)                 ||                                      \
    ((x) == ERMU_WTMR1)                 ||                                      \
    ((x) == ERMU_WTMR2)                 ||                                      \
    ((x) == ERMU_WTMR3))

#define IS_ERR_GROUP(x)                 (((x) == ERMU_ERR_GRP0) || ((x) == ERMU_ERR_GRP1))

#define IS_EOUT_MASK_GRP0(x)            (((x) | (ERMU_GRP0_ERR_ALL)) == (ERMU_GRP0_ERR_ALL))
#define IS_EOUT_MASK_GRP1(x)            (((x) | (ERMU_GRP1_ERR_ALL)) == (ERMU_GRP1_ERR_ALL))

#define IS_ERR_SRC_GRP0(x)              (((x) != 0UL) && (((x) | (ERMU_GRP0_ERR_ALL)) == (ERMU_GRP0_ERR_ALL)))
#define IS_ERR_SRC_GRP1(x)              (((x) != 0UL) && (((x) | (ERMU_GRP1_ERR_ALL)) == (ERMU_GRP1_ERR_ALL)))

#define IS_ERR_SRC_VALUE(group, src)    ((((group) == ERMU_ERR_GRP0) && IS_ERR_SRC_GRP0(src)) || \
                                        (((group) == ERMU_ERR_GRP1) && IS_ERR_SRC_GRP1(src)))

#define IS_CLK_DIV_VALUE(x)             (((x) > 0UL) && (x) <= 0x10000UL)

#define IS_TMR_CMP_VALUE(x)             ((x) <= 0xFFFFUL)

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
 * @defgroup ERMU_Global_Functions ERMU Global Functions
 * @{
 */

/**
 * @brief  Set the fields of structure stc_ermu_eout_t to default values
 * @param  [out] pstcEoutInit       Pointer to a @ref stc_ermu_eout_t structure
 * @retval int32_t:
 *           - LL_OK:               Initialize successfully
 *           - LL_ERR_INVD_PARAM:   The pointer pstcEoutInit value is NULL
 */
int32_t ERMU_EOUT_StructInit(stc_ermu_eout_t *pstcEoutInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcEoutInit) {
        pstcEoutInit->enClearTmrEn = DISABLE;
        pstcEoutInit->u32ClearTmrCmpValue = 0UL;
        pstcEoutInit->enToggleTmrEn = DISABLE;
        pstcEoutInit->u32ToggleTmrCmpValue = 0UL;
        pstcEoutInit->u32EoutMaskGroup0 = 0UL;
        pstcEoutInit->u32EoutMaskGroup1 = 0UL;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Initializes ERMU error output
 * @param  [in] pstcEoutInit    Pointer to a @ref stc_ermu_eout_t structure
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval int32_t:
 *           - LL_OK: Initializes success
 *           - LL_ERR_INVD_PARAM: pstcEoutInit == NULL
 */
int32_t ERMU_EOUT_Init(const stc_ermu_eout_t *pstcEoutInit, uint8_t u8Unit)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t *RegAddr;

    if (NULL == pstcEoutInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        /* Check parameters */
        DDL_ASSERT(IS_EOUT_UNIT(u8Unit));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcEoutInit->enClearTmrEn));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcEoutInit->enToggleTmrEn));
        DDL_ASSERT(IS_EOUT_MASK_GRP0(pstcEoutInit->u32EoutMaskGroup0));
        DDL_ASSERT(IS_EOUT_MASK_GRP1(pstcEoutInit->u32EoutMaskGroup1));
        /* Set Clear Timer */
        RegAddr = &ERMU_UNIT_REG(CM_ERMU->EO0CTCMP, u8Unit);
        MODIFY_REG32(*RegAddr, ERMU_EOCTCMP_CMP, (pstcEoutInit->u32ClearTmrCmpValue & 0xFFFFUL));
        RegAddr = &ERMU_UNIT_REG(CM_ERMU->EO0C, u8Unit);
        MODIFY_REG32(*RegAddr, ERMU_EOC_CTE, ((uint32_t)pstcEoutInit->enClearTmrEn << ERMU_EOC_CTE_POS));
        /* Set Toggle Timer */
        RegAddr = &ERMU_UNIT_REG(CM_ERMU->EO0TTCMP, u8Unit);
        MODIFY_REG32(*RegAddr, ERMU_EOTTCMP_CMP, (pstcEoutInit->u32ToggleTmrCmpValue & 0xFFFFUL));
        RegAddr = &ERMU_UNIT_REG(CM_ERMU->EO0TTC, u8Unit);
        MODIFY_REG32(*RegAddr, ERMU_EOTTC_TTE, pstcEoutInit->enToggleTmrEn);
        /* Set Error output Mask */
        RegAddr = &ERMU_UNIT_REG(CM_ERMU->EO0OM0, u8Unit);
        WRITE_REG32(*RegAddr, pstcEoutInit->u32EoutMaskGroup0);
        RegAddr = &ERMU_UNIT_REG(CM_ERMU->EO0OM1, u8Unit);
        WRITE_REG32(*RegAddr, pstcEoutInit->u32EoutMaskGroup1);
    }

    return i32Ret;
}

/**
 * @brief  Get ERMU error output error status
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval An @ref en_flag_status_t enumeration type value
 */
en_flag_status_t ERMU_EOUT_GetErrorStatus(uint8_t u8Unit)
{
    en_flag_status_t enFlagStatus = RESET;
    __IO uint32_t *EOxS;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));

    EOxS = &ERMU_UNIT_REG(CM_ERMU->EO0S, u8Unit);
    if (0UL != (READ_REG32_BIT(*EOxS, ERMU_EOS_EOS))) {
        enFlagStatus = SET;
    }

    return enFlagStatus;
}

/**
 * @brief  Clear ERMU error output error status
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval None
 * @note   Make sure that no unmasked errors occur and clear timer not enable
 */
void ERMU_EOUT_ClearErrorStatus(uint8_t u8Unit)
{
    __IO uint32_t *EOxC;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));

    EOxC = &ERMU_UNIT_REG(CM_ERMU->EO0C, u8Unit);
    SET_REG32_BIT(*EOxC, ERMU_EOC_CLR);
}

/**
 * @brief  Set ERMU error output error status
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval None
 */
void ERMU_EOUT_SetErrorStatus(uint8_t u8Unit)
{
    __IO uint32_t *EOxC;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));

    EOxC = &ERMU_UNIT_REG(CM_ERMU->EO0C, u8Unit);
    SET_REG32_BIT(*EOxC, ERMU_EOC_SET);
}

/**
 * @brief  Set the timer clock divider value
 * @param  [in] u32Div          Actual frequency division of the current timer
 *                              This parameter must be a value between 1~65536
 * @retval None
 */
void ERMU_SetClockDiv(uint32_t u32Div)
{
    DDL_ASSERT(IS_CLK_DIV_VALUE(u32Div));

    MODIFY_REG32(CM_ERMU->CCPS, ERMU_CCPS_PSS, (u32Div - 1U) << ERMU_CCPS_PSS_POS);
}

/**
 * @brief  Get the timer clock divider value
 * @param  None
 * @retval uint32_t             Actual frequency division of the current timer
 */
uint32_t ERMU_GetClockDiv(void)
{
    return ((READ_REG32_BIT(CM_ERMU->CCPS, ERMU_CCPS_PSS) >> ERMU_CCPS_PSS_POS) + 1U);
}

/**
 * @brief  Enable/Disable clock divider
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_ClockDivCmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    WRITE_REG32(bCM_ERMU->CCPS_b.PSE, enNewState);
}

/**
 * @brief  Enable/Disable clear timer
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_CTMR_Cmd(uint8_t u8Unit, en_functional_state_t enNewState)
{
    __IO uint32_t *EOxC;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    EOxC = &ERMU_UNIT_REG(CM_ERMU->EO0C, u8Unit);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*EOxC, ERMU_EOC_CTE);
    } else {
        CLR_REG32_BIT(*EOxC, ERMU_EOC_CTE);
    }
}

/**
 * @brief  Get ERMU clear timer status
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval An @ref en_flag_status_t enumeration type value
 */
en_flag_status_t ERMU_CTMR_GetStatus(uint8_t u8Unit)
{
    en_flag_status_t enFlagStatus = RESET;
    __IO uint32_t *EOxS;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));

    EOxS = &ERMU_UNIT_REG(CM_ERMU->EO0S, u8Unit);
    if (0UL != (READ_REG32_BIT(*EOxS, ERMU_EOS_CTS))) {
        enFlagStatus = SET;
    }

    return enFlagStatus;
}

/**
 * @brief  Get clear timer counter value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval uint16_t             The counter register value
 */
uint16_t ERMU_CTMR_GetCountValue(uint8_t u8Unit)
{
    __IO uint32_t *EOxCTCNT;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));

    EOxCTCNT = &ERMU_UNIT_REG(CM_ERMU->EO0CTCNT, u8Unit);

    return (uint16_t)READ_REG32(*EOxCTCNT);
}

/**
 * @brief  Get clear timer compare value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval uint16_t             The compare register value
 */
uint16_t ERMU_CTMR_GetCompareValue(uint8_t u8Unit)
{
    __IO uint32_t *EOxCTCMP;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));

    EOxCTCMP = &ERMU_UNIT_REG(CM_ERMU->EO0CTCMP, u8Unit);

    return (uint16_t)READ_REG32(*EOxCTCMP);
}

/**
 * @brief  Set clear timer compare value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @param  [in] u32Value        Compare Value of the timer
 *                              This parameter must be a value between 0~65535
 * @retval None
 */
void ERMU_CTMR_SetCompareValue(uint8_t u8Unit, uint32_t u32Value)
{
    __IO uint32_t *EOxCTCMP;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));
    DDL_ASSERT(IS_TMR_CMP_VALUE(u32Value));

    EOxCTCMP = &ERMU_UNIT_REG(CM_ERMU->EO0CTCMP, u8Unit);
    MODIFY_REG32(*EOxCTCMP, ERMU_EOCTCMP_CMP, u32Value);
}

/**
 * @brief  Enable/Disable toggle timer
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_TTMR_Cmd(uint8_t u8Unit, en_functional_state_t enNewState)
{
    __IO uint32_t *EOxTTC;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    EOxTTC = &ERMU_UNIT_REG(CM_ERMU->EO0TTC, u8Unit);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*EOxTTC, ERMU_EOTTC_TTE);
    } else {
        CLR_REG32_BIT(*EOxTTC, ERMU_EOTTC_TTE);
    }
}

/**
 * @brief  Get toggle timer counter value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval uint16_t             The counter register value
 */
uint16_t ERMU_TTMR_GetCountValue(uint8_t u8Unit)
{
    __IO uint32_t *EOxTTCNT;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));

    EOxTTCNT = &ERMU_UNIT_REG(CM_ERMU->EO0TTCNT, u8Unit);

    return (uint16_t)READ_REG32(*EOxTTCNT);
}

/**
 * @brief  Get toggle timer compare value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @retval uint16_t             The compare register value
 */
uint16_t ERMU_TTMR_GetCompareValue(uint8_t u8Unit)
{
    __IO uint32_t *EOxTTCMP;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));

    EOxTTCMP = &ERMU_UNIT_REG(CM_ERMU->EO0TTCMP, u8Unit);

    return (uint16_t)READ_REG32(*EOxTTCMP);
}

/**
 * @brief  Set toggle timer compare value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Eout_Unit
 * @param  [in] u32Value        Compare Value of the timer
 *                              This parameter must be a value between 0~65535
 * @retval None
 */
void ERMU_TTMR_SetCompareValue(uint8_t u8Unit, uint32_t u32Value)
{
    __IO uint32_t *EOxTTCMP;

    DDL_ASSERT(IS_EOUT_UNIT(u8Unit));
    DDL_ASSERT(IS_TMR_CMP_VALUE(u32Value));

    EOxTTCMP = &ERMU_UNIT_REG(CM_ERMU->EO0TTCMP, u8Unit);
    MODIFY_REG32(*EOxTTCMP, ERMU_EOTTCMP_CMP, u32Value);
}

/**
 * @brief  Enable/Disable wait timer
 * @param  [in] u8Unit          Error output unit @ref ERMU_Wtmr_Unit
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_WTMR_Cmd(uint8_t u8Unit, en_functional_state_t enNewState)
{
    __IO uint32_t *WTxC;

    DDL_ASSERT(IS_WTMR_UNIT(u8Unit));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    WTxC = &ERMU_WTUNIT_REG(CM_ERMU->WT0C, u8Unit);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*WTxC, ERMU_WTC_WTE);
    } else {
        CLR_REG32_BIT(*WTxC, ERMU_WTC_WTE);
    }
}

/**
 * @brief  Enable/Disable the high priority interrupt boot wait timer function
 * @param  [in] u8Unit          Error output unit @ref ERMU_Wtmr_Unit
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_WTMR_HighPriorityIntBootCmd(uint8_t u8Unit, en_functional_state_t enNewState)
{
    __IO uint32_t *WTxSE;

    DDL_ASSERT(IS_WTMR_UNIT(u8Unit));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    WTxSE = &ERMU_WTUNIT_REG(CM_ERMU->WT0SE, u8Unit);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*WTxSE, ERMU_WTSE_HPISE);
    } else {
        CLR_REG32_BIT(*WTxSE, ERMU_WTSE_HPISE);
    }
}

/**
 * @brief  Enable/Disable the low priority interrupt boot wait timer function
 * @param  [in] u8Unit          Error output unit @ref ERMU_Wtmr_Unit
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_WTMR_LowPriorityIntBootCmd(uint8_t u8Unit, en_functional_state_t enNewState)
{
    __IO uint32_t *WTxSE;

    DDL_ASSERT(IS_WTMR_UNIT(u8Unit));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    WTxSE = &ERMU_WTUNIT_REG(CM_ERMU->WT0SE, u8Unit);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*WTxSE, ERMU_WTSE_LPISE);
    } else {
        CLR_REG32_BIT(*WTxSE, ERMU_WTSE_LPISE);
    }
}

/**
 * @brief  Stop wait timer
 * @param  [in] u8Unit          Error output unit @ref ERMU_Wtmr_Unit
 * @retval None
 */
void ERMU_WTMR_Stop(uint8_t u8Unit)
{
    __IO uint32_t *WTxC;

    DDL_ASSERT(IS_WTMR_UNIT(u8Unit));

    WTxC = &ERMU_WTUNIT_REG(CM_ERMU->WT0C, u8Unit);
    SET_REG32_BIT(*WTxC, ERMU_WTC_STP);
}

/**
 * @brief  Get ERMU wait timer status
 * @param  [in] u8Unit          Error output unit @ref ERMU_Wtmr_Unit
 * @retval An @ref en_flag_status_t enumeration type value
 */
en_flag_status_t ERMU_WTMR_GetStatus(uint8_t u8Unit)
{
    en_flag_status_t enFlagStatus = RESET;
    __IO uint32_t *WTxS;

    DDL_ASSERT(IS_WTMR_UNIT(u8Unit));

    WTxS = &ERMU_WTUNIT_REG(CM_ERMU->WT0S, u8Unit);
    if (0UL != (READ_REG32_BIT(*WTxS, ERMU_WTS_WTS))) {
        enFlagStatus = SET;
    }

    return enFlagStatus;
}

/**
 * @brief  Get wait timer counter value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Wtmr_Unit
 * @retval uint16_t             The counter register value
 */
uint16_t ERMU_WTMR_GetCountValue(uint8_t u8Unit)
{
    __IO uint32_t *WTxCNT;

    DDL_ASSERT(IS_WTMR_UNIT(u8Unit));

    WTxCNT = &ERMU_WTUNIT_REG(CM_ERMU->WT0CNT, u8Unit);

    return (uint16_t)READ_REG32(*WTxCNT);
}

/**
 * @brief  Get wait timer compare value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Wtmr_Unit
 * @retval uint16_t             The compare register value
 */
uint16_t ERMU_WTMR_GetCompareValue(uint8_t u8Unit)
{
    __IO uint32_t *WTxCMP;

    DDL_ASSERT(IS_WTMR_UNIT(u8Unit));

    WTxCMP = &ERMU_WTUNIT_REG(CM_ERMU->WT0CMP, u8Unit);

    return (uint16_t)READ_REG32(*WTxCMP);
}

/**
 * @brief  Set wait timer compare value
 * @param  [in] u8Unit          Error output unit @ref ERMU_Wtmr_Unit
 * @param  [in] u32Value        Compare Value of the timer
 *                              This parameter must be a value between 0~65535
 * @retval None
 */
void ERMU_WTMR_SetCompareValue(uint8_t u8Unit, uint32_t u32Value)
{
    __IO uint32_t *WTxCMP;

    DDL_ASSERT(IS_WTMR_UNIT(u8Unit));
    DDL_ASSERT(IS_TMR_CMP_VALUE(u32Value));

    WTxCMP = &ERMU_WTUNIT_REG(CM_ERMU->WT0CMP, u8Unit);
    MODIFY_REG32(*WTxCMP, ERMU_WTCMP_CMP, u32Value);
}

/**
 * @brief  Get the specified error source status
 * @param  [in] u8Group         Error group @ref ERMU_Error_Group
 * @param  [in] u32ErrSrc       Error source selection
 *                              When u8Group is ERMU_ERR_GRP0, this parameter can be any combination of @ref ERMU_Error_Src_Group0
 *                              When u8Group is ERMU_ERR_GRP1, this parameter can be any combination of @ref ERMU_Error_Src_Group1
 * @retval An @ref en_flag_status_t enumeration type value
 */
en_flag_status_t ERMU_GetErrorSrcStatus(uint8_t u8Group, uint32_t u32ErrSrc)
{
    en_flag_status_t enStatus = RESET;
    __IO uint32_t *ESSx;

    DDL_ASSERT(IS_ERR_GROUP(u8Group));
    DDL_ASSERT(IS_ERR_SRC_VALUE(u8Group, u32ErrSrc));

    ESSx = &ERMU_GROUP_REG(CM_ERMU->ESS0, u8Group);
    if (0UL != READ_REG32_BIT(*ESSx, u32ErrSrc)) {
        enStatus = SET;
    }

    return enStatus;
}

/**
 * @brief  Clear the specified error source status
 * @param  [in] u8Group         Error group @ref ERMU_Error_Group
 * @param  [in] u32ErrSrc       Error source selection
 *                              When u8Group is ERMU_ERR_GRP0, this parameter can be any combination of @ref ERMU_Error_Src_Group0
 *                              When u8Group is ERMU_ERR_GRP1, this parameter can be any combination of @ref ERMU_Error_Src_Group1
 * @retval None
 */
void ERMU_ClearErrorSrcStatus(uint8_t u8Group, uint32_t u32ErrSrc)
{
    __IO uint32_t *ESSCx;

    DDL_ASSERT(IS_ERR_GROUP(u8Group));
    DDL_ASSERT(IS_ERR_SRC_VALUE(u8Group, u32ErrSrc));

    ESSCx = &ERMU_GROUP_REG(CM_ERMU->ESSC0, u8Group);
    WRITE_REG32(*ESSCx, u32ErrSrc);
}

/**
 * @brief  Set pseudo error trigger
 * @param  [in] u8Group         Error group @ref ERMU_Error_Group
 * @param  [in] u32ErrSrc       Error source selection
 *                              When u8Group is ERMU_ERR_GRP0, this parameter can be any combination of @ref ERMU_Error_Src_Group0
 *                              When u8Group is ERMU_ERR_GRP1, this parameter can be any combination of @ref ERMU_Error_Src_Group1
 * @retval None
 */
void ERMU_SetPseudoErrorTrigger(uint8_t u8Group, uint32_t u32ErrSrc)
{
    __IO uint32_t *PETx;

    DDL_ASSERT(IS_ERR_GROUP(u8Group));
    DDL_ASSERT(IS_ERR_SRC_VALUE(u8Group, u32ErrSrc));

    PETx = &ERMU_GROUP_REG(CM_ERMU->PET0, u8Group);
    WRITE_REG32(*PETx, u32ErrSrc);
}

/**
 * @brief  Enable/Disable error reset system function
 * @param  [in] u8Group         Error group @ref ERMU_Error_Group
 * @param  [in] u32ErrSrc       Error source selection
 *                              When u8Group is ERMU_ERR_GRP0, this parameter can be any combination of @ref ERMU_Error_Src_Group0
 *                              When u8Group is ERMU_ERR_GRP1, this parameter can be any combination of @ref ERMU_Error_Src_Group1
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_ResetCmd(uint8_t u8Group, uint32_t u32ErrSrc, en_functional_state_t enNewState)
{
    __IO uint32_t *REx;

    DDL_ASSERT(IS_ERR_GROUP(u8Group));
    DDL_ASSERT(IS_ERR_SRC_VALUE(u8Group, u32ErrSrc));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    REx = &ERMU_GROUP_REG(CM_ERMU->RE0, u8Group);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*REx, u32ErrSrc);
    } else {
        CLR_REG32_BIT(*REx, u32ErrSrc);
    }
}

/**
 * @brief  Enable/Disable low priority error interrupt
 * @param  [in] u8Group         Error group @ref ERMU_Error_Group
 * @param  [in] u32ErrSrc       Error source selection
 *                              When u8Group is ERMU_ERR_GRP0, this parameter can be any combination of @ref ERMU_Error_Src_Group0
 *                              When u8Group is ERMU_ERR_GRP1, this parameter can be any combination of @ref ERMU_Error_Src_Group1
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_LowPriorityIntCmd(uint8_t u8Group, uint32_t u32ErrSrc, en_functional_state_t enNewState)
{
    __IO uint32_t *LPIEx;

    DDL_ASSERT(IS_ERR_GROUP(u8Group));
    DDL_ASSERT(IS_ERR_SRC_VALUE(u8Group, u32ErrSrc));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    LPIEx = &ERMU_GROUP_REG(CM_ERMU->LPIE0, u8Group);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*LPIEx, u32ErrSrc);
    } else {
        CLR_REG32_BIT(*LPIEx, u32ErrSrc);
    }
}

/**
 * @brief  Enable/Disable high priority(NMI) error interrupt
 * @param  [in] u8Group         Error group @ref ERMU_Error_Group
 * @param  [in] u32ErrSrc       Error source selection
 *                              When u8Group is ERMU_ERR_GRP0, this parameter can be any combination of @ref ERMU_Error_Src_Group0
 *                              When u8Group is ERMU_ERR_GRP1, this parameter can be any combination of @ref ERMU_Error_Src_Group1
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value
 * @retval None
 */
void ERMU_HighPriorityIntCmd(uint8_t u8Group, uint32_t u32ErrSrc, en_functional_state_t enNewState)
{
    __IO uint32_t *HPIEx;

    DDL_ASSERT(IS_ERR_GROUP(u8Group));
    DDL_ASSERT(IS_ERR_SRC_VALUE(u8Group, u32ErrSrc));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    HPIEx = &ERMU_GROUP_REG(CM_ERMU->HPIE0, u8Group);
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*HPIEx, u32ErrSrc);
    } else {
        CLR_REG32_BIT(*HPIEx, u32ErrSrc);
    }
}

/**
 * @brief  De-Initialize ERMU function
 * @param  None
 * @retval int32_t:
 *         - LL_OK:           Reset success
 *         - LL_ERR_TIMEOUT:  Reset time out
 * @note   Call LL_PERIPH_WE(LL_PERIPH_PWC_CLK_RMU) unlock RMU_FRSTx register first
 */
int32_t ERMU_DeInit(void)
{
    int32_t i32Ret = LL_OK;
    __IO uint8_t u8TimeOut = 0U;

    DDL_ASSERT((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1);
    CLR_REG32(bCM_RMU->FRST0_b.ERMU);
    /* Ensure reset procedure is completed */
    while (1UL != READ_REG32(bCM_RMU->FRST0_b.ERMU)) {
        u8TimeOut++;
        if (u8TimeOut > ERMU_RMU_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }

    return i32Ret;
}

/**
 * @}
 */

#endif /* LL_ERMU_ENABLE */

/**
 * @}
 */

/**
* @}
*/

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
