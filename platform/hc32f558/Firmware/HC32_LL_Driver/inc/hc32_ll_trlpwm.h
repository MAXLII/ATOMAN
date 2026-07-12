/**
 *******************************************************************************
 * @file  hc32_ll_trlpwm.h
 * @brief This file contains all the functions prototypes of the trlpwm driver
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
#ifndef __HC32_LL_TRLPWM_H__
#define __HC32_LL_TRLPWM_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"
#include "hc32_ll_utility.h"

#include "hc32f5xx.h"
#include "hc32f5xx_conf.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup LL_TRLPWM
 * @{
 */
#if (LL_TRLPWM_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup TRLPWM_Global_Types TRLPWM Global Types
 * @{
 */

/**
 * @brief en_level_state_t level state definition
 * @{
 */
typedef enum {
    HIGH = 0U,
    LOW = 1U
} en_level_state_t;
/**
 * @}
 */

/**
 * @brief stc_trlpwm_base_t TRLPWM base function structure definition
 * @{
 */
typedef struct {
    en_functional_state_t enCBCEn;              /*!< Cycle-By-Cycle Current Limiting Enable */
    en_functional_state_t enResumeAlignEn;      /*!< Resume Align Wave */
    en_level_state_t enOutPutPolarity;          /*!< Output polarity level */
    en_level_state_t enBrakePolarity;           /*!< Input brake polarity active level */
    uint16_t u16CBCDelayDiv;                    /*!< Cycle-By-Cycle Current Limiting Delay Clock Division */
    uint16_t u16OpenShutDownDelayDiv;           /*!< Open/Shutdown Delay Clock Division */
} stc_trlpwm_base_t;
/**
 * @}
 */

/**
 * @brief stc_trlpwm_leg_t TRLPWM Leg structure definition
 * @{
 */
typedef struct {
    en_functional_state_t enOutputBypassEn;     /*!< Output bypass Enable */
    en_functional_state_t enInputInvEn;         /*!< Input inversion Enable */
    en_functional_state_t enANPCEn;             /*!< ANPC Enable */
    uint16_t u16FilterDiv;                      /*!< Filter Clock Division @ref TRLPWM_Filter_Div */
    uint16_t u16FilterN;                        /*!< Filter width */
    uint16_t u16ShutDownDelay;                  /*!< Shutdown Delay */
    uint16_t u16OpenDelay;                      /*!< Open Delay */
    uint16_t u16CBCDelay;                       /*!< Cycle-By-Cycle Current Limiting Delay */
    uint16_t u16AlignDelay;                     /*!< Start or Resume Align Delay */
} stc_trlpwm_leg_t;
/**
 * @}
 */
/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup TRLPWM_Global_Macros TRLPWM Global Macros
 * @{
 */

/**
 * @defgroup TRLPWM_BrakeSR_Flag TRLPWM Brake Status Flag
 * @{
 */
#define TRLPWM_BRAKE_FLAG                        (TRLPWM_BRAKESR_BRAKE)           /*!< Brake event flag */
/**
 * @}
 */

/**
 * @defgroup TRLPWM_Int_Flag TRLPWM Interrupt Flag
 * @{
 */
#define TRLPWM_SHUTDOWN_FLAG                     (TRLPWM_ISR_SHUTDOWNSEQOCCUR)    /*!< Shutdown interrupt flag */
/**
 * @}
 */

/**
 * @defgroup TRLPWM_Clr_Flag TRLPWM Interrupt Flag Clear
 * @{
 */
#define TRLPWM_SHUTDOWN_CLR                      (TRLPWM_ICR_SHUTDOWNSEQOCCURCLR)  /*!< Shutdown interrupt flag clear */
/**
 * @}
 */

/**
 * @defgroup TRLPWM_Int_Type TRLPWM Interrupt type
 * @{
 */
#define TRLPWM_SHUTDOWN_INT                      (TRLPWM_IER_SHUTDOWNSEQOCCURIE)  /*!< Shutdown interrupt */
/**
 * @}
 */

/**
 * @defgroup TRLPWM_Leg_Unit TRLPWM Leg Unit
 * @{
 */
#define TRLPWM_LEG0                              (0x01UL)  /*!< Unit 0 */
#define TRLPWM_LEG1                              (0x02UL)  /*!< Unit 1 */
#define TRLPWM_LEG2                              (0x04UL)  /*!< Unit 2 */
#define TRLPWM_LEG_ALL                           (TRLPWM_LEG0 | TRLPWM_LEG1 | TRLPWM_LEG2)
/**
 * @}
 */

/**
 * @defgroup TRLPWM_LegStatus_Flag TRLPWM Leg Status Flag
 * @{
 */
#define TRLPWM_STATUS_WORKNOW                   (TRLPWM_SRLEG_STATUS)  /*!< Leg Current working status */
/**
 * @}
 */

/**
 * @defgroup TRLPWM_Leg_WorkNow TRLPWM Leg Current working status
 * @{
 */
#define TRLPWM_LEG_WORK_NORMAL                   (0U)  /*!< Leg Current working normal */
#define TRLPWM_LEG_WORK_BRAKEPROCESS             (1U)  /*!< Leg Current working brake process */
#define TRLPWM_LEG_WORK_BRAKING                  (2U)  /*!< Leg Current working braking */
#define TRLPWM_LEG_WORK_OPENPROCESS              (3U)  /*!< Leg Current working open process */
/**
 * @}
 */

/**
 * @defgroup TRLPWM_Filter_Div TRLPWM Filter Clock Division
 * @{
 */
#define TRLPWM_FILTER_DIV1                       (0U)  /*!< 1 Division */
#define TRLPWM_FILTER_DIV2                       (1U)  /*!< 2 Division */
#define TRLPWM_FILTER_DIV4                       (2U)  /*!< 4 Division */
#define TRLPWM_FILTER_DIV8                       (3U)  /*!< 8 Division */
/**
 * @}
 */

/**
 * @defgroup TRLPWM_Check_Parameters_Validity TRLPWM Check Parameters Validity
 * @{
 */

#define IS_TRLPWM_UNIT(unit)               (((unit) == CM_TRLPWM1) || ((unit) == CM_TRLPWM2))

#define IS_LEVEL_STATE(x)                  (((x) == HIGH) || ((x) == LOW))

#define IS_CBCDLY_DIV(x)                   ((x) <= 0xFFU)

#define IS_OPENSHUTDOWNDLY_DIV(x)          ((x) <= 0x1FU)

#define IS_FILTER_DIV(x)                   ((x) <= TRLPWM_FILTER_DIV8)

#define IS_FILTER_N(x)                     ((x) <= 0x3FU)

#define IS_CBC_DELAY(x)                    ((x) <= 0x1FFUL)

#define IS_OPEN_DELAY(x)                   ((x) <= 0xFFU)

#define IS_SHUTDOWN_DELAY(x)               ((x) <= 0xFFU)

#define IS_ALIGN_DELAY(x)                  ((x) <= 0x3FU)

#define IS_BRAKESR_FLAG(x)                 ((x) == TRLPWM_BRAKE_FLAG)

#define IS_INT_FLAG(x)                     ((x) == TRLPWM_SHUTDOWN_FLAG)

#define IS_CLR_FLAG(x)                     ((x) == TRLPWM_SHUTDOWN_CLR)

#define IS_TRLPWM_INT(x)                   ((x) == TRLPWM_SHUTDOWN_INT)

#define IS_TRLPWM_LEG_UNIT(x)              (((x) != 0U) && (((x) | TRLPWM_LEG_ALL) == TRLPWM_LEG_ALL))

#define IS_WORKNOW_FLGA(x)                 ((x) == TRLPWM_STATUS_WORKNOW)

#define LEGxT_VALID(x)                     (((x) & 0x07U) <= 4U)

#define IS_MUX_ROUTE_MASK(x)               ((((x) & 0xFFFFFFUL) == (x)) && \
                                            LEGxT_VALID((x) >> TRLPWM_MISCCTRL_LEGT1IN_SEL_POS) && \
                                            LEGxT_VALID((x) >> TRLPWM_MISCCTRL_LEGT2IN_SEL_POS) && \
                                            LEGxT_VALID((x) >> TRLPWM_MISCCTRL_LEGT3IN_SEL_POS) && \
                                            LEGxT_VALID((x) >> TRLPWM_MISCCTRL_LEGT4IN_SEL_POS) && \
                                            LEGxT_VALID((x) >> TRLPWM_MISCCTRL_LEGT5IN_SEL_POS) && \
                                            LEGxT_VALID((x) >> TRLPWM_MISCCTRL_LEGT6IN_SEL_POS))

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
 * @addtogroup TRLPWM_Global_Functions
 * @{
 */

/**
 * @brief TRLPWM function enable or disable
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] enNewState           An @ref en_functional_state_t enumeration value.
 * @retval None
 */
__STATIC_INLINE void TRLPWM_Cmd(CM_TRLPWM_TypeDef *TRLPWMx, en_functional_state_t enNewState)
{
    /* Check parameters */
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(TRLPWMx->CR, TRLPWM_CR_TRLPWMEN, (uint32_t)enNewState << TRLPWM_CR_TRLPWMEN_POS);
}

/**
 * @brief TRLPWM input brake polarity configuration
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] enPolarity           An @ref en_level_state_t enumeration value.
 * @retval None
 */
__STATIC_INLINE void TRLPWM_SetBrakePolarity(CM_TRLPWM_TypeDef *TRLPWMx, en_level_state_t enPolarity)
{
    /* Check parameters */
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_LEVEL_STATE(enPolarity));

    MODIFY_REG32(TRLPWMx->CR, TRLPWM_CR_BRAKEPOL, (uint32_t)enPolarity << TRLPWM_CR_BRAKEPOL_POS);
}

/**
 * @brief TRLPWM output pwm polarity configuration
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] enPolarity           An @ref en_level_state_t enumeration value.
 * @retval None
 */
__STATIC_INLINE void TRLPWM_SetOutputPolarity(CM_TRLPWM_TypeDef *TRLPWMx, en_level_state_t enPolarity)
{
    /* Check parameters */
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_LEVEL_STATE(enPolarity));

    MODIFY_REG32(TRLPWMx->CR, TRLPWM_CR_POLARITY, (uint32_t)enPolarity << TRLPWM_CR_POLARITY_POS);
}

