/**
 *******************************************************************************
 * @file  hc32_ll_pid.c
 * @brief This file provides firmware functions to manage the high precision
 *        Reference Voltage(PID).
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
#include "hc32_ll_pid.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_PID PID
 * @brief PID Driver Library
 * @{
 */

#if (LL_PID_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup PID_Local_Macros PID Local Macros
 * @{
 */
#define PID_FLOAT_READ_REG32(reg)           (*(__IO float32_t *)((uint32_t)(&reg)))
#define PID_FLOAT_WRITE_REG32(reg, value)   WRITE_REG32((reg), (*(__IO uint32_t *)((uint32_t)&(value))));

#define PID_COEF_REG(reg_base, ch)          ((__IO uint32_t *)((uint32_t)(&(reg_base)) + ((ch) * 0xCUL)))

/**
 * @defgroup PID_Check_Parameters_Validity PID Check Parameters Validity
 * @{
 */
/* Parameter valid check for PID calculate mode */
#define IS_PID_UNIT(x)                                                          \
(   ((x) == CM_PID1)                                ||                          \
    ((x) == CM_PID2))

/* Parameter valid check for PID calculate mode */
#define IS_PID_CAL_MD(x)                                                        \
(   ((x) == PID_CAL_MD_SW)                          ||                          \
    ((x) == PID_CAL_MD_HW_AVAL)                     ||                          \
    ((x) == PID_CAL_MD_HW_AVAL_REF))

/* Parameter valid check for PID integrate mode */
#define IS_PID_INTEG_MD(x)                                                      \
(   ((x) == PID_INTEG_MD_EK)                        ||                          \
    ((x) == PID_INTEG_MD_EK_EK_1))

/* Parameter valid check for PID differential mode */
#define IS_PID_DIFF_MD(x)                                                       \
(   ((x) == PID_DIFF_MD_WITHOUT_FEEDBACK)           ||                          \
    ((x) == PID_DIFF_MD_WITH_FEEDBACK))

/* Parameter valid check for PID coefficient mode */
#define IS_PID_COEF_MD(x)                                                       \
(   ((x) == PID_COEF_MD_1)                          ||                          \
    ((x) == PID_COEF_MD_2)                          ||                          \
    ((x) == PID_COEF_MD_3)                          ||                          \
    ((x) == PID_COEF_MD_4))

#define IS_PID_U1_P2_DATA_SEL(x)                                               \
(   ((x) == PID_U1_P2_DATA_IIR1)                    ||                         \
    ((x) == PID_U1_P2_DATA_IIR2)                    ||                         \
    ((x) == PID_U1_P2_DATA_IIR3)                    ||                         \
    ((x) == PID_U1_P2_DATA_IIR4)                    ||                         \
    ((x) == PID_U1_P2_DATA_IIR5)                    ||                         \
    ((x) == PID_U1_P2_DATA_IIR6))

#define IS_PID_U3_P2_DATA_SEL(x)                                               \
(   ((x) == PID_U3_P2_DATA_SOGI_SIN)                ||                         \
    ((x) == PID_U3_P2_DATA_SOGI_COS)                ||                         \
    ((x) == PID_U3_P2_DATA_CORDIC_RES1)             ||                         \
    ((x) == PID_U3_P2_DATA_CORDIC_RES2))

#define IS_PID_U3_P2_WAIT_STATE(x)                                             \
(   ((x) == PID_U3_P2_WAIT_OFF)                     ||                         \
    ((x) == PID_U3_P2_WAIT_ON))

#define IS_PID_U3_P4_CAL_TYPE(x)                                               \
(   ((x) == PID_U3_P4_CAL_ADD)                     ||                          \
    ((x) == PID_U3_P4_CAL_SUB))

#define IS_PID_U3_P4_WAIT_STATE(x)                                             \
(   ((x) == PID_U3_P4_WAIT_OFF)                     ||                         \
    ((x) == PID_U3_P4_WAIT_ON))

/* Parameter valid check for PID init expected value selection while software start */
#define IS_PID_SW_INIT_EV_SEL(x)                                                \
(   ((x) == PID_SW_INIT_EV_REF)                     ||                          \
    ((x) == PID_SW_INIT_EV_AVAL))

/* Parameter valid check for PID coefficient number */
#define IS_PID_COEF_NUM(x)                          ((x) <= PID_COEF_NUM_3)

/* Parameter valid check for PID actual value selection */
#define IS_PID_AVAL_SEL(x)                                                      \
(   ((x) == PID_AVAL_REG)                           ||                          \
    ((x) == PID_AVAL_IIR1)                          ||                          \
    ((x) == PID_AVAL_IIR2)                          ||                          \
    ((x) == PID_AVAL_IIR3)                          ||                          \
    ((x) == PID_AVAL_IIR4)                          ||                          \
    ((x) == PID_AVAL_IIR5)                          ||                          \
    ((x) == PID_AVAL_IIR6))

/* Parameter valid check for PID expected value selection */
#define IS_PID_EV_SEL(x)                                                        \
(   ((x) == PID_EV_REG)                             ||                          \
    ((x) == PID_EV_P1_U3)                           ||                          \
    ((x) == PID_EV_P2_U3))

/* Parameter valid check for PID E(k) minimum selection */
#define IS_PID_EK_MIN_SEL(x)                                                    \
(   ((x) == PID_EK_MIN_EMIN)                        ||                          \
    ((x) == PID_EK_MIN_0))

/* Parameter valid check for PID KF minimum selection */
#define IS_PID_KF_MIN_SEL(x)                                                    \
(   ((x) == PID_KF_MIN_KFMIN)                       ||                          \
    ((x) == PID_KF_MIN_0))

/* Parameter valid check for PID Ki minimum selection */
#define IS_PID_KI_MIN_SEL(x)                                                    \
(   ((x) == PID_KI_MIN_IMIN)                        ||                          \
    ((x) == PID_KI_MIN_0))

/* Parameter valid check for PID Kd minimum selection */
#define IS_PID_KD_MIN_SEL(x)                                                    \
(   ((x) == PID_KD_MIN_DMIN)                        ||                          \
    ((x) == PID_KD_MIN_0))

/* Parameter valid check for PID U minimum selection */
#define IS_PID_U_MIN_SEL(x)                                                     \
(   ((x) == PID_U_MIN_UMIN)                         ||                          \
    ((x) == PID_U_MIN_0))

/* Parameter valid check for PID U1 minimum selection */
#define IS_PID_U1_MIN_SEL(x)                                                    \
(   ((x) == PID_U1_MIN_U1MIN)                       ||                          \
    ((x) == PID_U1_MIN_0))

/* Parameter valid check for PID U3 minimum selection */
#define IS_PID_U3_MIN_SEL(x)                                                    \
(   ((x) == PID_U3_MIN_U3MIN)                       ||                          \
    ((x) == PID_U3_MIN_0))

/* Parameter valid check for PID U5 minimum selection */
#define IS_PID_U5_MIN_SEL(x)                                                    \
(   ((x) == PID_U5_MIN_U5MIN)                       ||                          \
    ((x) == PID_U5_MIN_0))

/* Parameter valid check for PID U5 minimum selection */
#define IS_PID_FLAG(x)                                                          \
(   ((x) == PID_FLAG_DATA_RDY)                      ||                          \
    ((x) == PID_FLAG_CAL_OVER)                      ||                          \
    ((x) == PID_FLAG_CAL_INF)                       ||                          \
    ((x) == PID_FLAG_CAL_NAN))

/* Parameter valid check for PID competition output selection */
#define IS_PID_COMP_OUTPUT_SEL(x)                                               \
(   ((x) == PID_COMP_OUTPUT_PID1)                   ||                          \
    ((x) == PID_COMP_OUTPUT_PID2)                   ||                          \
    ((x) == PID_COMP_OUTPUT_MAX)                    ||                          \
    ((x) == PID_COMP_OUTPUT_MIN))

/* Parameter valid check for PID burst function */
#define IS_PID_BURST_FUNC(x)                                                    \
(   ((x) == PID_BURST_FUNC_NONE)                    ||                          \
    ((x) == PID_BURST_FUNC_PID1)                    ||                          \
    ((x) == PID_BURST_FUNC_PID2)                    ||                          \
    ((x) == PID_BURST_FUNC_COMP_OUTPUT))

/* Parameter valid check for PID burst mode */
#define IS_PID_BURST_MD(x)                                                      \
(   ((x) == PID_BURST_MD_1)                         ||                          \
    ((x) == PID_BURST_MD_2))

/* Parameter valid check for PID burst interrupt selection */
#define IS_PID_BURST_INT_SEL(x)                                                 \
(   ((x) == PID_BURST_INT_DONE)                     ||                          \
    ((x) == PID_BURST_INT_DONE_THRESHOLD))

#define IS_PID_CMP_UNIT(x)              ((x) == CM_PID_CMP)

#define IS_PID_COMP_FLAG(x)                                                    \
(   ((x) != 0UL)                                    &&                         \
    (((x) | PID_COMP_FLAG_ALL) == PID_COMP_FLAG_ALL))

#define IS_PID_COMP_CLR_FLAG(x)                                                \
(   ((x) != 0UL)                                    &&                         \
    (((x) | PID_COMP_FLAG_CLR_ALL) == PID_COMP_FLAG_CLR_ALL))

#define IS_PID_FUNCTIONAL_STATE(x)       (((x) == (uint32_t)DISABLE) || ((x) == (uint32_t)ENABLE))
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
 * @defgroup PID_Global_Functions PID Global Functions
 * @{
 */
/**
 * @brief  Enable or disable the PID calculate
 * @param  [in] PIDx                    PID unit
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PID_Cmd(CM_PID_TypeDef *PIDx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE ==  enNewState) {
        SET_REG32_BIT(PIDx->CR, PID_CR_PID_EN);
    } else {
        CLR_REG32_BIT(PIDx->CR, PID_CR_PID_EN);
    }
}

/**
 * @brief  Start PID calculate
 * @param  [in] PIDx                    PID unit
 * @retval None
 * @note   Please set PID_EN to 1 before called PID_Start().
 */
void PID_Start(CM_PID_TypeDef *PIDx)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));

    SET_REG32_BIT(PIDx->CR, PID_CR_START);
}

