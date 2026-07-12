/**
 *******************************************************************************
 * @file  hc32_ll_aos.c
 * @brief This file provides firmware functions to manage the AOS.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-01-15       CDT             First version
   2024-06-30       CDT             Modify assert IS_AOS_TARGET
   2024-08-31       CDT             Optimize assert IS_AOS_TARGET
   2025-11-03       CDT             Add group definition AOS_Const_Value_Def
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2025, Xiaohua Semiconductor Co., Ltd. All rights reserved.
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
#include "hc32_ll_aos.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_AOS AOS
 * @brief AOS Driver Library
 * @{
 */

#if (LL_AOS_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup AOS_Local_Macros AOS Local Macros
 * @{
 */

/**
 * @defgroup AOS_Const_Value_Def AOS Const Value Definition
 * @{
 */
#define AOS_TRG_START                   AOS_DMA_0
#define AOS_TRG_END                     AOS_COMM_2
/**
 * @}
 */

/**
 * @defgroup AOS_Common_Trigger_ID_Validity AOS Common Trigger ID Validity
 * @{
 */
#define IS_AOS_COMM_TRIG(x)                                                    \
(   ((x) != 0UL)                                &&                             \
    ((x) | AOS_COMM_TRIG_MASK) == AOS_COMM_TRIG_MASK)

/**
 * @}
 */

/**
 * @defgroup AOS_Target_Select_Validity AOS Target Select Validity
 * @{
 */
#define IS_AOS_TARGET(x)                                                       \
(   ((x) >= AOS_TRG_START) && ((x) <= AOS_TRG_END) && (((x) & 0x03UL) == 0UL))

/**
 * @}
 */

#define IS_AOS_PWC_UNLOCKED()           ((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1)

/* AOS reset timeout */
#define AOS_RMU_TIMEOUT                 (100UL)

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
 * @defgroup AOS_Global_Functions AOS Global Functions
 * @{
 */

/**
 * @brief  De-Initialize AOS.
 * @param  None
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_TIMEOUT:          Works timeout.
 */
int32_t AOS_DeInit(void)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeOut = 0U;

    /* Check parameters */
    DDL_ASSERT(IS_AOS_PWC_UNLOCKED());

    CLR_REG32_BIT(CM_RMU->FRST0, RMU_FRST0_AOS);
    /* Ensure reset procedure is completed */
    while (RMU_FRST0_AOS != READ_REG32_BIT(CM_RMU->FRST0, RMU_FRST0_AOS)) {
        u32TimeOut++;
        if (u32TimeOut > AOS_RMU_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }

    return i32Ret;
}

/**
 * @brief  Common trigger function command
 * @param  [in] u32Target           AOS target that need to be triggered by common trigger @ref AOS_Target_Select in details
 * @param  [in] u32CommonTrigger    Common trigger ID
 *                                  This parameter can be one of the following values:
 *   @arg  AOS_COMM_TRIG1:          Common trigger 1.
 *   @arg  AOS_COMM_TRIG2:          Common trigger 2.
 * @param  [in] enNewState          An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void AOS_CommonTriggerCmd(uint32_t u32Target, uint32_t u32CommonTrigger, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_AOS_TARGET(u32Target));
    DDL_ASSERT(IS_AOS_COMM_TRIG(u32CommonTrigger));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(*(__IO uint32_t *)u32Target, u32CommonTrigger);
    } else {
        CLR_REG32_BIT(*(__IO uint32_t *)u32Target, u32CommonTrigger);
    }
}

/**
 * @brief  Set trigger event source
 * @param  [in] u32Target       AOS target that need to be triggered by AOS source @ref AOS_Target_Select in details
 * @param  [in] enSource        AOS source that trigger the AOS target @ref en_event_src_t in details
 * @retval None
 */
void AOS_SetTriggerEventSrc(uint32_t u32Target, en_event_src_t enSource)
{
    DDL_ASSERT(IS_AOS_TARGET(u32Target));

    MODIFY_REG32(*(__IO uint32_t *)u32Target, AOS_TRIG_SEL_MASK, enSource);
}

/**
 * @}
 */

#endif /* LL_AOS_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
