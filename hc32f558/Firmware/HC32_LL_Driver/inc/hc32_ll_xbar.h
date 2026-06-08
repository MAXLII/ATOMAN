/**
 *******************************************************************************
 * @file  hc32_ll_xbar.h
 * @brief This file contains all the functions prototypes of the xbar driver
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
#ifndef __HC32_LL_XBAR_H__
#define __HC32_LL_XBAR_H__

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
 * @addtogroup LL_XBAR
 * @{
 */

#if (LL_XBAR_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup XBAR_Global_Types XBAR Global Types
 * @{
 */

/**
 * @brief XBAR HRPWM external event initialization configuration
 */
typedef struct {
    uint32_t u32Event1;             /*!< XBAR HRPWM external event1.
                                         This parameter can be a value of @ref XBAR_HRPWM_EVT1_Event_Selection */
    uint32_t u32Event2;             /*!< XBAR HRPWM external event2.
                                         This parameter can be a value of @ref XBAR_HRPWM_EVT2_Event_Selection */
    uint32_t u32Event3;             /*!< XBAR HRPWM external event3.
                                         This parameter can be a value of @ref XBAR_HRPWM_EVT3_Event_Selection */
} stc_xbar_hrpwm_ext_event_init_t;

/**
 * @brief XBAR HRPWM initialization configuration
 */
typedef struct {
    uint32_t u32MinDBEvent;         /*!< XBAR HRPWM Min dead time event selection.
                                         This parameter can be a value of @ref XBAR_HRPWM_MINDB_Selection */
    uint32_t u32ICLEvent;           /*!< XBAR HRPWM illegal input event selection.
                                         This parameter can be a value of @ref XBAR_HRPWM_Illegal_Input_Event_Selection */
} stc_xbar_hrpwm_mdb_icl_init_t;

/**
 * @brief XBAR HRPWMEMB initialization configuration
 */
typedef struct {
    uint32_t u32Event1;             /*!< XBAR HRPWMEMB external event1.
                                         This parameter can be a value of @ref XBAR_HRPWM_EMB_EVT1_Event_Selection */
    uint32_t u32Event2;             /*!< XBAR HRPWMEMB external event2.
                                         This parameter can be a value of @ref XBAR_HRPWM_EMB_EVT2_Selection */
    uint32_t u32Event3;             /*!< XBAR HRPWMEMB external event3.
                                         This parameter can be a value of @ref XBAR_HRPWM_EMB_EVT3_Selection */
} stc_xbar_hrpwm_emb_init_t;
/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup XBAR_Global_Macros XBAR Global Macros
 * @{
 */

/**
 * @defgroup XBAR_HRPWM_Channel_Index XBAR HRPWM Channel Index
 * @{
 */
#define XBAR_HRPWM_CH1                            (0UL)
#define XBAR_HRPWM_CH2                            (1UL)
#define XBAR_HRPWM_CH3                            (2UL)
#define XBAR_HRPWM_CH4                            (3UL)
#define XBAR_HRPWM_CH5                            (4UL)
#define XBAR_HRPWM_CH6                            (5UL)
#define XBAR_HRPWM_CH7                            (6UL)
#define XBAR_HRPWM_CH8                            (7UL)
#define XBAR_HRPWM_CH9                            (8UL)
#define XBAR_HRPWM_CH10                           (9UL)
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_MinDBIcl_Channel_Index XBAR HRPWM MinDB_ICL Channel Index
 * @{
 */
#define XBAR_HRPWM_MI_CH1                         (0UL)
#define XBAR_HRPWM_MI_CH2                         (1UL)
#define XBAR_HRPWM_MI_CH3                         (2UL)
#define XBAR_HRPWM_MI_CH4                         (3UL)
#define XBAR_HRPWM_MI_CH5                         (4UL)
#define XBAR_HRPWM_MI_CH6                         (5UL)
#define XBAR_HRPWM_MI_CH7                         (6UL)
#define XBAR_HRPWM_MI_CH8                         (7UL)
#define XBAR_HRPWM_MI_CH9                         (8UL)
#define XBAR_HRPWM_MI_CH10                        (9UL)
#define XBAR_HRPWM_MI_CH11                        (10UL)
#define XBAR_HRPWM_MI_CH12                        (11UL)
#define XBAR_HRPWM_MI_CH13                        (12UL)
#define XBAR_HRPWM_MI_CH14                        (13UL)
#define XBAR_HRPWM_MI_CH15                        (14UL)
#define XBAR_HRPWM_MI_CH16                        (15UL)
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_EMB_Channel_Index XBAR HRPWM EMB Channel Index
 * @{
 */
#define XBAR_HRPWM_EMB_CH1                        (0UL)
#define XBAR_HRPWM_EMB_CH2                        (1UL)
#define XBAR_HRPWM_EMB_CH3                        (2UL)
#define XBAR_HRPWM_EMB_CH4                        (3UL)
#define XBAR_HRPWM_EMB_CH5                        (4UL)
#define XBAR_HRPWM_EMB_CH6                        (5UL)
#define XBAR_HRPWM_EMB_CH7                        (6UL)
#define XBAR_HRPWM_EMB_CH8                        (7UL)
/**
 * @}
 */

/**
 * @defgroup XBAR_TMR6_EMB_Channel_Index XBAR TMR6 EMB Channel Index
 * @{
 */
#define XBAR_TMR6_EMB_CH1                         (0UL)
#define XBAR_TMR6_EMB_CH2                         (1UL)
#define XBAR_TMR6_EMB_CH3                         (2UL)
#define XBAR_TMR6_EMB_CH4                         (3UL)
#define XBAR_TMR6_EMB_CH5                         (4UL)
#define XBAR_TMR6_EMB_CH6                         (5UL)
#define XBAR_TMR6_EMB_CH7                         (6UL)
#define XBAR_TMR6_EMB_CH8                         (7UL)
#define XBAR_TMR6_EMB_CH9                         (8UL)
/**
 * @}
 */

/**
 * @defgroup XBAR_TRLPWM_Channel_Index XBAR TRLPWM Channel Index
 * @{
 */
#define XBAR_TRLPWM_CH1                           (0UL)
#define XBAR_TRLPWM_CH2                           (1UL)
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_External_Event XBAR HRPWM External_Event
 * @{
 */
/**
 * @defgroup XBAR_HRPWM_EVT1_Event_Selection XBAR HRPWM EVT1 Event Selection
 * @{
 */
#define XBAR_HRPWM_EVT1_PA12                      (0UL  << XBAR_HECR_SEL1_POS)  /*!< PA12 */
#define XBAR_HRPWM_EVT1_PA13                      (1UL  << XBAR_HECR_SEL1_POS)  /*!< PA13 */
#define XBAR_HRPWM_EVT1_PA14                      (2UL  << XBAR_HECR_SEL1_POS)  /*!< PA14 */
#define XBAR_HRPWM_EVT1_PA15                      (3UL  << XBAR_HECR_SEL1_POS)  /*!< PA15 */
#define XBAR_HRPWM_EVT1_PB3                       (4UL  << XBAR_HECR_SEL1_POS)  /*!< PB3 */
#define XBAR_HRPWM_EVT1_PB5                       (5UL  << XBAR_HECR_SEL1_POS)  /*!< PB5 */
#define XBAR_HRPWM_EVT1_PB6                       (6UL  << XBAR_HECR_SEL1_POS)  /*!< PB6 */
#define XBAR_HRPWM_EVT1_PB7                       (7UL  << XBAR_HECR_SEL1_POS)  /*!< PB7 */
#define XBAR_HRPWM_EVT1_PB8                       (8UL  << XBAR_HECR_SEL1_POS)  /*!< PB8 */
#define XBAR_HRPWM_EVT1_PB9                       (9UL  << XBAR_HECR_SEL1_POS)  /*!< PB9 */
#define XBAR_HRPWM_EVT1_PC10                      (10UL << XBAR_HECR_SEL1_POS)  /*!< PC10 */
#define XBAR_HRPWM_EVT1_PC11                      (11UL << XBAR_HECR_SEL1_POS)  /*!< PC11 */
#define XBAR_HRPWM_EVT1_PC12                      (12UL << XBAR_HECR_SEL1_POS)  /*!< PC12 */
#define XBAR_HRPWM_EVT1_PD0                       (13UL << XBAR_HECR_SEL1_POS)  /*!< PD0 */
#define XBAR_HRPWM_EVT1_PD1                       (14UL << XBAR_HECR_SEL1_POS)  /*!< PD1 */
#define XBAR_HRPWM_EVT1_PD2                       (15UL << XBAR_HECR_SEL1_POS)  /*!< PD2 */
#define XBAR_HRPWM_EVT1_PD3                       (16UL << XBAR_HECR_SEL1_POS)  /*!< PD3 */
#define XBAR_HRPWM_EVT1_PD4                       (17UL << XBAR_HECR_SEL1_POS)  /*!< PD4 */
#define XBAR_HRPWM_EVT1_PD5                       (18UL << XBAR_HECR_SEL1_POS)  /*!< PD5 */
#define XBAR_HRPWM_EVT1_PD6                       (19UL << XBAR_HECR_SEL1_POS)  /*!< PD6 */
#define XBAR_HRPWM_EVT1_PD7                       (20UL << XBAR_HECR_SEL1_POS)  /*!< PD7 */
#define XBAR_HRPWM_EVT1_PE0                       (21UL << XBAR_HECR_SEL1_POS)  /*!< PE0 */
#define XBAR_HRPWM_EVT1_PE1                       (22UL << XBAR_HECR_SEL1_POS)  /*!< PE1 */
#define XBAR_HRPWM_EVT1_PE2                       (23UL << XBAR_HECR_SEL1_POS)  /*!< PE2 */
#define XBAR_HRPWM_EVT1_PE3                       (24UL << XBAR_HECR_SEL1_POS)  /*!< PE3 */
#define XBAR_HRPWM_EVT1_PE4                       (25UL << XBAR_HECR_SEL1_POS)  /*!< PE4 */
#define XBAR_HRPWM_EVT1_PE5                       (26UL << XBAR_HECR_SEL1_POS)  /*!< PE5 */
#define XBAR_HRPWM_EVT1_PE6                       (27UL << XBAR_HECR_SEL1_POS)  /*!< PE6 */
#define XBAR_HRPWM_EVT1_PF2                       (28UL << XBAR_HECR_SEL1_POS)  /*!< PF2  */
#define XBAR_HRPWM_EVT1_PF3                       (29UL << XBAR_HECR_SEL1_POS)  /*!< PF3  */
#define XBAR_HRPWM_EVT1_PF4                       (30UL << XBAR_HECR_SEL1_POS)  /*!< PF4  */
#define XBAR_HRPWM_EVT1_PF5                       (31UL << XBAR_HECR_SEL1_POS)  /*!< PF5  */
#define XBAR_HRPWM_EVT1_PF6                       (32UL << XBAR_HECR_SEL1_POS)  /*!< PF6  */
#define XBAR_HRPWM_EVT1_PF7                       (33UL << XBAR_HECR_SEL1_POS)  /*!< PF7  */
#define XBAR_HRPWM_EVT1_PF8                       (34UL << XBAR_HECR_SEL1_POS)  /*!< PF8  */
#define XBAR_HRPWM_EVT1_PF9                       (35UL << XBAR_HECR_SEL1_POS)  /*!< PF9  */
#define XBAR_HRPWM_EVT1_PF10                      (36UL << XBAR_HECR_SEL1_POS)  /*!< PF10 */
#define XBAR_HRPWM_EVT1_PF11                      (37UL << XBAR_HECR_SEL1_POS)  /*!< PF11 */
#define XBAR_HRPWM_EVT1_PF12                      (38UL << XBAR_HECR_SEL1_POS)  /*!< PF12 */
#define XBAR_HRPWM_EVT1_PF13                      (39UL << XBAR_HECR_SEL1_POS)  /*!< PF13 */
#define XBAR_HRPWM_EVT1_PF14                      (40UL << XBAR_HECR_SEL1_POS)  /*!< PF14 */
#define XBAR_HRPWM_EVT1_PF15                      (41UL << XBAR_HECR_SEL1_POS)  /*!< PF15 */
#define XBAR_HRPWM_EVT1_PG0                       (42UL << XBAR_HECR_SEL1_POS)  /*!< PG0 */
#define XBAR_HRPWM_EVT1_PG1                       (43UL << XBAR_HECR_SEL1_POS)  /*!< PG1 */
#define XBAR_HRPWM_EVT1_PG2                       (44UL << XBAR_HECR_SEL1_POS)  /*!< PG2 */
#define XBAR_HRPWM_EVT1_PG3                       (45UL << XBAR_HECR_SEL1_POS)  /*!< PG3 */
#define XBAR_HRPWM_EVT1_PG4                       (46UL << XBAR_HECR_SEL1_POS)  /*!< PG4 */
#define XBAR_HRPWM_EVT1_PG5                       (47UL << XBAR_HECR_SEL1_POS)  /*!< PG5 */
#define XBAR_HRPWM_EVT1_PG6                       (48UL << XBAR_HECR_SEL1_POS)  /*!< PG6 */
#define XBAR_HRPWM_EVT1_PG7                       (49UL << XBAR_HECR_SEL1_POS)  /*!< PG7 */
#define XBAR_HRPWM_EVT1_PG8                       (50UL << XBAR_HECR_SEL1_POS)  /*!< PG8 */
#define XBAR_HRPWM_EVT1_PG9                       (51UL << XBAR_HECR_SEL1_POS)  /*!< PG9 */
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_EVT2_Event_Selection XBAR HRPWM EVT2 Event_Selection
 * @{
 */
#define XBAR_HRPWM_EVT2_CMP1                      (0UL << XBAR_HECR_SEL2_POS)   /*!< CMP1 */
#define XBAR_HRPWM_EVT2_CMP2                      (1UL << XBAR_HECR_SEL2_POS)   /*!< CMP2 */
#define XBAR_HRPWM_EVT2_CMP3                      (2UL << XBAR_HECR_SEL2_POS)   /*!< CMP3 */
#define XBAR_HRPWM_EVT2_CMP4                      (3UL << XBAR_HECR_SEL2_POS)   /*!< CMP4 */
#define XBAR_HRPWM_EVT2_CMP5                      (4UL << XBAR_HECR_SEL2_POS)   /*!< CMP5 */
#define XBAR_HRPWM_EVT2_CMP6                      (5UL << XBAR_HECR_SEL2_POS)   /*!< CMP6 */
#define XBAR_HRPWM_EVT2_CMP7                      (6UL << XBAR_HECR_SEL2_POS)   /*!< CMP7 */
#define XBAR_HRPWM_EVT2_CMP8                      (7UL << XBAR_HECR_SEL2_POS)   /*!< CMP8 */
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_EVT3_Event_Selection XBAR HRPWM EVT3 Event_Selection
 * @{
 */
#define XBAR_HRPWM_EVT3_SDFM_BK0                  (0UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_BK0 */
#define XBAR_HRPWM_EVT3_SDFM_BK1                  (1UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_BK1 */
#define XBAR_HRPWM_EVT3_SDFM_BK2                  (2UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_BK2 */
#define XBAR_HRPWM_EVT3_SDFM_BK3                  (3UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_BK3 */
#define XBAR_HRPWM_EVT3_SDFM_CEVT1_0              (4UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_CEVT1_0 */
#define XBAR_HRPWM_EVT3_SDFM_CEVT1_1              (5UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_CEVT1_1 */
#define XBAR_HRPWM_EVT3_SDFM_CEVT1_2              (6UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_CEVT1_2 */
#define XBAR_HRPWM_EVT3_SDFM_CEVT1_3              (7UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_CEVT1_3 */
#define XBAR_HRPWM_EVT3_SDFM_CEVT2_0              (8UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_CEVT2_0 */
#define XBAR_HRPWM_EVT3_SDFM_CEVT2_1              (9UL  << XBAR_HECR_SEL3_POS)  /*!< SDFM_CEVT2_1 */
#define XBAR_HRPWM_EVT3_SDFM_CEVT2_2              (10UL << XBAR_HECR_SEL3_POS)  /*!< SDFM_CEVT2_2 */
#define XBAR_HRPWM_EVT3_SDFM_CEVT2_3              (11UL << XBAR_HECR_SEL3_POS)  /*!< SDFM_CEVT2_3 */
#define XBAR_HRPWM_EVT3_SDFM_ZCD0                 (12UL << XBAR_HECR_SEL3_POS)  /*!< SDFM_ZCD0 */
#define XBAR_HRPWM_EVT3_SDFM_ZCD1                 (13UL << XBAR_HECR_SEL3_POS)  /*!< SDFM_ZCD1 */
#define XBAR_HRPWM_EVT3_SDFM_ZCD2                 (14UL << XBAR_HECR_SEL3_POS)  /*!< SDFM_ZCD2 */
#define XBAR_HRPWM_EVT3_SDFM_ZCD3                 (15UL << XBAR_HECR_SEL3_POS)  /*!< SDFM_ZCD3 */
#define XBAR_HRPWM_EVT3_ADC1_AWD0                 (16UL << XBAR_HECR_SEL3_POS)  /*!< ADC1_AWD0 */
#define XBAR_HRPWM_EVT3_ADC1_AWD1                 (17UL << XBAR_HECR_SEL3_POS)  /*!< ADC1_AWD1 */
#define XBAR_HRPWM_EVT3_ADC2_AWD0                 (18UL << XBAR_HECR_SEL3_POS)  /*!< ADC2_AWD0 */
#define XBAR_HRPWM_EVT3_ADC2_AWD1                 (19UL << XBAR_HECR_SEL3_POS)  /*!< ADC2_AWD1 */
#define XBAR_HRPWM_EVT3_ADC3_AWD0                 (20UL << XBAR_HECR_SEL3_POS)  /*!< ADC3_AWD0 */
#define XBAR_HRPWM_EVT3_ADC3_AWD1                 (21UL << XBAR_HECR_SEL3_POS)  /*!< ADC3_AWD1 */
#define XBAR_HRPWM_EVT3_ADC4_AWD0                 (22UL << XBAR_HECR_SEL3_POS)  /*!< ADC4_AWD0 */
#define XBAR_HRPWM_EVT3_ADC4_AWD1                 (23UL << XBAR_HECR_SEL3_POS)  /*!< ADC4_AWD1 */
#define XBAR_HRPWM_EVT3_TMR6_1_CMP_A              (24UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: GCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_2_CMP_A              (25UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: GCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_3_CMP_A              (26UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: GCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_4_CMP_A              (27UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: GCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_5_CMP_A              (28UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: GCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_6_CMP_A              (29UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: GCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_1_CMP_B              (30UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: GCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_2_CMP_B              (31UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: GCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_3_CMP_B              (32UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: GCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_4_CMP_B              (33UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: GCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_5_CMP_B              (34UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: GCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_6_CMP_B              (35UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: GCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_1_CMP_C              (36UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: GCMCR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_2_CMP_C              (37UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: GCMCR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_3_CMP_C              (38UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: GCMCR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_4_CMP_C              (39UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: GCMCR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_5_CMP_C              (40UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: GCMCR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_6_CMP_C              (41UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: GCMCR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_1_CMP_D              (42UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: GCMDR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_2_CMP_D              (43UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: GCMDR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_3_CMP_D              (44UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: GCMDR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_4_CMP_D              (45UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: GCMDR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_5_CMP_D              (46UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: GCMDR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_6_CMP_D              (47UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: GCMDR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_1_CMP_E              (48UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: GCMER register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_2_CMP_E              (49UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: GCMER register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_3_CMP_E              (50UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: GCMER register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_4_CMP_E              (51UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: GCMER register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_5_CMP_E              (52UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: GCMER register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_6_CMP_E              (53UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: GCMER register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_1_CMP_F              (54UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: GCMFR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_2_CMP_F              (55UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: GCMFR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_3_CMP_F              (56UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: GCMFR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_4_CMP_F              (57UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: GCMFR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_5_CMP_F              (58UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: GCMFR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_6_CMP_F              (59UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: GCMFR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_1_OVF                (60UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: Counter register overflow */
#define XBAR_HRPWM_EVT3_TMR6_2_OVF                (61UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: Counter register overflow */
#define XBAR_HRPWM_EVT3_TMR6_3_OVF                (62UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: Counter register overflow */
#define XBAR_HRPWM_EVT3_TMR6_4_OVF                (63UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: Counter register overflow */
#define XBAR_HRPWM_EVT3_TMR6_5_OVF                (64UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: Counter register overflow */
#define XBAR_HRPWM_EVT3_TMR6_6_OVF                (65UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: Counter register overflow */
#define XBAR_HRPWM_EVT3_TMR6_1_UDF                (66UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: Counter register underflow */
#define XBAR_HRPWM_EVT3_TMR6_2_UDF                (67UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: Counter register underflow */
#define XBAR_HRPWM_EVT3_TMR6_3_UDF                (68UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: Counter register underflow */
#define XBAR_HRPWM_EVT3_TMR6_4_UDF                (69UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: Counter register underflow */
#define XBAR_HRPWM_EVT3_TMR6_5_UDF                (70UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: Counter register underflow */
#define XBAR_HRPWM_EVT3_TMR6_6_UDF                (71UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: Counter register underflow */
#define XBAR_HRPWM_EVT3_TMR6_1_SCMP_A             (72UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: SCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_2_SCMP_A             (73UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: SCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_3_SCMP_A             (74UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: SCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_4_SCMP_A             (75UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: SCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_5_SCMP_A             (76UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: SCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_6_SCMP_A             (77UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: SCMAR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_1_SCMP_B             (78UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_1: SCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_2_SCMP_B             (79UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_2: SCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_3_SCMP_B             (80UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_3: SCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_4_SCMP_B             (81UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_4: SCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_5_SCMP_B             (82UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_5: SCMBR register compare-match */
#define XBAR_HRPWM_EVT3_TMR6_6_SCMP_B             (83UL << XBAR_HECR_SEL3_POS)  /*!< TMR6_6: SCMBR register compare-match */
#define XBAR_HRPWM_EVT3_PLA_OUT0                  (84UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT0  */
#define XBAR_HRPWM_EVT3_PLA_OUT1                  (85UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT1  */
#define XBAR_HRPWM_EVT3_PLA_OUT2                  (86UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT2  */
#define XBAR_HRPWM_EVT3_PLA_OUT3                  (87UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT3  */
#define XBAR_HRPWM_EVT3_PLA_OUT4                  (88UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT4  */
#define XBAR_HRPWM_EVT3_PLA_OUT5                  (89UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT5  */
#define XBAR_HRPWM_EVT3_PLA_OUT6                  (90UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT6  */
#define XBAR_HRPWM_EVT3_PLA_OUT7                  (91UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT7  */
#define XBAR_HRPWM_EVT3_PLA_OUT8                  (92UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT8  */
#define XBAR_HRPWM_EVT3_PLA_OUT9                  (93UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT9  */
#define XBAR_HRPWM_EVT3_PLA_OUT10                 (94UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT10 */
#define XBAR_HRPWM_EVT3_PLA_OUT11                 (95UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT11 */
#define XBAR_HRPWM_EVT3_PLA_OUT12                 (96UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT12 */
#define XBAR_HRPWM_EVT3_PLA_OUT13                 (97UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT13 */
#define XBAR_HRPWM_EVT3_PLA_OUT14                 (98UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT14 */
#define XBAR_HRPWM_EVT3_PLA_OUT15                 (99UL << XBAR_HECR_SEL3_POS)  /*!< PLA_OUT15 */
#define XBAR_HRPWM_EVT3_DE_OVF1                   (100UL << XBAR_HECR_SEL3_POS) /*!< HRPWM Diode simulation exceeded limit */
#define XBAR_HRPWM_EVT3_DE_OVF2                   (101UL << XBAR_HECR_SEL3_POS) /*!< HRPWM Diode simulation exceeded limit */
#define XBAR_HRPWM_EVT3_DE_OVF3                   (102UL << XBAR_HECR_SEL3_POS) /*!< HRPWM Diode simulation exceeded limit */
#define XBAR_HRPWM_EVT3_DE_OVF4                   (103UL << XBAR_HECR_SEL3_POS) /*!< HRPWM Diode simulation exceeded limit */
#define XBAR_HRPWM_EVT3_DE_OVF5                   (104UL << XBAR_HECR_SEL3_POS) /*!< HRPWM Diode simulation exceeded limit */
#define XBAR_HRPWM_EVT3_DE_OVF6                   (105UL << XBAR_HECR_SEL3_POS) /*!< HRPWM Diode simulation exceeded limit */
#define XBAR_HRPWM_EVT3_DE_OVF7                   (106UL << XBAR_HECR_SEL3_POS) /*!< HRPWM Diode simulation exceeded limit */
#define XBAR_HRPWM_EVT3_DE_OVF8                   (107UL << XBAR_HECR_SEL3_POS) /*!< HRPWM Diode simulation exceeded limit */
#define XBAR_HRPWM_EVT3_DSOGI_PLL_OUT1            (108UL << XBAR_HECR_SEL3_POS) /*!< DSOGI_PLL: A Positive or negative axis */
#define XBAR_HRPWM_EVT3_DSOGI_PLL_OUT2            (109UL << XBAR_HECR_SEL3_POS) /*!< DSOGI_PLL: B Positive or negative axis */
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_MINDB_Selection XBAR HRPWM Minimum Dead Time Selection
 * @{
 */
#define XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP1         (0UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMA_SWAP_NO_HP1 */
#define XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP2         (1UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMA_SWAP_NO_HP2 */
#define XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP3         (2UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMA_SWAP_NO_HP3 */
#define XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP4         (3UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMA_SWAP_NO_HP4 */
#define XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP5         (4UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMA_SWAP_NO_HP5 */
#define XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP6         (5UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMA_SWAP_NO_HP6 */
#define XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP7         (6UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMA_SWAP_NO_HP7 */
#define XBAR_HRPWM_MINDB_PWMA_SWAP_NO_HP8         (7UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMA_SWAP_NO_HP8 */
#define XBAR_HRPWM_MINDB_PWMB_SWAP_NO_HP1         (8UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMB_SWAP_NO_HP1 */
#define XBAR_HRPWM_MINDB_PWMB_SWAP_NO_HP2         (9UL  << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMB_SWAP_NO_HP2 */
#define XBAR_HRPWM_MINDB_PWMB_SWAP_NO_HP3         (10UL << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMB_SWAP_NO_HP3 */
#define XBAR_HRPWM_MINDB_PWMB_SWAP_NO_HP4         (11UL << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMB_SWAP_NO_HP4 */
#define XBAR_HRPWM_MINDB_PWMB_SWAP_NO_HP5         (12UL << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMB_SWAP_NO_HP5 */
#define XBAR_HRPWM_MINDB_PWMB_SWAP_NO_HP6         (13UL << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMB_SWAP_NO_HP6 */
#define XBAR_HRPWM_MINDB_PWMB_SWAP_NO_HP7         (14UL << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMB_SWAP_NO_HP7 */
#define XBAR_HRPWM_MINDB_PWMB_SWAP_NO_HP8         (15UL << XBAR_MICR_MDSEL_POS)    /*!< HRPWM_PWMB_SWAP_NO_HP8 */
#define XBAR_HRPWM_MINDB_PLA_OUT0                 (16UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT0  */
#define XBAR_HRPWM_MINDB_PLA_OUT1                 (17UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT1  */
#define XBAR_HRPWM_MINDB_PLA_OUT2                 (18UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT2  */
#define XBAR_HRPWM_MINDB_PLA_OUT3                 (19UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT3  */
#define XBAR_HRPWM_MINDB_PLA_OUT4                 (20UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT4  */
#define XBAR_HRPWM_MINDB_PLA_OUT5                 (21UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT5  */
#define XBAR_HRPWM_MINDB_PLA_OUT6                 (22UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT6  */
#define XBAR_HRPWM_MINDB_PLA_OUT7                 (23UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT7  */
#define XBAR_HRPWM_MINDB_PLA_OUT8                 (24UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT8  */
#define XBAR_HRPWM_MINDB_PLA_OUT9                 (25UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT9  */
#define XBAR_HRPWM_MINDB_PLA_OUT10                (26UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT10 */
#define XBAR_HRPWM_MINDB_PLA_OUT11                (27UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT11 */
#define XBAR_HRPWM_MINDB_PLA_OUT12                (28UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT12 */
#define XBAR_HRPWM_MINDB_PLA_OUT13                (29UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT13 */
#define XBAR_HRPWM_MINDB_PLA_OUT14                (30UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT14 */
#define XBAR_HRPWM_MINDB_PLA_OUT15                (31UL << XBAR_MICR_MDSEL_POS)    /*!< PLA_OUT15 */
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_Illegal_Input_Event_Selection XBAR HRPWM Illegal Input Event Selection
 * @{
 */
#define XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP1          (0UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMA MINDB NO HP1 */
#define XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP2          (1UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMA MINDB NO HP2 */
#define XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP3          (2UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMA MINDB NO HP3 */
#define XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP4          (3UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMA MINDB NO HP4 */
#define XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP5          (4UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMA MINDB NO HP5 */
#define XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP6          (5UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMA MINDB NO HP6 */
#define XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP7          (6UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMA MINDB NO HP7 */
#define XBAR_HRPWM_ICL_PWMA_MINDB_NO_HP8          (7UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMA MINDB NO HP8 */
#define XBAR_HRPWM_ICL_PWMB_MINDB_NO_HP1          (8UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMB MINDB NO HP1 */
#define XBAR_HRPWM_ICL_PWMB_MINDB_NO_HP2          (9UL  << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMB MINDB NO HP2 */
#define XBAR_HRPWM_ICL_PWMB_MINDB_NO_HP3          (10UL << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMB MINDB NO HP3 */
#define XBAR_HRPWM_ICL_PWMB_MINDB_NO_HP4          (11UL << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMB MINDB NO HP4 */
#define XBAR_HRPWM_ICL_PWMB_MINDB_NO_HP5          (12UL << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMB MINDB NO HP5 */
#define XBAR_HRPWM_ICL_PWMB_MINDB_NO_HP6          (13UL << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMB MINDB NO HP6 */
#define XBAR_HRPWM_ICL_PWMB_MINDB_NO_HP7          (14UL << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMB MINDB NO HP7 */
#define XBAR_HRPWM_ICL_PWMB_MINDB_NO_HP8          (15UL << XBAR_MICR_ICLSEL_POS)    /*!< HRPWM PWMB MINDB NO HP8 */
#define XBAR_HRPWM_ICL_PLA_OUT0                   (16UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT0  */
#define XBAR_HRPWM_ICL_PLA_OUT1                   (17UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT1  */
#define XBAR_HRPWM_ICL_PLA_OUT2                   (18UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT2  */
#define XBAR_HRPWM_ICL_PLA_OUT3                   (19UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT3  */
#define XBAR_HRPWM_ICL_PLA_OUT4                   (20UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT4  */
#define XBAR_HRPWM_ICL_PLA_OUT5                   (21UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT5  */
#define XBAR_HRPWM_ICL_PLA_OUT6                   (22UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT6  */
#define XBAR_HRPWM_ICL_PLA_OUT7                   (23UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT7  */
#define XBAR_HRPWM_ICL_PLA_OUT8                   (24UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT8  */
#define XBAR_HRPWM_ICL_PLA_OUT9                   (25UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT9  */
#define XBAR_HRPWM_ICL_PLA_OUT10                  (26UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT10 */
#define XBAR_HRPWM_ICL_PLA_OUT11                  (27UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT11 */
#define XBAR_HRPWM_ICL_PLA_OUT12                  (28UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT12 */
#define XBAR_HRPWM_ICL_PLA_OUT13                  (29UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT13 */
#define XBAR_HRPWM_ICL_PLA_OUT14                  (30UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT14 */
#define XBAR_HRPWM_ICL_PLA_OUT15                  (31UL << XBAR_MICR_ICLSEL_POS)    /*!< PLA_OUT15 */
/**
 * @}
 */
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_EMB_Event_Selection XBAR HRPWM EMB(HRPWMEMB) Event Selection
 * @{
 */
/**
 * @defgroup XBAR_HRPWM_EMB_EVT1_Event_Selection XBAR HRPWM EMB EVT1 Event Selection
 * @{
 */
#define XBAR_HRPWM_EMB_EVT1_PA12                  (0UL  << XBAR_HEBCR_SEL1_POS)  /*!< PA12 */
#define XBAR_HRPWM_EMB_EVT1_PA13                  (1UL  << XBAR_HEBCR_SEL1_POS)  /*!< PA13 */
#define XBAR_HRPWM_EMB_EVT1_PA14                  (2UL  << XBAR_HEBCR_SEL1_POS)  /*!< PA14 */
#define XBAR_HRPWM_EMB_EVT1_PA15                  (3UL  << XBAR_HEBCR_SEL1_POS)  /*!< PA15 */
#define XBAR_HRPWM_EMB_EVT1_PB3                   (4UL  << XBAR_HEBCR_SEL1_POS)  /*!< PB3 */
#define XBAR_HRPWM_EMB_EVT1_PB5                   (5UL  << XBAR_HEBCR_SEL1_POS)  /*!< PB5 */
#define XBAR_HRPWM_EMB_EVT1_PB6                   (6UL  << XBAR_HEBCR_SEL1_POS)  /*!< PB6 */
#define XBAR_HRPWM_EMB_EVT1_PB7                   (7UL  << XBAR_HEBCR_SEL1_POS)  /*!< PB7 */
#define XBAR_HRPWM_EMB_EVT1_PB8                   (8UL  << XBAR_HEBCR_SEL1_POS)  /*!< PB8 */
#define XBAR_HRPWM_EMB_EVT1_PB9                   (9UL  << XBAR_HEBCR_SEL1_POS)  /*!< PB9 */
#define XBAR_HRPWM_EMB_EVT1_PC10                  (10UL << XBAR_HEBCR_SEL1_POS)  /*!< PC10 */
#define XBAR_HRPWM_EMB_EVT1_PC11                  (11UL << XBAR_HEBCR_SEL1_POS)  /*!< PC11 */
#define XBAR_HRPWM_EMB_EVT1_PC12                  (12UL << XBAR_HEBCR_SEL1_POS)  /*!< PC12 */
#define XBAR_HRPWM_EMB_EVT1_PD0                   (13UL << XBAR_HEBCR_SEL1_POS)  /*!< PD0 */
#define XBAR_HRPWM_EMB_EVT1_PD1                   (14UL << XBAR_HEBCR_SEL1_POS)  /*!< PD1 */
#define XBAR_HRPWM_EMB_EVT1_PD2                   (15UL << XBAR_HEBCR_SEL1_POS)  /*!< PD2 */
#define XBAR_HRPWM_EMB_EVT1_PD3                   (16UL << XBAR_HEBCR_SEL1_POS)  /*!< PD3 */
#define XBAR_HRPWM_EMB_EVT1_PD4                   (17UL << XBAR_HEBCR_SEL1_POS)  /*!< PD4 */
#define XBAR_HRPWM_EMB_EVT1_PD5                   (18UL << XBAR_HEBCR_SEL1_POS)  /*!< PD5 */
#define XBAR_HRPWM_EMB_EVT1_PD6                   (19UL << XBAR_HEBCR_SEL1_POS)  /*!< PD6 */
#define XBAR_HRPWM_EMB_EVT1_PD7                   (20UL << XBAR_HEBCR_SEL1_POS)  /*!< PD7 */
#define XBAR_HRPWM_EMB_EVT1_PE0                   (21UL << XBAR_HEBCR_SEL1_POS)  /*!< PE0 */
#define XBAR_HRPWM_EMB_EVT1_PE1                   (22UL << XBAR_HEBCR_SEL1_POS)  /*!< PE1 */
#define XBAR_HRPWM_EMB_EVT1_PE2                   (23UL << XBAR_HEBCR_SEL1_POS)  /*!< PE2 */
#define XBAR_HRPWM_EMB_EVT1_PE3                   (24UL << XBAR_HEBCR_SEL1_POS)  /*!< PE3 */
#define XBAR_HRPWM_EMB_EVT1_PE4                   (25UL << XBAR_HEBCR_SEL1_POS)  /*!< PE4 */
#define XBAR_HRPWM_EMB_EVT1_PE5                   (26UL << XBAR_HEBCR_SEL1_POS)  /*!< PE5 */
#define XBAR_HRPWM_EMB_EVT1_PE6                   (27UL << XBAR_HEBCR_SEL1_POS)  /*!< PE6 */
#define XBAR_HRPWM_EMB_EVT1_PF2                   (28UL << XBAR_HEBCR_SEL1_POS)  /*!< PF2  */
#define XBAR_HRPWM_EMB_EVT1_PF3                   (29UL << XBAR_HEBCR_SEL1_POS)  /*!< PF3  */
#define XBAR_HRPWM_EMB_EVT1_PF4                   (30UL << XBAR_HEBCR_SEL1_POS)  /*!< PF4  */
#define XBAR_HRPWM_EMB_EVT1_PF5                   (31UL << XBAR_HEBCR_SEL1_POS)  /*!< PF5  */
#define XBAR_HRPWM_EMB_EVT1_PF6                   (32UL << XBAR_HEBCR_SEL1_POS)  /*!< PF6  */
#define XBAR_HRPWM_EMB_EVT1_PF7                   (33UL << XBAR_HEBCR_SEL1_POS)  /*!< PF7  */
#define XBAR_HRPWM_EMB_EVT1_PF8                   (34UL << XBAR_HEBCR_SEL1_POS)  /*!< PF8  */
#define XBAR_HRPWM_EMB_EVT1_PF9                   (35UL << XBAR_HEBCR_SEL1_POS)  /*!< PF9  */
#define XBAR_HRPWM_EMB_EVT1_PF10                  (36UL << XBAR_HEBCR_SEL1_POS)  /*!< PF10 */
#define XBAR_HRPWM_EMB_EVT1_PF11                  (37UL << XBAR_HEBCR_SEL1_POS)  /*!< PF11 */
#define XBAR_HRPWM_EMB_EVT1_PF12                  (38UL << XBAR_HEBCR_SEL1_POS)  /*!< PF12 */
#define XBAR_HRPWM_EMB_EVT1_PF13                  (39UL << XBAR_HEBCR_SEL1_POS)  /*!< PF13 */
#define XBAR_HRPWM_EMB_EVT1_PF14                  (40UL << XBAR_HEBCR_SEL1_POS)  /*!< PF14 */
#define XBAR_HRPWM_EMB_EVT1_PF15                  (41UL << XBAR_HEBCR_SEL1_POS)  /*!< PF15 */
#define XBAR_HRPWM_EMB_EVT1_PG0                   (42UL << XBAR_HEBCR_SEL1_POS)  /*!< PG0 */
#define XBAR_HRPWM_EMB_EVT1_PG1                   (43UL << XBAR_HEBCR_SEL1_POS)  /*!< PG1 */
#define XBAR_HRPWM_EMB_EVT1_PG2                   (44UL << XBAR_HEBCR_SEL1_POS)  /*!< PG2 */
#define XBAR_HRPWM_EMB_EVT1_PG3                   (45UL << XBAR_HEBCR_SEL1_POS)  /*!< PG3 */
#define XBAR_HRPWM_EMB_EVT1_PG4                   (46UL << XBAR_HEBCR_SEL1_POS)  /*!< PG4 */
#define XBAR_HRPWM_EMB_EVT1_PG5                   (47UL << XBAR_HEBCR_SEL1_POS)  /*!< PG5 */
#define XBAR_HRPWM_EMB_EVT1_PG6                   (48UL << XBAR_HEBCR_SEL1_POS)  /*!< PG6 */
#define XBAR_HRPWM_EMB_EVT1_PG7                   (49UL << XBAR_HEBCR_SEL1_POS)  /*!< PG7 */
#define XBAR_HRPWM_EMB_EVT1_PG8                   (50UL << XBAR_HEBCR_SEL1_POS)  /*!< PG8 */
#define XBAR_HRPWM_EMB_EVT1_PG9                   (51UL << XBAR_HEBCR_SEL1_POS)  /*!< PG9 */
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_EMB_EVT2_Selection XBAR HRPWM EMB EVT2 Selection
 * @{
 */
#define XBAR_HRPWM_EMB_EVT2_CMP1                  (0UL << XBAR_HEBCR_SEL2_POS)   /*!< CMP1 */
#define XBAR_HRPWM_EMB_EVT2_CMP2                  (1UL << XBAR_HEBCR_SEL2_POS)   /*!< CMP2 */
#define XBAR_HRPWM_EMB_EVT2_CMP3                  (2UL << XBAR_HEBCR_SEL2_POS)   /*!< CMP3 */
#define XBAR_HRPWM_EMB_EVT2_CMP4                  (3UL << XBAR_HEBCR_SEL2_POS)   /*!< CMP4 */
#define XBAR_HRPWM_EMB_EVT2_CMP5                  (4UL << XBAR_HEBCR_SEL2_POS)   /*!< CMP5 */
#define XBAR_HRPWM_EMB_EVT2_CMP6                  (5UL << XBAR_HEBCR_SEL2_POS)   /*!< CMP6 */
#define XBAR_HRPWM_EMB_EVT2_CMP7                  (6UL << XBAR_HEBCR_SEL2_POS)   /*!< CMP7 */
#define XBAR_HRPWM_EMB_EVT2_CMP8                  (7UL << XBAR_HEBCR_SEL2_POS)   /*!< CMP8 */
/**
 * @}
 */

/**
 * @defgroup XBAR_HRPWM_EMB_EVT3_Selection XBAR HRPWM EMB EVT3 Selection
 * @{
 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_BK0              (0UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_BK0 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_BK1              (1UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_BK1 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_BK2              (2UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_BK2 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_BK3              (3UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_BK3 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_CEVT1_0          (4UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_CEVT1_0 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_CEVT1_1          (5UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_CEVT1_1 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_CEVT1_2          (6UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_CEVT1_2 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_CEVT1_3          (7UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_CEVT1_3 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_CEVT2_0          (8UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_CEVT2_0 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_CEVT2_1          (9UL  << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_CEVT2_1 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_CEVT2_2          (10UL << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_CEVT2_2 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_CEVT2_3          (11UL << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_CEVT2_3 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_ZCD0             (12UL << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_ZCD0 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_ZCD1             (13UL << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_ZCD1 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_ZCD2             (14UL << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_ZCD2 */
#define XBAR_HRPWM_EMB_EVT3_SDFM_ZCD3             (15UL << XBAR_HEBCR_SEL3_POS)  /*!< SDFM_ZCD3 */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT0              (16UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT0  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT1              (17UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT1  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT2              (18UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT2  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT3              (19UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT3  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT4              (20UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT4  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT5              (21UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT5  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT6              (22UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT6  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT7              (23UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT7  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT8              (24UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT8  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT9              (25UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT9  */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT10             (26UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT10 */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT11             (27UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT11 */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT12             (28UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT12 */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT13             (29UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT13 */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT14             (30UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT14 */
#define XBAR_HRPWM_EMB_EVT3_PLA_OUT15             (31UL << XBAR_HEBCR_SEL3_POS)  /*!< PLA_OUT15 */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT1            (32UL << XBAR_HEBCR_SEL3_POS)  /*!< External event1 output by the HRPWM module OUT1  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT2            (33UL << XBAR_HEBCR_SEL3_POS)  /*!< External event2 output by the HRPWM module OUT2  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT3            (34UL << XBAR_HEBCR_SEL3_POS)  /*!< External event3 output by the HRPWM module OUT3  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT4            (35UL << XBAR_HEBCR_SEL3_POS)  /*!< External event4 output by the HRPWM module OUT4  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT5            (36UL << XBAR_HEBCR_SEL3_POS)  /*!< External event5 output by the HRPWM module OUT5  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT6            (37UL << XBAR_HEBCR_SEL3_POS)  /*!< External event6 output by the HRPWM module OUT6  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT7            (38UL << XBAR_HEBCR_SEL3_POS)  /*!< External event7 output by the HRPWM module OUT7  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT8            (39UL << XBAR_HEBCR_SEL3_POS)  /*!< External event8 output by the HRPWM module OUT8  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT9            (40UL << XBAR_HEBCR_SEL3_POS)  /*!< External event9 output by the HRPWM module OUT9  */
#define XBAR_HRPWM_EMB_EVT3_HRPWM_OUT10           (41UL << XBAR_HEBCR_SEL3_POS)  /*!< External event10 output by the HRPWM module OUT10 */
#define XBAR_HRPWM_EMB_EVT3_DSOGI_PLL_FAILURE     (42UL << XBAR_HEBCR_SEL3_POS)  /*!< DSOGI_PLL phase locking failure */
/**
 * @}
 */
/**
 * @}
 */

/**
 * @defgroup XBAR_TMR6_EMB_Event_Selection XBAR TMR6 EMB(TMR6EMB) Event Selection
 * @{
 */
#define XBAR_TMR6_EMB_PA12                        (0UL  << XBAR_TECR_TESEL_POS)     /*!< PA12 */
#define XBAR_TMR6_EMB_PA13                        (1UL  << XBAR_TECR_TESEL_POS)     /*!< PA13 */
#define XBAR_TMR6_EMB_PA14                        (2UL  << XBAR_TECR_TESEL_POS)     /*!< PA14 */
#define XBAR_TMR6_EMB_PA15                        (3UL  << XBAR_TECR_TESEL_POS)     /*!< PA15 */
#define XBAR_TMR6_EMB_PB3                         (4UL  << XBAR_TECR_TESEL_POS)     /*!< PB3 */
#define XBAR_TMR6_EMB_PB5                         (5UL  << XBAR_TECR_TESEL_POS)     /*!< PB5 */
#define XBAR_TMR6_EMB_PB6                         (6UL  << XBAR_TECR_TESEL_POS)     /*!< PB6 */
#define XBAR_TMR6_EMB_PB7                         (7UL  << XBAR_TECR_TESEL_POS)     /*!< PB7 */
#define XBAR_TMR6_EMB_PB8                         (8UL  << XBAR_TECR_TESEL_POS)     /*!< PB8 */
#define XBAR_TMR6_EMB_PB9                         (9UL  << XBAR_TECR_TESEL_POS)     /*!< PB9 */
#define XBAR_TMR6_EMB_PC10                        (10UL << XBAR_TECR_TESEL_POS)     /*!< PC10 */
#define XBAR_TMR6_EMB_PC11                        (11UL << XBAR_TECR_TESEL_POS)     /*!< PC11 */
#define XBAR_TMR6_EMB_PC12                        (12UL << XBAR_TECR_TESEL_POS)     /*!< PC12 */
#define XBAR_TMR6_EMB_PD0                         (13UL << XBAR_TECR_TESEL_POS)     /*!< PD0 */
#define XBAR_TMR6_EMB_PD1                         (14UL << XBAR_TECR_TESEL_POS)     /*!< PD1 */
#define XBAR_TMR6_EMB_PD2                         (15UL << XBAR_TECR_TESEL_POS)     /*!< PD2 */
#define XBAR_TMR6_EMB_PD3                         (16UL << XBAR_TECR_TESEL_POS)     /*!< PD3 */
#define XBAR_TMR6_EMB_PD4                         (17UL << XBAR_TECR_TESEL_POS)     /*!< PD4 */
#define XBAR_TMR6_EMB_PD5                         (18UL << XBAR_TECR_TESEL_POS)     /*!< PD5 */
#define XBAR_TMR6_EMB_PD6                         (19UL << XBAR_TECR_TESEL_POS)     /*!< PD6 */
#define XBAR_TMR6_EMB_PD7                         (20UL << XBAR_TECR_TESEL_POS)     /*!< PD7 */
#define XBAR_TMR6_EMB_PE0                         (21UL << XBAR_TECR_TESEL_POS)     /*!< PE0 */
#define XBAR_TMR6_EMB_PE1                         (22UL << XBAR_TECR_TESEL_POS)     /*!< PE1 */
#define XBAR_TMR6_EMB_PE2                         (23UL << XBAR_TECR_TESEL_POS)     /*!< PE2 */
#define XBAR_TMR6_EMB_PE3                         (24UL << XBAR_TECR_TESEL_POS)     /*!< PE3 */
#define XBAR_TMR6_EMB_PE4                         (25UL << XBAR_TECR_TESEL_POS)     /*!< PE4 */
#define XBAR_TMR6_EMB_PE5                         (26UL << XBAR_TECR_TESEL_POS)     /*!< PE5 */
#define XBAR_TMR6_EMB_PE6                         (27UL << XBAR_TECR_TESEL_POS)     /*!< PE6 */
#define XBAR_TMR6_EMB_PF2                         (28UL << XBAR_TECR_TESEL_POS)     /*!< PF2  */
#define XBAR_TMR6_EMB_PF3                         (29UL << XBAR_TECR_TESEL_POS)     /*!< PF3  */
#define XBAR_TMR6_EMB_PF4                         (30UL << XBAR_TECR_TESEL_POS)     /*!< PF4  */
#define XBAR_TMR6_EMB_PF5                         (31UL << XBAR_TECR_TESEL_POS)     /*!< PF5  */
#define XBAR_TMR6_EMB_PF6                         (32UL << XBAR_TECR_TESEL_POS)     /*!< PF6  */
#define XBAR_TMR6_EMB_PF7                         (33UL << XBAR_TECR_TESEL_POS)     /*!< PF7  */
#define XBAR_TMR6_EMB_PF8                         (34UL << XBAR_TECR_TESEL_POS)     /*!< PF8  */
#define XBAR_TMR6_EMB_PF9                         (35UL << XBAR_TECR_TESEL_POS)     /*!< PF9  */
#define XBAR_TMR6_EMB_PF10                        (36UL << XBAR_TECR_TESEL_POS)     /*!< PF10 */
#define XBAR_TMR6_EMB_PF11                        (37UL << XBAR_TECR_TESEL_POS)     /*!< PF11 */
#define XBAR_TMR6_EMB_PF12                        (38UL << XBAR_TECR_TESEL_POS)     /*!< PF12 */
#define XBAR_TMR6_EMB_PF13                        (39UL << XBAR_TECR_TESEL_POS)     /*!< PF13 */
#define XBAR_TMR6_EMB_PF14                        (40UL << XBAR_TECR_TESEL_POS)     /*!< PF14 */
#define XBAR_TMR6_EMB_PF15                        (41UL << XBAR_TECR_TESEL_POS)     /*!< PF15 */
#define XBAR_TMR6_EMB_PG0                         (42UL << XBAR_TECR_TESEL_POS)     /*!< PG0 */
#define XBAR_TMR6_EMB_PG1                         (43UL << XBAR_TECR_TESEL_POS)     /*!< PG1 */
#define XBAR_TMR6_EMB_PG2                         (44UL << XBAR_TECR_TESEL_POS)     /*!< PG2 */
#define XBAR_TMR6_EMB_PG3                         (45UL << XBAR_TECR_TESEL_POS)     /*!< PG3 */
#define XBAR_TMR6_EMB_PG4                         (46UL << XBAR_TECR_TESEL_POS)     /*!< PG4 */
#define XBAR_TMR6_EMB_PG5                         (47UL << XBAR_TECR_TESEL_POS)     /*!< PG5 */
#define XBAR_TMR6_EMB_PG6                         (48UL << XBAR_TECR_TESEL_POS)     /*!< PG6 */
#define XBAR_TMR6_EMB_PG7                         (49UL << XBAR_TECR_TESEL_POS)     /*!< PG7 */
#define XBAR_TMR6_EMB_PG8                         (50UL << XBAR_TECR_TESEL_POS)     /*!< PG8 */
#define XBAR_TMR6_EMB_PG9                         (51UL << XBAR_TECR_TESEL_POS)     /*!< PG9 */
#define XBAR_TMR6_EMB_SDFM_BK0                    (52UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_BK0 */
#define XBAR_TMR6_EMB_SDFM_BK1                    (53UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_BK1 */
#define XBAR_TMR6_EMB_SDFM_BK2                    (54UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_BK2 */
#define XBAR_TMR6_EMB_SDFM_BK3                    (55UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_BK3 */
#define XBAR_TMR6_EMB_SDFM_CEVT1_0                (56UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_CEVT1_0 */
#define XBAR_TMR6_EMB_SDFM_CEVT1_1                (57UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_CEVT1_1 */
#define XBAR_TMR6_EMB_SDFM_CEVT1_2                (58UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_CEVT1_2 */
#define XBAR_TMR6_EMB_SDFM_CEVT1_3                (59UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_CEVT1_3 */
#define XBAR_TMR6_EMB_SDFM_CEVT2_0                (60UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_CEVT2_0 */
#define XBAR_TMR6_EMB_SDFM_CEVT2_1                (61UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_CEVT2_1 */
#define XBAR_TMR6_EMB_SDFM_CEVT2_2                (62UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_CEVT2_2 */
#define XBAR_TMR6_EMB_SDFM_CEVT2_3                (63UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_CEVT2_3 */
#define XBAR_TMR6_EMB_SDFM_ZCD0                   (64UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_ZCD0 */
#define XBAR_TMR6_EMB_SDFM_ZCD1                   (65UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_ZCD1 */
#define XBAR_TMR6_EMB_SDFM_ZCD2                   (66UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_ZCD2 */
#define XBAR_TMR6_EMB_SDFM_ZCD3                   (67UL << XBAR_TECR_TESEL_POS)     /*!< SDFM_ZCD3 */
/**
 * @}
 */

/**
 * @defgroup XBAR_TRLPWM_Event_Selection XBAR TRLPWM Event Selection
 * @{
 */
#define XBAR_TRLPWM_PLA_OUT0                      (0UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT0  */
#define XBAR_TRLPWM_PLA_OUT1                      (1UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT1  */
#define XBAR_TRLPWM_PLA_OUT2                      (2UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT2  */
#define XBAR_TRLPWM_PLA_OUT3                      (3UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT3  */
#define XBAR_TRLPWM_PLA_OUT4                      (4UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT4  */
#define XBAR_TRLPWM_PLA_OUT5                      (5UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT5  */
#define XBAR_TRLPWM_PLA_OUT6                      (6UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT6  */
#define XBAR_TRLPWM_PLA_OUT7                      (7UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT7  */
#define XBAR_TRLPWM_PLA_OUT8                      (8UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT8  */
#define XBAR_TRLPWM_PLA_OUT9                      (9UL  << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT9  */
#define XBAR_TRLPWM_PLA_OUT10                     (10UL << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT10 */
#define XBAR_TRLPWM_PLA_OUT11                     (11UL << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT11 */
#define XBAR_TRLPWM_PLA_OUT12                     (12UL << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT12 */
#define XBAR_TRLPWM_PLA_OUT13                     (13UL << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT13 */
#define XBAR_TRLPWM_PLA_OUT14                     (14UL << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT14 */
#define XBAR_TRLPWM_PLA_OUT15                     (15UL << XBAR_TRLCR_TRLSEL_POS)   /*!< PLA_OUT15 */
#define XBAR_TRLPWM_TMR6_EMB0                     (16UL << XBAR_TRLCR_TRLSEL_POS)   /*!< TMR6_EMB0 */
#define XBAR_TRLPWM_TMR6_EMB1                     (17UL << XBAR_TRLCR_TRLSEL_POS)   /*!< TMR6_EMB1 */
#define XBAR_TRLPWM_TMR6_EMB2                     (18UL << XBAR_TRLCR_TRLSEL_POS)   /*!< TMR6_EMB2 */
#define XBAR_TRLPWM_TMR6_EMB3                     (19UL << XBAR_TRLCR_TRLSEL_POS)   /*!< TMR6_EMB3 */
#define XBAR_TRLPWM_TMR6_EMB4                     (20UL << XBAR_TRLCR_TRLSEL_POS)   /*!< TMR6_EMB4 */
#define XBAR_TRLPWM_TMR6_EMB5                     (21UL << XBAR_TRLCR_TRLSEL_POS)   /*!< TMR6_EMB5 */
#define XBAR_TRLPWM_HRPWM_EMB0                    (22UL << XBAR_TRLCR_TRLSEL_POS)   /*!< HRPWM_EMB0 */
#define XBAR_TRLPWM_HRPWM_EMB1                    (23UL << XBAR_TRLCR_TRLSEL_POS)   /*!< HRPWM_EMB1 */
#define XBAR_TRLPWM_HRPWM_EMB2                    (24UL << XBAR_TRLCR_TRLSEL_POS)   /*!< HRPWM_EMB2 */
#define XBAR_TRLPWM_HRPWM_EMB3                    (25UL << XBAR_TRLCR_TRLSEL_POS)   /*!< HRPWM_EMB3 */
#define XBAR_TRLPWM_HRPWM_EMB4                    (26UL << XBAR_TRLCR_TRLSEL_POS)   /*!< HRPWM_EMB4 */
#define XBAR_TRLPWM_HRPWM_EMB5                    (27UL << XBAR_TRLCR_TRLSEL_POS)   /*!< HRPWM_EMB5 */
#define XBAR_TRLPWM_HRPWM_EMB6                    (28UL << XBAR_TRLCR_TRLSEL_POS)   /*!< HRPWM_EMB6 */
#define XBAR_TRLPWM_HRPWM_EMB7                    (29UL << XBAR_TRLCR_TRLSEL_POS)   /*!< HRPWM_EMB7 */
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
 * @addtogroup XBAR_Global_Functions
 * @{
 */
int32_t XBAR_DeInit(CM_XBAR_TypeDef *XBARx);

int32_t XBAR_HRPWM_ExtEventStructInit(stc_xbar_hrpwm_ext_event_init_t *pstcXbarHrpwmExtEventInit);
int32_t XBAR_HRPWM_ExtEventInit(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch,
                                const stc_xbar_hrpwm_ext_event_init_t *pstcXbarHrpwmExtEventInit);
void XBAR_HRPWM_SetEvent1(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event1);
void XBAR_HRPWM_SetEvent2(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event2);
void XBAR_HRPWM_SetEvent3(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event3);

int32_t XBAR_HRPWM_MdbIclStructInit(stc_xbar_hrpwm_mdb_icl_init_t *pstcXbarHrpwmMdbIclInit);
int32_t XBAR_HRPWM_MdbIclInit(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch,
                              const stc_xbar_hrpwm_mdb_icl_init_t *pstcXbarHrpwmMdbIclInit);
void XBAR_HRPWM_SetMinDBEvent(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32MinDBEvent);
void XBAR_HRPWM_SetIllegalInputEvent(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32ICLEvent);

int32_t XBAR_HRPWMEMB_StructInit(stc_xbar_hrpwm_emb_init_t *pstcXbarHrpwmEmbInit);
int32_t XBAR_HRPWMEMB_Init(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch,
                           const stc_xbar_hrpwm_emb_init_t *pstcXbarHrpwmEmbInit);
void XBAR_HRPWMEMB_SetEvent1(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event1);
void XBAR_HRPWMEMB_SetEvent2(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event2);
void XBAR_HRPWMEMB_SetEvent3(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event3);

void XBAR_TMR6EMB_SetEvent(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event);

void XBAR_TRLPWM_SetEvent(CM_XBAR_TypeDef *XBARx, uint32_t u32Ch, uint32_t u32Event);
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

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_XBAR_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