/**
 * @brief  Clear PID data(all)
 * @param  [in] PIDx                    PID unit
 * @retval None
 */
void PID_ClearData(CM_PID_TypeDef *PIDx)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));

    SET_REG32_BIT(PIDx->CR, PID_CR_CLEAR);
}

/**
 * @brief  Reset PID FSM
 * @param  [in] PIDx                    PID unit
 * @retval None
 */
void PID_ResetFsm(CM_PID_TypeDef *PIDx)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));

    SET_REG32_BIT(PIDx->CR, PID_CR_FSM_RST);
}

/**
 * @brief  Enable or disable the PID interrupt
 * @param  [in] PIDx                    PID unit
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PID_IntCmd(CM_PID_TypeDef *PIDx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE ==  enNewState) {
        SET_REG32_BIT(PIDx->CR2, PID_CR2_INT_EN);
    } else {
        CLR_REG32_BIT(PIDx->CR2, PID_CR2_INT_EN);
    }
}

/**
 * @brief  Enable or disable software start calculate
 * @param  [in] PIDx                    PID unit
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PID_SWStartCmd(CM_PID_TypeDef *PIDx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE ==  enNewState) {
        SET_REG32_BIT(PIDx->CR2, PID_CR2_RAMP_EN);
    } else {
        CLR_REG32_BIT(PIDx->CR2, PID_CR2_RAMP_EN);
    }
}

/**
 * @brief  Init PID  initial structure with default value.
 * @param  [in] pstcPidInit             Specifies the parameter of PID initial structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_StructInit(stc_pid_init_t *pstcPidInit)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcPidInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcPidInit->u32PidEn        = (uint32_t)DISABLE;
        pstcPidInit->u32CalMode      = PID_CAL_MD_SW;
        pstcPidInit->u32IMode        = PID_INTEG_MD_EK;
        pstcPidInit->u32DMode        = PID_DIFF_MD_WITHOUT_FEEDBACK;
        pstcPidInit->u32CoefMode     = PID_COEF_MD_1;
        pstcPidInit->u32AntiWindupEn = (uint32_t)DISABLE;
        pstcPidInit->u32U1P1En       = (uint32_t)DISABLE;
        pstcPidInit->u32U1P2En       = (uint32_t)DISABLE;
        pstcPidInit->u32U3P1En       = (uint32_t)DISABLE;
        pstcPidInit->u32U3P2En       = (uint32_t)DISABLE;
        pstcPidInit->u32U3P3En       = (uint32_t)DISABLE;
        pstcPidInit->u32U3P4En       = (uint32_t)DISABLE;
        pstcPidInit->u32U4P1En       = (uint32_t)DISABLE;
        pstcPidInit->u32U5P1En       = (uint32_t)DISABLE;
        pstcPidInit->u32U5P2En       = (uint32_t)DISABLE;
    }

    return i32Ret;
}

/**
 * @brief  PID initialize
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstcPidInit             Specifies the PID initial config.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_Init(CM_PID_TypeDef *PIDx, stc_pid_init_t *pstcPidInit)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32Value;

    /* Check if pointer is NULL */
    if (NULL == pstcPidInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32PidEn));
        DDL_ASSERT(IS_PID_CAL_MD(pstcPidInit->u32CalMode));
        DDL_ASSERT(IS_PID_INTEG_MD(pstcPidInit->u32IMode));
        DDL_ASSERT(IS_PID_DIFF_MD(pstcPidInit->u32DMode));
        DDL_ASSERT(IS_PID_COEF_MD(pstcPidInit->u32CoefMode));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32AntiWindupEn));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U1P1En));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U1P2En));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U3P1En));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U3P2En));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U3P3En));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U3P4En));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U4P1En));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U5P1En));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcPidInit->u32U5P2En));

        u32Value = ((uint32_t)pstcPidInit->u32PidEn << PID_CR_PID_EN_POS)         | \
                   ((uint32_t)pstcPidInit->u32U1P1En << PID_CR_U1P1_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32U1P2En << PID_CR_U1P2_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32U3P1En << PID_CR_U3P1_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32U3P2En << PID_CR_U3P2_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32U3P3En << PID_CR_U3P3_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32U3P4En << PID_CR_U3P4_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32U4P1En << PID_CR_U4P1_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32U5P1En << PID_CR_U5P1_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32U5P2En << PID_CR_U5P2_EN_POS)       | \
                   ((uint32_t)pstcPidInit->u32AntiWindupEn << PID_CR_ISAT_EN_POS) | \
                   pstcPidInit->u32CoefMode | pstcPidInit->u32DMode | \
                   pstcPidInit->u32IMode    | pstcPidInit->u32CalMode;
        if ((uint32_t)ENABLE == pstcPidInit->u32U1P2En) {
            DDL_ASSERT(IS_PID_U1_P2_DATA_SEL(pstcPidInit->u32U1P2Sel));
            u32Value |= pstcPidInit->u32U1P2Sel;
        }
        if ((uint32_t)ENABLE == pstcPidInit->u32U3P2En) {
            DDL_ASSERT(IS_PID_U3_P2_DATA_SEL(pstcPidInit->u32U3P2Sel));
            DDL_ASSERT(IS_PID_U3_P2_WAIT_STATE(pstcPidInit->u32U3P2Wait));
            u32Value |= (pstcPidInit->u32U3P2Sel | pstcPidInit->u32U3P2Wait);
        }
        if ((uint32_t)ENABLE == pstcPidInit->u32U3P4En) {
            DDL_ASSERT(IS_PID_U3_P4_CAL_TYPE(pstcPidInit->u32U3P4Type));
            DDL_ASSERT(IS_PID_U3_P4_WAIT_STATE(pstcPidInit->u32U3P4Wait));
            u32Value |= (pstcPidInit->u32U3P4Type | pstcPidInit->u32U3P4Wait);
        }

        WRITE_REG32(PIDx->CR, u32Value);
    }

    return i32Ret;
}

