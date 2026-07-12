/**
 *******************************************************************************
 * @file  hc32_ll_dsogi_pll.h
 * @brief This file contains all the functions prototypes of the DSOGI_PLL driver
 *        library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2026-04-16       CDT             First version
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
#ifndef __HC32_LL_DSOGI_PLL_H__
#define __HC32_LL_DSOGI_PLL_H__

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
 * @addtogroup LL_DSOGI_PLL
 * @{
 */

#if (LL_DSOGI_PLL_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup DSOGI_PLL_Global_Types DSOGI_PLL Global Types
 * @{
 */

/**
 * @brief Base init structure definition
 */
typedef struct {
    float32_t f32Ts;                        /*!< Specifies the sample time. relate of ADC sample frequency */
    float32_t f32BaseFreq;                  /*!< Specifies the base frequency of grid */
} stc_base_init_t;

/**
 * @brief Clark init structure definition
 */
typedef struct {
    uint32_t u32SrcSelect;              /*!< Specifies the input source of clark.
                                             When input source is not abc, a/b/c register prohibit config.
                                             Data can only be written through a/b/c register when clark independent run.
                                             This parameter can be a value of @ref DSOGI_PLL_CLARK_INPUT_SEL */
    uint32_t u32VoltageSelect;          /*!< Specifies the calculation voltage, valid only when three-phase voltage input  .
                                              This parameter can be a value of @ref DSOGI_PLL_CLARK_VOLT_SEL */
    uint32_t u32AdcGain;                /*!< Specifies the gain of adc when adc as clark input, valid only when adc as clark input.
                                             This parameter can be a value of @ref DSOGI_PLL_ADC_GAIN_SEL */
} stc_clark_init_t;

/**
 * @brief Sogi init structure definition
 */
typedef struct {
    uint32_t u32SogiSCSelect;           /*!< Specifies the calculation select of sogi or enable/disable pll sin/cos calculation.
                                             Independent mode: support 1 or 2 calculation; combo mode(use sogi): enable or disable pll sin/cos calculation
                                             This parameter can be a value of @ref DSOGI_PLL_SOGI_SC_SEL */
    uint32_t u32FLLState;               /*!< Enable or disable the frequency lock loop function of SOGI.
                                             When FLL is enable the w of sogi is updated with new value of FLL.
                                             This parameter can be a value of @ref DSOGI_PLL_FLL_SEL */
} stc_sogi_init_t;

/**
 * @brief Park Init structure definition
 */
typedef struct {
    uint32_t u32ThetaSelect;            /*!< Specifies the d/q signal of park output to pll.
                                             When the system is independent mode, it is invalid
                                             This parameter can be a value of @ref DSOGI_PLL_PARK_THETA_SEL */
} stc_park_init_t;

/**
 * @brief Pll Init structure definition
 */
typedef struct {
    uint32_t u32PLLState;               /*!< Enable or disable hardware the phase lock loop function of PLL.
                                             When the system is comb mode and PLL is disable, w and theta be write through sogi's w register and park's theta register.
                                             It is invalid when the system is independent mode.
                                             This parameter can be a value of @ref DSOGI_PLL_PLL_SEL */
    uint32_t u32PIState;                /*!< Enable or disable PI of the phase lock loop function of PLL.
                                             When PI is disable the out frequency of pll equal the base frequency of sogi.
                                             This parameter can be a value of @ref DSOGI_PLL_PI_SEL */
    uint32_t u32JudgeState;             /*!< Enable or disable the voltage-judge function to control PWM.
                                             It is invalid when the system is independent mode.
                                             This parameter details refer @ref DSOGI_PLL_JUDGE_SEL */
    uint32_t u32DelayTime;              /*!< Pll fail delaytime. less than DSOGI_PLL_DELAYTIME_MAX. actual value equal delaytime * 256 */
} stc_pll_init_t;

/**
 * @brief DSOGI_PLL Phase-Locked init structure definition
 */
typedef struct {
    uint32_t u32Func;                   /*!< Specifies the function of DSOGI_PLL.
                                             This parameter can be a value of @ref DSOGI_PLL_COMB_FUNC_SEL */
    stc_clark_init_t stcClarkInit;      /*!< Clark initialize structure.
                                             This parameter details refer @ref stc_clark_init_t */
    stc_sogi_init_t stcSogiInit;        /*!< Sogi initialize structure.
                                             This parameter details refer @ref stc_sogi_init_t */
    stc_park_init_t stcParkInit;        /*!< Park initialize structure.
                                             This parameter details refer @ref stc_park_init_t */
    stc_pll_init_t stcPllInit;          /*!< Pll initialize structure.
                                             This parameter details refer @ref stc_pll_init_t */
    stc_base_init_t stcBaseInit;        /*!< Base initialize structure.
                                             This parameter details refer @ref stc_base_init_t */
} stc_sogi_pll_init_t;

/**
 * @brief Clark configuration structure definition
 */
typedef struct {
    float32_t clark_k;                      /*!< Specifies the coefficient of low pass filter of clark */
} stc_clark_config_t;

/**
 * @brief SOGI configuration structure definition
 */
typedef struct {
    float32_t sogi_k;                       /*!< Specifies the gain coefficient of sogi */

    float32_t sogi_d_k;                     /*!< Specifies the coefficient of low pass filter of d axis*/

    float32_t sogi_q_k;                     /*!< Specifies the coefficient of low pass filter of q axis */

    float32_t sogi_fll_k;                   /*!< Specifies the gain coefficient of d fll */

    float32_t sogi_max;                     /*!< Specifies the upper limit value of signal */

    float32_t sogi_min;                     /*!< Specifies the lower limit value of signal */

    float32_t sogi_w;                       /*!< Specifies the central angular frequency of signal
                                             When MODE is 1 and FLL_EN is 0, SOGI's w comes from the PLL's w register; when MODE is 1 and FLL_EN is 1, or when MODE is 0, SOGI's w comes from SOGI's own w register
                                             When FLL is 1, if iterative operation must configure w on the first Calculation then prohibit configure again until reset sogi. the w of sogi is updated with new value of FLL */
} stc_sogi_config_t;

/**
 * @brief Park configuration structure definition
 */
typedef struct {
    float32_t park_theta;                   /*!< Specifies the phase for park Calculation, range -pi ~ pi
                                             When MODE is 1 and PLL_EN is 1, theta come from PLL theta register. when MODE is 1 and PLL_EN is 0, or when MODE is 0 theta is sourced from own register */
} stc_park_config_t;

/**
 * @brief Pll configuration structure definition
 */
typedef struct {
    float32_t pll_k;                        /*!< Specifies coefficient of low pass filter of pll */

    float32_t pll_kp;                       /*!< Specifies the gain coefficient of pll */

    float32_t pll_ki_ts;                    /*!< Specifies the Integral coefficient of pll */

    float32_t pll_max;                      /*!< Specifies the upper limit value of pi */

    float32_t pll_min;                      /*!< Specifies the lower limit value of pi */

    float32_t pll_err;                      /*!< Specifies the threshold of err value for pll lock phase */

    float32_t pll_theta_cmp1;               /*!< Specifies the lower limit phase for pwm1 event judge */

    float32_t pll_theta_cmp2;               /*!< Specifies the upper limit phase for pwm1 event judge */

    float32_t pll_theta_cmp3;               /*!< Specifies the lower limit phase for pwm2 event judge */

    float32_t pll_theta_cmp4;               /*!< Specifies the upper limit phase for pwm2 event judge */

    float32_t pll_v_cmp;                    /*!< Specifies the threshold voltage for pwm event judge if the JUDGE_EN bit in CR is enable */

    float32_t pll_theta_step;                /*!< Specifies the phase step for period trigger interrupt */

    float32_t pll_w;                        /*!< angular frequency of pll.
                                             When MODE is 1 and PLL_EN is 1, or when MODE is 0 FUNC is 3, the w of PLL is updated with new value of Calculation result
                                             Must configure w on the first Calculation then prohibit configure again until reset PLL. */
} stc_pll_config_t;

/**
 * @brief Clark input structure definition
 */
typedef struct {
    float32_t a;                            /*!< Specifies input value of a phase */
    float32_t b;                            /*!< Specifies input value of b phase */
    float32_t c;                            /*!< Specifies input value of c phase */
} stc_clark_input_t;

/**
 * @brief Clark output structure definition
 */
typedef struct {
    float32_t alpha;                        /*!< Specifies output value of alpha axis*/
    float32_t beta;                         /*!< Specifies output value of beta axis*/
    float32_t a_filter;                     /*!< Specifies output value of a phase filter*/
    float32_t b_filter;                     /*!< Specifies output value of b phase filter*/
    float32_t c_filter;                     /*!< Specifies output value of c phase filter*/
} stc_clark_output_t;

/**
 * @brief Sogi input structure definition
 */
typedef struct {
    float32_t alpha;                        /*!< Specifies input signal of alpha axis */
    float32_t beta;                         /*!< Specifies input signal of beta axis */
} stc_sogi_input_t;

/**
 * @brief Sogi output structure definition
 */
typedef struct {
    float32_t alpha_dv;                     /*!< output signal of alpha axis follow input */
    float32_t alpha_qv;                     /*!< output Quadrature signal of alpha axis follow input */
    float32_t beta_dv;                      /*!< output signal of beta axis follow input */
    float32_t beta_qv;                      /*!< output Quadrature signal of beta axis follow input */
    float32_t alpha_p;                      /*!< output positive signal of alpha axis follow input */
    float32_t beta_p;                       /*!< output positive signal of beta axis follow input */
    float32_t alpha_n;                      /*!< output negative signal of alpha axis follow input */
    float32_t beta_n;                       /*!< output negative signal of beta axis follow input */
} stc_sogi_output_t;

/**
 * @brief Park input structure definition
 */
typedef struct {
    float32_t alpha;                        /*!< Specifies input signal of alpha axis */
    float32_t beta;                         /*!< Specifies input signal of beta axis */
    float32_t theta;                        /*!< Specifies the phase for park Calculation, range -pi ~ pi */
} stc_park_input_t;

/**
 * @brief Park output structure definition
 */
typedef struct {
    float32_t d;                            /*!< Park transform output signal of d axis */
    float32_t q;                            /*!< Park transform output signal of q axis */
    float32_t sin_theta;                    /*!< Sin value of output theta */
    float32_t cos_theta;                    /*!< Cos value of output theta */
} stc_park_output_t;

/**
 * @brief Pll input structure definition
 */
typedef struct {
    float32_t din;                          /*!< Pll input signal */
} stc_pll_input_t;

/**
 * @brief Pll output structure definition
 */
typedef struct {
    float32_t w;                            /*!< angle frequency output of pll */
    float32_t theta;                        /*!< Phase output of pll */
    float32_t p;                            /*!< P value of pll pi */
    float32_t i;                            /*!< I value of pll pi */
    float32_t pi;                           /*!< Output of pll pi */
    float32_t theta_pi;                     /*!< Output theta to park module */
    float32_t freq;                         /*!< Frequency output of pll */
    float32_t din_filter;                   /*!< Filtered input signal of pll */
    float32_t sin_theta;                    /*!< Sin value of output theta */
    float32_t cos_theta;                    /*!< Cos value of output theta */
} stc_pll_output_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup DSOGI_PLL_Global_Macros DSOGI_PLL Global Macros
 * @{
 */

/**
 * @defgroup DSOGI_PLL_COMB_FUNC_SEL DSOGI_PLL Comb Function Select
 * @{
 */
#define DSOGI_PLL_COMB_THREE_PHASE_DSOGI         (DSOGI_PLL_CR_MODE)
#define DSOGI_PLL_COMB_THREE_PHASE_SYNC          (DSOGI_PLL_CR_MODE | DSOGI_PLL_CR_FUNC_0)
#define DSOGI_PLL_COMB_SINGLE_PHASE              (DSOGI_PLL_CR_MODE | DSOGI_PLL_CR_FUNC_1)
#define DSOGI_PLL_COMB_FUN_MASK                  (DSOGI_PLL_CR_MODE | DSOGI_PLL_CR_FUNC)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_FUNC_SEL DSOGI_PLL Function Select
 * @{
 */
#define DSOGI_PLL_THREE_PHASE_DSOGI         (DSOGI_PLL_CR_MODE)
#define DSOGI_PLL_THREE_PHASE_SYNC          (DSOGI_PLL_CR_MODE | DSOGI_PLL_CR_FUNC_0)
#define DSOGI_PLL_SINGLE_PHASE              (DSOGI_PLL_CR_MODE | DSOGI_PLL_CR_FUNC_1)
#define DSOGI_PLL_CLARK                     (0UL)
#define DSOGI_PLL_SOGI                      (DSOGI_PLL_CR_FUNC_0)
#define DSOGI_PLL_PARK                      (DSOGI_PLL_CR_FUNC_1)
#define DSOGI_PLL_PLL                       (DSOGI_PLL_CR_FUNC)
#define DSOGI_PLL_FUN_MASK                  (DSOGI_PLL_CR_MODE | DSOGI_PLL_CR_FUNC)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_CLARK_INPUT_SEL DSOGI_PLL Clark Input Select
 * @{
 */
#define DSOGI_PLL_CLARK_ABC                 (0UL)
#define DSOGI_PLL_CLARK_ADC1_234            (DSOGI_PLL_CR_DIN_SEL)
#define DSOGI_PLL_CLARK_ADC123_2            (DSOGI_PLL_CR_DIN_SEL | DSOGI_PLL_CR_ADC_SEL)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_CLARK_VOLT_SEL DSOGI_PLL Clark Voltage Select
 * @{
 */
#define DSOGI_PLL_CLARK_PHASE_VOLT_AB       (0UL)
#define DSOGI_PLL_CLARK_PHASE_VOLT_BC       (DSOGI_PLL_CR_VOL_SEL_0)
#define DSOGI_PLL_CLARK_PHASE_VOLT_AC       (DSOGI_PLL_CR_VOL_SEL_1)
#define DSOGI_PLL_CLARK_PHASE_VOLT_ABC      (DSOGI_PLL_CR_VOL_SEL)
#define DSOGI_PLL_CLARK_LINE_VOLT_AC_BC     (DSOGI_PLL_CR_LVOL_EN)
#define DSOGI_PLL_CLARK_LINE_VOLT_AB_AC     (DSOGI_PLL_CR_LVOL_EN | DSOGI_PLL_CR_VOL_SEL_0)
#define DSOGI_PLL_CLARK_LINE_VOLT_AB_BC     (DSOGI_PLL_CR_LVOL_EN | DSOGI_PLL_CR_VOL_SEL_1)
#define DSOGI_PLL_CLARK_VOLT_MASK           (DSOGI_PLL_CR_LVOL_EN | DSOGI_PLL_CR_VOL_SEL)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_ADC_GAIN_SEL DSOGI_PLL ADC Gain Select
 * @{
 */
#define DSOGI_PLL_ADC_DIV1                  (0UL)
#define DSOGI_PLL_ADC_DIV256                (DSOGI_PLL_CR_ADC_GN_0)
#define DSOGI_PLL_ADC_DIV1024               (DSOGI_PLL_CR_ADC_GN_1)
#define DSOGI_PLL_ADC_DIV2048               (DSOGI_PLL_CR_ADC_GN)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_SOGI_SC_SEL DSOGI_PLL SOGI Calculation Select or Sin/Cos Calculation Enable/Disable
 * @{
 */
#define DSOGI_PLL_SOGI_CAUL_1               (0UL)
#define DSOGI_PLL_SOGI_CAUL_2               (DSOGI_PLL_CR_SOGI_SC)
#define DSOGI_PLL_SOGI_SIN_COS_DISABLE      (0UL)
#define DSOGI_PLL_SOGI_SIN_COS_ENABLE       (DSOGI_PLL_CR_SOGI_SC)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_FLL_SEL DSOGI_PLL Frequency-Lock Select
 * @{
 */
#define DSOGI_PLL_FLL_DISABLE               (0UL)
#define DSOGI_PLL_FLL_ENABLE                (DSOGI_PLL_CR_FLL_EN)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_PARK_THETA_SEL DSOGI_PLL PARK Theta Select
 * @{
 */
#define DSOGI_PLL_PARK_THETA_Q              (0UL)
#define DSOGI_PLL_PARK_THETA_D              (DSOGI_PLL_CR_THETA_SEL)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_PI_SEL DSOGI_PLL PI Select
 * @{
 */
#define DSOGI_PLL_PI_DISABLE                (0UL)
#define DSOGI_PLL_PI_ENABLE                 (DSOGI_PLL_CR_PI_EN)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_PLL_SEL DSOGI_PLL PLL Select
 * @{
 */
#define DSOGI_PLL_PLL_DISABLE               (0UL)
#define DSOGI_PLL_PLL_ENABLE                (DSOGI_PLL_CR_PLL_EN)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_JUDGE_SEL DSOGI_PLL Judge Select
 * @{
 */
#define DSOGI_PLL_JUDGE_DISABLE             (0UL)
#define DSOGI_PLL_JUDGE_ENABLE              (DSOGI_PLL_CR_JUDGE_EN)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_DELAYTIME DSOGI_PLL DelayTime
 * @{
 */
#define DSOGI_PLL_DELAYTIME_MAX             (0x1FUL)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_INT DSOGI_PLL INT
 * @{
 */
#define DSOGI_PLL_INT_CALC_CPLT             (DSOGI_PLL_CR_INTC1_EN)
#define DSOGI_PLL_INT_PERIOD_CPLT           (DSOGI_PLL_CR_INTC2_EN)
#define DSOGI_PLL_INT_MASK                  (DSOGI_PLL_INT_CALC_CPLT | DSOGI_PLL_INT_PERIOD_CPLT)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_FLAG DSOGI_PLL Flag
 * @{
 */
#define DSOGI_PLL_FLAG_PLL_SC               (DSOGI_PLL_ST_SC_FLAG)
#define DSOGI_PLL_FLAG_HRPWM2               (DSOGI_PLL_ST_HRPWM_ENTB)
#define DSOGI_PLL_FLAG_HRPWM1               (DSOGI_PLL_ST_HRPWM_ENTA)
#define DSOGI_PLL_FLAG_FP                   (DSOGI_PLL_ST_FP_FLAG)
#define DSOGI_PLL_FLAG_PERIOD               (DSOGI_PLL_ST_INTC2_FLAG)
#define DSOGI_PLL_FLAG_CALC                 (DSOGI_PLL_ST_INTC1_FLAG)
#define DSOGI_PLL_FLAG_PHASE_FAIL           (DSOGI_PLL_ST_PLL_FAIL)
#define DSOGI_PLL_FLAG_PHASE_RESULT         (DSOGI_PLL_ST_RES)
#define DSOGI_PLL_FLAG_MASK                 (DSOGI_PLL_FLAG_PLL_SC | DSOGI_PLL_FLAG_HRPWM2 | DSOGI_PLL_FLAG_HRPWM1 | \
                                             DSOGI_PLL_FLAG_FP     | DSOGI_PLL_FLAG_PERIOD | DSOGI_PLL_FLAG_CALC | \
                                             DSOGI_PLL_FLAG_PHASE_FAIL | DSOGI_PLL_FLAG_PHASE_RESULT)
/**
 * @}
 */

/**
 * @defgroup DSOGI_PLL_CLR_FLAG DSOGI_PLL Clear Flag
 * @{
 */
#define DSOGI_PLL_CLR_FLAG_PLL_SC           (DSOGI_PLL_ST_SC_FLAG)
#define DSOGI_PLL_CLR_FLAG_FP               (DSOGI_PLL_ST_FP_FLAG)
#define DSOGI_PLL_CLR_FLAG_PERIOD           (DSOGI_PLL_ST_INTC2_FLAG)
#define DSOGI_PLL_CLR_FLAG_CALC             (DSOGI_PLL_ST_INTC1_FLAG)
#define DSOGI_PLL_CLR_FLAG_PHASE_FAIL       (DSOGI_PLL_ST_PLL_FAIL)
#define DSOGI_PLL_CLR_FLAG_MASK             (DSOGI_PLL_CLR_FLAG_PLL_SC | DSOGI_PLL_CLR_FLAG_FP | \
                                             DSOGI_PLL_CLR_FLAG_PERIOD | DSOGI_PLL_CLR_FLAG_CALC | \
                                             DSOGI_PLL_CLR_FLAG_PHASE_FAIL)
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
 * @addtogroup DSOGI_PLL_Global_Functions
 * @{
 */

/* Initialization and configuration functions */
int32_t DSOGI_PLL_DeInit(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx);

int32_t DSOGI_PLL_StructInit(stc_sogi_pll_init_t *pstcSogiPllInit);
int32_t DSOGI_PLL_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_sogi_pll_init_t *pstcSogiPllInit);

int32_t DSOGI_PLL_Base_StructInit(stc_base_init_t *pstcBaseInit);
int32_t DSOGI_PLL_Base_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_base_init_t *pstcBaseInit);
int32_t DSOGI_PLL_Clark_StructInit(stc_clark_init_t *pstcClarkInit);
int32_t DSOGI_PLL_Clark_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_clark_init_t *pstcClarkInit);
int32_t DSOGI_PLL_Clark_ConfigInit(stc_clark_config_t *pstcClarkConfig);
int32_t DSOGI_PLL_Clark_Config(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_clark_config_t *pstcClarkConfig);
int32_t DSOGI_PLL_Sogi_StructInit(stc_sogi_init_t *pstcSogiInit);
int32_t DSOGI_PLL_Sogi_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_sogi_init_t *pstcSogiInit);
int32_t DSOGI_PLL_Sogi_ConfigInit(stc_sogi_config_t *pstcSogiConfig);
int32_t DSOGI_PLL_Sogi_Config(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_sogi_config_t *pstcSogiConfig);
int32_t DSOGI_PLL_Park_StructInit(stc_park_init_t *pstcParkInit);
int32_t DSOGI_PLL_Park_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_park_init_t *pstcParkInit);
int32_t DSOGI_PLL_Park_ConfigInit(stc_park_config_t *pstcParkConfig);
int32_t DSOGI_PLL_Park_Config(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_park_config_t *pstcParkConfig);
int32_t DSOGI_PLL_Pll_StructInit(stc_pll_init_t *pstcPllInit);
int32_t DSOGI_PLL_Pll_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_pll_init_t *pstcPllInit);
int32_t DSOGI_PLL_Pll_ConfigInit(stc_pll_config_t *pstcPllConfig);
int32_t DSOGI_PLL_Pll_Config(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_pll_config_t *pstcPllConfig);

void DSOGI_PLL_Set_Func(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32Func);
void DSOGI_PLL_PLL_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState);
void DSOGI_PLL_PI_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState);
void DSOGI_PLL_FLL_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState);
void DSOGI_PLL_Judge_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState);
void DSOGI_PLL_SWReset(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState);
void DSOGI_PLL_Start_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState);
void DSOGI_PLL_PLL_SC_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState);

void DSOGI_PLL_Sogi_Calc_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32SogiSelect);
void DSOGI_PLL_Park_Theta_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32ThetaSelect);
void DSOGI_PLL_Clark_Input_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32InputSelect);
void DSOGI_PLL_Clark_Voltage_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32VoltageSelect);
void DSOGI_PLL_Clark_Adc_Gain_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32AdcGainSelect);

void DSOGI_PLL_Set_Delaytime(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32DelayTime);

void DSOGI_PLL_Int_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32IntType, en_functional_state_t enNewState);

en_flag_status_t DSOGI_PLL_GetStatus(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32Flag);
void DSOGI_PLL_ClearStatus(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32Flag);

int32_t DSOGI_PLL_Clark_Input(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_clark_input_t *pstcClarkInput);
int32_t DSOGI_PLL_Clark_Output(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_clark_output_t *pstcClarkOutput);
int32_t DSOGI_PLL_Sogi_Input(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_sogi_input_t *pstcSogiInput);
int32_t DSOGI_PLL_Sogi_Output(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_sogi_output_t *pstcSogiOutput);
int32_t DSOGI_PLL_Park_Input(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_park_input_t *pstcParkInput);
int32_t DSOGI_PLL_Park_Output(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_park_output_t *pstcParkOutput);
int32_t DSOGI_PLL_Pll_Input(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_pll_input_t *pstcPllInput);
int32_t DSOGI_PLL_Pll_Output(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_pll_output_t *pstcPllOutput);
/**
 * @}
 */
#endif /* LL_DSOGI_PLL_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_DSOGI_PLL_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
