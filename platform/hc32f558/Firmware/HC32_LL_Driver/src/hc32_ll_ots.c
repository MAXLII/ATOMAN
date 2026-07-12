/**
 *******************************************************************************
 * @file  hc32_ll_ots.c
 * @brief This file provides firmware functions to manage the OTS.
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
#include "hc32_ll_ots.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_OTS OTS
 * @brief OTS Driver Library
 * @{
 */

#if (LL_OTS_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup OTS_Local_Macros OTS Local Macros
 * @{
 */

#define OTS_RMU_TIMEOUT             (100UL)

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
 * @defgroup OTS_Global_Functions OTS Global Functions
 * @{
 */

/**
 * @brief  De-initializes OTS peripheral. Reset the registers of OTS.
 * @param  None
 * @retval int32_t:
 *           - LL_OK:                   De-Initialize success.
 *           - LL_ERR_TIMEOUT:          Timeout.
 */
int32_t OTS_DeInit(void)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeOut = 0U;
    /* Check RMU_FRST register protect */
    DDL_ASSERT((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1);

    /* Reset */
    WRITE_REG32(bCM_RMU->FRST3_b.OTS, 0UL);
    /* Ensure reset procedure is completed */
    while (0UL == READ_REG32(bCM_RMU->FRST3_b.OTS)) {
        u32TimeOut++;
        if (u32TimeOut > OTS_RMU_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }
    return i32Ret;
}

/**
 * @brief  Get the value for OTS preset constant.
 * @param  [in] pstcPresetConstant      Pointer to a stc_ots_preset_constant_t structure that
 *                                      contains configuration information.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcPresetConstant == NULL.
 */
int32_t OTS_GetPresetConstant(stc_ots_preset_constant_t *pstcPresetConstant)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (pstcPresetConstant != NULL) {
        pstcPresetConstant->u32C = (READ_REG32(CM_OTS->PDR1) & OTS_PDR1_C) >> OTS_PDR1_C_POS;
        pstcPresetConstant->u32M = READ_REG32(CM_OTS->PDR1) & OTS_PDR1_M;
        pstcPresetConstant->u16K = READ_REG16(CM_OTS->PDR0);
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Enable or disable the OTS function.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void OTS_Cmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    WRITE_REG32(bCM_OTS->CTL_b.OTSE, enNewState);
}

/**
 * @}
 */

#endif /* LL_OTS_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