/**
 * @brief  Init PID software start initial structure with default value.
 * @param  [in] pstcSWInit              Specifies the parameter of PID software start initial structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_SWStructInit(stc_pid_sw_init_t *pstcSWInit)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcSWInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcSWInit->u32State     = (uint32_t)DISABLE;
        pstcSWInit->u32InitEvSel = PID_SW_INIT_EV_REF;
        pstcSWInit->f32FinalEv   = 0.0f;
        pstcSWInit->f32Step      = 0.0f;
    }

    return i32Ret;
}

/**
 * @brief  PID initialize
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstcSWInit              Specifies the PID initial config.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_SWInit(CM_PID_TypeDef *PIDx, stc_pid_sw_init_t *pstcSWInit)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcSWInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcSWInit->u32State));
        DDL_ASSERT(IS_PID_SW_INIT_EV_SEL(pstcSWInit->u32InitEvSel));

        PID_FLOAT_WRITE_REG32(PIDx->REF_FIN, pstcSWInit->f32FinalEv);
        PID_FLOAT_WRITE_REG32(PIDx->REF_STEP, pstcSWInit->f32Step);
        MODIFY_REG32(PIDx->CR2, PID_CR2_RAMP_EN | PID_CR2_REF_INIT,
                     (pstcSWInit->u32InitEvSel | ((uint32_t)pstcSWInit->u32State << PID_CR2_RAMP_EN_POS)));
    }

    return i32Ret;
}

/**
 * @brief  Init PID configure structure with default value.
 * @param  [in] pstcConfig              Specifies the parameter of PID configure structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_ConfigStructInit(stc_pid_config_t *pstcConfig)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcConfig->u32AValSel  = PID_AVAL_REG;
        pstcConfig->u32EValSel  = PID_EV_REG;
        pstcConfig->u32EkMinSel = PID_EK_MIN_EMIN;
        pstcConfig->u32KfMinSel = PID_KF_MIN_KFMIN;
        pstcConfig->u32KiMinSel = PID_KI_MIN_IMIN;
        pstcConfig->u32KdMinSel = PID_KD_MIN_DMIN;
        pstcConfig->u32UMinSel  = PID_U_MIN_UMIN;
        pstcConfig->u32U1MinSel = PID_U1_MIN_U1MIN;
        pstcConfig->u32U3MinSel = PID_U3_MIN_U3MIN;
        pstcConfig->u32U5MinSel = PID_U5_MIN_U5MIN;
    }

    return i32Ret;
}

/**
 * @brief  Config PID
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstcConfig              Specifies the PID config
 * @retval int32_t:
 *         - LL_OK: config success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_Config(CM_PID_TypeDef *PIDx, stc_pid_config_t *pstcConfig)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32Mask;
    uint32_t u32Value;

    /* Check if pointer is NULL */
    if (NULL == pstcConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));
        DDL_ASSERT(IS_PID_AVAL_SEL(pstcConfig->u32AValSel));
        DDL_ASSERT(IS_PID_EV_SEL(pstcConfig->u32EValSel));
        DDL_ASSERT(IS_PID_EK_MIN_SEL(pstcConfig->u32EkMinSel));
        DDL_ASSERT(IS_PID_KF_MIN_SEL(pstcConfig->u32KfMinSel));
        DDL_ASSERT(IS_PID_KI_MIN_SEL(pstcConfig->u32KiMinSel));
        DDL_ASSERT(IS_PID_KD_MIN_SEL(pstcConfig->u32KdMinSel));
        DDL_ASSERT(IS_PID_U_MIN_SEL(pstcConfig->u32UMinSel));
        DDL_ASSERT(IS_PID_U1_MIN_SEL(pstcConfig->u32U1MinSel));
        DDL_ASSERT(IS_PID_U3_MIN_SEL(pstcConfig->u32U3MinSel));
        DDL_ASSERT(IS_PID_U5_MIN_SEL(pstcConfig->u32U5MinSel));

        u32Mask = PID_CR2_EMIN_SEL  | PID_CR2_IMIN_SEL  | PID_CR2_DMIN_SEL  | \
                  PID_CR2_UMIN_SEL  | PID_CR2_U1MIN_SEL | PID_CR2_U3MIN_SEL | \
                  PID_CR2_U5MIN_SEL | PID_CR2_KFMIN_SEL | PID_CR2_DSEL      | \
                  PID_CR2_ASEL;
        u32Value = pstcConfig->u32AValSel  | pstcConfig->u32EValSel  | pstcConfig->u32EkMinSel | \
                   pstcConfig->u32KfMinSel | pstcConfig->u32KiMinSel | pstcConfig->u32KdMinSel | \
                   pstcConfig->u32UMinSel  | pstcConfig->u32U1MinSel | pstcConfig->u32U3MinSel | \
                   pstcConfig->u32U5MinSel;

        MODIFY_REG32(PIDx->CR2, u32Mask, u32Value);
    }

    return i32Ret;
}