/**
 * @brief TRLPWM CBC configuration
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] enNewState           An @ref en_functional_state_t enumeration value.
 * @retval None
 */
__STATIC_INLINE void TRLPWM_CBCCmd(CM_TRLPWM_TypeDef *TRLPWMx, en_functional_state_t enNewState)
{
    /* Check parameters */
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(TRLPWMx->CR, TRLPWM_CR_CBCEB, (uint32_t)enNewState << TRLPWM_CR_CBCEB_POS);
}

/**
 * @brief TRLPWM Resume Align Wave configuration
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] enNewState           An @ref en_functional_state_t enumeration value.
 * @retval None
 */
__STATIC_INLINE void TRLPWM_ResumeAlignCmd(CM_TRLPWM_TypeDef *TRLPWMx, en_functional_state_t enNewState)
{
    /* Check parameters */
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(TRLPWMx->CR, TRLPWM_CR_RESUMEALIGNEN, (uint32_t)enNewState << TRLPWM_CR_RESUMEALIGNEN_POS);
}

/**
 * @brief TRLPWM interrupt command
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] u32IntType           Specifies the TRLPWM interrupts @ref TRLPWM_Int_Type
 * @param [in] enNewState           An @ref en_functional_state_t enumeration value.
 * @retval None
 */
__STATIC_INLINE void TRLPWM_IntCmd(CM_TRLPWM_TypeDef *TRLPWMx, uint32_t u32IntType, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_TRLPWM_INT(u32IntType));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(TRLPWMx->IER, u32IntType);
    } else {
        CLR_REG32_BIT(TRLPWMx->IER, u32IntType);
    }
}

