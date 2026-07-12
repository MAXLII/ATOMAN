/**
 *******************************************************************************
 * @file  hc32_ll_pid.h
 * @brief This file contains all the functions prototypes of the PID driver
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
#ifndef __HC32_LL_PID_H__
#define __HC32_LL_PID_H__

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
 * @addtogroup LL_PID
 * @{
 */

#if (LL_PID_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup PID_Global_Types PID Global Types
 * @{
 */

/**
 * @brief PID initial structure definition
 */
typedef struct {
    uint32_t u32PidEn;                  /*!< Specifies the PID function.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32CalMode;                /*!< Specifies the PID calculate mode.
                                            This parameter can be a value of @ref PID_Cal_Mode            */
    uint32_t u32IMode;                  /*!< Specifies the PID integrate mode.
                                            This parameter can be a value of @ref PID_Integ_Mode          */
    uint32_t u32DMode;                  /*!< Specifies the PID differential mode.
                                            This parameter can be a value of @ref PID_Diff_Mode           */
    uint32_t u32CoefMode;               /*!< Specifies the PID coefficient mode.
                                            This parameter can be a value of @ref PID_Coef_Mode           */
    uint32_t u32AntiWindupEn;           /*!< Specifies wether use anti windup or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U1P1En;                 /*!< Specifies wether use U1_P1 to calculate U1 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U1P2En;                 /*!< Specifies wether use U1_P2 to calculate U1 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U1P2Sel;                /*!< Specifies the PID U1_P2 data selection.
                                            This parameter can be a value of @ref PID_U1_P2_Data_Sel     */
    uint32_t u32U3P1En;                 /*!< Specifies wether use U3_P1 to calculate U3 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U3P2En;                 /*!< Specifies wether use U3_P2 to calculate U3 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U3P2Wait;              /*!< Specifies wether wait the U3_P2 data ready or not while calculate U3.
                                            This parameter can be a value of @ref PID_U3_P2_Wait_State   */
    uint32_t u32U3P2Sel;                /*!< Specifies the PID U3_P2 data selection.
                                            This parameter can be a value of @ref PID_U3_P2_Data_Sel     */
    uint32_t u32U3P3En;                 /*!< Specifies wether use U3_P3 to calculate U3 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U3P4En;                 /*!< Specifies wether use U3_P4 to calculate U3 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U3P4Wait;              /*!< Specifies wether wait the U3_P4 data ready or not while calculate U3.
                                            This parameter can be a value of @ref PID_U3_P4_Wait_State   */
    uint32_t u32U3P4Type;              /*!< Specifies the PID U3_P4 calculate type.
                                            This parameter can be a value of @ref PID_U3_P4_Cal_Type     */
    uint32_t u32U4P1En;                 /*!< Specifies wether use U4_P1 to calculate U4 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U5P1En;                 /*!< Specifies wether use U5_P1 to calculate U5 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32U5P2En;                 /*!< Specifies wether use U5_P2 to calculate U5 or not.
                                            This parameter can be a value of @ref en_functional_state_t   */
} stc_pid_init_t;

/**
 * @brief PID software start initial structure definition
 */
typedef struct {
    uint32_t u32State;                  /*!< Specifies the state of software start
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32InitEvSel;              /*!< Specifies the initial expected value source
                                            This parameter can be a value of @ref PID_SW_Init_Ev_Sel      */
    float32_t f32FinalEv;               /*!< Specifies the finally expected value                         */
    float32_t f32Step;                  /*!< Specifies the step value of expected value while calculate   */
} stc_pid_sw_init_t;

/**
 * @brief PID gain configure structure definition
 */
typedef struct {
    float32_t f32Kp;                     /*!< Proportional gain */
    float32_t f32Ki;                     /*!< Integral gain     */
    float32_t f32Kd;                     /*!< Derivative gain   */
    float32_t f32Kdf;                    /*!< Derivative feedback gain   */
} stc_pid_gain_config_t;

/**
 * @brief PID gain structure definition
 */
typedef struct {
    float32_t f32Kf;                     /*!< Absolute err coefficient */
    float32_t f32Kp;                     /*!< Proportional gain */
    float32_t f32Ki;                     /*!< Integral gain     */
    float32_t f32Kd;                     /*!< Derivative gain   */
    float32_t f32Kdf;                    /*!< Derivative feedback gain   */
} stc_pid_gain_t;

/**
 * @brief PID gain switch structure definition
 */
typedef struct {
    float32_t f32threshold1;             /*!< Gain switch threshold1  */
    float32_t f32threshold2;             /*!< Gain switch threshold2  */
    float32_t f32Delay1;                 /*!< Gain switch delay1      */
    float32_t f32Delay2;                 /*!< Gain switch delay2      */
} stc_pid_gain_switch_t;

/**
 * @brief PID configure structure definition
 */
typedef struct {
    uint32_t u32AValSel;                /*!< Specifies the PID actual value selection.
                                            This parameter can be a value of @ref PID_AVAL_Sel     */
    uint32_t u32EValSel;                /*!< Specifies the PID expected value selection.
                                            This parameter can be a value of @ref PID_EV_Sel     */
    uint32_t u32EkMinSel;               /*!< Specifies the PID E(k) minimum selection.
                                            This parameter can be a value of @ref PID_Ek_Min_Sel   */
    uint32_t u32KfMinSel;               /*!< Specifies the PID E(k) minimum selection.
                                            This parameter can be a value of @ref PID_Kf_Min_Sel   */
    uint32_t u32KiMinSel;               /*!< Specifies the PID Ki minimum selection.
                                            This parameter can be a value of @ref PID_Ki_Min_Sel   */
    uint32_t u32KdMinSel;               /*!< Specifies the PID Kd minimum selection.
                                            This parameter can be a value of @ref PID_Kd_Min_Sel   */
    uint32_t u32UMinSel;                /*!< Specifies the PID U minimum selection.
                                            This parameter can be a value of @ref PID_U_Min_Sel    */
    uint32_t u32U1MinSel;               /*!< Specifies the PID U1 minimum selection.
                                            This parameter can be a value of @ref PID_U1_Min_Sel   */
    uint32_t u32U3MinSel;               /*!< Specifies the PID U3 minimum selection.
                                            This parameter can be a value of @ref PID_U3_Min_Sel   */
    uint32_t u32U5MinSel;               /*!< Specifies the PID U5 minimum selection.
                                            This parameter can be a value of @ref PID_U5_Min_Sel   */
} stc_pid_config_t;

/**
 * @brief PID value configure structure definition
 */
typedef struct {
    float32_t f32EkMax;                  /*!< Specifies the PID E(k) maximum   */
    float32_t f32EkMin;                  /*!< Specifies the PID E(k) minimum   */
    float32_t f32KiMax;                  /*!< Specifies the PID Ki maximum     */
    float32_t f32KiMin;                  /*!< Specifies the PID Ki minimum     */
    float32_t f32KdMax;                  /*!< Specifies the PID Kd maximum     */
    float32_t f32KdMin;                  /*!< Specifies the PID Kd minimum     */
    float32_t f32UMax;                   /*!< Specifies the PID U maximum      */
    float32_t f32UMin;                   /*!< Specifies the PID U minimum      */
    float32_t f32U1Max;                  /*!< Specifies the PID U1 maximum     */
    float32_t f32U1Min;                  /*!< Specifies the PID U1 minimum     */
    float32_t f32U3Max;                  /*!< Specifies the PID U3 maximum     */
    float32_t f32U3Min;                  /*!< Specifies the PID U3 minimum     */
    float32_t f32U5Max;                  /*!< Specifies the PID U5 maximum     */
    float32_t f32U5Min;                  /*!< Specifies the PID U5 minimum     */
} stc_pid_value_config_t;

/**
 * @brief PID UxPx value structure definition
 */
typedef struct {
    float32_t f32U1P1;                   /*!< Specifies the PID U1P1 value                                       */
    float32_t f32U1P2;                   /*!< Specifies the PID U1P2 value, read only, valid while U1P2_EN = 1   */
    float32_t f32U3P1;                   /*!< Specifies the PID U3P1 value                                       */
    float32_t f32U3P2;                   /*!< Specifies the PID U3P2 value, read only, valid while U3P2_EN = 1   */
    float32_t f32U3P3;                   /*!< Specifies the PID U3P3 value                                       */
    float32_t f32U3P4;                   /*!< Specifies the PID U3P4 value  read only, valid while U3P4_EN = 1   */
    float32_t f32U4P1;                   /*!< Specifies the PID U4P1 value                                       */
    float32_t f32U5P1;                   /*!< Specifies the PID U5P1 value                                       */
    float32_t f32U5P2;                   /*!< Specifies the PID U5P2 value                                       */
} stc_pid_uxpx_t;

/**
 * @brief PID value structure definition
 */
typedef struct {
    float32_t f32Ek;                     /*!< Current E(k)                        */
    float32_t f32PVal;                   /*!< The value after proportion          */
    float32_t f32IVal;                   /*!< The value after integration         */
    float32_t f32DVal;                   /*!< The value after derivative          */
    float32_t f32U;                      /*!< Control value U                     */
    float32_t f32U1;                     /*!< Control value U1                    */
    float32_t f32U2;                     /*!< Control value U2                    */
    float32_t f32U3;                     /*!< Control value U3                    */
    float32_t f32U4;                     /*!< Control value U4                    */
    float32_t f32U5;                     /*!< Control value U5                    */
    uint32_t u32Uf;                     /*!< Control value U fixed-point number  */
    uint32_t u32U5f;                    /*!< Control value U5 fixed-point number */
} stc_pid_value_t;

/**
 * @brief PID compare initial structure definition
 */
typedef struct {
    uint32_t u32CompEn;                 /*!< Specifies the PID competition state.
                                            This parameter can be a value of @ref en_functional_state_t     */
    uint32_t u32CompIntEn;              /*!< Specifies the PID competition complete interrupt state.
                                            This parameter can be a value of @ref en_functional_state_t     */
    uint32_t u32CompOutputSel;          /*!< Specifies the PID competition output selection.
                                            This parameter can be a value of @ref PID_Comp_Output_Sel       */
    uint32_t u32BurstIntEn;             /*!< Specifies the PID burst complete interrupt state.
                                            This parameter can be a value of @ref en_functional_state_t     */
    uint32_t u32BurstFuncSel;           /*!< Specifies the PID burst function selection.
                                            This parameter can be a value of @ref PID_Burst_Function_Sel    */
    uint32_t u32BurstMode;              /*!< Specifies the PID burst mode.
                                            This parameter can be a value of @ref PID_Burst_Mode_Sel        */
    uint32_t u32BurstIntSel;            /*!< Specifies the PID burst interrupt selection.
                                            This parameter can be a value of @ref PID_Burst_Int_Sel         */
} stc_pid_cmp_init_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/**
 * @defgroup PID_Global_Macros PID Global Macros
 * @{
 */

/**
 * @defgroup PID_U1_P2_Data_Sel PID U1 P2 data selection
 * @{
 */
#define PID_U1_P2_DATA_IIR1             (0UL << PID_CR_U1P2_SEL_POS)            /*!< U1_P2 select IIR1  */
#define PID_U1_P2_DATA_IIR2             (1UL << PID_CR_U1P2_SEL_POS)            /*!< U1_P2 select IIR2  */
#define PID_U1_P2_DATA_IIR3             (2UL << PID_CR_U1P2_SEL_POS)            /*!< U1_P2 select IIR3  */
#define PID_U1_P2_DATA_IIR4             (3UL << PID_CR_U1P2_SEL_POS)            /*!< U1_P2 select IIR4  */
#define PID_U1_P2_DATA_IIR5             (5UL << PID_CR_U1P2_SEL_POS)            /*!< U1_P2 select IIR5  */
#define PID_U1_P2_DATA_IIR6             (6UL << PID_CR_U1P2_SEL_POS)            /*!< U1_P2 select IIR6  */
/**
 * @}
 */

/**
 * @defgroup PID_U3_P2_Data_Sel PID U3 P2 data selection
 * @{
 */
#define PID_U3_P2_DATA_SOGI_SIN         (0UL << PID_CR_U3P2_SEL_POS)            /*!< U3_P2 select SOGI sine       */
#define PID_U3_P2_DATA_SOGI_COS         (1UL << PID_CR_U3P2_SEL_POS)            /*!< U3_P2 select SOGI cosine     */
#define PID_U3_P2_DATA_CORDIC_RES1      (2UL << PID_CR_U3P2_SEL_POS)            /*!< U3_P2 select CORDIC result 1 */
#define PID_U3_P2_DATA_CORDIC_RES2      (3UL << PID_CR_U3P2_SEL_POS)            /*!< U3_P2 select CORDIC result 2 */
/**
 * @}
 */

/**
 * @defgroup PID_U3_P2_Wait_State PID wait or not wait wait  U3_P2
 * @{
 */
#define PID_U3_P2_WAIT_OFF              (0UL << PID_CR_U3P4_WAIT_POS)           /*!< Use U3_P2 to calculate U3, not wait the external data */
#define PID_U3_P2_WAIT_ON               (1UL << PID_CR_U3P4_WAIT_POS)           /*!< Wait the external data, then calculate U3             */
/**
 * @}
 */

/**
 * @defgroup PID_U3_P4_Cal_Type PID U3 P4 calculate type
 * @{
 */
#define PID_U3_P4_CAL_ADD               (0UL << PID_CR_U3P4_TYPE_POS)           /*!< Add U3_P4  */
#define PID_U3_P4_CAL_SUB               (1UL << PID_CR_U3P4_TYPE_POS)           /*!< Sub U3_P4  */
/**
 * @}
 */

/**
 * @defgroup PID_U3_P4_Wait_State PID wait or not wait wait  U3_P4
 * @{
 */
#define PID_U3_P4_WAIT_OFF              (0UL << PID_CR_U3P4_WAIT_POS)           /*!< Use U3_P4 to calculate U3, not wait the external data */
#define PID_U3_P4_WAIT_ON               (1UL << PID_CR_U3P4_WAIT_POS)           /*!< Wait the external data, then calculate U3             */
/**
 * @}
 */

/**
 * @defgroup PID_Coef_Mode PID coefficient mode
 * @{
 */
#define PID_COEF_MD_1                   (0UL << PID_CR_COEF_MODE_POS)           /*!< Use 1 coefficient only                              */
#define PID_COEF_MD_2                   (1UL << PID_CR_COEF_MODE_POS)           /*!< Use E(k) and threshold via 3 coefficients           */
#define PID_COEF_MD_3                   (2UL << PID_CR_COEF_MODE_POS)           /*!< Use expected value and threshold via 3 coefficients */
#define PID_COEF_MD_4                   (3UL << PID_CR_COEF_MODE_POS)           /*!< Use fixed coefficient & E(k)                        */
/**
 * @}
 */

/**
 * @defgroup PID_Integ_Mode  PID integrate mode
 * @{
 */
#define PID_INTEG_MD_EK                 (0UL << PID_CR_IMODE_POS)               /*!< PID integrate use E(k)             */
#define PID_INTEG_MD_EK_EK_1            (1UL << PID_CR_IMODE_POS)               /*!< PID integrate use E(k) and E(k-1)  */
/**
 * @}
 */

/**
 * @defgroup PID_Diff_Mode  PID differential mode
 * @{
 */
#define PID_DIFF_MD_WITHOUT_FEEDBACK    (0UL << PID_CR_DMODE_POS)               /*!< PID without differential feedback  */
#define PID_DIFF_MD_WITH_FEEDBACK       (1UL << PID_CR_DMODE_POS)               /*!< PID with differential feedback     */
/**
 * @}
 */

/**
 * @defgroup PID_Cal_Mode PID calculate mode
 * @{
 */
#define PID_CAL_MD_SW                   (0UL << PID_CR_CAL_MODE_POS)            /*!< PID calculate after software start       */
#define PID_CAL_MD_HW_AVAL              (1UL << PID_CR_CAL_MODE_POS)            /*!< PID calculate after write PID AVAL       */
#define PID_CAL_MD_HW_AVAL_REF          (2UL << PID_CR_CAL_MODE_POS)            /*!< PID calculate after write PID AVAL & REF */
/**
 * @}
 */

/**
 * @defgroup PID_AVAL_Sel PID actual value selection
 * @{
 */
#define PID_AVAL_REG                    (0UL << PID_CR2_ASEL_POS)               /*!< PID actual value is PID_AVAL register value */
#define PID_AVAL_IIR1                   (1UL << PID_CR2_ASEL_POS)               /*!< PID actual value is IIR1                    */
#define PID_AVAL_IIR2                   (2UL << PID_CR2_ASEL_POS)               /*!< PID actual value is IIR2                    */
#define PID_AVAL_IIR3                   (3UL << PID_CR2_ASEL_POS)               /*!< PID actual value is IIR3                    */
#define PID_AVAL_IIR4                   (4UL << PID_CR2_ASEL_POS)               /*!< PID actual value is IIR4                    */
#define PID_AVAL_IIR5                   (5UL << PID_CR2_ASEL_POS)               /*!< PID actual value is IIR5                    */
#define PID_AVAL_IIR6                   (6UL << PID_CR2_ASEL_POS)               /*!< PID actual value is IIR6                    */
/**
 * @}
 */

/**
 * @defgroup PID_EV_Sel PID expected value selection
 * @{
 */
#define PID_EV_REG                      (0UL << PID_CR2_DSEL_POS)               /*!< PID expected value is PID_REF register value */
#define PID_EV_P1_U3                    (1UL << PID_CR2_DSEL_POS)               /*!< PID expected value is P1_U3                  */
#define PID_EV_P2_U3                    (2UL << PID_CR2_DSEL_POS)               /*!< PID expected value is P2_U3                  */
#define PID_EV_IIR1                     (3UL << PID_CR2_ASEL_POS)               /*!< PID expected value is IIR1                    */
#define PID_EV_IIR2                     (4UL << PID_CR2_ASEL_POS)               /*!< PID expected value is IIR2                    */
#define PID_EV_IIR3                     (5UL << PID_CR2_ASEL_POS)               /*!< PID expected value is IIR3                    */
#define PID_EV_IIR4                     (6UL << PID_CR2_ASEL_POS)               /*!< PID expected value is IIR4                    */
#define PID_EV_IIR5                     (7UL << PID_CR2_ASEL_POS)               /*!< PID expected value is IIR5                    */
#define PID_EV_IIR6                     (8UL << PID_CR2_ASEL_POS)               /*!< PID expected value is IIR6                    */
/**
 * @}
 */

/**
 * @defgroup PID_Ek_Min_Sel PID KF minimum selection
 * @{
 */
#define PID_EK_MIN_EMIN                 (0UL << PID_CR2_EMIN_SEL_POS)           /*!< If E(k) < PID_EMIN, E(k) = PID_EMIN */
#define PID_EK_MIN_0                    (1UL << PID_CR2_EMIN_SEL_POS)           /*!< If E(k) < PID_EMIN, E(k) = 0        */
/**
 * @}
 */

/**
 * @defgroup PID_Kf_Min_Sel PID Kf minimum selection
 * @{
 */
#define PID_KF_MIN_KFMIN                (0UL << PID_CR2_KFMIN_SEL_POS)          /*!< If KF < PID_KFMIN, KF = PID_KFMIN */
#define PID_KF_MIN_0                    (1UL << PID_CR2_KFMIN_SEL_POS)          /*!< If KF < PID_KFMIN, KF = 0         */
/**
 * @}
 */

/**
 * @defgroup PID_Ki_Min_Sel PID Ki minimum selection
 * @{
 */
#define PID_KI_MIN_IMIN                 (0UL << PID_CR2_IMIN_SEL_POS)           /*!< If Ki < PID_IMIN, Ki = PID_IMIN */
#define PID_KI_MIN_0                    (1UL << PID_CR2_IMIN_SEL_POS)           /*!< If Ki < PID_IMIN, Ki = 0        */
/**
 * @}
 */

/**
 * @defgroup PID_Kd_Min_Sel PID Kd minimum selection
 * @{
 */
#define PID_KD_MIN_DMIN                 (0UL << PID_CR2_DMIN_SEL_POS)           /*!< If Kd < PID_DMIN, Kd = PID_DMIN */
#define PID_KD_MIN_0                    (1UL << PID_CR2_DMIN_SEL_POS)           /*!< If Kd < PID_DMIN, Kd = 0        */
/**
 * @}
 */

/**
 * @defgroup PID_U_Min_Sel PID U minimum selection
 * @{
 */
#define PID_U_MIN_UMIN                  (0UL << PID_CR2_UMIN_SEL_POS)           /*!< If U < PID_UMIN, U = PID_UMIN */
#define PID_U_MIN_0                     (1UL << PID_CR2_UMIN_SEL_POS)           /*!< If U < PID_UMIN, U = 0        */
/**
 * @}
 */

/**
 * @defgroup PID_U1_Min_Sel PID U1 minimum selection
 * @{
 */
#define PID_U1_MIN_U1MIN                (0UL << PID_CR2_U1MIN_SEL_POS)          /*!< If U1 < PID_U1MIN, U1 = PID_U1MIN */
#define PID_U1_MIN_0                    (1UL << PID_CR2_U1MIN_SEL_POS)          /*!< If U1 < PID_U1MIN, U1 = 0         */
/**
 * @}
 */

/**
 * @defgroup PID_U3_Min_Sel PID U3 minimum selection
 * @{
 */
#define PID_U3_MIN_U3MIN                (0UL << PID_CR2_U3MIN_SEL_POS)          /*!< If U3 < PID_U3MIN, U3 = PID_U3MIN */
#define PID_U3_MIN_0                    (1UL << PID_CR2_U3MIN_SEL_POS)          /*!< If U3 < PID_U3MIN, U3 = 0         */
/**
 * @}
 */

/**
 * @defgroup PID_U5_Min_Sel PID U5 minimum selection
 * @{
 */
#define PID_U5_MIN_U5MIN                (0UL << PID_CR2_U5MIN_SEL_POS)          /*!< If U5 < PID_U5MIN, U5 = PID_U5MIN */
#define PID_U5_MIN_0                    (1UL << PID_CR2_U5MIN_SEL_POS)          /*!< If U5 < PID_U5MIN, U5 = 0         */
/**
 * @}
 */

/**
 * @defgroup PID_SW_Init_Ev_Sel PID software initial value selection
 * @{
 */
#define PID_SW_INIT_EV_REF              (0UL << PID_CR2_REF_INIT_POS)           /*!< While software start, the expected value is the written value of register PID_REF */
#define PID_SW_INIT_EV_AVAL             (1UL << PID_CR2_REF_INIT_POS)           /*!< While software start, the expected value is the PID_AVAL                          */
/**
 * @}
 */

/**
 * @defgroup PID_Flag_Sel PID flag selection
 * @{
 */
#define PID_FLAG_DATA_RDY               (1UL << PID_STA_DATA_RDY_POS)           /*!< Data ready to be read                   */
#define PID_FLAG_CAL_OVER               (1UL << PID_STA_CAL_OOR_POS)            /*!< Calculate over run                      */
#define PID_FLAG_CAL_INF                (1UL << PID_STA_CAL_INF_POS)            /*!< There is infinite value while calculate */
#define PID_FLAG_CAL_NAN                (1UL << PID_STA_CAL_NAN_POS)            /*!< There is invalid value while calculate  */
/**
 * @}
 */

/**
 * @defgroup PID_Coef_Num_Sel PID coefficient num selection
 * @{
 */
#define PID_COEF_NUM_1                  (0U)                                    /*!< First coefficient                */
#define PID_COEF_NUM_2                  (1U)                                    /*!< Second coefficient               */
#define PID_COEF_NUM_3                  (2U)                                    /*!< Third coefficient                */
/**
 * @}
 */

/**
 * @defgroup PID_Comp_Output_Sel PID competitive output selection
 * @{
 */
#define PID_COMP_OUTPUT_PID1            (0UL << PID_CMP_CR_CMP_SEL_POS)         /*!< Competitive output PID1 data                           */
#define PID_COMP_OUTPUT_PID2            (1UL << PID_CMP_CR_CMP_SEL_POS)         /*!< Competitive output PID2 data                           */
#define PID_COMP_OUTPUT_MAX             (2UL << PID_CMP_CR_CMP_SEL_POS)         /*!< Competitive output the max data between PID1 and PID2  */
#define PID_COMP_OUTPUT_MIN             (3UL << PID_CMP_CR_CMP_SEL_POS)         /*!< Competitive output the min data between PID1 and PID2  */
/**
 * @}
 */

/**
 * @defgroup PID_Burst_Function_Sel PID burst function selection
 * @{
 */
#define PID_BURST_FUNC_NONE             (0UL << PID_CMP_CR_BSEL_POS)            /*!< Burst function valid                       */
#define PID_BURST_FUNC_PID1             (1UL << PID_CMP_CR_BSEL_POS)            /*!< Burst function select PID1                 */
#define PID_BURST_FUNC_PID2             (2UL << PID_CMP_CR_BSEL_POS)            /*!< Burst function select PID2                 */
#define PID_BURST_FUNC_COMP_OUTPUT      (3UL << PID_CMP_CR_BSEL_POS)            /*!< Burst function select competitive output   */
/**
 * @}
 */

/**
 * @defgroup PID_Burst_Int_Sel PID burst interrupt selection
 * @{
 */
#define PID_BURST_INT_DONE_THRESHOLD    (0UL << PID_CMP_CR_BURST_INT_SEL_POS)   /*!< Burst compare done & data < threshold      */
#define PID_BURST_INT_DONE              (1UL << PID_CMP_CR_BURST_INT_SEL_POS)   /*!< Burst compare done                         */
/**
 * @}
 */

/**
 * @defgroup PID_Burst_Mode_Sel PID burst mode selection
 * @{
 */
#define PID_BURST_MD_1                  (0UL << PID_CMP_CR_BMODE_POS)           /*!< Mode 1, if PCMP < threshold, BMCMP = HRPWM period + 1;     \
                                                                                             if PCMP >= threshold, BMCMP = 0                      */
#define PID_BURST_MD_2                  (1UL << PID_CMP_CR_BMODE_POS)           /*!< Mode 2, if PCMP < threshold, BMCMP = U5, BMPU = threshold; \
                                                                                             if PCMP >= threshold, BMCMP = 0, BMPU = PCMP         */
/**
 * @}
 */

/**
 * @defgroup PID_Comp_Flag PID compete Flag
 * @{
 */
#define PID_COMP_FLAG_CPLT              (PID_CMP_CSTA_CMP_STA)          /*!< compete complete */
#define PID_COMP_FLAG_BURST_STA         (PID_CMP_CSTA_BSTA)             /*!< 1: burst < PID_BTH; 0: burst >= PID_BTH */
#define PID_COMP_FLAG_BURST_CPLT        (PID_CMP_CSTA_BST_DONE)         /*!< Burst complete */
#define PID_COMP_FLAG_RESULT            (PID_CMP_CSTA_UCSEL)            /*!< 2: PID2 data; 1: PID1 data  */
#define PID_COMP_FLAG_ALL               (PID_COMP_FLAG_CPLT | PID_COMP_FLAG_BURST_STA | PID_COMP_FLAG_BURST_CPLT | PID_COMP_FLAG_RESULT)
#define PID_COMP_FLAG_CLR_ALL           (PID_COMP_FLAG_CPLT | PID_COMP_FLAG_BURST_STA | PID_COMP_FLAG_BURST_CPLT)  /*!< Clear all compete/burst flags */
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
 * @addtogroup PID_Global_Functions
 * @{
 */

/**
 * @brief  Write PID burst threshold.
 * @param  [in] PID_CMPx        PID compete unit
 * @param  [in] f32Data         Specifies the threshold
 * @retval None.
 */
__STATIC_INLINE void PID_CMP_WriteBurstThreshold(CM_PID_CMP_TypeDef *PID_CMPx, float32_t f32Data)
{
    WRITE_REG32(PID_CMPx->BTH, *(uint32_t *)(uint32_t)&f32Data);
}

/**
 * @brief  Get PID burst result BMCMP.
 * @param  [in] PID_CMPx        PID compete unit
 * @retval float32_t            The BMCMP value
 */
__STATIC_INLINE float32_t PID_CMP_GetBurstBmcmp(CM_PID_CMP_TypeDef *PID_CMPx)
{
    return *(__IO float32_t *)(uint32_t)&PID_CMPx->BMCMP;
}

/**
 * @brief  Get PID burst result BMPU.
 * @param  [in] PID_CMPx        PID compete unit
 * @retval float32_t            The BMPU value
 */
__STATIC_INLINE float32_t PID_CMP_GetBurstBmpu(CM_PID_CMP_TypeDef *PID_CMPx)
{
    return *(__IO float32_t *)(uint32_t)&PID_CMPx->BMPU;
}

/**
 * @brief  Get PID competitive result.
 * @param  [in] PID_CMPx        PID compete unit
 * @retval float32_t            The competitive result
 */
__STATIC_INLINE float32_t PID_CMP_GetCompResult(CM_PID_CMP_TypeDef *PID_CMPx)
{
    return *(__IO float32_t *)(uint32_t)&PID_CMPx->UC;
}

void PID_Cmd(CM_PID_TypeDef *PIDx, en_functional_state_t enNewState);
void PID_Start(CM_PID_TypeDef *PIDx);
void PID_ClearData(CM_PID_TypeDef *PIDx);
void PID_ResetFsm(CM_PID_TypeDef *PIDx);

void PID_IntCmd(CM_PID_TypeDef *PIDx, en_functional_state_t enNewState);
void PID_SWStartCmd(CM_PID_TypeDef *PIDx, en_functional_state_t enNewState);

int32_t PID_StructInit(stc_pid_init_t *pstcPidInit);
int32_t PID_Init(CM_PID_TypeDef *PIDx, stc_pid_init_t *pstcPidInit);

int32_t PID_SWStructInit(stc_pid_sw_init_t *pstcSWInit);
int32_t PID_SWInit(CM_PID_TypeDef *PIDx, stc_pid_sw_init_t *pstcSWInit);

int32_t PID_ConfigStructInit(stc_pid_config_t *pstcConfig);
int32_t PID_Config(CM_PID_TypeDef *PIDx, stc_pid_config_t *pstcConfig);

int32_t PID_GainStructInit(stc_pid_gain_config_t *pstcGainConfig);
int32_t PID_GainConfig(CM_PID_TypeDef *PIDx, uint8_t u8CoefNum, stc_pid_gain_config_t *pstcGainConfig);

int32_t PID_ReadActualGain(CM_PID_TypeDef *PIDx, stc_pid_gain_t *pstcGain);

int32_t PID_GainSwitchStructInit(stc_pid_gain_switch_t *pstcGainSwitch);
int32_t PID_GainSwitchConfig(CM_PID_TypeDef *PIDx, stc_pid_gain_switch_t *pstcGainSwitch);

int32_t PID_ValueStructInit(stc_pid_value_config_t *pstValueConfig);
int32_t PID_ValueConfig(CM_PID_TypeDef *PIDx, stc_pid_value_config_t *pstValueConfig);
int32_t PID_ReadValue(CM_PID_TypeDef *PIDx, stc_pid_value_t *pstcValue);

void PID_SetExpectedValue(CM_PID_TypeDef *PIDx, float32_t f32Ev);
void PID_SetActualValue(CM_PID_TypeDef *PIDx, float32_t f32Av);

float32_t PID_GetUValue(CM_PID_TypeDef *PIDx);
float32_t PID_GetFinalValue(CM_PID_TypeDef *PIDx);
float32_t PID_GetU5Value(CM_PID_TypeDef *PIDx);

int32_t PID_UxPxStructInit(stc_pid_uxpx_t *pstcUxPx);
int32_t PID_UxPxConfig(CM_PID_TypeDef *PIDx, stc_pid_uxpx_t *pstcUxPx);
int32_t PID_ReadUxPx(CM_PID_TypeDef *PIDx, stc_pid_uxpx_t *pstcUxPx);

en_flag_status_t PID_GetStatus(CM_PID_TypeDef *PIDx, uint32_t u32Flag);
void PID_ClearStatus(CM_PID_TypeDef *PIDx, uint32_t u32Flag);

int32_t PID_CMP_StructInit(stc_pid_cmp_init_t *pstcCmpInit);
int32_t PID_CMP_Init(CM_PID_CMP_TypeDef *PID_CMP, stc_pid_cmp_init_t *pstcCmpInit);

uint32_t PID_CMP_GetStatus(CM_PID_CMP_TypeDef *PID_CMPx, uint32_t u32Flag);
void PID_CMP_ClearStatus(CM_PID_CMP_TypeDef *PID_CMPx, uint32_t u32Flag);

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

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_PID_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