/**
 * @brief  Init PID gain config structure with default value.
 * @param  [in] pstcGainConfig          Specifies the parameter of PID gain config structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_GainStructInit(stc_pid_gain_config_t *pstcGainConfig)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcGainConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcGainConfig->f32Kp = 0.0f;
        pstcGainConfig->f32Ki = 0.0f;
        pstcGainConfig->f32Kd = 0.0f;
        pstcGainConfig->f32Kdf = 0.0f;
    }

    return i32Ret;
}

/**
 * @brief  Config PID gain
 * @param  [in] PIDx                    PID unit
 * @param  [in] u8CoefNum               Specifies the PID coefficient number @ref PID_Coef_Num_Sel
 * @param  [in] pstcGainConfig          Specifies the PID gain config
 * @retval int32_t:
 *         - LL_OK: config success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_GainConfig(CM_PID_TypeDef *PIDx, uint8_t u8CoefNum, stc_pid_gain_config_t *pstcGainConfig)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcGainConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));
        DDL_ASSERT(IS_PID_COEF_NUM(u8CoefNum));

        PID_FLOAT_WRITE_REG32(*PID_COEF_REG(PIDx->KP1, u8CoefNum), pstcGainConfig->f32Kp);
        PID_FLOAT_WRITE_REG32(*PID_COEF_REG(PIDx->KI1, u8CoefNum), pstcGainConfig->f32Ki);
        PID_FLOAT_WRITE_REG32(*PID_COEF_REG(PIDx->KD1, u8CoefNum), pstcGainConfig->f32Kd);
        switch (u8CoefNum) {
            case PID_COEF_NUM_1:
                PID_FLOAT_WRITE_REG32(PIDx->KDF1, pstcGainConfig->f32Kdf);
                break;
            case PID_COEF_NUM_2:
                PID_FLOAT_WRITE_REG32(PIDx->KDF2, pstcGainConfig->f32Kdf);
                break;
            case PID_COEF_NUM_3:
                PID_FLOAT_WRITE_REG32(PIDx->KDF3, pstcGainConfig->f32Kdf);
                break;
            default:
                PID_FLOAT_WRITE_REG32(PIDx->KDF1, pstcGainConfig->f32Kdf);
                break;
        }
    }

    return i32Ret;
}

/**
 * @brief  Read PID actual gain
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstcGain                Specifies the PID actual gain
 * @retval int32_t:
 *         - LL_OK: Read success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_ReadActualGain(CM_PID_TypeDef *PIDx, stc_pid_gain_t *pstcGain)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcGain) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcGain->f32Kf = PID_FLOAT_READ_REG32(PIDx->KF_ACT);
        pstcGain->f32Kp = PID_FLOAT_READ_REG32(PIDx->KP_ACT);
        pstcGain->f32Ki = PID_FLOAT_READ_REG32(PIDx->KI_ACT);
        pstcGain->f32Kd = PID_FLOAT_READ_REG32(PIDx->KD_ACT);
        pstcGain->f32Kdf = PID_FLOAT_READ_REG32(PIDx->KDF_ACT);
    }

    return i32Ret;
}

/**
 * @brief  Init PID gain switch structure with default value.
 * @param  [in] pstcGainSwitch          Specifies the parameter of PID gain switch structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_GainSwitchStructInit(stc_pid_gain_switch_t *pstcGainSwitch)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcGainSwitch) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcGainSwitch->f32threshold1 = 0.0f;
        pstcGainSwitch->f32threshold2 = 0.0f;
        pstcGainSwitch->f32Delay1 = 0.0f;
        pstcGainSwitch->f32Delay2 = 0.0f;
    }

    return i32Ret;
}

/**
 * @brief  Config PID gain switch
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstcGainSwitch          Specifies the PID gain switch config value
 * @retval int32_t:
 *         - LL_OK: config success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_GainSwitchConfig(CM_PID_TypeDef *PIDx, stc_pid_gain_switch_t *pstcGainSwitch)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcGainSwitch) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));

        PID_FLOAT_WRITE_REG32(PIDx->CSTH1, pstcGainSwitch->f32threshold1);
        PID_FLOAT_WRITE_REG32(PIDx->CSTH2, pstcGainSwitch->f32threshold2);
        PID_FLOAT_WRITE_REG32(PIDx->DLY1, pstcGainSwitch->f32Delay1);
        PID_FLOAT_WRITE_REG32(PIDx->DLY2, pstcGainSwitch->f32Delay2);
    }

    return i32Ret;
}

/**
 * @brief  Init PID value config structure with default value.
 * @param  [in] pstValueConfig          Specifies the parameter of PID value config structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_ValueStructInit(stc_pid_value_config_t *pstValueConfig)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstValueConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstValueConfig->f32EkMax = 0.0f;
        pstValueConfig->f32EkMin = 0.0f;
        pstValueConfig->f32KiMax = 0.0f;
        pstValueConfig->f32KiMin = 0.0f;
        pstValueConfig->f32KdMax = 0.0f;
        pstValueConfig->f32KdMin = 0.0f;
        pstValueConfig->f32UMax  = 0.0f;
        pstValueConfig->f32UMin  = 0.0f;
        pstValueConfig->f32U1Max = 0.0f;
        pstValueConfig->f32U3Max = 0.0f;
        pstValueConfig->f32U3Min = 0.0f;
        pstValueConfig->f32U5Max = 0.0f;
        pstValueConfig->f32U5Min = 0.0f;
    }

    return i32Ret;
}

/**
 * @brief  Config PID max & min value
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstValueConfig          Specifies the PID max & min value config
 * @retval int32_t:
 *         - LL_OK: config success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_ValueConfig(CM_PID_TypeDef *PIDx, stc_pid_value_config_t *pstValueConfig)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstValueConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));
        PID_FLOAT_WRITE_REG32(PIDx->EMAX, pstValueConfig->f32EkMax);
        PID_FLOAT_WRITE_REG32(PIDx->EMIN, pstValueConfig->f32EkMin);
        PID_FLOAT_WRITE_REG32(PIDx->IMAX, pstValueConfig->f32KiMax);
        PID_FLOAT_WRITE_REG32(PIDx->IMIN, pstValueConfig->f32KiMin);
        PID_FLOAT_WRITE_REG32(PIDx->DMAX, pstValueConfig->f32KdMax);
        PID_FLOAT_WRITE_REG32(PIDx->DMIN, pstValueConfig->f32KdMin);
        PID_FLOAT_WRITE_REG32(PIDx->UMAX, pstValueConfig->f32UMax);
        PID_FLOAT_WRITE_REG32(PIDx->UMIN, pstValueConfig->f32UMin);
        PID_FLOAT_WRITE_REG32(PIDx->U1MAX, pstValueConfig->f32U1Max);
        PID_FLOAT_WRITE_REG32(PIDx->U1MIN, pstValueConfig->f32U1Min);
        PID_FLOAT_WRITE_REG32(PIDx->U3MAX, pstValueConfig->f32U3Max);
        PID_FLOAT_WRITE_REG32(PIDx->U3MIN, pstValueConfig->f32U3Min);
        PID_FLOAT_WRITE_REG32(PIDx->U5MAX, pstValueConfig->f32U5Max);
        PID_FLOAT_WRITE_REG32(PIDx->U5MIN, pstValueConfig->f32U5Min);
    }

    return i32Ret;
}

/**
 * @brief  Set PID expected value.
 * @param  [in] PIDx                    PID unit
 * @param  [in] f32Ev                    Expected value
 * @retval None
 */