/**
 * @brief TRLPWM Use the software to open
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] enNewState           An @ref en_functional_state_t enumeration value.
 * @retval None
 */
__STATIC_INLINE void TRLPWM_SWOpenCmd(CM_TRLPWM_TypeDef *TRLPWMx, en_functional_state_t enNewState)
{
    /* Check parameters */
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(TRLPWMx->CR, TRLPWM_CR_SFTOPEN, (uint32_t)enNewState << TRLPWM_CR_SFTOPEN_POS);
}

/**
 * @brief TRLPWM Use the software to shut down
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] enNewState           An @ref en_functional_state_t enumeration value.
 * @retval None
 */
__STATIC_INLINE void TRLPWM_SWShutDownCmd(CM_TRLPWM_TypeDef *TRLPWMx, en_functional_state_t enNewState)
{
    /* Check parameters */
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(TRLPWMx->CR, TRLPWM_CR_SFTSHUTDOWN, (uint32_t)enNewState << TRLPWM_CR_SFTSHUTDOWN_POS);
}

/**
 * @brief Get the TRLPWM status
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] u32Flag              Specify the flags to check, This parameter can be any combination of the member from
 *                                  @ref TRLPWM_BrakeSR_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t TRLPWM_GetStatus(CM_TRLPWM_TypeDef *TRLPWMx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_BRAKESR_FLAG(u32Flag));

    return ((0UL != READ_REG32_BIT(TRLPWMx->BRAKESR, u32Flag)) ? SET : RESET);
}

/**
 * @brief Get the TRLPWM Interrupt status
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] u32Flag              Specify the flags to check, This parameter can be any combination of the member from
 *                                  @ref TRLPWM_Int_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t TRLPWM_GetIntStatus(CM_TRLPWM_TypeDef *TRLPWMx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_INT_FLAG(u32Flag));

    return ((0UL != READ_REG32_BIT(TRLPWMx->ISR, u32Flag)) ? SET : RESET);
}

/**
 * @brief Clear the TRLPWM Interrupt status
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param [in] u32Flag              Specify the flags to check, This parameter can be any combination of the member from
 *                                  @ref TRLPWM_Clr_Flag
 * @retval None
 */
