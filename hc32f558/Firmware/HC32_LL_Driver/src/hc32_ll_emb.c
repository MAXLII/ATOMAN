/**
 *******************************************************************************
 * @file  hc32_ll_emb.c
 * @brief This file provides firmware functions to manage the EMB
 *        (Emergency Brake).
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
#include "hc32_ll_emb.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_EMB EMB
 * @brief Emergency Brake Driver Library
 * @{
 */

#if (LL_EMB_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup EMB_Local_Macros EMB Local Macros
 * @{
 */

/**
 * @defgroup EMB_Check_Parameters_Validity EMB Check Parameters Validity
 * @{
 */
#define IS_EMB_GROUP(x)                                                        \
(   ((x) == CM_EMB0)                        ||                                 \
    ((x) == CM_EMB1)                        ||                                 \
    ((x) == CM_EMB2)                        ||                                 \
    ((x) == CM_EMB3)                        ||                                 \
    ((x) == CM_EMB4)                        ||                                 \
    ((x) == CM_EMB5))
#define IS_EMB_TMR6_GROUP(x)                (IS_EMB_GROUP(x))

#define IS_EMB_OSC_STAT(x)                                                     \
(   ((x) == EMB_OSC_ENABLE)                 ||                                 \
    ((x) == EMB_OSC_DISABLE))

#define IS_EMB_SRAM_ECC_ERR_STAT(x)                                            \
(   ((x) == EMB_SRAM_ECC_ERR_ENABLE)        ||                                 \
    ((x) == EMB_SRAM_ECC_ERR_DISABLE))

#define IS_EMB_LOCKUP_STAT(x)                                                  \
(   ((x) == EMB_LOCKUP_ENABLE)              ||                                 \
    ((x) == EMB_LOCKUP_DISABLE))

#define IS_EMB_LVD_STAT(x)                                                     \
(   ((x) == EMB_LVD_ENABLE)                 ||                                 \
    ((x) == EMB_LVD_DISABLE))

#define IS_EMB_FLASH_ECC_ERR_STAT(x)                                           \
(   ((x) == EMB_FLASH_ECC_ERR_ENABLE)       ||                                 \
    ((x) == EMB_FLASH_ECC_ERR_DISABLE))

#define IS_EMB_FCM_STAT(x)                                                     \
(   ((x) == EMB_FCM_ENABLE)                 ||                                 \
    ((x) == EMB_FCM_DISABLE))

#define IS_EMB_CMP1_STAT(x)                                                    \
(   ((x) == EMB_CMP1_ENABLE)                ||                                 \
    ((x) == EMB_CMP1_DISABLE))

#define IS_EMB_CMP2_STAT(x)                                                    \
(   ((x) == EMB_CMP2_ENABLE)                ||                                 \
    ((x) == EMB_CMP2_DISABLE))

#define IS_EMB_CMP3_STAT(x)                                                    \
(   ((x) == EMB_CMP3_ENABLE)                ||                                 \
    ((x) == EMB_CMP3_DISABLE))

#define IS_EMB_CMP4_STAT(x)                                                    \
(   ((x) == EMB_CMP4_ENABLE)                ||                                 \
    ((x) == EMB_CMP4_DISABLE))

#define IS_EMB_CMP5_STAT(x)                                                    \
(   ((x) == EMB_CMP5_ENABLE)                ||                                 \
    ((x) == EMB_CMP5_DISABLE))

#define IS_EMB_CMP6_STAT(x)                                                    \
(   ((x) == EMB_CMP6_ENABLE)                ||                                 \
    ((x) == EMB_CMP6_DISABLE))

#define IS_EMB_CMP7_STAT(x)                                                    \
(   ((x) == EMB_CMP7_ENABLE)                ||                                 \
    ((x) == EMB_CMP7_DISABLE))

#define IS_EMB_CMP8_STAT(x)                                                    \
(   ((x) == EMB_CMP8_ENABLE)                ||                                 \
    ((x) == EMB_CMP8_DISABLE))

#define IS_EMB_PORT1_STAT(x)                                                   \
(   ((x) == EMB_PORT1_ENABLE)               ||                                 \
    ((x) == EMB_PORT1_DISABLE))

#define IS_EMB_PORT1_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT1_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT1_DETECT_LVL_HIGH))

#define IS_EMB_PORT1_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT1_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT1_FILTER_DISABLE))

#define IS_EMB_PORT1_FILTER_DIV(x)          (((x) & (~EMB_PORT1_FILTER_CLK_DIV_MASK)) == 0UL)

#define IS_EMB_PORT2_STAT(x)                                                   \
(   ((x) == EMB_PORT2_ENABLE)               ||                                 \
    ((x) == EMB_PORT2_DISABLE))

#define IS_EMB_PORT2_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT2_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT2_DETECT_LVL_HIGH))

#define IS_EMB_PORT2_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT2_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT2_FILTER_DISABLE))

#define IS_EMB_PORT2_FILTER_DIV(x)          (((x) & (~EMB_PORT2_FILTER_CLK_DIV_MASK)) == 0UL)

#define IS_EMB_PORT3_STAT(x)                                                   \
(   ((x) == EMB_PORT3_ENABLE)               ||                                 \
    ((x) == EMB_PORT3_DISABLE))

#define IS_EMB_PORT3_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT3_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT3_DETECT_LVL_HIGH))

#define IS_EMB_PORT3_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT3_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT3_FILTER_DISABLE))

#define IS_EMB_PORT3_FILTER_DIV(x)          (((x) & (~EMB_PORT3_FILTER_CLK_DIV_MASK)) == 0UL)

#define IS_EMB_PORT4_STAT(x)                                                   \
(   ((x) == EMB_PORT4_ENABLE)               ||                                 \
    ((x) == EMB_PORT4_DISABLE))

#define IS_EMB_PORT4_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT4_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT4_DETECT_LVL_HIGH))

#define IS_EMB_PORT4_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT4_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT4_FILTER_DISABLE))

#define IS_EMB_PORT4_FILTER_DIV(x)          (((x) & (~EMB_PORT4_FILTER_CLK_DIV_MASK)) == 0UL)

#define IS_EMB_PORT5_STAT(x)                                                   \
(   ((x) == EMB_PORT5_ENABLE)               ||                                 \
    ((x) == EMB_PORT5_DISABLE))

#define IS_EMB_PORT5_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT5_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT5_DETECT_LVL_HIGH))

#define IS_EMB_PORT5_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT5_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT5_FILTER_DISABLE))

#define IS_EMB_PORT5_FILTER_DIV(x)          (((x) & (~EMB_PORT5_FILTER_CLK_DIV_MASK)) == 0UL)
#define IS_EMB_PORT6_STAT(x)                                                   \
(   ((x) == EMB_PORT6_ENABLE)               ||                                 \
    ((x) == EMB_PORT6_DISABLE))

#define IS_EMB_PORT6_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT6_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT6_DETECT_LVL_HIGH))

#define IS_EMB_PORT6_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT6_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT6_FILTER_DISABLE))

#define IS_EMB_PORT6_FILTER_DIV(x)          (((x) & (~EMB_PORT6_FILTER_CLK_DIV_MASK)) == 0UL)
#define IS_EMB_PORT7_STAT(x)                                                   \
(   ((x) == EMB_PORT7_ENABLE)               ||                                 \
    ((x) == EMB_PORT7_DISABLE))

#define IS_EMB_PORT7_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT7_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT7_DETECT_LVL_HIGH))

#define IS_EMB_PORT7_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT7_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT7_FILTER_DISABLE))

#define IS_EMB_PORT7_FILTER_DIV(x)          (((x) & (~EMB_PORT7_FILTER_CLK_DIV_MASK)) == 0UL)

#define IS_EMB_PORT8_STAT(x)                                                   \
(   ((x) == EMB_PORT8_ENABLE)               ||                                 \
    ((x) == EMB_PORT8_DISABLE))

#define IS_EMB_PORT8_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT8_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT8_DETECT_LVL_HIGH))

#define IS_EMB_PORT8_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT8_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT8_FILTER_DISABLE))

#define IS_EMB_PORT8_FILTER_DIV(x)          (((x) & (~EMB_PORT8_FILTER_CLK_DIV_MASK)) == 0UL)

#define IS_EMB_PORT9_STAT(x)                                                   \
(   ((x) == EMB_PORT9_ENABLE)               ||                                 \
    ((x) == EMB_PORT9_DISABLE))

#define IS_EMB_PORT9_DETECT_LVL(x)                                             \
(   ((x) == EMB_PORT9_DETECT_LVL_LOW)       ||                                 \
    ((x) == EMB_PORT9_DETECT_LVL_HIGH))

#define IS_EMB_PORT9_FILTER_STAT(x)                                            \
(   ((x) == EMB_PORT9_FILTER_ENABLE)        ||                                 \
    ((x) == EMB_PORT9_FILTER_DISABLE))

#define IS_EMB_PORT9_FILTER_DIV(x)          (((x) & (~EMB_PORT9_FILTER_CLK_DIV_MASK)) == 0UL)

#define IS_EMB_PORT_FILTER_CNT(x)                                              \
(   ((x) == EMB_PORT_FILTER_CNT_3)          ||                                 \
    ((x) == EMB_PORT_FILTER_CNT_2))

#define IS_EMB_TMR6_1_PWM_STAT(x)                                              \
(   ((x) == EMB_TMR6_1_PWM_ENABLE)          ||                                 \
    ((x) == EMB_TMR6_1_PWM_DISABLE))

#define IS_EMB_DETECT_TMR6_1_PWM_LVL(x)                                        \
(   ((x) == EMB_DETECT_TMR6_1_PWM_BOTH_LOW) ||                                 \
    ((x) == EMB_DETECT_TMR6_1_PWM_BOTH_HIGH))

#define IS_EMB_TMR6_2_PWM_STAT(x)                                              \
(   ((x) == EMB_TMR6_2_PWM_ENABLE)          ||                                 \
    ((x) == EMB_TMR6_2_PWM_DISABLE))

#define IS_EMB_DETECT_TMR6_2_PWM_LVL(x)                                        \
(   ((x) == EMB_DETECT_TMR6_2_PWM_BOTH_LOW) ||                                 \
    ((x) == EMB_DETECT_TMR6_2_PWM_BOTH_HIGH))
#define IS_EMB_TMR6_3_PWM_STAT(x)                                              \
(   ((x) == EMB_TMR6_3_PWM_ENABLE)          ||                                 \
    ((x) == EMB_TMR6_3_PWM_DISABLE))

#define IS_EMB_DETECT_TMR6_3_PWM_LVL(x)                                        \
(   ((x) == EMB_DETECT_TMR6_3_PWM_BOTH_LOW) ||                                 \
    ((x) == EMB_DETECT_TMR6_3_PWM_BOTH_HIGH))
#define IS_EMB_TMR6_4_PWM_STAT(x)                                              \
(   ((x) == EMB_TMR6_4_PWM_ENABLE)          ||                                 \
    ((x) == EMB_TMR6_4_PWM_DISABLE))

#define IS_EMB_DETECT_TMR6_4_PWM_LVL(x)                                        \
(   ((x) == EMB_DETECT_TMR6_4_PWM_BOTH_LOW) ||                                 \
    ((x) == EMB_DETECT_TMR6_4_PWM_BOTH_HIGH))
#define IS_EMB_TMR6_5_PWM_STAT(x)                                              \
(   ((x) == EMB_TMR6_5_PWM_ENABLE)          ||                                 \
    ((x) == EMB_TMR6_5_PWM_DISABLE))

#define IS_EMB_DETECT_TMR6_5_PWM_LVL(x)                                        \
(   ((x) == EMB_DETECT_TMR6_5_PWM_BOTH_LOW) ||                                 \
    ((x) == EMB_DETECT_TMR6_5_PWM_BOTH_HIGH))

#define IS_EMB_TMR6_6_PWM_STAT(x)                                              \
(   ((x) == EMB_TMR6_6_PWM_ENABLE)          ||                                 \
    ((x) == EMB_TMR6_6_PWM_DISABLE))

#define IS_EMB_DETECT_TMR6_6_PWM_LVL(x)                                        \
(   ((x) == EMB_DETECT_TMR6_6_PWM_BOTH_LOW) ||                                 \
    ((x) == EMB_DETECT_TMR6_6_PWM_BOTH_HIGH))

#define IS_EMB_INT(x)                                                          \
(   ((x) != 0UL)                            &&                                 \
    (((x) | EMB_INT_ALL) == EMB_INT_ALL))

#define IS_EMB_FLAG(reg,flag)                                                   \
(   ((flag) != 0UL)                         &&                                 \
    ((((reg) == EMB_GET_FLAG_STAT1) && (((flag) | EMB_FLAG_STAT1_MASK) == EMB_FLAG_STAT1_MASK)) || \
    (((reg) == EMB_GET_FLAG_STAT2) && (((flag) | EMB_FLAG_STAT2_MASK) == EMB_FLAG_STAT2_MASK)) || \
    (((reg) == EMB_GET_FLAG_STAT3) && (((flag) | EMB_FLAG_STAT3_MASK) == EMB_FLAG_STAT3_MASK))))

#define IS_EMB_CLR_FLAG(reg,flag)                                               \
(   ((flag) != 0UL)                         &&                                 \
    ((((reg) == EMB_CLR_FLAG_STATCLR) && (((flag) | EMB_FLAG_CLR_ALL) == EMB_FLAG_CLR_ALL))             || \
    (((reg) == EMB_CLR_FLAG_STAT2) && (((flag) | EMB_FLAG_CLR_STAT2_ALL) == EMB_FLAG_CLR_STAT2_ALL)) || \
    (((reg) == EMB_CLR_FLAG_STAT3) && (((flag) | EMB_FLAG_CLR_STAT3_ALL) == EMB_FLAG_CLR_STAT3_ALL))))

#define IS_EMB_RELEASE_PWM_COND(x)                                             \
(   ((x) == EMB_RELEASE_PWM_COND_FLAG_ZERO) ||                                 \
    ((x) == EMB_RELEASE_PWM_COND_STAT_ZERO))

#define IS_EMB_MONITOR_EVT(x)                                                  \
(   ((x) != 0UL)                            &&                                 \
    (((x) | EMB_EVT_ALL) == EMB_EVT_ALL))

#define IS_EMB_XBAR_FILTER_CLK_DIV(x)       ((x) <= 7UL)

#define IS_EMB_XBAR_CH1_STAT(x)                                                \
(   ((x) == EMB_XBAR_CH1_DISABLE)           ||                                 \
    ((x) == EMB_XBAR_CH1_ENABLE))
#define IS_EMB_XBAR_CH1_DETECT_LVL(x)                                          \
(   ((x) == EMB_XBAR_CH1_DETECT_LVL_HIGH)   ||                                 \
    ((x) == EMB_XBAR_CH1_DETECT_LVL_LOW))

#define IS_EMB_XBAR_CH2_STAT(x)                                                \
(   ((x) == EMB_XBAR_CH2_DISABLE)           ||                                 \
    ((x) == EMB_XBAR_CH2_ENABLE))
#define IS_EMB_XBAR_CH2_DETECT_LVL(x)                                          \
(   ((x) == EMB_XBAR_CH2_DETECT_LVL_HIGH)   ||                                 \
    ((x) == EMB_XBAR_CH2_DETECT_LVL_LOW))

#define IS_EMB_XBAR_CH3_STAT(x)                                                \
(   ((x) == EMB_XBAR_CH3_DISABLE)           ||                                 \
    ((x) == EMB_XBAR_CH3_ENABLE))
#define IS_EMB_XBAR_CH3_DETECT_LVL(x)                                          \
(   ((x) == EMB_XBAR_CH3_DETECT_LVL_HIGH)   ||                                 \
    ((x) == EMB_XBAR_CH3_DETECT_LVL_LOW))

#define IS_EMB_XBAR_CH4_STAT(x)                                                \
(   ((x) == EMB_XBAR_CH4_DISABLE)           ||                                 \
    ((x) == EMB_XBAR_CH4_ENABLE))
#define IS_EMB_XBAR_CH4_DETECT_LVL(x)                                          \
(   ((x) == EMB_XBAR_CH4_DETECT_LVL_HIGH)   ||                                 \
    ((x) == EMB_XBAR_CH4_DETECT_LVL_LOW))

#define IS_EMB_XBAR_CH5_STAT(x)                                                \
(   ((x) == EMB_XBAR_CH5_DISABLE)           ||                                 \
    ((x) == EMB_XBAR_CH5_ENABLE))
#define IS_EMB_XBAR_CH5_DETECT_LVL(x)                                          \
(   ((x) == EMB_XBAR_CH5_DETECT_LVL_HIGH)   ||                                 \
    ((x) == EMB_XBAR_CH5_DETECT_LVL_LOW))

#define IS_EMB_XBAR_CH6_STAT(x)                                                \
(   ((x) == EMB_XBAR_CH6_DISABLE)           ||                                 \
    ((x) == EMB_XBAR_CH6_ENABLE))
#define IS_EMB_XBAR_CH6_DETECT_LVL(x)                                          \
(   ((x) == EMB_XBAR_CH6_DETECT_LVL_HIGH)   ||                                 \
    ((x) == EMB_XBAR_CH6_DETECT_LVL_LOW))

#define IS_EMB_DSOGI_PLL_STAT(x)                                               \
(   ((x) == EMB_DSOGI_PLL_DISABLE)          ||                                 \
    ((x) == EMB_DSOGI_PLL_ENABLE))

#define IS_EMB_PLA_OUT(x)                   (((x) != 0UL) && (((x) | EMB_PLA_OUT_ALL) == EMB_PLA_OUT_ALL))
/**
 * @}
 */

#define EMB_PORT1_FILTER_CLK_DIV_MASK       (EMB_CTL2_NFSEL1 | EMB_CTL2_NFSEL1_2)
#define EMB_PORT2_FILTER_CLK_DIV_MASK       (EMB_CTL2_NFSEL2 | EMB_CTL2_NFSEL2_2)
#define EMB_PORT3_FILTER_CLK_DIV_MASK       (EMB_CTL2_NFSEL3 | EMB_CTL2_NFSEL3_2)
#define EMB_PORT4_FILTER_CLK_DIV_MASK       (EMB_CTL2_NFSEL4 | EMB_CTL2_NFSEL4_2)
#define EMB_PORT5_FILTER_CLK_DIV_MASK       (EMB_CTL2_NFSEL5 | EMB_CTL2_NFSEL5_2)
#define EMB_PORT6_FILTER_CLK_DIV_MASK       (EMB_CTL3_NFSEL6 | EMB_CTL3_NFSEL6_2)
#define EMB_PORT7_FILTER_CLK_DIV_MASK       (EMB_CTL3_NFSEL7 | EMB_CTL3_NFSEL7_2)
#define EMB_PORT8_FILTER_CLK_DIV_MASK       (EMB_CTL3_NFSEL8 | EMB_CTL3_NFSEL8_2)
#define EMB_PORT9_FILTER_CLK_DIV_MASK       (EMB_CTL3_NFSEL9 | EMB_CTL3_NFSEL9_2)
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
 * @defgroup EMB_Global_Functions EMB Global Functions
 * @{
 */

/**
 * @brief  Set the fields of structure stc_emb_tmr6_init_t to default values
 * @param  [out] pstcEmbInit            Pointer to a @ref stc_emb_tmr6_init_t structure
 * @retval int32_t:
 *           - LL_OK:                   Initialize successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcEmbInit value is NULL.
 */
int32_t EMB_TMR6_StructInit(stc_emb_tmr6_init_t *pstcEmbInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcEmbInit) {

        /* CMP */
        pstcEmbInit->stcCmp.u32Cmp1State = EMB_CMP1_DISABLE;
        pstcEmbInit->stcCmp.u32Cmp2State = EMB_CMP2_DISABLE;
        pstcEmbInit->stcCmp.u32Cmp3State = EMB_CMP3_DISABLE;
        pstcEmbInit->stcCmp.u32Cmp4State = EMB_CMP4_DISABLE;
        pstcEmbInit->stcCmp.u32Cmp5State = EMB_CMP5_DISABLE;
        pstcEmbInit->stcCmp.u32Cmp6State = EMB_CMP6_DISABLE;
        pstcEmbInit->stcCmp.u32Cmp7State = EMB_CMP7_DISABLE;
        pstcEmbInit->stcCmp.u32Cmp8State = EMB_CMP8_DISABLE;

        /* Port */
        pstcEmbInit->stcPort.stcPort1.u32PortState = EMB_PORT1_DISABLE;
        pstcEmbInit->stcPort.stcPort1.u32PortLevel = EMB_PORT1_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort1.u32PortFilterDiv = EMB_PORT1_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort1.u32PortFilterState = EMB_PORT1_FILTER_DISABLE;
        pstcEmbInit->stcPort.stcPort2.u32PortState = EMB_PORT2_DISABLE;
        pstcEmbInit->stcPort.stcPort2.u32PortLevel = EMB_PORT2_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort2.u32PortFilterDiv = EMB_PORT2_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort2.u32PortFilterState = EMB_PORT2_FILTER_DISABLE;
        pstcEmbInit->stcPort.stcPort3.u32PortState = EMB_PORT3_DISABLE;
        pstcEmbInit->stcPort.stcPort3.u32PortLevel = EMB_PORT3_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort3.u32PortFilterDiv = EMB_PORT3_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort3.u32PortFilterState = EMB_PORT3_FILTER_DISABLE;
        pstcEmbInit->stcPort.stcPort4.u32PortState = EMB_PORT4_DISABLE;
        pstcEmbInit->stcPort.stcPort4.u32PortLevel = EMB_PORT4_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort4.u32PortFilterDiv = EMB_PORT4_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort4.u32PortFilterState = EMB_PORT4_FILTER_DISABLE;
        pstcEmbInit->stcPort.stcPort5.u32PortState = EMB_PORT5_DISABLE;
        pstcEmbInit->stcPort.stcPort5.u32PortLevel = EMB_PORT5_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort5.u32PortFilterDiv = EMB_PORT5_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort5.u32PortFilterState = EMB_PORT5_FILTER_DISABLE;
        pstcEmbInit->stcPort.stcPort6.u32PortState = EMB_PORT6_DISABLE;
        pstcEmbInit->stcPort.stcPort6.u32PortLevel = EMB_PORT6_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort6.u32PortFilterDiv = EMB_PORT6_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort6.u32PortFilterState = EMB_PORT6_FILTER_DISABLE;
        pstcEmbInit->stcPort.stcPort7.u32PortState = EMB_PORT7_DISABLE;
        pstcEmbInit->stcPort.stcPort7.u32PortLevel = EMB_PORT7_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort7.u32PortFilterDiv = EMB_PORT7_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort7.u32PortFilterState = EMB_PORT7_FILTER_DISABLE;
        pstcEmbInit->stcPort.stcPort8.u32PortState = EMB_PORT8_DISABLE;
        pstcEmbInit->stcPort.stcPort8.u32PortLevel = EMB_PORT8_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort8.u32PortFilterDiv = EMB_PORT8_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort8.u32PortFilterState = EMB_PORT8_FILTER_DISABLE;
        pstcEmbInit->stcPort.stcPort9.u32PortState = EMB_PORT9_DISABLE;
        pstcEmbInit->stcPort.stcPort9.u32PortLevel = EMB_PORT9_DETECT_LVL_HIGH;
        pstcEmbInit->stcPort.stcPort9.u32PortFilterDiv = EMB_PORT9_FILTER_CLK_DIV1;
        pstcEmbInit->stcPort.stcPort9.u32PortFilterState = EMB_PORT9_FILTER_DISABLE;

        pstcEmbInit->stcPort.u32PortFilterCount = EMB_PORT_FILTER_CNT_3;
        /* PWM */
        pstcEmbInit->stcTmr6.stcTmr6_1.u32PwmLevel = EMB_DETECT_TMR6_1_PWM_BOTH_LOW;
        pstcEmbInit->stcTmr6.stcTmr6_1.u32PwmState = EMB_TMR6_1_PWM_DISABLE;
        pstcEmbInit->stcTmr6.stcTmr6_2.u32PwmLevel = EMB_DETECT_TMR6_2_PWM_BOTH_LOW;
        pstcEmbInit->stcTmr6.stcTmr6_2.u32PwmState = EMB_TMR6_2_PWM_DISABLE;
        pstcEmbInit->stcTmr6.stcTmr6_3.u32PwmLevel = EMB_DETECT_TMR6_3_PWM_BOTH_LOW;
        pstcEmbInit->stcTmr6.stcTmr6_3.u32PwmState = EMB_TMR6_3_PWM_DISABLE;
        pstcEmbInit->stcTmr6.stcTmr6_4.u32PwmLevel = EMB_DETECT_TMR6_4_PWM_BOTH_LOW;
        pstcEmbInit->stcTmr6.stcTmr6_4.u32PwmState = EMB_TMR6_4_PWM_DISABLE;
        pstcEmbInit->stcTmr6.stcTmr6_5.u32PwmLevel = EMB_DETECT_TMR6_5_PWM_BOTH_LOW;
        pstcEmbInit->stcTmr6.stcTmr6_5.u32PwmState = EMB_TMR6_5_PWM_DISABLE;
        pstcEmbInit->stcTmr6.stcTmr6_6.u32PwmLevel = EMB_DETECT_TMR6_6_PWM_BOTH_LOW;
        pstcEmbInit->stcTmr6.stcTmr6_6.u32PwmState = EMB_TMR6_6_PWM_DISABLE;

        /* System */
        pstcEmbInit->stcSys.u32Osc             = EMB_OSC_DISABLE;
        pstcEmbInit->stcSys.u32SramEccError    = EMB_SRAM_ECC_ERR_DISABLE;
        pstcEmbInit->stcSys.u32Lockup          = EMB_LOCKUP_DISABLE;
        pstcEmbInit->stcSys.u32Lvd             = EMB_LVD_DISABLE;
        pstcEmbInit->stcSys.u32FlashEccError   = EMB_FLASH_ECC_ERR_DISABLE;
        pstcEmbInit->stcSys.u32Fcm             = EMB_FCM_DISABLE;
        pstcEmbInit->stcXbar.stcCh1.u32XbarState = EMB_XBAR_CH1_DISABLE;
        pstcEmbInit->stcXbar.stcCh2.u32XbarState = EMB_XBAR_CH2_DISABLE;
        pstcEmbInit->stcXbar.stcCh3.u32XbarState = EMB_XBAR_CH3_DISABLE;
        pstcEmbInit->stcXbar.stcCh4.u32XbarState = EMB_XBAR_CH4_DISABLE;
        pstcEmbInit->stcXbar.stcCh5.u32XbarState = EMB_XBAR_CH5_DISABLE;
        pstcEmbInit->stcXbar.stcCh6.u32XbarState = EMB_XBAR_CH6_DISABLE;
        pstcEmbInit->stcXbar.stcCh1.u32XbarLevel = EMB_XBAR_CH1_DETECT_LVL_HIGH;
        pstcEmbInit->stcXbar.stcCh2.u32XbarLevel = EMB_XBAR_CH2_DETECT_LVL_HIGH;
        pstcEmbInit->stcXbar.stcCh3.u32XbarLevel = EMB_XBAR_CH3_DETECT_LVL_HIGH;
        pstcEmbInit->stcXbar.stcCh4.u32XbarLevel = EMB_XBAR_CH4_DETECT_LVL_HIGH;
        pstcEmbInit->stcXbar.stcCh5.u32XbarLevel = EMB_XBAR_CH5_DETECT_LVL_HIGH;
        pstcEmbInit->stcXbar.stcCh6.u32XbarLevel = EMB_XBAR_CH6_DETECT_LVL_HIGH;

        pstcEmbInit->stcDsogiPll.u32DsogiPllState  = EMB_DSOGI_PLL_DISABLE;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Initialize EMB for TMR6.
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @param  [in] pstcEmbInit             Pointer to a @ref stc_emb_tmr6_init_t structure
 * @retval int32_t:
 *           - LL_OK:                   Initialize successfully.
 *           - LL_ERR_INVD_PARAM:       The pointer pstcEmbInit value is NULL.
 */
int32_t EMB_TMR6_Init(CM_EMB_TypeDef *EMBx, const stc_emb_tmr6_init_t *pstcEmbInit)
{
    uint32_t u32Reg1Value;
    uint32_t u32Reg2Value;
    uint32_t u32Reg3Value;
    uint32_t u32Reg4Value;
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcEmbInit) {
        DDL_ASSERT(IS_EMB_TMR6_GROUP(EMBx));
        DDL_ASSERT(IS_EMB_CMP1_STAT(pstcEmbInit->stcCmp.u32Cmp1State));
        DDL_ASSERT(IS_EMB_CMP2_STAT(pstcEmbInit->stcCmp.u32Cmp2State));
        DDL_ASSERT(IS_EMB_CMP3_STAT(pstcEmbInit->stcCmp.u32Cmp3State));
        DDL_ASSERT(IS_EMB_CMP4_STAT(pstcEmbInit->stcCmp.u32Cmp4State));
        DDL_ASSERT(IS_EMB_CMP5_STAT(pstcEmbInit->stcCmp.u32Cmp5State));
        DDL_ASSERT(IS_EMB_CMP6_STAT(pstcEmbInit->stcCmp.u32Cmp6State));
        DDL_ASSERT(IS_EMB_CMP7_STAT(pstcEmbInit->stcCmp.u32Cmp7State));
        DDL_ASSERT(IS_EMB_CMP8_STAT(pstcEmbInit->stcCmp.u32Cmp8State));
        DDL_ASSERT(IS_EMB_PORT1_STAT(pstcEmbInit->stcPort.stcPort1.u32PortState));
        DDL_ASSERT(IS_EMB_PORT1_DETECT_LVL(pstcEmbInit->stcPort.stcPort1.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT1_FILTER_DIV(pstcEmbInit->stcPort.stcPort1.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT1_FILTER_STAT(pstcEmbInit->stcPort.stcPort1.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT2_STAT(pstcEmbInit->stcPort.stcPort2.u32PortState));
        DDL_ASSERT(IS_EMB_PORT2_DETECT_LVL(pstcEmbInit->stcPort.stcPort2.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT2_FILTER_DIV(pstcEmbInit->stcPort.stcPort2.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT2_FILTER_STAT(pstcEmbInit->stcPort.stcPort2.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT3_STAT(pstcEmbInit->stcPort.stcPort3.u32PortState));
        DDL_ASSERT(IS_EMB_PORT3_DETECT_LVL(pstcEmbInit->stcPort.stcPort3.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT3_FILTER_DIV(pstcEmbInit->stcPort.stcPort3.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT3_FILTER_STAT(pstcEmbInit->stcPort.stcPort3.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT4_STAT(pstcEmbInit->stcPort.stcPort4.u32PortState));
        DDL_ASSERT(IS_EMB_PORT4_DETECT_LVL(pstcEmbInit->stcPort.stcPort4.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT4_FILTER_DIV(pstcEmbInit->stcPort.stcPort4.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT4_FILTER_STAT(pstcEmbInit->stcPort.stcPort4.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT5_STAT(pstcEmbInit->stcPort.stcPort5.u32PortState));
        DDL_ASSERT(IS_EMB_PORT5_DETECT_LVL(pstcEmbInit->stcPort.stcPort5.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT5_FILTER_DIV(pstcEmbInit->stcPort.stcPort5.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT5_FILTER_STAT(pstcEmbInit->stcPort.stcPort5.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT6_STAT(pstcEmbInit->stcPort.stcPort6.u32PortState));
        DDL_ASSERT(IS_EMB_PORT6_DETECT_LVL(pstcEmbInit->stcPort.stcPort6.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT6_FILTER_DIV(pstcEmbInit->stcPort.stcPort6.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT6_FILTER_STAT(pstcEmbInit->stcPort.stcPort6.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT7_STAT(pstcEmbInit->stcPort.stcPort7.u32PortState));
        DDL_ASSERT(IS_EMB_PORT7_DETECT_LVL(pstcEmbInit->stcPort.stcPort7.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT7_FILTER_DIV(pstcEmbInit->stcPort.stcPort7.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT7_FILTER_STAT(pstcEmbInit->stcPort.stcPort7.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT8_STAT(pstcEmbInit->stcPort.stcPort8.u32PortState));
        DDL_ASSERT(IS_EMB_PORT8_DETECT_LVL(pstcEmbInit->stcPort.stcPort8.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT8_FILTER_DIV(pstcEmbInit->stcPort.stcPort8.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT8_FILTER_STAT(pstcEmbInit->stcPort.stcPort8.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT9_STAT(pstcEmbInit->stcPort.stcPort9.u32PortState));
        DDL_ASSERT(IS_EMB_PORT9_DETECT_LVL(pstcEmbInit->stcPort.stcPort9.u32PortLevel));
        DDL_ASSERT(IS_EMB_PORT9_FILTER_DIV(pstcEmbInit->stcPort.stcPort9.u32PortFilterDiv));
        DDL_ASSERT(IS_EMB_PORT9_FILTER_STAT(pstcEmbInit->stcPort.stcPort9.u32PortFilterState));
        DDL_ASSERT(IS_EMB_PORT_FILTER_CNT(pstcEmbInit->stcPort.u32PortFilterCount));
        DDL_ASSERT(IS_EMB_TMR6_1_PWM_STAT(pstcEmbInit->stcTmr6.stcTmr6_1.u32PwmState));
        DDL_ASSERT(IS_EMB_DETECT_TMR6_1_PWM_LVL(pstcEmbInit->stcTmr6.stcTmr6_1.u32PwmLevel));
        DDL_ASSERT(IS_EMB_TMR6_2_PWM_STAT(pstcEmbInit->stcTmr6.stcTmr6_2.u32PwmState));
        DDL_ASSERT(IS_EMB_DETECT_TMR6_2_PWM_LVL(pstcEmbInit->stcTmr6.stcTmr6_2.u32PwmLevel));
        DDL_ASSERT(IS_EMB_TMR6_3_PWM_STAT(pstcEmbInit->stcTmr6.stcTmr6_3.u32PwmState));
        DDL_ASSERT(IS_EMB_DETECT_TMR6_3_PWM_LVL(pstcEmbInit->stcTmr6.stcTmr6_3.u32PwmLevel));
        DDL_ASSERT(IS_EMB_TMR6_4_PWM_STAT(pstcEmbInit->stcTmr6.stcTmr6_4.u32PwmState));
        DDL_ASSERT(IS_EMB_DETECT_TMR6_4_PWM_LVL(pstcEmbInit->stcTmr6.stcTmr6_4.u32PwmLevel));
        DDL_ASSERT(IS_EMB_TMR6_5_PWM_STAT(pstcEmbInit->stcTmr6.stcTmr6_5.u32PwmState));
        DDL_ASSERT(IS_EMB_DETECT_TMR6_5_PWM_LVL(pstcEmbInit->stcTmr6.stcTmr6_5.u32PwmLevel));
        DDL_ASSERT(IS_EMB_TMR6_6_PWM_STAT(pstcEmbInit->stcTmr6.stcTmr6_6.u32PwmState));
        DDL_ASSERT(IS_EMB_DETECT_TMR6_6_PWM_LVL(pstcEmbInit->stcTmr6.stcTmr6_6.u32PwmLevel));
        DDL_ASSERT(IS_EMB_OSC_STAT(pstcEmbInit->stcSys.u32Osc));
        DDL_ASSERT(IS_EMB_SRAM_ECC_ERR_STAT(pstcEmbInit->stcSys.u32SramEccError));
        DDL_ASSERT(IS_EMB_LOCKUP_STAT(pstcEmbInit->stcSys.u32Lockup));
        DDL_ASSERT(IS_EMB_LVD_STAT(pstcEmbInit->stcSys.u32Lvd));
        DDL_ASSERT(IS_EMB_FLASH_ECC_ERR_STAT(pstcEmbInit->stcSys.u32FlashEccError));
        DDL_ASSERT(IS_EMB_FCM_STAT(pstcEmbInit->stcSys.u32Fcm));
        DDL_ASSERT(IS_EMB_XBAR_CH1_STAT(pstcEmbInit->stcXbar.stcCh1.u32XbarState));
        DDL_ASSERT(IS_EMB_XBAR_CH2_STAT(pstcEmbInit->stcXbar.stcCh2.u32XbarState));
        DDL_ASSERT(IS_EMB_XBAR_CH3_STAT(pstcEmbInit->stcXbar.stcCh3.u32XbarState));
        DDL_ASSERT(IS_EMB_XBAR_CH4_STAT(pstcEmbInit->stcXbar.stcCh4.u32XbarState));
        DDL_ASSERT(IS_EMB_XBAR_CH5_STAT(pstcEmbInit->stcXbar.stcCh5.u32XbarState));
        DDL_ASSERT(IS_EMB_XBAR_CH6_STAT(pstcEmbInit->stcXbar.stcCh6.u32XbarState));
        DDL_ASSERT(IS_EMB_XBAR_CH1_DETECT_LVL(pstcEmbInit->stcXbar.stcCh1.u32XbarLevel));
        DDL_ASSERT(IS_EMB_XBAR_CH2_DETECT_LVL(pstcEmbInit->stcXbar.stcCh2.u32XbarLevel));
        DDL_ASSERT(IS_EMB_XBAR_CH3_DETECT_LVL(pstcEmbInit->stcXbar.stcCh3.u32XbarLevel));
        DDL_ASSERT(IS_EMB_XBAR_CH4_DETECT_LVL(pstcEmbInit->stcXbar.stcCh4.u32XbarLevel));
        DDL_ASSERT(IS_EMB_XBAR_CH5_DETECT_LVL(pstcEmbInit->stcXbar.stcCh5.u32XbarLevel));
        DDL_ASSERT(IS_EMB_XBAR_CH6_DETECT_LVL(pstcEmbInit->stcXbar.stcCh6.u32XbarLevel));
        DDL_ASSERT(IS_EMB_DSOGI_PLL_STAT(pstcEmbInit->stcDsogiPll.u32DsogiPllState));

        u32Reg2Value = 0UL;

        /* PWM */
        u32Reg1Value = (pstcEmbInit->stcTmr6.stcTmr6_1.u32PwmState | pstcEmbInit->stcTmr6.stcTmr6_2.u32PwmState | \
                        pstcEmbInit->stcTmr6.stcTmr6_3.u32PwmState | pstcEmbInit->stcTmr6.stcTmr6_4.u32PwmState | \
                        pstcEmbInit->stcTmr6.stcTmr6_5.u32PwmState | pstcEmbInit->stcTmr6.stcTmr6_6.u32PwmState);
        u32Reg2Value |= (pstcEmbInit->stcTmr6.stcTmr6_1.u32PwmLevel | pstcEmbInit->stcTmr6.stcTmr6_2.u32PwmLevel | \
                         pstcEmbInit->stcTmr6.stcTmr6_3.u32PwmLevel | pstcEmbInit->stcTmr6.stcTmr6_4.u32PwmLevel | \
                         pstcEmbInit->stcTmr6.stcTmr6_5.u32PwmLevel | pstcEmbInit->stcTmr6.stcTmr6_6.u32PwmLevel);

        /* CMP */
        u32Reg1Value |= (pstcEmbInit->stcCmp.u32Cmp1State | pstcEmbInit->stcCmp.u32Cmp2State | \
                         pstcEmbInit->stcCmp.u32Cmp3State | pstcEmbInit->stcCmp.u32Cmp4State);
        u32Reg4Value = (pstcEmbInit->stcCmp.u32Cmp5State | pstcEmbInit->stcCmp.u32Cmp6State | \
                        pstcEmbInit->stcCmp.u32Cmp7State | pstcEmbInit->stcCmp.u32Cmp8State);

        /* PORT */
        u32Reg1Value |= (pstcEmbInit->stcPort.stcPort1.u32PortState | pstcEmbInit->stcPort.stcPort1.u32PortLevel | \
                         pstcEmbInit->stcPort.stcPort2.u32PortState | pstcEmbInit->stcPort.stcPort2.u32PortLevel | \
                         pstcEmbInit->stcPort.stcPort3.u32PortState | pstcEmbInit->stcPort.stcPort3.u32PortLevel | \
                         pstcEmbInit->stcPort.stcPort4.u32PortState | pstcEmbInit->stcPort.stcPort4.u32PortLevel | \
                         pstcEmbInit->stcPort.stcPort5.u32PortState | pstcEmbInit->stcPort.stcPort5.u32PortLevel);
        u32Reg2Value |= (pstcEmbInit->stcPort.stcPort1.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort1.u32PortFilterState | \
                         pstcEmbInit->stcPort.stcPort2.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort2.u32PortFilterState | \
                         pstcEmbInit->stcPort.stcPort3.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort3.u32PortFilterState | \
                         pstcEmbInit->stcPort.stcPort4.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort4.u32PortFilterState | \
                         pstcEmbInit->stcPort.stcPort5.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort5.u32PortFilterState | \
                         pstcEmbInit->stcPort.u32PortFilterCount);
        u32Reg3Value = (pstcEmbInit->stcPort.stcPort6.u32PortState | pstcEmbInit->stcPort.stcPort6.u32PortLevel | \
                        pstcEmbInit->stcPort.stcPort7.u32PortState | pstcEmbInit->stcPort.stcPort7.u32PortLevel | \
                        pstcEmbInit->stcPort.stcPort8.u32PortState | pstcEmbInit->stcPort.stcPort8.u32PortLevel | \
                        pstcEmbInit->stcPort.stcPort9.u32PortState | pstcEmbInit->stcPort.stcPort9.u32PortLevel | \
                        pstcEmbInit->stcPort.stcPort6.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort6.u32PortFilterState | \
                        pstcEmbInit->stcPort.stcPort7.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort7.u32PortFilterState | \
                        pstcEmbInit->stcPort.stcPort8.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort8.u32PortFilterState | \
                        pstcEmbInit->stcPort.stcPort9.u32PortFilterDiv | pstcEmbInit->stcPort.stcPort9.u32PortFilterState);

        u32Reg1Value |= (pstcEmbInit->stcSys.u32Osc    | pstcEmbInit->stcSys.u32SramEccError | \
                         pstcEmbInit->stcSys.u32Lockup | pstcEmbInit->stcSys.u32Lvd          | \
                         pstcEmbInit->stcSys.u32FlashEccError | pstcEmbInit->stcSys.u32Fcm);
        if ((pstcEmbInit->stcSys.u32Osc != 0UL)  || (pstcEmbInit->stcSys.u32SramEccError != 0UL) || \
            (pstcEmbInit->stcSys.u32Lockup != 0UL) || (pstcEmbInit->stcSys.u32Lvd != 0UL) || \
            (pstcEmbInit->stcSys.u32FlashEccError != 0UL) || (pstcEmbInit->stcSys.u32Fcm != 0UL)) {
            u32Reg1Value |= EMB_CTL1_SYSEN;
        }
        u32Reg4Value |= (pstcEmbInit->stcXbar.stcCh1.u32XbarState | pstcEmbInit->stcXbar.stcCh1.u32XbarLevel | \
                         pstcEmbInit->stcXbar.stcCh2.u32XbarState | pstcEmbInit->stcXbar.stcCh2.u32XbarLevel | \
                         pstcEmbInit->stcXbar.stcCh3.u32XbarState | pstcEmbInit->stcXbar.stcCh3.u32XbarLevel | \
                         pstcEmbInit->stcXbar.stcCh4.u32XbarState | pstcEmbInit->stcXbar.stcCh4.u32XbarLevel | \
                         pstcEmbInit->stcXbar.stcCh5.u32XbarState | pstcEmbInit->stcXbar.stcCh5.u32XbarLevel | \
                         pstcEmbInit->stcXbar.stcCh6.u32XbarState | pstcEmbInit->stcXbar.stcCh6.u32XbarLevel | \
                         pstcEmbInit->stcDsogiPll.u32DsogiPllState);

        WRITE_REG32(EMBx->CTL2, u32Reg2Value);
        WRITE_REG32(EMBx->CTL1, u32Reg1Value);
        WRITE_REG32(EMBx->CTL3, u32Reg3Value);
        WRITE_REG32(EMBx->CTL4, u32Reg4Value);
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  De-Initialize EMB function
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @retval None
 */
void EMB_DeInit(CM_EMB_TypeDef *EMBx)
{
    DDL_ASSERT(IS_EMB_GROUP(EMBx));

    WRITE_REG32(EMBx->SOE, 0x00UL);
    WRITE_REG32(EMBx->INTEN, 0x00UL);
    WRITE_REG32(EMBx->RLSSEL, 0x00UL);
    WRITE_REG32(EMBx->STATCLR, EMB_FLAG_CLR_ALL);
    WRITE_REG32(EMBx->STAT2, EMB_FLAG_CLR_STAT2_ALL);
    WRITE_REG32(EMBx->STAT3, EMB_FLAG_CLR_STAT3_ALL);
}

/**
 * @brief  Set the EMB interrupt function
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @param  [in] u32IntType              EMB interrupt source
 *         This parameter can be any composed value of the macros group @ref EMB_Interrupt.
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void EMB_IntCmd(CM_EMB_TypeDef *EMBx, uint32_t u32IntType, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_EMB_GROUP(EMBx));
    DDL_ASSERT(IS_EMB_INT(u32IntType));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(EMBx->INTEN, u32IntType);
    } else {
        CLR_REG32_BIT(EMBx->INTEN, u32IntType);
    }
}

/**
 * @brief  Clear EMB flag status.
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @param  [in] u8ClearFlagReg          Clear EMB flag register.
 *          This parameter can be any one value of the macros group @ref EMB_ClearFlag_Status_Reg.
 * @param  [in] u32Flag                 EMB flag
 *         This parameter can be any composed value(prefix with EMB_FLAG_CLR) of the macros group @ref EMB_Flag_State.
 *         The valid values of this parameter depend on the value of u8ClearFlagReg.
 * @retval None
 */
void EMB_ClearStatus(CM_EMB_TypeDef *EMBx, uint8_t u8ClearFlagReg, uint32_t u32Flag)
{
    /* Check parameters */
    DDL_ASSERT(IS_EMB_GROUP(EMBx));
    DDL_ASSERT(IS_EMB_CLR_FLAG(u8ClearFlagReg, u32Flag));

    if (EMB_CLR_FLAG_STAT2 == u8ClearFlagReg) {
        WRITE_REG32(EMBx->STAT2, u32Flag);
    } else if (EMB_CLR_FLAG_STAT3 == u8ClearFlagReg) {
        WRITE_REG32(EMBx->STAT3, u32Flag);
    } else {
        WRITE_REG32(EMBx->STATCLR, u32Flag);
    }
}

/**
 * @brief  Get EMB flag status.
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @param  [in] u8GetFlagReg            Get EMB status register.
 *          This parameter can be any one value of the macros group @ref EMB_GetFlag_Status_Reg.
 * @param  [in] u32Flag                 EMB flag
 *         This parameter can be any composed value(exclude prefix with EMB_FLAG_CLR) of the macros group @ref EMB_Flag_State.
 *         The valid values of this parameter depend on the value of u8GetFlagReg.
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t EMB_GetStatus(const CM_EMB_TypeDef *EMBx, uint8_t u8GetFlagReg, uint32_t u32Flag)
{
    en_flag_status_t enStatus;

    DDL_ASSERT(IS_EMB_GROUP(EMBx));
    DDL_ASSERT(IS_EMB_FLAG(u8GetFlagReg, u32Flag));

    if (EMB_GET_FLAG_STAT1 == u8GetFlagReg) {
        enStatus = (READ_REG32_BIT(EMBx->STAT1, u32Flag) == 0UL) ? RESET : SET;
    } else if (EMB_GET_FLAG_STAT2 == u8GetFlagReg) {
        enStatus = (READ_REG32_BIT(EMBx->STAT2, u32Flag) == 0UL) ? RESET : SET;
    } else {
        enStatus = (READ_REG32_BIT(EMBx->STAT3, u32Flag) == 0UL) ? RESET : SET;
    }

    return enStatus;
}

/**
 * @brief  Start/stop EMB brake by software control
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void EMB_SWBrake(CM_EMB_TypeDef *EMBx, en_functional_state_t enNewState)
{
    /* Check parameters */
    DDL_ASSERT(IS_EMB_GROUP(EMBx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    WRITE_REG32(EMBx->SOE, enNewState);
}

/**
 * @brief  Set EMB release PWM condition for the specified event.
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @param  [in] u32Event                Monitor event
 *         This parameter can be any composed value of the macros group @ref EMB_Monitor_Event.
 * @param  [in] u32Cond                 Release PWM condition
 *         This parameter can be one of the macros group @ref EMB_Release_TMR_PWM_Condition
 *           @arg EMB_RELEASE_PWM_COND_FLAG_ZERO: Release PWM when flag bit of the specified event is zero
 *           @arg EMB_RELEASE_PWM_COND_STAT_ZERO: Release PWM when state bit of the specified event is zero
 * @retval None
 */
void EMB_SetReleasePwmCond(CM_EMB_TypeDef *EMBx, uint32_t u32Event, uint32_t u32Cond)
{
    DDL_ASSERT(IS_EMB_GROUP(EMBx));
    DDL_ASSERT(IS_EMB_MONITOR_EVT(u32Event));
    DDL_ASSERT(IS_EMB_RELEASE_PWM_COND(u32Cond));

    if (EMB_RELEASE_PWM_COND_FLAG_ZERO == u32Cond) {
        CLR_REG32_BIT(EMBx->RLSSEL, u32Event);
    } else {
        SET_REG32_BIT(EMBx->RLSSEL, u32Event);
    }
}

/**
 * @brief  Set the fields of structure stc_emb_xbar_filter_t to default values
 * @param  [in] pstcEmbXbarFilterInit   Pointer to a @ref stc_emb_xbar_filter_t structure
 * @retval int32_t:
 *           - LL_OK: Initialize successfully.
 *           - LL_ERR_INVD_PARAM: The pointer pstcEmbXbarFilterInit value is NULL.
 */
int32_t EMB_XBAR_FilterStructInit(stc_emb_xbar_filter_t *pstcEmbXbarFilterInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcEmbXbarFilterInit) {
        pstcEmbXbarFilterInit->CTL_f.u32In1FilterDiv = EMB_XBAR_FILTER_CLK_DIV1;
        pstcEmbXbarFilterInit->CTL_f.u32In2FilterDiv = EMB_XBAR_FILTER_CLK_DIV1;
        pstcEmbXbarFilterInit->CTL_f.u32In3FilterDiv = EMB_XBAR_FILTER_CLK_DIV1;
        pstcEmbXbarFilterInit->CTL_f.u32In4FilterDiv = EMB_XBAR_FILTER_CLK_DIV1;
        pstcEmbXbarFilterInit->CTL_f.u32In5FilterDiv = EMB_XBAR_FILTER_CLK_DIV1;
        pstcEmbXbarFilterInit->CTL_f.u32In6FilterDiv = EMB_XBAR_FILTER_CLK_DIV1;
        pstcEmbXbarFilterInit->CTL_f.u32In1FilterState = EMB_XBAR_FILTER_DISABLE;
        pstcEmbXbarFilterInit->CTL_f.u32In2FilterState = EMB_XBAR_FILTER_DISABLE;
        pstcEmbXbarFilterInit->CTL_f.u32In3FilterState = EMB_XBAR_FILTER_DISABLE;
        pstcEmbXbarFilterInit->CTL_f.u32In4FilterState = EMB_XBAR_FILTER_DISABLE;
        pstcEmbXbarFilterInit->CTL_f.u32In5FilterState = EMB_XBAR_FILTER_DISABLE;
        pstcEmbXbarFilterInit->CTL_f.u32In6FilterState = EMB_XBAR_FILTER_DISABLE;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Initialize the filter of the EMB XBAR
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @param  [in] pstcEmbXbarFilterInit   Pointer to a @ref stc_emb_xbar_filter_t structure
 * @retval int32_t:
 *           - LL_OK: Initialize successfully.
 *           - LL_ERR_INVD_PARAM: The pointer pstcEmbXbarFilterInit value is NULL.
 */
int32_t EMB_XBAR_FilterInit(CM_EMB_TypeDef *EMBx, const stc_emb_xbar_filter_t *pstcEmbXbarFilterInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcEmbXbarFilterInit) {
        DDL_ASSERT(IS_EMB_XBAR_FILTER_CLK_DIV(pstcEmbXbarFilterInit->CTL_f.u32In1FilterDiv));
        DDL_ASSERT(IS_EMB_XBAR_FILTER_CLK_DIV(pstcEmbXbarFilterInit->CTL_f.u32In2FilterDiv));
        DDL_ASSERT(IS_EMB_XBAR_FILTER_CLK_DIV(pstcEmbXbarFilterInit->CTL_f.u32In3FilterDiv));
        DDL_ASSERT(IS_EMB_XBAR_FILTER_CLK_DIV(pstcEmbXbarFilterInit->CTL_f.u32In4FilterDiv));
        DDL_ASSERT(IS_EMB_XBAR_FILTER_CLK_DIV(pstcEmbXbarFilterInit->CTL_f.u32In5FilterDiv));
        DDL_ASSERT(IS_EMB_XBAR_FILTER_CLK_DIV(pstcEmbXbarFilterInit->CTL_f.u32In6FilterDiv));

        WRITE_REG32(EMBx->CTL6, pstcEmbXbarFilterInit->CTL);
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Select the PLA output of the EMB
 * @param  [in] EMBx                    Pointer to EMB instance register base
 *         This parameter can be one of the following values:
 *           @arg CM_EMBx:              EMB group instance register base
 * @param  [in] u32PlaOut               Specifies the PLA out index @ref EMB_PLA_OUT_Selection selection
 * @retval None
 */
void EMB_PLA_SelectPlaOut(CM_EMB_TypeDef *EMBx, uint32_t u32PlaOut)
{
    DDL_ASSERT(IS_EMB_PLA_OUT(u32PlaOut));

    WRITE_REG32(EMBx->CTL5, u32PlaOut);
}

/**
 * @}
 */

#endif /* LL_EMB_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