void PID_SetExpectedValue(CM_PID_TypeDef *PIDx, float32_t f32Ev)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));

    PID_FLOAT_WRITE_REG32(PIDx->REF, f32Ev);
}

/**
 * @brief  Set PID actual value.
 * @param  [in] PIDx                    PID unit
 * @param  [in] f32Av                    Actual value
 * @retval None
 */
void PID_SetActualValue(CM_PID_TypeDef *PIDx, float32_t f32Av)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));

    PID_FLOAT_WRITE_REG32(PIDx->AVAL, f32Av);
}

/**
 * @brief  Read PID control output value.
 * @param  [in] PIDx                    PID unit
 * @retval float32_t: PID control output value
 */
float32_t PID_GetUValue(CM_PID_TypeDef *PIDx)
{
    float32_t f32U;

    DDL_ASSERT(IS_PID_UNIT(PIDx));

    f32U = PID_FLOAT_READ_REG32(PIDx->U);

    return f32U;
}

/**
 * @brief  Read PID final value.
 * @param  [in] PIDx                    PID unit
 * @retval float32_t: PID final value
 */
float32_t PID_GetFinalValue(CM_PID_TypeDef *PIDx)
{
    float32_t f32Final;

    DDL_ASSERT(IS_PID_UNIT(PIDx));

    f32Final = PID_FLOAT_READ_REG32(PIDx->U4);

    return f32Final;
}

