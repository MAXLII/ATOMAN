/**
 *******************************************************************************
 * @file  hc32_ll_emb.h
 * @brief This file contains all the functions prototypes of the EMB
 *        (Emergency Brake) driver library.
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
#ifndef __HC32_LL_EMB_H__
#define __HC32_LL_EMB_H__

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
 * @addtogroup LL_EMB
 * @{
 */

#if (LL_EMB_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup EMB_Global_Types EMB Global Types
 * @{
 */

/**
 * @brief EMB monitor system exception configuration
 */
typedef struct {
    uint32_t u32Osc;                /*!< Enable or disable EMB detect OSC failure function
                                         This parameter can be a value of @ref EMB_OSC_Selection */
    uint32_t u32SramEccError;       /*!< EMB detect SRAM ECC error function
                                         This parameter can be a value of @ref EMB_SRAM_ECC_Error_Selection */
    uint32_t u32Lockup;             /*!< EMB detect lockup function
                                         This parameter can be a value of @ref EMB_Lockup_Selection */
    uint32_t u32Lvd;                /*!< EMB detect LVD function
                                         This parameter can be a value of @ref EMB_LVD_Selection */
    uint32_t u32Fcm;                /*!< EMB detect FCM error function
                                         This parameter can be a value of @ref EMB_FCM_Selection */
    uint32_t u32FlashEccError;      /*!< EMB detect Flash ECC error function
                                         This parameter can be a value of @ref EMB_Flash_ECC_Error_Selection */
} stc_emb_monitor_sys_t;

/**
 * @brief EMB monitor EMB port configuration
 */
typedef struct {
    uint32_t u32PortState;          /*!< Enable or disable EMB detect port in control function
                                         This parameter can be a value of @ref EMB_Port_Selection */
    uint32_t u32PortLevel;          /*!< EMB detect port level
                                         This parameter can be a value of @ref EMB_Detect_Port_Level */
    uint32_t u32PortFilterDiv;      /*!< EMB port filter division
                                         This parameter can be a value of @ref EMB_Port_Filter_Clock_Division */
    uint32_t u32PortFilterState;    /*!< Enable or disable EMB detect port filter in control function
                                         This parameter can be a value of @ref EMB_Port_Filter_Selection */
} stc_emb_monitor_port_config_t;

/**
 * @brief EMB monitor XBAR configuration
 */
typedef struct {
    uint32_t u32XbarState;          /*!< Enable or disable EMB detect xbar in control function
                                         This parameter can be a value of @ref EMB_XBAR_State_Selection */
    uint32_t u32XbarLevel;          /*!< EMB detect xbar level
                                         This parameter can be a value of @ref EMB_Detect_XBAR_Level */
} stc_emb_monitor_xbar_config_t;

/**
 * @brief EMB XBAR filter_configuration
 */
typedef struct {
    union {
        uint32_t CTL;                         /*!< EMB XBAR filter control register */
        struct {
            uint32_t u32In1FilterDiv   : 3;   /*!< EMB XBAR filter division @ref EMB_XBAR_Filter_Clock_Division */
            uint32_t u32In1FilterState : 1;   /*!< EMB XBAR filter state @ref EMB_XBAR_Filter_State_Selection */
            uint32_t u32In2FilterDiv   : 3;   /*!< EMB XBAR filter division @ref EMB_XBAR_Filter_Clock_Division */
            uint32_t u32In2FilterState : 1;   /*!< EMB XBAR filter state @ref EMB_XBAR_Filter_State_Selection */
            uint32_t u32In3FilterDiv   : 3;   /*!< EMB XBAR filter division @ref EMB_XBAR_Filter_Clock_Division */
            uint32_t u32In3FilterState : 1;   /*!< EMB XBAR filter state @ref EMB_XBAR_Filter_State_Selection */
            uint32_t u32In4FilterDiv   : 3;   /*!< EMB XBAR filter division @ref EMB_XBAR_Filter_Clock_Division */
            uint32_t u32In4FilterState : 1;   /*!< EMB XBAR filter state @ref EMB_XBAR_Filter_State_Selection */
            uint32_t u32In5FilterDiv   : 3;   /*!< EMB XBAR filter division @ref EMB_XBAR_Filter_Clock_Division */
            uint32_t u32In5FilterState : 1;   /*!< EMB XBAR filter state @ref EMB_XBAR_Filter_State_Selection */
            uint32_t u32In6FilterDiv   : 3;   /*!< EMB XBAR filter division @ref EMB_XBAR_Filter_Clock_Division */
            uint32_t u32In6FilterState : 1;   /*!< EMB XBAR filter state @ref EMB_XBAR_Filter_State_Selection */
        } CTL_f;
    };
} stc_emb_xbar_filter_t;

/**
 * @brief EMB monitor PLA configuration
 */
typedef struct {
    uint32_t u32PlaState;           /*!< Enable or disable EMB detect pla in control function
                                         This parameter can be a value of @ref EMB_PLA_OUT_Selection */
} stc_emb_monitor_pla_config_t;

/**
 * @brief EMB monitor DSOGI_PLL configuration
 */
typedef struct {
    uint32_t u32DsogiPllState;      /*!< Enable or disable EMB detect DSOGI_PLL in control function
                                         This parameter can be a value of @ref EMB_DSOGI_PLL_Selection */
} stc_emb_monitor_dsogi_pll_t;

/**
 * @brief EMB monitor PWM configuration
 */
typedef struct {
    uint32_t u32PwmState;   /*!< Enable or disable EMB detect timer same phase function
                                 This parameter can be a value of @ref EMB_Detect_PWM state. */
    uint32_t u32PwmLevel;   /*!< Detect timer polarity level
                                 This parameter can be a value of @ref EMB_Detect_PWM level */
} stc_emb_monitor_tmr_pwm_t;

/**
 * @brief EMB monitor port in configuration
 */
typedef struct {
    stc_emb_monitor_port_config_t stcPort1; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    stc_emb_monitor_port_config_t stcPort2; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    stc_emb_monitor_port_config_t stcPort3; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    stc_emb_monitor_port_config_t stcPort4; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    stc_emb_monitor_port_config_t stcPort5; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    stc_emb_monitor_port_config_t stcPort6; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    stc_emb_monitor_port_config_t stcPort7; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    stc_emb_monitor_port_config_t stcPort8; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    stc_emb_monitor_port_config_t stcPort9; /*!< EMB detect EMB port in function
                                                 This parameter details refer @ref stc_emb_monitor_port_config_t structure */
    uint32_t    u32PortFilterCount;         /*!< EMB detect EMB port filter count
                                                 This parameter can be a value of @ref EMB_Port_Filter_Count_Selection     */
} stc_emb_monitor_port_t;

/**
 * @brief EMB monitor XBAR in configuration
 */
typedef struct {
    stc_emb_monitor_xbar_config_t stcCh1;   /*!< EMB detect TMR6EMB_XBAR function
                                                 This parameter details refer @ref stc_emb_monitor_xbar_config_t structure */
    stc_emb_monitor_xbar_config_t stcCh2;   /*!< EMB detect TMR6EMB_XBAR function
                                                 This parameter details refer @ref stc_emb_monitor_xbar_config_t structure */
    stc_emb_monitor_xbar_config_t stcCh3;   /*!< EMB detect TMR6EMB_XBAR function
                                                 This parameter details refer @ref stc_emb_monitor_xbar_config_t structure */
    stc_emb_monitor_xbar_config_t stcCh4;   /*!< EMB detect TMR6EMB_XBAR function
                                                 This parameter details refer @ref stc_emb_monitor_xbar_config_t structure */
    stc_emb_monitor_xbar_config_t stcCh5;   /*!< EMB detect TMR6EMB_XBAR function
                                                 This parameter details refer @ref stc_emb_monitor_xbar_config_t structure */
    stc_emb_monitor_xbar_config_t stcCh6;   /*!< EMB detect TMR6EMB_XBAR function
                                                 This parameter details refer @ref stc_emb_monitor_xbar_config_t structure */
} stc_emb_monitor_xbar_t;

/**
 * @brief EMB monitor PLA in configuration
 */
typedef struct {
    stc_emb_monitor_pla_config_t stcPlaOut0;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut1;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut2;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut3;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut4;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut5;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut6;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut7;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut8;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut9;    /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut10;   /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut11;   /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut12;   /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut13;   /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut14;   /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
    stc_emb_monitor_pla_config_t stcPlaOut15;   /*!< EMB detect PLA in function
                                                     This parameter details refer @ref stc_emb_monitor_pla_config_t structure */
} stc_emb_monitor_pla_t;

/**
 * @brief EMB monitor CMP configuration
 */
typedef struct {
    uint32_t u32Cmp1State;                  /*!< Enable or disable EMB detect CMP1 result function
                                                 This parameter can be a value of @ref EMB_CMP_Selection */
    uint32_t u32Cmp2State;                  /*!< Enable or disable EMB detect CMP2 result function
                                                 This parameter can be a value of @ref EMB_CMP_Selection */
    uint32_t u32Cmp3State;                  /*!< Enable or disable EMB detect CMP3 result function
                                                 This parameter can be a value of @ref EMB_CMP_Selection */
    uint32_t u32Cmp4State;                  /*!< Enable or disable EMB detect CMP4 result function
                                                 This parameter can be a value of @ref EMB_CMP_Selection */
    uint32_t u32Cmp5State;                  /*!< Enable or disable EMB detect CMP1 result function
                                                 This parameter can be a value of @ref EMB_CMP_Selection */
    uint32_t u32Cmp6State;                  /*!< Enable or disable EMB detect CMP2 result function
                                                 This parameter can be a value of @ref EMB_CMP_Selection */
    uint32_t u32Cmp7State;                  /*!< Enable or disable EMB detect CMP3 result function
                                                 This parameter can be a value of @ref EMB_CMP_Selection */
    uint32_t u32Cmp8State;                  /*!< Enable or disable EMB detect CMP4 result function
                                                 This parameter can be a value of @ref EMB_CMP_Selection */
} stc_emb_monitor_cmp_t;

/**
 * @brief EMB monitor TMR6 configuration
 */
typedef struct {
    stc_emb_monitor_tmr_pwm_t stcTmr6_1;    /*!< EMB detect TMR6 function
                                                 This parameter details refer @ref stc_emb_monitor_tmr_pwm_t structure */
    stc_emb_monitor_tmr_pwm_t stcTmr6_2;    /*!< EMB detect TMR6 function
                                                 This parameter details refer @ref stc_emb_monitor_tmr_pwm_t structure */
    stc_emb_monitor_tmr_pwm_t stcTmr6_3;    /*!< EMB detect TMR6 function
                                                 This parameter details refer @ref stc_emb_monitor_tmr_pwm_t structure */
    stc_emb_monitor_tmr_pwm_t stcTmr6_4;    /*!< EMB detect TMR6 function
                                                 This parameter details refer @ref stc_emb_monitor_tmr_pwm_t structure */
    stc_emb_monitor_tmr_pwm_t stcTmr6_5;    /*!< EMB detect TMR6 function
                                                 This parameter details refer @ref stc_emb_monitor_tmr_pwm_t structure */
    stc_emb_monitor_tmr_pwm_t stcTmr6_6;    /*!< EMB detect TMR6 function
                                                 This parameter details refer @ref stc_emb_monitor_tmr_pwm_t structure */
} stc_emb_monitor_tmr6_t;

/**
 * @brief EMB control TMR6 initialization configuration
 */
typedef struct {
    stc_emb_monitor_cmp_t   stcCmp;     /*!< EMB detect CMP function
                                             This parameter details refer @ref stc_emb_monitor_cmp_t structure */
    stc_emb_monitor_port_t  stcPort;    /*!< EMB detect EMB port function
                                             This parameter details refer @ref stc_emb_monitor_port_t structure */
    stc_emb_monitor_tmr6_t  stcTmr6;    /*!< EMB detect TMR6 function
                                             This parameter details refer @ref stc_emb_monitor_tmr6_t structure */
    stc_emb_monitor_sys_t   stcSys;     /*!< EMB detect System function
                                             This parameter details refer @ref stc_emb_monitor_sys_t structure */
    stc_emb_monitor_xbar_t          stcXbar;        /*!< EMB detect EMB XBAR function
                                                         This parameter details refer @ref stc_emb_monitor_xbar_t structure */
    stc_emb_monitor_dsogi_pll_t     stcDsogiPll;    /*!< EMB detect EMB DSOGI_PLL function
                                                         This parameter details refer @ref stc_emb_monitor_dsogi_pll_t structure */
} stc_emb_tmr6_init_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup EMB_Global_Macros EMB Global Macros
 * @{
 */

/**
 * @defgroup EMB_CMP_Selection EMB CMP Selection
 * @{
 */
#define EMB_CMP1_DISABLE                    (0UL)
#define EMB_CMP2_DISABLE                    (0UL)
#define EMB_CMP3_DISABLE                    (0UL)
#define EMB_CMP4_DISABLE                    (0UL)
#define EMB_CMP5_DISABLE                    (0UL)
#define EMB_CMP6_DISABLE                    (0UL)
#define EMB_CMP7_DISABLE                    (0UL)
#define EMB_CMP8_DISABLE                    (0UL)

#define EMB_CMP1_ENABLE                     (EMB_CTL1_CMPEN1)
#define EMB_CMP2_ENABLE                     (EMB_CTL1_CMPEN2)
#define EMB_CMP3_ENABLE                     (EMB_CTL1_CMPEN3)
#define EMB_CMP4_ENABLE                     (EMB_CTL1_CMPEN4)
#define EMB_CMP5_ENABLE                     (EMB_CTL4_CMPEN5)
#define EMB_CMP6_ENABLE                     (EMB_CTL4_CMPEN6)
#define EMB_CMP7_ENABLE                     (EMB_CTL4_CMPEN7)
#define EMB_CMP8_ENABLE                     (EMB_CTL4_CMPEN8)
/**
 * @}
 */

/**
 * @defgroup EMB_OSC_Selection EMB OSC Selection
 * @{
 */
#define EMB_OSC_DISABLE                     (0UL)
#define EMB_OSC_ENABLE                      (EMB_CTL1_OSCSTPEN)
/**
 * @}
 */

/**
 * @defgroup EMB_SRAM_ECC_Error_Selection EMB SRAM ECC Error Selection
 * @{
 */
#define EMB_SRAM_ECC_ERR_DISABLE            (0UL)
#define EMB_SRAM_ECC_ERR_ENABLE             (EMB_CTL1_SRAMECCERREN)
/**
 * @}
 */

/**
 * @defgroup EMB_Lockup_Selection EMB Lockup Selection
 * @{
 */
#define EMB_LOCKUP_DISABLE                  (0UL)
#define EMB_LOCKUP_ENABLE                   (EMB_CTL1_LOCKUPEN)
/**
 * @}
 */

/**
 * @defgroup EMB_LVD_Selection EMB LVD Selection
 * @{
 */
#define EMB_LVD_DISABLE                     (0UL)
#define EMB_LVD_ENABLE                      (EMB_CTL1_PVDEN)
/**
 * @}
 */

/**
 * @defgroup EMB_FCM_Selection EMB FCM Error Selection
 * @{
 */
#define EMB_FCM_DISABLE                     (0UL)
#define EMB_FCM_ENABLE                      (EMB_CTL1_FCMERREN)
/**
 * @}
 */

/**
 * @defgroup EMB_Flash_ECC_Error_Selection EMB FLASH ECC Error Selection
 * @{
 */
#define EMB_FLASH_ECC_ERR_DISABLE           (0UL)
#define EMB_FLASH_ECC_ERR_ENABLE            (EMB_CTL1_FLASHECCERREN)
/**
 * @}
 */

/**
 * @defgroup EMB_Detect_PWM EMB Detect PWM
 * @{
 */
/**
 * @defgroup EMB_TMR4_PWM_Selection EMB TMR4 PWM Selection
 * @{
 */
/**
 * @}
 */

/**
 * @defgroup EMB_Detect_TMR4_PWM_Level EMB Detect TMR4 PWM Level
 * @{
 */
/**
 * @}
 */

/**
 * @defgroup EMB_TMR6_PWM_Selection EMB TMR6 PWM Selection
 * @{
 */
#define EMB_TMR6_1_PWM_DISABLE              (0UL)
#define EMB_TMR6_2_PWM_DISABLE              (0UL)
#define EMB_TMR6_3_PWM_DISABLE              (0UL)
#define EMB_TMR6_4_PWM_DISABLE              (0UL)
#define EMB_TMR6_5_PWM_DISABLE              (0UL)
#define EMB_TMR6_6_PWM_DISABLE              (0UL)

#define EMB_TMR6_1_PWM_ENABLE               (EMB_CTL1_PWMSEN0)
#define EMB_TMR6_2_PWM_ENABLE               (EMB_CTL1_PWMSEN1)
#define EMB_TMR6_3_PWM_ENABLE               (EMB_CTL1_PWMSEN2)
#define EMB_TMR6_4_PWM_ENABLE               (EMB_CTL1_PWMSEN3)
#define EMB_TMR6_5_PWM_ENABLE               (EMB_CTL1_PWMSEN4)
#define EMB_TMR6_6_PWM_ENABLE               (EMB_CTL1_PWMSEN5)
/**
 * @}
 */

/**
 * @defgroup EMB_Detect_TMR6_PWM_Level EMB Detect TMR6 PWM Level
 * @{
 */
#define EMB_DETECT_TMR6_1_PWM_BOTH_LOW      (0UL)
#define EMB_DETECT_TMR6_2_PWM_BOTH_LOW      (0UL)
#define EMB_DETECT_TMR6_3_PWM_BOTH_LOW      (0UL)
#define EMB_DETECT_TMR6_4_PWM_BOTH_LOW      (0UL)
#define EMB_DETECT_TMR6_5_PWM_BOTH_LOW      (0UL)
#define EMB_DETECT_TMR6_6_PWM_BOTH_LOW      (0UL)

#define EMB_DETECT_TMR6_1_PWM_BOTH_HIGH     (EMB_CTL2_PWMLV0)
#define EMB_DETECT_TMR6_2_PWM_BOTH_HIGH     (EMB_CTL2_PWMLV1)
#define EMB_DETECT_TMR6_3_PWM_BOTH_HIGH     (EMB_CTL2_PWMLV2)
#define EMB_DETECT_TMR6_4_PWM_BOTH_HIGH     (EMB_CTL2_PWMLV3)
#define EMB_DETECT_TMR6_5_PWM_BOTH_HIGH     (EMB_CTL2_PWMLV4)
#define EMB_DETECT_TMR6_6_PWM_BOTH_HIGH     (EMB_CTL2_PWMLV5)
/**
 * @}
 */

/**
 * @}
 */

/**
 * @defgroup EMB_Port_Selection EMB Port Selection
 * @{
 */
#define EMB_PORT1_DISABLE                   (0UL)
#define EMB_PORT2_DISABLE                   (0UL)
#define EMB_PORT3_DISABLE                   (0UL)
#define EMB_PORT4_DISABLE                   (0UL)
#define EMB_PORT5_DISABLE                   (0UL)
#define EMB_PORT6_DISABLE                   (0UL)
#define EMB_PORT7_DISABLE                   (0UL)
#define EMB_PORT8_DISABLE                   (0UL)
#define EMB_PORT9_DISABLE                   (0UL)

#define EMB_PORT1_ENABLE                    (EMB_CTL1_PORTINEN1)
#define EMB_PORT2_ENABLE                    (EMB_CTL1_PORTINEN2)
#define EMB_PORT3_ENABLE                    (EMB_CTL1_PORTINEN3)
#define EMB_PORT4_ENABLE                    (EMB_CTL1_PORTINEN4)
#define EMB_PORT5_ENABLE                    (EMB_CTL1_PORTINEN5)
#define EMB_PORT6_ENABLE                    (EMB_CTL3_PORTINEN6)
#define EMB_PORT7_ENABLE                    (EMB_CTL3_PORTINEN7)
#define EMB_PORT8_ENABLE                    (EMB_CTL3_PORTINEN8)
#define EMB_PORT9_ENABLE                    (EMB_CTL3_PORTINEN9)
/**
 * @}
 */

/**
 * @defgroup EMB_Detect_Port_Level EMB Detect Port Level
 * @{
 */
#define EMB_PORT1_DETECT_LVL_HIGH           (0UL)
#define EMB_PORT2_DETECT_LVL_HIGH           (0UL)
#define EMB_PORT3_DETECT_LVL_HIGH           (0UL)
#define EMB_PORT4_DETECT_LVL_HIGH           (0UL)
#define EMB_PORT5_DETECT_LVL_HIGH           (0UL)
#define EMB_PORT6_DETECT_LVL_HIGH           (0UL)
#define EMB_PORT7_DETECT_LVL_HIGH           (0UL)
#define EMB_PORT8_DETECT_LVL_HIGH           (0UL)
#define EMB_PORT9_DETECT_LVL_HIGH           (0UL)

#define EMB_PORT1_DETECT_LVL_LOW            (EMB_CTL1_INVSEL1)
#define EMB_PORT2_DETECT_LVL_LOW            (EMB_CTL1_INVSEL2)
#define EMB_PORT3_DETECT_LVL_LOW            (EMB_CTL1_INVSEL3)
#define EMB_PORT4_DETECT_LVL_LOW            (EMB_CTL1_INVSEL4)
#define EMB_PORT5_DETECT_LVL_LOW            (EMB_CTL1_INVSEL5)
#define EMB_PORT6_DETECT_LVL_LOW            (EMB_CTL3_INVSEL6)
#define EMB_PORT7_DETECT_LVL_LOW            (EMB_CTL3_INVSEL7)
#define EMB_PORT8_DETECT_LVL_LOW            (EMB_CTL3_INVSEL8)
#define EMB_PORT9_DETECT_LVL_LOW            (EMB_CTL3_INVSEL9)
/**
 * @}
 */

/**
 * @defgroup EMB_Port_Filter_Selection EMB Port Filter Selection
 * @{
 */
#define EMB_PORT1_FILTER_DISABLE            (0UL)
#define EMB_PORT2_FILTER_DISABLE            (0UL)
#define EMB_PORT3_FILTER_DISABLE            (0UL)
#define EMB_PORT4_FILTER_DISABLE            (0UL)
#define EMB_PORT5_FILTER_DISABLE            (0UL)
#define EMB_PORT6_FILTER_DISABLE            (0UL)
#define EMB_PORT7_FILTER_DISABLE            (0UL)
#define EMB_PORT8_FILTER_DISABLE            (0UL)
#define EMB_PORT9_FILTER_DISABLE            (0UL)

#define EMB_PORT1_FILTER_ENABLE             (EMB_CTL2_NFEN1)
#define EMB_PORT2_FILTER_ENABLE             (EMB_CTL2_NFEN2)
#define EMB_PORT3_FILTER_ENABLE             (EMB_CTL2_NFEN3)
#define EMB_PORT4_FILTER_ENABLE             (EMB_CTL2_NFEN4)
#define EMB_PORT5_FILTER_ENABLE             (EMB_CTL2_NFEN5)
#define EMB_PORT6_FILTER_ENABLE             (EMB_CTL3_NFEN6)
#define EMB_PORT7_FILTER_ENABLE             (EMB_CTL3_NFEN7)
#define EMB_PORT8_FILTER_ENABLE             (EMB_CTL3_NFEN8)
#define EMB_PORT9_FILTER_ENABLE             (EMB_CTL3_NFEN9)
/**
 * @}
 */

/**
 * @defgroup EMB_Port_Filter_Clock_Division EMB Port Filter Clock Division
 * @{
 */
#define EMB_PORT1_FILTER_CLK_DIV1           (0UL << EMB_CTL2_NFSEL1_POS)
#define EMB_PORT1_FILTER_CLK_DIV2           ((0UL << EMB_CTL2_NFSEL1_POS) | EMB_CTL2_NFSEL1_2)
#define EMB_PORT1_FILTER_CLK_DIV4           ((1UL << EMB_CTL2_NFSEL1_POS) | EMB_CTL2_NFSEL1_2)
#define EMB_PORT1_FILTER_CLK_DIV8           (1UL << EMB_CTL2_NFSEL1_POS)
#define EMB_PORT1_FILTER_CLK_DIV16          ((2UL << EMB_CTL2_NFSEL1_POS) | EMB_CTL2_NFSEL1_2)
#define EMB_PORT1_FILTER_CLK_DIV32          (2UL << EMB_CTL2_NFSEL1_POS)
#define EMB_PORT1_FILTER_CLK_DIV64          ((3UL << EMB_CTL2_NFSEL1_POS) | EMB_CTL2_NFSEL1_2)
#define EMB_PORT1_FILTER_CLK_DIV128         (3UL << EMB_CTL2_NFSEL1_POS)

#define EMB_PORT2_FILTER_CLK_DIV1           (0UL << EMB_CTL2_NFSEL2_POS)
#define EMB_PORT2_FILTER_CLK_DIV2           ((0UL << EMB_CTL2_NFSEL2_POS) | EMB_CTL2_NFSEL2_2)
#define EMB_PORT2_FILTER_CLK_DIV4           ((1UL << EMB_CTL2_NFSEL2_POS) | EMB_CTL2_NFSEL2_2)
#define EMB_PORT2_FILTER_CLK_DIV8           (1UL << EMB_CTL2_NFSEL2_POS)
#define EMB_PORT2_FILTER_CLK_DIV16          ((2UL << EMB_CTL2_NFSEL2_POS) | EMB_CTL2_NFSEL2_2)
#define EMB_PORT2_FILTER_CLK_DIV32          (2UL << EMB_CTL2_NFSEL2_POS)
#define EMB_PORT2_FILTER_CLK_DIV64          ((3UL << EMB_CTL2_NFSEL2_POS) | EMB_CTL2_NFSEL2_2)
#define EMB_PORT2_FILTER_CLK_DIV128         (3UL << EMB_CTL2_NFSEL2_POS)

#define EMB_PORT3_FILTER_CLK_DIV1           (0UL << EMB_CTL2_NFSEL3_POS)
#define EMB_PORT3_FILTER_CLK_DIV2           ((0UL << EMB_CTL2_NFSEL3_POS) | EMB_CTL2_NFSEL3_2)
#define EMB_PORT3_FILTER_CLK_DIV4           ((1UL << EMB_CTL2_NFSEL3_POS) | EMB_CTL2_NFSEL3_2)
#define EMB_PORT3_FILTER_CLK_DIV8           (1UL << EMB_CTL2_NFSEL3_POS)
#define EMB_PORT3_FILTER_CLK_DIV16          ((2UL << EMB_CTL2_NFSEL3_POS) | EMB_CTL2_NFSEL3_2)
#define EMB_PORT3_FILTER_CLK_DIV32          (2UL << EMB_CTL2_NFSEL3_POS)
#define EMB_PORT3_FILTER_CLK_DIV64          ((3UL << EMB_CTL2_NFSEL3_POS) | EMB_CTL2_NFSEL3_2)
#define EMB_PORT3_FILTER_CLK_DIV128         (3UL << EMB_CTL2_NFSEL3_POS)

#define EMB_PORT4_FILTER_CLK_DIV1           (0UL << EMB_CTL2_NFSEL4_POS)
#define EMB_PORT4_FILTER_CLK_DIV2           ((0UL << EMB_CTL2_NFSEL4_POS) | EMB_CTL2_NFSEL4_2)
#define EMB_PORT4_FILTER_CLK_DIV4           ((1UL << EMB_CTL2_NFSEL4_POS) | EMB_CTL2_NFSEL4_2)
#define EMB_PORT4_FILTER_CLK_DIV8           (1UL << EMB_CTL2_NFSEL4_POS)
#define EMB_PORT4_FILTER_CLK_DIV16          ((2UL << EMB_CTL2_NFSEL4_POS) | EMB_CTL2_NFSEL4_2)
#define EMB_PORT4_FILTER_CLK_DIV32          (2UL << EMB_CTL2_NFSEL4_POS)
#define EMB_PORT4_FILTER_CLK_DIV64          ((3UL << EMB_CTL2_NFSEL4_POS) | EMB_CTL2_NFSEL4_2)
#define EMB_PORT4_FILTER_CLK_DIV128         (3UL << EMB_CTL2_NFSEL4_POS)

#define EMB_PORT5_FILTER_CLK_DIV1           (0UL << EMB_CTL2_NFSEL5_POS)
#define EMB_PORT5_FILTER_CLK_DIV2           ((0UL << EMB_CTL2_NFSEL5_POS) | EMB_CTL2_NFSEL5_2)
#define EMB_PORT5_FILTER_CLK_DIV4           ((1UL << EMB_CTL2_NFSEL5_POS) | EMB_CTL2_NFSEL5_2)
#define EMB_PORT5_FILTER_CLK_DIV8           (1UL << EMB_CTL2_NFSEL5_POS)
#define EMB_PORT5_FILTER_CLK_DIV16          ((2UL << EMB_CTL2_NFSEL5_POS) | EMB_CTL2_NFSEL5_2)
#define EMB_PORT5_FILTER_CLK_DIV32          (2UL << EMB_CTL2_NFSEL5_POS)
#define EMB_PORT5_FILTER_CLK_DIV64          ((3UL << EMB_CTL2_NFSEL5_POS) | EMB_CTL2_NFSEL5_2)
#define EMB_PORT5_FILTER_CLK_DIV128         (3UL << EMB_CTL2_NFSEL5_POS)

#define EMB_PORT6_FILTER_CLK_DIV1           (0UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT6_FILTER_CLK_DIV2           ((0UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT6_FILTER_CLK_DIV4           ((1UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT6_FILTER_CLK_DIV8           (1UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT6_FILTER_CLK_DIV16          ((2UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT6_FILTER_CLK_DIV32          (2UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT6_FILTER_CLK_DIV64          ((3UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT6_FILTER_CLK_DIV128         (3UL << EMB_CTL3_NFSEL6_POS)

#define EMB_PORT7_FILTER_CLK_DIV1           (0UL << EMB_CTL3_NFSEL7_POS)
#define EMB_PORT7_FILTER_CLK_DIV2           ((0UL << EMB_CTL3_NFSEL7_POS) | EMB_CTL3_NFSEL7_2)
#define EMB_PORT7_FILTER_CLK_DIV4           ((1UL << EMB_CTL3_NFSEL7_POS) | EMB_CTL3_NFSEL7_2)
#define EMB_PORT7_FILTER_CLK_DIV8           (1UL << EMB_CTL3_NFSEL7_POS)
#define EMB_PORT7_FILTER_CLK_DIV16          ((2UL << EMB_CTL3_NFSEL7_POS) | EMB_CTL3_NFSEL7_2)
#define EMB_PORT7_FILTER_CLK_DIV32          (2UL << EMB_CTL3_NFSEL7_POS)
#define EMB_PORT7_FILTER_CLK_DIV64          ((3UL << EMB_CTL3_NFSEL7_POS) | EMB_CTL3_NFSEL7_2)
#define EMB_PORT7_FILTER_CLK_DIV128         (3UL << EMB_CTL3_NFSEL7_POS)

#define EMB_PORT8_FILTER_CLK_DIV1           (0UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT8_FILTER_CLK_DIV2           ((0UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT8_FILTER_CLK_DIV4           ((1UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT8_FILTER_CLK_DIV8           (1UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT8_FILTER_CLK_DIV16          ((2UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT8_FILTER_CLK_DIV32          (2UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT8_FILTER_CLK_DIV64          ((3UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT8_FILTER_CLK_DIV128         (3UL << EMB_CTL3_NFSEL6_POS)

#define EMB_PORT9_FILTER_CLK_DIV1           (0UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT9_FILTER_CLK_DIV2           ((0UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT9_FILTER_CLK_DIV4           ((1UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT9_FILTER_CLK_DIV8           (1UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT9_FILTER_CLK_DIV16          ((2UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT9_FILTER_CLK_DIV32          (2UL << EMB_CTL3_NFSEL6_POS)
#define EMB_PORT9_FILTER_CLK_DIV64          ((3UL << EMB_CTL3_NFSEL6_POS) | EMB_CTL3_NFSEL6_2)
#define EMB_PORT9_FILTER_CLK_DIV128         (3UL << EMB_CTL3_NFSEL6_POS)
/**
 * @}
 */

/**
 * @defgroup EMB_Port_Filter_Count_Selection EMB Port Filter Count Selection
 * @{
 */
#define EMB_PORT_FILTER_CNT_2               (EMB_CTL2_NFMD)
#define EMB_PORT_FILTER_CNT_3               (0U)
/**
 * @}
 */

/**
 * @defgroup EMB_XBAR_State_Selection EMB XBAR State Selection
 * @{
 */
#define EMB_XBAR_CH1_DISABLE                (0UL)
#define EMB_XBAR_CH2_DISABLE                (0UL)
#define EMB_XBAR_CH3_DISABLE                (0UL)
#define EMB_XBAR_CH4_DISABLE                (0UL)
#define EMB_XBAR_CH5_DISABLE                (0UL)
#define EMB_XBAR_CH6_DISABLE                (0UL)

#define EMB_XBAR_CH1_ENABLE                 (EMB_CTL4_XBAREN1)
#define EMB_XBAR_CH2_ENABLE                 (EMB_CTL4_XBAREN2)
#define EMB_XBAR_CH3_ENABLE                 (EMB_CTL4_XBAREN3)
#define EMB_XBAR_CH4_ENABLE                 (EMB_CTL4_XBAREN4)
#define EMB_XBAR_CH5_ENABLE                 (EMB_CTL4_XBAREN5)
#define EMB_XBAR_CH6_ENABLE                 (EMB_CTL4_XBAREN6)
/**
 * @}
 */

/**
 * @defgroup EMB_Detect_XBAR_Level EMB Detect XBAR Level
 * @{
 */
#define EMB_XBAR_CH1_DETECT_LVL_HIGH        (0UL)
#define EMB_XBAR_CH2_DETECT_LVL_HIGH        (0UL)
#define EMB_XBAR_CH3_DETECT_LVL_HIGH        (0UL)
#define EMB_XBAR_CH4_DETECT_LVL_HIGH        (0UL)
#define EMB_XBAR_CH5_DETECT_LVL_HIGH        (0UL)
#define EMB_XBAR_CH6_DETECT_LVL_HIGH        (0UL)

#define EMB_XBAR_CH1_DETECT_LVL_LOW         (EMB_CTL4_XBARISEL1)
#define EMB_XBAR_CH2_DETECT_LVL_LOW         (EMB_CTL4_XBARISEL2)
#define EMB_XBAR_CH3_DETECT_LVL_LOW         (EMB_CTL4_XBARISEL3)
#define EMB_XBAR_CH4_DETECT_LVL_LOW         (EMB_CTL4_XBARISEL4)
#define EMB_XBAR_CH5_DETECT_LVL_LOW         (EMB_CTL4_XBARISEL5)
#define EMB_XBAR_CH6_DETECT_LVL_LOW         (EMB_CTL4_XBARISEL6)
/**
 * @}
 */

/**
 * @defgroup EMB_XBAR_Filter_State_Selection EMB XBAR Filter State Selection
 * @{
 */
#define EMB_XBAR_FILTER_DISABLE             (0UL)
#define EMB_XBAR_FILTER_ENABLE              (1UL)
/**
 * @}
 */

/**
 * @defgroup EMB_XBAR_Filter_Clock_Division EMB XBAR Filter Clock Division
 * @{
 */
#define EMB_XBAR_FILTER_CLK_DIV1            (0UL)
#define EMB_XBAR_FILTER_CLK_DIV2            (4UL)
#define EMB_XBAR_FILTER_CLK_DIV4            (5UL)
#define EMB_XBAR_FILTER_CLK_DIV8            (1UL)
#define EMB_XBAR_FILTER_CLK_DIV16           (6UL)
#define EMB_XBAR_FILTER_CLK_DIV32           (2UL)
#define EMB_XBAR_FILTER_CLK_DIV64           (7UL)
#define EMB_XBAR_FILTER_CLK_DIV128          (3UL)
/**
 * @}
 */

/**
 * @defgroup EMB_PLA_OUT_Selection EMB PLA_OUT Selection
 * @{
 */
#define EMB_PLA_OUT0                        (EMB_CTL5_PLAEN0)
#define EMB_PLA_OUT1                        (EMB_CTL5_PLAEN1)
#define EMB_PLA_OUT2                        (EMB_CTL5_PLAEN2)
#define EMB_PLA_OUT3                        (EMB_CTL5_PLAEN3)
#define EMB_PLA_OUT4                        (EMB_CTL5_PLAEN4)
#define EMB_PLA_OUT5                        (EMB_CTL5_PLAEN5)
#define EMB_PLA_OUT6                        (EMB_CTL5_PLAEN6)
#define EMB_PLA_OUT7                        (EMB_CTL5_PLAEN7)
#define EMB_PLA_OUT8                        (EMB_CTL5_PLAEN8)
#define EMB_PLA_OUT9                        (EMB_CTL5_PLAEN9)
#define EMB_PLA_OUT10                       (EMB_CTL5_PLAEN10)
#define EMB_PLA_OUT11                       (EMB_CTL5_PLAEN11)
#define EMB_PLA_OUT12                       (EMB_CTL5_PLAEN12)
#define EMB_PLA_OUT13                       (EMB_CTL5_PLAEN13)
#define EMB_PLA_OUT14                       (EMB_CTL5_PLAEN14)
#define EMB_PLA_OUT15                       (EMB_CTL5_PLAEN15)
#define EMB_PLA_OUT_ALL                     (0xFFFFUL)
/**
 * @}
 */

/**
 * @defgroup EMB_DSOGI_PLL_Selection EMB DSOGI_PLL Selection
 * @{
 */
#define EMB_DSOGI_PLL_DISABLE               (0UL)
#define EMB_DSOGI_PLL_ENABLE                (EMB_CTL4_DSOGI_PLLEN)
/**
 * @}
 */

/**
 * @defgroup EMB_Flag_State EMB Flag State
 * @{
 */
#define EMB_FLAG_PWMS                       (EMB_STAT1_PWMSF)
#define EMB_FLAG_CMP                        (EMB_STAT1_CMPF)
#define EMB_FLAG_SYS                        (EMB_STAT1_SYSF)
#define EMB_STAT_PWMS                       (EMB_STAT1_PWMST)
#define EMB_STAT_CMP                        (EMB_STAT1_CMPST)
#define EMB_STAT_SYS                        (EMB_STAT1_SYSST)
#define EMB_FLAG_PORT1                      (EMB_STAT1_PORTINF1)
#define EMB_FLAG_PORT2                      (EMB_STAT1_PORTINF2)
#define EMB_FLAG_PORT3                      (EMB_STAT1_PORTINF3)
#define EMB_FLAG_PORT4                      (EMB_STAT1_PORTINF4)
#define EMB_FLAG_PORT5                      (EMB_STAT1_PORTINF5)
#define EMB_FLAG_FCM                        (EMB_STAT1_FCMERRF)
#define EMB_STAT_PORT1                      (EMB_STAT1_PORTINST1)
#define EMB_STAT_PORT2                      (EMB_STAT1_PORTINST2)
#define EMB_STAT_PORT3                      (EMB_STAT1_PORTINST3)
#define EMB_STAT_PORT4                      (EMB_STAT1_PORTINST4)
#define EMB_STAT_PORT5                      (EMB_STAT1_PORTINST5)
#define EMB_FLAG_OSC                        (EMB_STAT1_OSCF)
#define EMB_FLAG_SRAM_ECC                   (EMB_STAT1_SRAMECCERRF)
#define EMB_FLAG_FLASH_ECC                  (EMB_STAT1_FLASHECCERRF)
#define EMB_FLAG_LOCKUP                     (EMB_STAT1_LOCKUPF)
#define EMB_FLAG_PVD                        (EMB_STAT1_PVDF)
#define EMB_FLAG_CMP1                       (EMB_STAT1_CMP1F)
#define EMB_FLAG_CMP2                       (EMB_STAT1_CMP2F)
#define EMB_FLAG_CMP3                       (EMB_STAT1_CMP3F)
#define EMB_FLAG_CMP4                       (EMB_STAT1_CMP4F)
#define EMB_FLAG_CMP5                       (EMB_STAT1_CMP5F)
#define EMB_FLAG_CMP6                       (EMB_STAT1_CMP6F)
#define EMB_FLAG_CMP7                       (EMB_STAT1_CMP7F)
#define EMB_FLAG_CMP8                       (EMB_STAT1_CMP8F)
#define EMB_FLAG_STAT1_MASK                 (0xFFFFFFEEUL)

#define EMB_FLAG_PORT6                      (EMB_STAT2_PORTINF6)
#define EMB_FLAG_PORT7                      (EMB_STAT2_PORTINF7)
#define EMB_FLAG_PORT8                      (EMB_STAT2_PORTINF8)
#define EMB_FLAG_PORT9                      (EMB_STAT2_PORTINF9)
#define EMB_STAT_PORT6                      (EMB_STAT2_PORTINST6)
#define EMB_STAT_PORT7                      (EMB_STAT2_PORTINST7)
#define EMB_STAT_PORT8                      (EMB_STAT2_PORTINST8)
#define EMB_STAT_PORT9                      (EMB_STAT2_PORTINST9)
#define EMB_FLAG_DSOGI_PLL                  (EMB_STAT2_DSOGI_PLLF)
#define EMB_STAT_DSOGI_PLL                  (EMB_STAT2_DSOGI_PLLST)
#define EMB_STAT_XBAR_CH1                   (EMB_STAT2_XBARST1)
#define EMB_STAT_XBAR_CH2                   (EMB_STAT2_XBARST2)
#define EMB_STAT_XBAR_CH3                   (EMB_STAT2_XBARST3)
#define EMB_STAT_XBAR_CH4                   (EMB_STAT2_XBARST4)
#define EMB_STAT_XBAR_CH5                   (EMB_STAT2_XBARST5)
#define EMB_STAT_XBAR_CH6                   (EMB_STAT2_XBARST6)
#define EMB_FLAG_XBAR_CH1                   (EMB_STAT2_XBARF1)
#define EMB_FLAG_XBAR_CH2                   (EMB_STAT2_XBARF2)
#define EMB_FLAG_XBAR_CH3                   (EMB_STAT2_XBARF3)
#define EMB_FLAG_XBAR_CH4                   (EMB_STAT2_XBARF4)
#define EMB_FLAG_XBAR_CH5                   (EMB_STAT2_XBARF5)
#define EMB_FLAG_XBAR_CH6                   (EMB_STAT2_XBARF6)
#define EMB_FLAG_STAT2_MASK                 (0x3F3FC1EFUL)

#define EMB_STAT_PLA_OUT0                   (EMB_STAT3_PLAST0)
#define EMB_STAT_PLA_OUT1                   (EMB_STAT3_PLAST1)
#define EMB_STAT_PLA_OUT2                   (EMB_STAT3_PLAST2)
#define EMB_STAT_PLA_OUT3                   (EMB_STAT3_PLAST3)
#define EMB_STAT_PLA_OUT4                   (EMB_STAT3_PLAST4)
#define EMB_STAT_PLA_OUT5                   (EMB_STAT3_PLAST5)
#define EMB_STAT_PLA_OUT6                   (EMB_STAT3_PLAST6)
#define EMB_STAT_PLA_OUT7                   (EMB_STAT3_PLAST7)
#define EMB_STAT_PLA_OUT8                   (EMB_STAT3_PLAST8)
#define EMB_STAT_PLA_OUT9                   (EMB_STAT3_PLAST9)
#define EMB_STAT_PLA_OUT10                  (EMB_STAT3_PLAST10)
#define EMB_STAT_PLA_OUT11                  (EMB_STAT3_PLAST11)
#define EMB_STAT_PLA_OUT12                  (EMB_STAT3_PLAST12)
#define EMB_STAT_PLA_OUT13                  (EMB_STAT3_PLAST13)
#define EMB_STAT_PLA_OUT14                  (EMB_STAT3_PLAST14)
#define EMB_STAT_PLA_OUT15                  (EMB_STAT3_PLAST15)
#define EMB_FLAG_PLA_OUT0                   (EMB_STAT3_PLAF0)
#define EMB_FLAG_PLA_OUT1                   (EMB_STAT3_PLAF1)
#define EMB_FLAG_PLA_OUT2                   (EMB_STAT3_PLAF2)
#define EMB_FLAG_PLA_OUT3                   (EMB_STAT3_PLAF3)
#define EMB_FLAG_PLA_OUT4                   (EMB_STAT3_PLAF4)
#define EMB_FLAG_PLA_OUT5                   (EMB_STAT3_PLAF5)
#define EMB_FLAG_PLA_OUT6                   (EMB_STAT3_PLAF6)
#define EMB_FLAG_PLA_OUT7                   (EMB_STAT3_PLAF7)
#define EMB_FLAG_PLA_OUT8                   (EMB_STAT3_PLAF8)
#define EMB_FLAG_PLA_OUT9                   (EMB_STAT3_PLAF9)
#define EMB_FLAG_PLA_OUT10                  (EMB_STAT3_PLAF10)
#define EMB_FLAG_PLA_OUT11                  (EMB_STAT3_PLAF11)
#define EMB_FLAG_PLA_OUT12                  (EMB_STAT3_PLAF12)
#define EMB_FLAG_PLA_OUT13                  (EMB_STAT3_PLAF13)
#define EMB_FLAG_PLA_OUT14                  (EMB_STAT3_PLAF14)
#define EMB_FLAG_PLA_OUT15                  (EMB_STAT3_PLAF15)
#define EMB_FLAG_STAT3_MASK                 (0xFFFFFFFFUL)

#define EMB_FLAG_CLR_DSOGI_PLL              (EMB_STAT2_DSOGI_PLLF)
#define EMB_FLAG_CLR_XBAR_CH1               (EMB_STAT2_XBARF1)
#define EMB_FLAG_CLR_XBAR_CH2               (EMB_STAT2_XBARF2)
#define EMB_FLAG_CLR_XBAR_CH3               (EMB_STAT2_XBARF3)
#define EMB_FLAG_CLR_XBAR_CH4               (EMB_STAT2_XBARF4)
#define EMB_FLAG_CLR_XBAR_CH5               (EMB_STAT2_XBARF5)
#define EMB_FLAG_CLR_XBAR_CH6               (EMB_STAT2_XBARF6)
#define EMB_FLAG_CLR_STAT2_ALL              (0x3F008000UL)

#define EMB_FLAG_CLR_PLA_OUT0               (EMB_STAT3_PLAF0)
#define EMB_FLAG_CLR_PLA_OUT1               (EMB_STAT3_PLAF1)
#define EMB_FLAG_CLR_PLA_OUT2               (EMB_STAT3_PLAF2)
#define EMB_FLAG_CLR_PLA_OUT3               (EMB_STAT3_PLAF3)
#define EMB_FLAG_CLR_PLA_OUT4               (EMB_STAT3_PLAF4)
#define EMB_FLAG_CLR_PLA_OUT5               (EMB_STAT3_PLAF5)
#define EMB_FLAG_CLR_PLA_OUT6               (EMB_STAT3_PLAF6)
#define EMB_FLAG_CLR_PLA_OUT7               (EMB_STAT3_PLAF7)
#define EMB_FLAG_CLR_PLA_OUT8               (EMB_STAT3_PLAF8)
#define EMB_FLAG_CLR_PLA_OUT9               (EMB_STAT3_PLAF9)
#define EMB_FLAG_CLR_PLA_OUT10              (EMB_STAT3_PLAF10)
#define EMB_FLAG_CLR_PLA_OUT11              (EMB_STAT3_PLAF11)
#define EMB_FLAG_CLR_PLA_OUT12              (EMB_STAT3_PLAF12)
#define EMB_FLAG_CLR_PLA_OUT13              (EMB_STAT3_PLAF13)
#define EMB_FLAG_CLR_PLA_OUT14              (EMB_STAT3_PLAF14)
#define EMB_FLAG_CLR_PLA_OUT15              (EMB_STAT3_PLAF15)
#define EMB_FLAG_CLR_STAT3_ALL              (0xFFFF0000UL)

#define EMB_FLAG_CLR_PWMS                   (EMB_STATCLR_PWMSFCLR)
#define EMB_FLAG_CLR_CMP                    (EMB_STATCLR_CMPFCLR)
#define EMB_FLAG_CLR_SYS                    (EMB_STATCLR_SYSFCLR)
#define EMB_FLAG_CLR_PORT1                  (EMB_STATCLR_PORTINFCLR1)
#define EMB_FLAG_CLR_PORT2                  (EMB_STATCLR_PORTINFCLR2)
#define EMB_FLAG_CLR_PORT3                  (EMB_STATCLR_PORTINFCLR3)
#define EMB_FLAG_CLR_PORT4                  (EMB_STATCLR_PORTINFCLR4)
#define EMB_FLAG_CLR_PORT5                  (EMB_STATCLR_PORTINFCLR5)
#define EMB_FLAG_CLR_PORT6                  (EMB_STATCLR_PORTINFCLR6)
#define EMB_FLAG_CLR_PORT7                  (EMB_STATCLR_PORTINFCLR7)
#define EMB_FLAG_CLR_PORT8                  (EMB_STATCLR_PORTINFCLR8)
#define EMB_FLAG_CLR_PORT9                  (EMB_STATCLR_PORTINFCLR9)
#define EMB_FLAG_CLR_FCM                    (EMB_STATCLR_FCMERRFCLR)
#define EMB_FLAG_CLR_OSC                    (EMB_STATCLR_OSCFCLR)
#define EMB_FLAG_CLR_SRAM_ECC               (EMB_STATCLR_SRAMECCERRFCLR)
#define EMB_FLAG_CLR_FLASH_ECC              (EMB_STATCLR_FLASHECCERRFCLR)
#define EMB_FLAG_CLR_LOCKUP                 (EMB_STATCLR_LOCKUPFCLR)
#define EMB_FLAG_CLR_PVD                    (EMB_STATCLR_PVDFCLR)
#define EMB_FLAG_CLR_CMP1                   (EMB_STATCLR_CMP1FCLR)
#define EMB_FLAG_CLR_CMP2                   (EMB_STATCLR_CMP2FCLR)
#define EMB_FLAG_CLR_CMP3                   (EMB_STATCLR_CMP3FCLR)
#define EMB_FLAG_CLR_CMP4                   (EMB_STATCLR_CMP4FCLR)
#define EMB_FLAG_CLR_CMP5                   (EMB_STATCLR_CMP5FCLR)
#define EMB_FLAG_CLR_CMP6                   (EMB_STATCLR_CMP6FCLR)
#define EMB_FLAG_CLR_CMP7                   (EMB_STATCLR_CMP7FCLR)
#define EMB_FLAG_CLR_CMP8                   (EMB_STATCLR_CMP8FCLR)
#define EMB_FLAG_CLR_ALL                    (0xFFFDFF0EUL)
/**
 * @}
 */

/**
 * @defgroup EMB_GetFlag_Status_Reg EMB Get Flag Status Register
 * @{
 */
#define EMB_GET_FLAG_STAT1                  (0U)    /*!< EMB flag status1 register.
                                                         This parameter corresponding flag mask: EMB_FLAG_STAT1_MASK */
#define EMB_GET_FLAG_STAT2                  (1U)    /*!< EMB flag status2 register.
                                                         This parameter corresponding flag mask: EMB_FLAG_STAT2_MASK */
#define EMB_GET_FLAG_STAT3                  (2U)    /*!< EMB flag status3 register.
                                                         This parameter corresponding flag mask: EMB_FLAG_STAT3_MASK */
/**
 * @}
 */

/**
 * @defgroup EMB_ClearFlag_Status_Reg EMB Clear Flag Status Register
 * @{
 */
#define EMB_CLR_FLAG_STAT2                  (1U)    /*!< EMB clear flag status2 register.
                                                         This parameter corresponding flag mask: EMB_FLAG_CLR_STAT2_ALL */
#define EMB_CLR_FLAG_STAT3                  (2U)    /*!< EMB clear flag status3 register.
                                                         This parameter corresponding flag mask: EMB_FLAG_CLR_STAT3_ALL */
#define EMB_CLR_FLAG_STATCLR                (0xFFU) /*!< EMB clear flag register.
                                                         This parameter corresponding flag mask: EMB_FLAG_CLR_ALL */
/**
 * @}
 */

/**
 * @defgroup EMB_Interrupt EMB Interrupt
 * @{
 */
#define EMB_INT_PWMS                        (EMB_INTEN_PWMSINTEN)
#define EMB_INT_CMP                         (EMB_INTEN_CMPINTEN)
#define EMB_INT_SYS                         (EMB_INTEN_SYSINTEN)
#define EMB_INT_PORT1                       (EMB_INTEN_PORTININTEN1)
#define EMB_INT_PORT2                       (EMB_INTEN_PORTININTEN2)
#define EMB_INT_PORT3                       (EMB_INTEN_PORTININTEN3)
#define EMB_INT_PORT4                       (EMB_INTEN_PORTININTEN4)
#define EMB_INT_PORT5                       (EMB_INTEN_PORTININTEN5)
#define EMB_INT_PORT6                       (EMB_INTEN_PORTININTEN6)
#define EMB_INT_PORT7                       (EMB_INTEN_PORTININTEN7)
#define EMB_INT_PORT8                       (EMB_INTEN_PORTININTEN8)
#define EMB_INT_PORT9                       (EMB_INTEN_PORTININTEN9)
#define EMB_INT_XBAR                        (EMB_INTEN_XBARINTEN)
#define EMB_INT_PLA                         (EMB_INTEN_PLAINTEN)
#define EMB_INT_DSOGI_PLL                   (EMB_INTEN_DSOGI_PLLINTEN)
#define EMB_INT_ALL                         (EMB_INT_PWMS  | EMB_INT_CMP    | EMB_INT_SYS   | EMB_INT_PORT1 | \
                                             EMB_INT_PORT2 | EMB_INT_PORT3  | EMB_INT_PORT4 | EMB_INT_PORT5 | \
                                             EMB_INT_PORT6 | EMB_INT_PORT7  | EMB_INT_PORT8 | EMB_INT_PORT9 | \
                                             EMB_INT_XBAR  | EMB_INT_PLA    | EMB_INT_DSOGI_PLL)
/**
 * @}
 */

/**
 * @defgroup EMB_Release_TMR_PWM_Condition EMB Release TMR PWM Condition
 * @{
 */
#define EMB_RELEASE_PWM_COND_FLAG_ZERO      (0UL)
#define EMB_RELEASE_PWM_COND_STAT_ZERO      (1UL)
/**
 * @}
 */

/**
 * @defgroup EMB_Monitor_Event EMB Monitor Event
 * @{
 */
#define EMB_EVT_PWMS                        (EMB_RLSSEL_PWMRSEL)
#define EMB_EVT_CMP                         (EMB_RLSSEL_CMPRSEL)
#define EMB_EVT_SYS                         (EMB_RLSSEL_SYSRSEL)
#define EMB_EVT_PORT1                       (EMB_RLSSEL_PORTINRSEL1)
#define EMB_EVT_PORT2                       (EMB_RLSSEL_PORTINRSEL2)
#define EMB_EVT_PORT3                       (EMB_RLSSEL_PORTINRSEL3)
#define EMB_EVT_PORT4                       (EMB_RLSSEL_PORTINRSEL4)
#define EMB_EVT_PORT5                       (EMB_RLSSEL_PORTINRSEL5)
#define EMB_EVT_PORT6                       (EMB_RLSSEL_PORTINRSEL6)
#define EMB_EVT_PORT7                       (EMB_RLSSEL_PORTINRSEL7)
#define EMB_EVT_PORT8                       (EMB_RLSSEL_PORTINRSEL8)
#define EMB_EVT_PORT9                       (EMB_RLSSEL_PORTINRSEL9)
#define EMB_EVT_XBAR                        (EMB_RLSSEL_XBARRSEL)
#define EMB_EVT_PLA                         (EMB_RLSSEL_PLARSEL)
#define EMB_EVT_DSOGI_PLL                   (EMB_RLSSEL_DSOGI_PLLRSEL)
#define EMB_EVT_ALL                         (EMB_EVT_PWMS   | EMB_EVT_CMP   | EMB_EVT_SYS   | EMB_EVT_PORT1 | \
                                            EMB_EVT_PORT2   | EMB_EVT_PORT3 | EMB_EVT_PORT4 | EMB_EVT_PORT5 | \
                                            EMB_EVT_PORT6   | EMB_EVT_PORT7 | EMB_EVT_PORT8 | EMB_EVT_PORT9 | \
                                            EMB_EVT_XBAR    | EMB_EVT_PLA   | EMB_EVT_DSOGI_PLL)
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
 * @addtogroup EMB_Global_Functions
 * @{
 */

int32_t EMB_TMR6_StructInit(stc_emb_tmr6_init_t *pstcEmbInit);
int32_t EMB_TMR6_Init(CM_EMB_TypeDef *EMBx, const stc_emb_tmr6_init_t *pstcEmbInit);

void EMB_DeInit(CM_EMB_TypeDef *EMBx);
void EMB_IntCmd(CM_EMB_TypeDef *EMBx, uint32_t u32IntType, en_functional_state_t enNewState);
void EMB_ClearStatus(CM_EMB_TypeDef *EMBx, uint8_t u8ClearFlagReg, uint32_t u32Flag);
en_flag_status_t EMB_GetStatus(const CM_EMB_TypeDef *EMBx, uint8_t u8GetFlagReg, uint32_t u32Flag);
void EMB_SWBrake(CM_EMB_TypeDef *EMBx, en_functional_state_t enNewState);

void EMB_SetReleasePwmCond(CM_EMB_TypeDef *EMBx, uint32_t u32Event, uint32_t u32Cond);

int32_t EMB_XBAR_FilterStructInit(stc_emb_xbar_filter_t *pstcEmbXbarFilterInit);
int32_t EMB_XBAR_FilterInit(CM_EMB_TypeDef *EMBx, const stc_emb_xbar_filter_t *pstcEmbXbarFilterInit);

void EMB_PLA_SelectPlaOut(CM_EMB_TypeDef *EMBx, uint32_t u32PlaOut);
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

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_EMB_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