__STATIC_INLINE void TRLPWM_ClearStatus(CM_TRLPWM_TypeDef *TRLPWMx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_CLR_FLAG(u32Flag));

    WRITE_REG32(TRLPWMx->ICR, u32Flag);
}

/* Base Functions API */
int32_t TRLPWM_StructInit(stc_trlpwm_base_t *pstcInit);
int32_t TRLPWM_Init(CM_TRLPWM_TypeDef *TRLPWMx, const stc_trlpwm_base_t *pstcInit);
uint32_t TRLPWM_GetVersion(CM_TRLPWM_TypeDef *TRLPWMx);
int32_t TRLPWM_DeInit(void);

/* LEGx Functions API */
int32_t TRLPWM_LegStructInit(stc_trlpwm_leg_t *pstcInit);
int32_t TRLPWM_LegInit(CM_TRLPWM_TypeDef *TRLPWMx, uint16_t u16LEGx, const stc_trlpwm_leg_t *pstcInit);
void TRLPWM_LegCmd(CM_TRLPWM_TypeDef *TRLPWMx, uint16_t u16LEGx, en_functional_state_t enNewState);
void TRLPWM_LegMuxRouteMask(CM_TRLPWM_TypeDef *TRLPWMx, uint16_t u16LEGx, uint32_t u32Mask);
uint32_t TRLPWM_LegGetStatus(CM_TRLPWM_TypeDef *TRLPWMx, uint16_t u16LEGx, uint32_t u32Flag);

/**
 * @}
 */

#endif /* LL_TRLPWM_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_TRLPWM_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