/**
 * @brief  Read PID U5 value.
 * @param  [in] PIDx                    PID unit
 * @retval float32_t: PID U5 value
 */
float32_t PID_GetU5Value(CM_PID_TypeDef *PIDx)
{
    float32_t f32U5;

    DDL_ASSERT(IS_PID_UNIT(PIDx));

    f32U5 = PID_FLOAT_READ_REG32(PIDx->U5);

    return f32U5;
}
/**
 * @brief  Init PID UxPx structure with default value.
 * @param  [in] pstcUxPx                Specifies the parameter of PID UxPx structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_UxPxStructInit(stc_pid_uxpx_t *pstcUxPx)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcUxPx) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcUxPx->f32U1P1 = 0.0f;
        pstcUxPx->f32U3P1 = 0.0f;
        pstcUxPx->f32U3P3 = 0.0f;
        pstcUxPx->f32U4P1 = 0.0f;
        pstcUxPx->f32U5P1 = 0.0f;
        pstcUxPx->f32U5P2 = 0.0f;
    }

    return i32Ret;
}

/**
 * @brief  Config PID UxPx value if need
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstcUxPx                Specifies the PID UxPx value
 * @retval int32_t:
 *         - LL_OK: config success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_UxPxConfig(CM_PID_TypeDef *PIDx, stc_pid_uxpx_t *pstcUxPx)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcUxPx) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));

        PID_FLOAT_WRITE_REG32(PIDx->U1P1, pstcUxPx->f32U1P1);
        PID_FLOAT_WRITE_REG32(PIDx->U3P1, pstcUxPx->f32U3P1);
        PID_FLOAT_WRITE_REG32(PIDx->U3P3, pstcUxPx->f32U3P3);
        PID_FLOAT_WRITE_REG32(PIDx->U4P1, pstcUxPx->f32U4P1);
        PID_FLOAT_WRITE_REG32(PIDx->U5P1, pstcUxPx->f32U5P1);
        PID_FLOAT_WRITE_REG32(PIDx->U5P2, pstcUxPx->f32U5P2);
    }

    return i32Ret;
}

/**
 * @brief  Read PID UxPx value.
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstcUxPx                Specifies the parameter of PID UxPx structure.
 * @retval int32_t:
 *         - LL_OK: Read success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_ReadUxPx(CM_PID_TypeDef *PIDx, stc_pid_uxpx_t *pstcUxPx)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcUxPx) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));

        pstcUxPx->f32U1P1 = PID_FLOAT_READ_REG32(PIDx->U1P1);
        pstcUxPx->f32U1P2 = PID_FLOAT_READ_REG32(PIDx->U1P2);
        pstcUxPx->f32U3P1 = PID_FLOAT_READ_REG32(PIDx->U3P1);
        pstcUxPx->f32U3P2 = PID_FLOAT_READ_REG32(PIDx->U3P2);
        pstcUxPx->f32U3P3 = PID_FLOAT_READ_REG32(PIDx->U3P3);
        pstcUxPx->f32U3P4 = PID_FLOAT_READ_REG32(PIDx->U3P4);
        pstcUxPx->f32U4P1 = PID_FLOAT_READ_REG32(PIDx->U4P1);
        pstcUxPx->f32U5P1 = PID_FLOAT_READ_REG32(PIDx->U5P1);
        pstcUxPx->f32U5P2 = PID_FLOAT_READ_REG32(PIDx->U5P2);
    }

    return i32Ret;
}

/**
 * @brief  Read PID value while calculating
 * @param  [in] PIDx                    PID unit
 * @param  [in] pstcValue               Specifies the PID value
 * @retval int32_t:
 *         - LL_OK: Read success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_ReadValue(CM_PID_TypeDef *PIDx, stc_pid_value_t *pstcValue)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcValue) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_UNIT(PIDx));

        pstcValue->f32Ek   = PID_FLOAT_READ_REG32(PIDx->ERR);
        pstcValue->f32PVal = PID_FLOAT_READ_REG32(PIDx->PVAL);
        pstcValue->f32IVal = PID_FLOAT_READ_REG32(PIDx->IVAL);
        pstcValue->f32DVal = PID_FLOAT_READ_REG32(PIDx->DVAL);
        pstcValue->f32U    = PID_FLOAT_READ_REG32(PIDx->U);
        pstcValue->f32U1   = PID_FLOAT_READ_REG32(PIDx->U1);
        pstcValue->f32U2   = PID_FLOAT_READ_REG32(PIDx->U2);
        pstcValue->f32U3   = PID_FLOAT_READ_REG32(PIDx->U3);
        pstcValue->f32U4   = PID_FLOAT_READ_REG32(PIDx->U4);
        pstcValue->f32U5   = PID_FLOAT_READ_REG32(PIDx->U5);
        pstcValue->u32Uf   = READ_REG32(PIDx->UF);
        pstcValue->u32U5f  = READ_REG32(PIDx->U5F);
    }

    return i32Ret;
}

/**
 * @brief  Get PID status.
 * @param  [in] PIDx                    PID unit
 * @param  [in] u32Flag                 Specifies the flag to be read. @ref PID_Flag_Sel
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t PID_GetStatus(CM_PID_TypeDef *PIDx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));
    DDL_ASSERT(IS_PID_FLAG(u32Flag));

    return ((0x00U != READ_REG32_BIT(PIDx->STA, u32Flag)) ? SET : RESET);
}

/**
 * @brief  Clear PID status.
 * @param  [in] PIDx                    PID unit
 * @param  [in] u32Flag                 Specifies the flag to be read. @ref PID_Flag_Sel
 * @retval None.
 */
