/**
 *******************************************************************************
 * @file  hc32_ll_gpio.h
 * @brief This file contains all the functions prototypes of the GPIO driver
 *        library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-01-15       CDT             First version
   2025-11-03       CDT             Modify API paramater type: uint16_t->uint32_t
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
#ifndef __HC32_LL_GPIO_H__
#define __HC32_LL_GPIO_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"

#include "hc32f3xx.h"
#include "hc32f3xx_conf.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup LL_GPIO
 * @{
 */

#if (LL_GPIO_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup GPIO_Global_Types GPIO Global Types
 * @{
 */

/**
 * @brief  GPIO Pin Set and Reset enumeration
 */
typedef enum {
    PIN_RESET = 0U,           /*!< Pin reset    */
    PIN_SET   = 1U            /*!< Pin set      */
} en_pin_state_t;

/**
 * @brief  GPIO Init structure definition
 */
typedef struct {
    uint16_t u16PinState;       /*!< Set pin state to High or Low, @ref GPIO_PinState_Sel for details       */
    uint16_t u16PinDir;         /*!< Pin mode setting, @ref GPIO_PinDirection_Sel for details               */
    uint16_t u16PinOutputType;  /*!< Output type setting, @ref GPIO_PinOutType_Sel for details              */
    uint16_t u16PinDrv;         /*!< Pin drive capacity setting, @ref GPIO_PinDrv_Sel for details           */
    uint16_t u16Latch;          /*!< Pin latch setting, @ref GPIO_PinLatch_Sel for details                  */
    uint16_t u16PullUp;         /*!< Internal pull-up resistor setting, @ref GPIO_PinPU_Sel for details     */
    uint16_t u16PullDown;       /*!< Internal pull-down register setting, @ref GPIO_PinPD_Sel for details   */
    uint16_t u16InputMos;       /*!< Input MOS always on selection, @ref GPIO_PinInputMOS_Sel for details   */
    uint16_t u16Invert;         /*!< Pin input/output invert setting, @ref GPIO_PinInvert_Sel for details   */
    uint16_t u16ExtInt;         /*!< External interrupt pin setting, @ref GPIO_PinExtInt_Sel for details    */
    uint16_t u16PinInputType;   /*!< Input type setting, @ref GPIO_PinInType_Sel for details                */
    uint16_t u16PinAttr;        /*!< Digital or analog attribute setting, @ref GPIO_PinMode_Sel for details */
} stc_gpio_init_t;
/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup GPIO_Global_Macros GPIO Global Macros
 * @{
 */

/**
 * @defgroup GPIO_Pins_Define GPIO Pin Source
 * @{
 */
#define GPIO_PIN_00                 (0x0001U)  /*!< Pin 00 selected   */
#define GPIO_PIN_01                 (0x0002U)  /*!< Pin 01 selected   */
#define GPIO_PIN_02                 (0x0004U)  /*!< Pin 02 selected   */
#define GPIO_PIN_03                 (0x0008U)  /*!< Pin 03 selected   */
#define GPIO_PIN_04                 (0x0010U)  /*!< Pin 04 selected   */
#define GPIO_PIN_05                 (0x0020U)  /*!< Pin 05 selected   */
#define GPIO_PIN_06                 (0x0040U)  /*!< Pin 06 selected   */
#define GPIO_PIN_07                 (0x0080U)  /*!< Pin 07 selected   */
#define GPIO_PIN_08                 (0x0100U)  /*!< Pin 08 selected   */
#define GPIO_PIN_09                 (0x0200U)  /*!< Pin 09 selected   */
#define GPIO_PIN_10                 (0x0400U)  /*!< Pin 10 selected   */
#define GPIO_PIN_11                 (0x0800U)  /*!< Pin 11 selected   */
#define GPIO_PIN_12                 (0x1000U)  /*!< Pin 12 selected   */
#define GPIO_PIN_13                 (0x2000U)  /*!< Pin 13 selected   */
#define GPIO_PIN_14                 (0x4000U)  /*!< Pin 14 selected   */
#define GPIO_PIN_15                 (0x8000U)  /*!< Pin 15 selected   */
#define GPIO_PIN_ALL                (0xFFFFU)  /*!< All pins selected */
/**
 * @}
 */

/**
 * @defgroup GPIO_All_Pins_Define GPIO All Pin Definition for Each Product
 * @{
 */
#define GPIO_PIN_A_ALL              (0xFFFFU)   /*!< Pin A all*/
#define GPIO_PIN_B_ALL              (0xFFFFU)   /*!< Pin B all*/
#define GPIO_PIN_C_ALL              (0xFFFFU)   /*!< Pin C all*/
#define GPIO_PIN_D_ALL              (0x0001U)   /*!< Pin D all*/
#define GPIO_PIN_F_ALL              (0x000FU)   /*!< Pin F all*/
/**
 * @}
 */

/**
 * @defgroup GPIO_Port_Source GPIO Port Source
 * @{
 */
#define GPIO_PORT_A                 (0x00U)     /*!< Port A selected  */
#define GPIO_PORT_B                 (0x01U)     /*!< Port B selected  */
#define GPIO_PORT_C                 (0x02U)     /*!< Port C selected  */
#define GPIO_PORT_D                 (0x03U)     /*!< Port D selected  */
#define GPIO_PORT_F                 (0x05U)     /*!< Port F selected  */
/**
 * @}
 */

/**
 * @defgroup GPIO_Function_Sel GPIO Function Selection
 * @{
 */
#define GPIO_FUNC_0               (0U)
#define GPIO_FUNC_1               (1U)
#define GPIO_FUNC_2               (2U)
#define GPIO_FUNC_3               (3U)
#define GPIO_FUNC_4               (4U)
#define GPIO_FUNC_5               (5U)
#define GPIO_FUNC_6               (6U)
#define GPIO_FUNC_7               (7U)
#define GPIO_FUNC_8               (8U)
#define GPIO_FUNC_9               (9U)
#define GPIO_FUNC_10              (10U)
#define GPIO_FUNC_11              (11U)
#define GPIO_FUNC_12              (12U)
#define GPIO_FUNC_13              (13U)
#define GPIO_FUNC_14              (14U)
#define GPIO_FUNC_15              (15U)
#define GPIO_FUNC_16              (16U)
#define GPIO_FUNC_17              (17U)
#define GPIO_FUNC_20              (20U)
#define GPIO_FUNC_21              (21U)
#define GPIO_FUNC_22              (22U)
#define GPIO_FUNC_23              (23U)
#define GPIO_FUNC_24              (24U)
#define GPIO_FUNC_25              (25U)
#define GPIO_FUNC_26              (26U)
#define GPIO_FUNC_32              (32U)
#define GPIO_FUNC_33              (33U)
#define GPIO_FUNC_34              (34U)
#define GPIO_FUNC_35              (35U)
#define GPIO_FUNC_36              (36U)
#define GPIO_FUNC_37              (37U)
#define GPIO_FUNC_38              (38U)
#define GPIO_FUNC_39              (39U)
#define GPIO_FUNC_40              (40U)
#define GPIO_FUNC_41              (41U)
#define GPIO_FUNC_42              (42U)
#define GPIO_FUNC_43              (43U)
#define GPIO_FUNC_44              (44U)
#define GPIO_FUNC_45              (45U)
#define GPIO_FUNC_46              (46U)
#define GPIO_FUNC_47              (47U)
#define GPIO_FUNC_48              (48U)
#define GPIO_FUNC_49              (49U)
#define GPIO_FUNC_50              (50U)
#define GPIO_FUNC_51              (51U)
#define GPIO_FUNC_52              (52U)
#define GPIO_FUNC_53              (53U)
#define GPIO_FUNC_54              (54U)
#define GPIO_FUNC_55              (55U)
#define GPIO_FUNC_56              (56U)
#define GPIO_FUNC_57              (57U)
/**
 * @}
 */

/**
 * @defgroup GPIO_DebugPin_Sel GPIO Debug Pin Selection
 * @{
 */
#define GPIO_PIN_TCK                (0x01U)
#define GPIO_PIN_TMS                (0x02U)
#define GPIO_PIN_TDO                (0x04U)
#define GPIO_PIN_TDI                (0x08U)
#define GPIO_PIN_TRST               (0x10U)
#define GPIO_PIN_DEBUG_JTAG         (0x1FU)
#define GPIO_PIN_SWCLK              (0x01U)
#define GPIO_PIN_SWDIO              (0x02U)
#define GPIO_PIN_SWO                (0x04U)
#define GPIO_PIN_DEBUG_SWD          (0x07U)
#define GPIO_PIN_DEBUG              (0x1FU)
/**
 * @}
 */

/**
 * @defgroup GPIO_ReadCycle_Sel GPIO Pin Read Wait Cycle Selection
 * @{
 */
#define GPIO_RD_WAIT0              (0x00U << GPIO_PCCR_RDWT_POS)
#define GPIO_RD_WAIT1              (0x01U << GPIO_PCCR_RDWT_POS)
#define GPIO_RD_WAIT2              (0x02U << GPIO_PCCR_RDWT_POS)
#define GPIO_RD_WAIT3              (0x03U << GPIO_PCCR_RDWT_POS)
#define GPIO_RD_WAIT4              (0x04U << GPIO_PCCR_RDWT_POS)
#define GPIO_RD_WAIT5              (0x05U << GPIO_PCCR_RDWT_POS)
#define GPIO_RD_WAIT6              (0x06U << GPIO_PCCR_RDWT_POS)
#define GPIO_RD_WAIT7              (0x07U << GPIO_PCCR_RDWT_POS)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinState_Sel GPIO Pin Output State Selection
 * @{
 */
#define PIN_STAT_RST               (0U)
#define PIN_STAT_SET               (GPIO_PCR_POUT)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinDirection_Sel GPIO Pin Input/Output Direction Selection
 * @{
 */
#define PIN_DIR_IN                  (0U)
#define PIN_DIR_OUT                 (GPIO_PCR_POUTE)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinOutType_Sel GPIO Pin Output Type Selection
 * @{
 */
#define PIN_OUT_TYPE_CMOS           (0U)
#define PIN_OUT_TYPE_NMOS           (GPIO_PCR_NOD)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinDrv_Sel GPIO Pin Drive Capacity Selection
 * @{
 */
#define PIN_LOW_DRV                 (0U)
#define PIN_MID_DRV                 (GPIO_PCR_DRV_0)
#define PIN_HIGH_DRV                (GPIO_PCR_DRV_1)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinLatch_Sel GPIO Pin Output Latch Selection
 * @{
 */
#define PIN_LATCH_OFF               (0U)
#define PIN_LATCH_ON                (GPIO_PCR_LTE)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinPU_Sel GPIO Pin Internal Pull-Up Resistor Selection
 * @{
 */
#define PIN_PU_OFF                  (0U)
#define PIN_PU_ON                   (GPIO_PCR_PUU)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinPD_Sel GPIO Pin Internal Pull-Down Resistor Selection
 * @{
 */
#define PIN_PD_OFF                  (0U)
#define PIN_PD_ON                   (GPIO_PCR_PUD)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinInputMOS_Sel GPIO Pin Input MOS Always On selection
 * @{
 */
#define PIN_IN_MOS_OFF              (0U)
#define PIN_IN_MOS_ON               (GPIO_PCR_PINAE)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinInvert_Sel GPIO Pin I/O Invert Selection
 * @{
 */
#define PIN_INVT_OFF                (0U)
#define PIN_INVT_ON                 (GPIO_PCR_INVE)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinInType_Sel GPIO Pin Input Type Selection
 * @{
 */
#define PIN_IN_TYPE_SMT             (0U)
#define PIN_IN_TYPE_CMOS            (GPIO_PCR_CINSEL)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinExtInt_Sel GPIO Pin External Interrupt Selection
 * @{
 */
#define PIN_EXTINT_OFF              (0U)
#define PIN_EXTINT_ON               (GPIO_PCR_INTE)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinMode_Sel GPIO Pin Mode Selection
 * @{
 */
#define PIN_ATTR_DIGITAL            (0U)
#define PIN_ATTR_ANALOG             (GPIO_PCR_DDIS)
/**
 * @}
 */

/**
 * @defgroup GPIO_PinSubFuncSet_Sel GPIO Pin Sub-function Enable or Disable
 * @{
 */
#define PIN_SUBFUNC_DISABLE         (0U)
#define PIN_SUBFUNC_ENABLE          (GPIO_PFSR_BFE)
/**
 * @}
 */

/**
 * @defgroup GPIO_Register_Protect_Key GPIO Registers Protect Key
 * @{
 */
#define GPIO_REG_LOCK_KEY            (0xA500U)
#define GPIO_REG_UNLOCK_KEY          (0xA501U)
/**
 * @}
 */

/**
 * @defgroup GPIO_HRPWM_Pin GPIO HRPWM Pin Define
 * @{
 */
#define GPIO_HRPWM1_PWMA            (GPIO_HRPWM_HRSEL0) /*!< PC6 HRPWM_1_PWMA */
#define GPIO_HRPWM1_PWMB            (GPIO_HRPWM_HRSEL1) /*!< PC7 HRPWM_1_PWMB */
#define GPIO_HRPWM2_PWMA            (GPIO_HRPWM_HRSEL2) /*!< PA8 HRPWM_2_PWMA */
#define GPIO_HRPWM2_PWMB            (GPIO_HRPWM_HRSEL3) /*!< PA9 HRPWM_2_PWMB */
#define GPIO_HRPWM3_PWMA            (GPIO_HRPWM_HRSEL4) /*!< PA10 HRPWM_3_PWMA */
#define GPIO_HRPWM3_PWMB            (GPIO_HRPWM_HRSEL5) /*!< PA11 HRPWM_3_PWMB */
#define GPIO_HRPWM4_PWMA            (GPIO_HRPWM_HRSEL6) /*!< PB12 HRPWM_4_PWMA */
#define GPIO_HRPWM4_PWMB            (GPIO_HRPWM_HRSEL7) /*!< PB13 HRPWM_4_PWMB */
#define GPIO_HRPWM5_PWMA            (GPIO_HRPWM_HRSEL8) /*!< PB14 HRPWM_5_PWMA */
#define GPIO_HRPWM5_PWMB            (GPIO_HRPWM_HRSEL9) /*!< PB15 HRPWM_5_PWMB */
#define GPIO_HRPWM6_PWMA            (GPIO_HRPWM_HRSEL10)/*!< PC8 HRPWM_6_PWMA */
#define GPIO_HRPWM6_PWMB            (GPIO_HRPWM_HRSEL11)/*!< PC9 HRPWM_6_PWMB */
#define GPIO_HRPWM_ALL              (GPIO_HRPWM1_PWMA | GPIO_HRPWM1_PWMB | GPIO_HRPWM2_PWMA | GPIO_HRPWM2_PWMB | \
                                     GPIO_HRPWM3_PWMA | GPIO_HRPWM3_PWMB | GPIO_HRPWM4_PWMA | GPIO_HRPWM4_PWMB | \
                                     GPIO_HRPWM5_PWMA | GPIO_HRPWM5_PWMB | GPIO_HRPWM6_PWMA | GPIO_HRPWM6_PWMB)
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
 * @addtogroup GPIO_Global_Functions
 * @{
 */
/**
 * @brief  GPIO lock. PSPCR, PCCR, PINAER, PCRxy, PFSRxy write disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void GPIO_REG_Lock(void)
{
    WRITE_REG16(CM_GPIO->PWPR, GPIO_REG_LOCK_KEY);
}

/**
 * @brief  GPIO unlock. PSPCR, PCCR, PINAER, PCRxy, PFSRxy write enable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void GPIO_REG_Unlock(void)
{
    WRITE_REG16(CM_GPIO->PWPR, GPIO_REG_UNLOCK_KEY);
}

int32_t GPIO_Init(uint8_t u8Port, uint32_t u32Pin, const stc_gpio_init_t *pstcGpioInit);
void GPIO_DeInit(void);
int32_t GPIO_StructInit(stc_gpio_init_t *pstcGpioInit);
void GPIO_SetDebugPort(uint8_t u8DebugPort, en_functional_state_t enNewState);
void GPIO_SetFunc(uint8_t u8Port, uint32_t u32Pin, uint16_t u16Func);
void GPIO_SubFuncCmd(uint8_t u8Port, uint32_t u32Pin, en_functional_state_t enNewState);
void GPIO_SetSubFunc(uint8_t u8Func);
void GPIO_SetReadWaitCycle(uint16_t u16ReadWait);
void GPIO_InputMOSCmd(uint8_t u8Port, uint32_t u32Pin, en_functional_state_t enNewState);
void GPIO_OutputCmd(uint8_t u8Port, uint32_t u32Pin, en_functional_state_t enNewState);
void GPIO_OutputLatchCmd(uint8_t u8Port, uint32_t u32Pin, en_functional_state_t enNewState);
en_pin_state_t GPIO_ReadInputPins(uint8_t u8Port, uint32_t u32Pin);
uint32_t GPIO_ReadInputPort(uint8_t u8Port);
en_pin_state_t GPIO_ReadOutputPins(uint8_t u8Port, uint32_t u32Pin);
uint32_t GPIO_ReadOutputPort(uint8_t u8Port);
void GPIO_SetPins(uint8_t u8Port, uint32_t u32Pin);
void GPIO_ResetPins(uint8_t u8Port, uint32_t u32Pin);
void GPIO_WritePort(uint8_t u8Port, uint32_t u32PortVal);
void GPIO_TogglePins(uint8_t u8Port, uint32_t u32Pin);
void GPIO_ExtIntCmd(uint8_t u8Port, uint32_t u32Pin, en_functional_state_t enNewState);
void GPIO_AnalogCmd(uint8_t u8Port, uint32_t u32Pin, en_functional_state_t enNewState);
void GPIO_HrpwmPinCmd(uint32_t u32HrpwmPin, en_functional_state_t enNewState);
void GPIO_HrpwmPinSwapCmd(uint32_t u32HrpwmPin, en_functional_state_t enNewState);

/**
 * @}
 */

#endif /* LL_GPIO_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_GPIO_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
