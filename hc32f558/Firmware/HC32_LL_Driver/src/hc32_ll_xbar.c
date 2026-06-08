/**
 *******************************************************************************
 * @file  hc32_ll_xbar.c
 * @brief This file provides firmware functions to manage the Cross Select
 *        Module Unit(XBAR).
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
#include "hc32_ll_xbar.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_XBAR XBAR
 * @brief Crossbar Selector Driver Library
 * @{
 */

#if (LL_XBAR_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup XBAR_Local_Macros XBAR Local Macros
 * @{
 */
#define XBAR_RMU_TIMEOUT                    (100UL)

#define XBAR_REG(ch_base, ch)               ((__IO uint32_t *)((uint32_t)(&(ch_base)) + ((ch) * 0x04UL)))
#define XBAR_HECR(x, ch)                    XBAR_REG((x)->HECR1, (ch))
#define XBAR_MICR(x, ch)                    XBAR_REG((x)->MICR1, (ch))
#define XBAR_HEBCR(x, ch)                   XBAR_REG((x)->HEBCR1, (ch))
#define XBAR_TECR(x, ch)                    XBAR_REG((x)->TECR1, (ch))
#define XBAR_TRLCR(x, ch)                   XBAR_REG((x)->TRLCR1, (ch))

#define XBAR_FLD2VAL(field, pos)            ((uint32_t)(field) >> (pos))

#define XBAR_HRPWM_EVT1_MAX                 XBAR_FLD2VAL(XBAR_HRPWM_EVT1_PG9,  XBAR_HECR_SEL1_POS)
#define XBAR_HRPWM_EVT2_MAX                 XBAR_FLD2VAL(XBAR_HRPWM_EVT2_CMP8, XBAR_HECR_SEL2_POS)
#define XBAR_HRPWM_EVT3_MAX                 XBAR_FLD2VAL(XBAR_HRPWM_EVT3_DSOGI_PLL_OUT2, XBAR_HECR_SEL3_POS)
#define XBAR_HRPWM_MINDB_MAX                XBAR_FLD2VAL(XBAR_HRPWM_MINDB_PLA_OUT15, XBAR_MICR_MDSEL_POS)
#define XBAR_HRPWM_ICL_MAX                  XBAR_FLD2VAL(XBAR_HRPWM_ICL_PLA_OUT15, XBAR_MICR_ICLSEL_POS)
#define XBAR_HRPWM_EMB_EVT1_MAX             XBAR_FLD2VAL(XBAR_HRPWM_EMB_EVT1_PG9, XBAR_HEBCR_SEL1_POS)
#define XBAR_HRPWM_EMB_EVT2_MAX             XBAR_FLD2VAL(XBAR_HRPWM_EMB_EVT2_CMP8, XBAR_HEBCR_SEL2_POS)
#define XBAR_HRPWM_EMB_EVT3_MAX             XBAR_FLD2VAL(XBAR_HRPWM_EMB_EVT3_DSOGI_PLL_FAILURE, XBAR_HEBCR_SEL3_POS)
#define XBAR_TMR6_EMB_EVT_MAX               XBAR_FLD2VAL(XBAR_TMR6_EMB_SDFM_ZCD3, XBAR_TECR_TESEL_POS)
#define XBAR_TRLPWM_EMB_EVT_MAX             XBAR_FLD2VAL(XBAR_TRLPWM_HRPWM_EMB7, XBAR_TRLCR_TRLSEL_POS)

/**
 * @defgroup XBAR_Check_Parameters_Validity XBAR Check Parameters Validity
 * @{
 */
#define IS_XBAR_UNIT(x)                     ((x) == CM_XBAR)

#define IS_XBAR_HRPWM_CH(x)                 ((x) <= XBAR_HRPWM_CH10)
#define IS_XBAR_HRPWM_MI_CH(x)              ((x) <= XBAR_HRPWM_MI_CH16)
#define IS_XBAR_HRPWM_EMB_CH(x)             ((x) <= XBAR_HRPWM_EMB_CH8)
#define IS_XBAR_TMR6_EMB_CH(x)              ((x) <= XBAR_TMR6_EMB_CH9)
#define IS_XBAR_TRLPWM_CH(x)                ((x) <= XBAR_TRLPWM_CH2)

#define IS_XBAR_HRPWM_EVT1(x)               ((XBAR_FLD2VAL(x, XBAR_HECR_SEL1_POS)) <= XBAR_HRPWM_EVT1_MAX)
#define IS_XBAR_HRPWM_EVT2(x)               ((XBAR_FLD2VAL(x, XBAR_HECR_SEL2_POS)) <= XBAR_HRPWM_EVT2_MAX)
#define IS_XBAR_HRPWM_EVT3(x)               ((XBAR_FLD2VAL(x, XBAR_HECR_SEL3_POS)) <= XBAR_HRPWM_EVT3_MAX)
#define IS_XBAR_HRPWM_MINDB_EVT(x)          ((XBAR_FLD2VAL(x, XBAR_MICR_MDSEL_POS)) <= XBAR_HRPWM_MINDB_MAX)
#define IS_XBAR_HRPWM_ICL_EVT(x)            ((XBAR_FLD2VAL(x, XBAR_MICR_ICLSEL_POS)) <= XBAR_HRPWM_ICL_MAX)
#define IS_XBAR_HRPWM_EMB_EVT1(x)           ((XBAR_FLD2VAL(x, XBAR_HEBCR_SEL1_POS)) <= XBAR_HRPWM_EMB_EVT1_MAX)
#define IS_XBAR_HRPWM_EMB_EVT2(x)           ((XBAR_FLD2VAL(x, XBAR_HEBCR_SEL2_POS)) <= XBAR_HRPWM_EMB_EVT2_MAX)
#define IS_XBAR_HRPWM_EMB_EVT3(x)           ((XBAR_FLD2VAL(x, XBAR_HEBCR_SEL3_POS)) <= XBAR_HRPWM_EMB_EVT3_MAX)
#define IS_XBAR_TMR6_EMB_EVT(x)             ((XBAR_FLD2VAL(x, XBAR_TECR_TESEL_POS)) <= XBAR_TMR6_EMB_EVT_MAX)
#define IS_XBAR_TRLPWM_EVT(x)               ((XBAR_FLD2VAL(x, XBAR_TRLCR_TRLSEL_POS)) <= XBAR_TRLPWM_EMB_EVT_MAX)
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
 * @defgroup XBAR_Global_Functions XBAR Global Functions
 * @{
 */

/**
 * @brief  De-Initialize XBAR function
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @retval int32_t:
 *         - LL_OK:           Reset successfully
 *         - LL_ERR_TIMEOUT:  Reset time out
 * @note   Call LL_PERIPH_WE(LL_PERIPH_PWC_CLK_RMU) unlock RMU_FRSTx register first
 */
int32_t XBAR_DeInit(CM_XBAR_TypeDef *XBARx)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeOut = 0UL;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1);

    (void)XBARx;
    CLR_REG32(bCM_RMU->FRST2_b.XBAR);
    /* Ensure reset procedure is completed */
    while (1UL != READ_REG32(bCM_RMU->FRST2_b.XBAR)) {
        u32TimeOut++;
        if (u32TimeOut > XBAR_RMU_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_xbar_hrpwm_ext_event_init_t to default values
 * @param  [out] pstcXbarHrpwmExtEventInit      Pointer to a @ref stc_xbar_hrpwm_ext_event_init_t structure
 * @retval int32_t:
 *           - LL_OK: Initialize successfully.
 *           - LL_ERR_INVD_PARAM: The pointer pstcXbarHrpwmExtEventInit value is NULL.
 */
int32_t XBAR_HRPWM_ExtEventStructInit(stc_xbar_hrpwm_ext_event_init_t *pstcXbarHrpwmExtEventInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcXbarHrpwmExtEventInit) {
        pstcXbarHrpwmExtEventInit->u32Event1 = XBAR_HRPWM_EVT1_PA12;
        pstcXbarHrpwmExtEventInit->u32Event2 = XBAR_HRPWM_EVT2_CMP1;
        pstcXbarHrpwmExtEventInit->u32Event3 = XBAR_HRPWM_EVT3_SDFM_BK0;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief Initialize the XBAR HRPWM
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_Channel_Index
 * @param [in] pstcXbarHrpwmExtEventInit    Pointer to a @ref stc_xbar_hrpwm_ext_event_init_t structure
 * @retval int32_t:
 *           - LL_OK: Initialize successfully.
 *           - LL_ERR_INVD_PARAM: The pointer pstcXbarHrpwmExtEventInit value is NULL.
 */
int32_t XBAR_HRPWM_ExtEventInit(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch,
                                const stc_xbar_hrpwm_ext_event_init_t *pstcXbarHrpwmExtEventInit)
{
    __IO uint32_t *HECR;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcXbarHrpwmExtEventInit) {
        DDL_ASSERT(IS_XBAR_UNIT(XBARx));
        DDL_ASSERT(IS_XBAR_HRPWM_CH(u32Ch));
        DDL_ASSERT(IS_XBAR_HRPWM_EVT1(pstcXbarHrpwmExtEventInit->u32Event1));
        DDL_ASSERT(IS_XBAR_HRPWM_EVT2(pstcXbarHrpwmExtEventInit->u32Event2));
        DDL_ASSERT(IS_XBAR_HRPWM_EVT3(pstcXbarHrpwmExtEventInit->u32Event3));

        HECR = XBAR_HECR(XBARx, u32Ch);
        WRITE_REG32(*HECR, (pstcXbarHrpwmExtEventInit->u32Event1 | \
                            pstcXbarHrpwmExtEventInit->u32Event2 | \
                            pstcXbarHrpwmExtEventInit->u32Event3));
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief XBAR Hrpwm External Event1 Select
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_Channel_Index
 * @param [in] u32Event1        Specifies the event1 @ref XBAR_HRPWM_EVT1_Event_Selection
 * @retval None
 */
void XBAR_HRPWM_SetEvent1(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event1)
{
    __IO uint32_t *HECR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_HRPWM_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_HRPWM_EVT1(u32Event1));

    HECR = XBAR_HECR(XBARx, u32Ch);
    MODIFY_REG32(*HECR, XBAR_HECR_SEL1, u32Event1);
}

/**
 * @brief XBAR Hrpwm External Event2 Select
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_Channel_Index
 * @param [in] u32Event2        Specifies the event2 @ref XBAR_HRPWM_EVT2_Event_Selection
 * @retval None
 */
void XBAR_HRPWM_SetEvent2(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event2)
{
    __IO uint32_t *HECR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_HRPWM_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_HRPWM_EVT2(u32Event2));

    HECR = XBAR_HECR(XBARx, u32Ch);
    MODIFY_REG32(*HECR, XBAR_HECR_SEL2, u32Event2);
}

/**
 * @brief XBAR Hrpwm External Event3 Select
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_Channel_Index
 * @param [in] u32Event3        Specifies the event3 @ref XBAR_HRPWM_EVT3_Event_Selection
 * @retval None
 */
void XBAR_HRPWM_SetEvent3(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event3)
{
    __IO uint32_t *HECR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_HRPWM_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_HRPWM_EVT3(u32Event3));

    HECR = XBAR_HECR(XBARx, u32Ch);
    MODIFY_REG32(*HECR, XBAR_HECR_SEL3, u32Event3);
}

/**
 * @brief  Set the fields of structure stc_xbar_hrpwm_mdb_icl_init_t to default values
 * @param  [out] pstcXbarHrpwmMdbIclInit    Pointer to a @ref stc_xbar_hrpwm_mdb_icl_init_t structure
 * @retval int32_t:
 *           - LL_OK: Initialize successfully.
 *           - LL_ERR_INVD_PARAM: The pointer pstcXbarHrpwmMdbIclInit value is NULL.
 */
int32_t XBAR_HRPWM_MdbIclStructInit(stc_xbar_hrpwm_mdb_icl_init_t *pstcXbarHrpwmMdbIclInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcXbarHrpwmMdbIclInit) {
        pstcXbarHrpwmMdbIclInit->u32MinDBEvent = XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP1;
        pstcXbarHrpwmMdbIclInit->u32ICLEvent = XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP1;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Initialize the XBAR HRPWM min dead time event and illegal input event
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param  [in] u32Ch           Specifies the channel index @ref XBAR_HRPWM_MinDBIcl_Channel_Index
 * @param  [in] pstcXbarHrpwmMdbIclInit     Pointer to a @ref stc_xbar_hrpwm_mdb_icl_init_t structure
 * @retval int32_t:
 *           - LL_OK: Initialize successfully.
 *           - LL_ERR_INVD_PARAM: The pointer pstcXbarHrpwmMdbIclInit value is NULL.
 */
int32_t XBAR_HRPWM_MdbIclInit(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch,
                              const stc_xbar_hrpwm_mdb_icl_init_t *pstcXbarHrpwmMdbIclInit)
{
    __IO uint32_t *MICR;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcXbarHrpwmMdbIclInit) {
        DDL_ASSERT(IS_XBAR_UNIT(XBARx));
        DDL_ASSERT(IS_XBAR_HRPWM_MI_CH(u32Ch));
        DDL_ASSERT(IS_XBAR_HRPWM_MINDB_EVT(pstcXbarHrpwmMdbIclInit->u32MinDBEvent));
        DDL_ASSERT(IS_XBAR_HRPWM_ICL_EVT(pstcXbarHrpwmMdbIclInit->u32ICLEvent));

        MICR = XBAR_MICR(XBARx, u32Ch);
        WRITE_REG32(*MICR, (pstcXbarHrpwmMdbIclInit->u32MinDBEvent | pstcXbarHrpwmMdbIclInit->u32ICLEvent));
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief Set XBAR HRPWM min dead time event select
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_MinDBIcl_Channel_Index
 * @param [in] u32MinDBEvent    Specifies the minimum dead time event @ref XBAR_HRPWM_MINDB_Selection
 * @retval None
 */
void XBAR_HRPWM_SetMinDBEvent(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32MinDBEvent)
{
    __IO uint32_t *MICR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_HRPWM_MI_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_HRPWM_MINDB_EVT(u32MinDBEvent));

    MICR = XBAR_MICR(XBARx, u32Ch);
    MODIFY_REG32(*MICR, XBAR_MICR_MDSEL, u32MinDBEvent);
}

/**
 * @brief Set XBAR HRPWM illegal input event
 * @param [in] XBARx            Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_MinDBIcl_Channel_Index
 * @param [in] u32ICLEvent      Specifies the illegal input event @ref XBAR_HRPWM_Illegal_Input_Event_Selection
 * @retval None
 */
void XBAR_HRPWM_SetIllegalInputEvent(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32ICLEvent)
{
    __IO uint32_t *MICR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_HRPWM_MI_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_HRPWM_ICL_EVT(u32ICLEvent));

    MICR = XBAR_MICR(XBARx, u32Ch);
    MODIFY_REG32(*MICR, XBAR_MICR_ICLSEL, u32ICLEvent);
}

/**
 * @brief  Set the fields of structure stc_xbar_hrpwm_emb_init_t to default values
 * @param  [out] pstcXbarHrpwmEmbInit   Pointer to a @ref stc_xbar_hrpwm_emb_init_t structure
 * @retval int32_t:
 *           - LL_OK: Initialize successfully.
 *           - LL_ERR_INVD_PARAM: The pointer pstcXbarHrpwmEmbInit value is NULL.
 */
int32_t XBAR_HRPWMEMB_StructInit(stc_xbar_hrpwm_emb_init_t *pstcXbarHrpwmEmbInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcXbarHrpwmEmbInit) {
        pstcXbarHrpwmEmbInit->u32Event1 = XBAR_HRPWM_EMB_EVT1_PA12;
        pstcXbarHrpwmEmbInit->u32Event2 = XBAR_HRPWM_EMB_EVT2_CMP1;
        pstcXbarHrpwmEmbInit->u32Event3 = XBAR_HRPWM_EMB_EVT3_SDFM_BK0;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Initialize the XBAR HRPWMEMB
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_EMB_Channel_Index
 * @param  [in] pstcXbarHrpwmEmbInit    Pointer to a @ref stc_xbar_hrpwm_emb_init_t structure
 * @retval int32_t:
 *           - LL_OK: Initialize successfully.
 *           - LL_ERR_INVD_PARAM: The pointer pstcXbarHrpwmEmbInit value is NULL.
 */
int32_t XBAR_HRPWMEMB_Init(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch,
                           const stc_xbar_hrpwm_emb_init_t *pstcXbarHrpwmEmbInit)
{
    __IO uint32_t *HEBCR;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcXbarHrpwmEmbInit) {
        DDL_ASSERT(IS_XBAR_UNIT(XBARx));
        DDL_ASSERT(IS_XBAR_HRPWM_EMB_CH(u32Ch));
        DDL_ASSERT(IS_XBAR_HRPWM_EMB_EVT1(pstcXbarHrpwmEmbInit->u32Event1));
        DDL_ASSERT(IS_XBAR_HRPWM_EMB_EVT2(pstcXbarHrpwmEmbInit->u32Event2));
        DDL_ASSERT(IS_XBAR_HRPWM_EMB_EVT3(pstcXbarHrpwmEmbInit->u32Event3));

        HEBCR = XBAR_HEBCR(XBARx, u32Ch);
        WRITE_REG32(*HEBCR, (pstcXbarHrpwmEmbInit->u32Event1 | \
                             pstcXbarHrpwmEmbInit->u32Event2 | pstcXbarHrpwmEmbInit->u32Event3));
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief XBAR HRPWMEMB event1 select
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_EMB_Channel_Index
 * @param [in] u32Event1        Specifies the event @ref XBAR_HRPWM_EMB_EVT1_Event_Selection
 * @retval None
 */
void XBAR_HRPWMEMB_SetEvent1(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event1)
{
    __IO uint32_t *HEBCR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_HRPWM_EMB_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_HRPWM_EMB_EVT1(u32Event1));

    HEBCR = XBAR_HEBCR(XBARx, u32Ch);
    MODIFY_REG32(*HEBCR, XBAR_HEBCR_SEL1, u32Event1);
}

/**
 * @brief XBAR HRPWMEMB event2 select
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_EMB_Channel_Index
 * @param [in] u32Event2        Specifies the event @ref XBAR_HRPWM_EMB_EVT2_Selection
 * @retval None
 */
void XBAR_HRPWMEMB_SetEvent2(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event2)
{
    __IO uint32_t *HEBCR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_HRPWM_EMB_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_HRPWM_EMB_EVT2(u32Event2));

    HEBCR = XBAR_HEBCR(XBARx, u32Ch);
    MODIFY_REG32(*HEBCR, XBAR_HEBCR_SEL2, u32Event2);
}

/**
 * @brief XBAR HRPWMEMB event3 select
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param [in] u32Ch            Specifies the channel index @ref XBAR_HRPWM_EMB_Channel_Index
 * @param [in] u32Event3        Specifies the event @ref XBAR_HRPWM_EMB_EVT3_Selection
 * @retval None
 */
void XBAR_HRPWMEMB_SetEvent3(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event3)
{
    __IO uint32_t *HEBCR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_HRPWM_EMB_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_HRPWM_EMB_EVT3(u32Event3));

    HEBCR = XBAR_HEBCR(XBARx, u32Ch);
    MODIFY_REG32(*HEBCR, XBAR_HEBCR_SEL3, u32Event3);
}

/**
 * @brief  Initialize the XBAR TMR6MEMB
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param  [in] u32Ch           Specifies the channel index @ref XBAR_TMR6_EMB_Channel_Index
 * @param  [in] u32Event        Specifies the event @ref XBAR_TMR6_EMB_Event_Selection
 * @retval None
 */
void XBAR_TMR6EMB_SetEvent(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event)
{
    __IO uint32_t *TECR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_TMR6_EMB_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_TMR6_EMB_EVT(u32Event));

    TECR = XBAR_TECR(XBARx, u32Ch);
    WRITE_REG32(*TECR, u32Event);
}

/**
 * @brief  Initialize the XBAR TRLPWM
 * @param  [in] XBARx           Pointer to XBARx instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_XBAR:      XBAR instance register base
 * @param  [in] u32Ch           Specifies the channel index @ref XBAR_TRLPWM_Channel_Index
 * @param  [in] u32Event        Specifies the channel index @ref XBAR_TRLPWM_Event_Selection
 * @retval None
 */
void XBAR_TRLPWM_SetEvent(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event)
{
    __IO uint32_t *TRLCR;

    DDL_ASSERT(IS_XBAR_UNIT(XBARx));
    DDL_ASSERT(IS_XBAR_TRLPWM_CH(u32Ch));
    DDL_ASSERT(IS_XBAR_TRLPWM_EVT(u32Event));

    TRLCR = XBAR_TRLCR(XBARx, u32Ch);
    WRITE_REG32(*TRLCR, u32Event);
}

/**
 * @}
 */

#endif /* LL_XBAR_ENABLE */

/**
 * @}
 */

/**
* @}
*/

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