void PID_ClearStatus(CM_PID_TypeDef *PIDx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_PID_UNIT(PIDx));
    DDL_ASSERT(IS_PID_FLAG(u32Flag));

    WRITE_REG32(PIDx->STA, u32Flag);
}

/**
 * @brief  Init PID compete initial structure with default value.
 * @param  [in] pstcCmpInit             Specifies the parameter of PID compete initial structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_CMP_StructInit(stc_pid_cmp_init_t *pstcCmpInit)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcCmpInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcCmpInit->u32CompEn        = (uint32_t)DISABLE;
        pstcCmpInit->u32CompIntEn     = (uint32_t)DISABLE;
        pstcCmpInit->u32CompOutputSel = PID_COMP_OUTPUT_PID1;
        pstcCmpInit->u32BurstIntEn    = (uint32_t)DISABLE;
        pstcCmpInit->u32BurstFuncSel  = PID_BURST_FUNC_NONE;
        pstcCmpInit->u32BurstMode     = PID_BURST_MD_1;
        pstcCmpInit->u32BurstIntSel   = PID_BURST_INT_DONE_THRESHOLD;
    }

    return i32Ret;
}

/**
 * @brief  Init PID compete.
 * @param  [in] PID_CMP                 PID compete unit
 * @param  [in] pstcCmpInit             Specifies the parameter of PID compete initial structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PID_CMP_Init(CM_PID_CMP_TypeDef *PID_CMP, stc_pid_cmp_init_t *pstcCmpInit)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32Value;

    /* Check if pointer is NULL */
    if (NULL == pstcCmpInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcCmpInit->u32CompEn));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcCmpInit->u32CompIntEn));
        DDL_ASSERT(IS_PID_COMP_OUTPUT_SEL(pstcCmpInit->u32CompOutputSel));
        DDL_ASSERT(IS_PID_FUNCTIONAL_STATE(pstcCmpInit->u32BurstIntEn));
        DDL_ASSERT(IS_PID_BURST_FUNC(pstcCmpInit->u32BurstFuncSel));
        DDL_ASSERT(IS_PID_BURST_MD(pstcCmpInit->u32BurstMode));
        DDL_ASSERT(IS_PID_BURST_INT_SEL(pstcCmpInit->u32BurstIntSel));

        u32Value = (uint32_t)pstcCmpInit->u32CompEn | pstcCmpInit->u32CompOutputSel                       | \
                   ((uint32_t)pstcCmpInit->u32CompIntEn << PID_CMP_CR_CMP_INT_EN_POS)                        | \
                   pstcCmpInit->u32BurstFuncSel | pstcCmpInit->u32BurstMode | pstcCmpInit->u32BurstIntSel | \
                   ((uint32_t)pstcCmpInit->u32BurstIntEn << PID_CMP_CR_BURST_INT_EN_POS);

        WRITE_REG32(PID_CMP->CR, u32Value);
    }

    return i32Ret;
}

/**
 * @brief  Get PID compete status.
 * @param  [in] PID_CMPx                PID compete unit
 * @param  [in] u32Flag                 Specifies the flag to be read. @ref PID_Comp_Flag
 * @return Flag status of the specified flag.
 */
uint32_t PID_CMP_GetStatus(CM_PID_CMP_TypeDef *PID_CMPx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_PID_CMP_UNIT(PID_CMPx));
    DDL_ASSERT(IS_PID_COMP_FLAG(u32Flag));

    return READ_REG32_BIT(PID_CMPx->CSTA, u32Flag);
}

/**
 * @brief  Clear PID compete status.
 * @param  [in] PID_CMPx                PID compete unit
 * @param  [in] u32Flag                 Specifies the flag to be cleared. @ref PID_Comp_Flag
 * @retval None.
 */
void PID_CMP_ClearStatus(CM_PID_CMP_TypeDef *PID_CMPx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_PID_CMP_UNIT(PID_CMPx));
    DDL_ASSERT(IS_PID_COMP_CLR_FLAG(u32Flag));

    WRITE_REG32(PID_CMPx->CSTA, u32Flag);
}

/**
 * @}
 */

#endif /* LL_PID_ENABLE */

/**
 * @}
 */

/**
 * @}
 */
/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
