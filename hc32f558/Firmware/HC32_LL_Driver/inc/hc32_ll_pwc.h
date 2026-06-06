/**
 *******************************************************************************
 * @file  hc32_ll_pwc.h
 * @brief This file contains all the functions prototypes of the PWC driver
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
#ifndef __HC32_LL_PWC_H__
#define __HC32_LL_PWC_H__

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
 * @addtogroup LL_PWC
 * @{
 */

#if (LL_PWC_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup PWC_Global_Types PWC Global Types
 * @{
 */
/**
 * @brief PWC LVD Init
 */
typedef struct {
    uint32_t u32State;              /*!< LVD function setting, @ref PWC_LVD_Config for details */
    uint32_t u32CompareOutputState; /*!< LVD compare output function setting, @ref PWC_LVD_CMP_Config for details */
    uint32_t u32ExceptionType;      /*!< LVD interrupt or reset selection, @ref PWC_LVD_Exception_Type_Sel for details */
    uint32_t u32Filter;             /*!< LVD digital filter function setting, @ref PWC_LVD_DF_Config for details */
    uint32_t u32FilterClock;        /*!< LVD digital filter clock setting, @ref PWC_LVD_DFS_Clk_Sel for details */
    uint32_t u32ThresholdVoltage;   /*!< LVD detect voltage setting, @ref PWC_LVD_Detection_Voltage_Sel for details */
    uint32_t u32TriggerEdge;        /*!< LVD trigger setting, @ref PWC_LVD_TRIG_Sel for details */
} stc_pwc_lvd_init_t;

/**
 * @brief PWC Stop mode Init
 */
typedef struct {
    uint16_t u16Clock;          /*!< System clock setting after wake-up from stop mode,
                                    @ref PWC_STOP_CLK_Sel for details.        */
    uint16_t u16FlashWait;      /*!< Waiting flash stable after wake-up from stop mode,
                                    @ref PWC_STOP_Flash_Wait_Sel for details. */
} stc_pwc_stop_mode_config_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup PWC_Global_Macros PWC Global Macros
 * @{
 */

/**
 * @defgroup PWC_STOP_CLK_Sel System clock setting after wake-up from stop mode
 * @{
 */
#define PWC_STOP_CLK_KEEP               (0x00U)                 /*!< Keep System clock setting after wake-up from stop mode */
#define PWC_STOP_CLK_MRC                (PWC_STPMCR_CKSMRC)     /*!< System clock switch to MRC after wake-up from stop mode */

/**
 * @}
 */

/**
 * @defgroup PWC_STOP_Flash_Wait_Sel Whether wait flash stable or not after wake-up from stop mode
 * @{
 */
#define PWC_STOP_FLASH_WAIT_ON          (0x00U)                 /*!< Wait flash stable after wake-up from stop mode */
#define PWC_STOP_FLASH_WAIT_OFF         (PWC_STPMCR_FLNWT)      /*!< Don't wait flash stable after wake-up from stop mode */
/**
 * @}
 */

/**
 * @defgroup PWC_Stop_Type PWC stop mode type.
 * @{
 */
#define PWC_STOP_WFI                    (0x00U)                 /*!< Enter stop mode by WFI instruction. */
#define PWC_STOP_WFE                    (0x01U)                 /*!< Enter stop mode by WFE instruction. */
/**
 * @}
 */

/**
 * @defgroup PWC_Sleep_Type PWC sleep mode type.
 * @{
 */
#define PWC_SLEEP_WFI                   (0x00U)                 /*!< Enter sleep mode by WFI instruction. */
#define PWC_SLEEP_WFE                   (0x01U)                 /*!< Enter sleep mode by WFE instruction. */
/**
 * @}
 */

/**
 * @defgroup PWC_PD_Periph_Ram  Peripheral ram to power down
 * @{
 */
#define PWC_RAM_PD_CACHE                (PWC_PRAMLPC_PRAMPDC2)
#define PWC_RAM_PD_FMAC0                (PWC_PRAMLPC_PRAMPDC5)
#define PWC_RAM_PD_FMAC1                (PWC_PRAMLPC_PRAMPDC6)
#define PWC_RAM_PD_MCAN                 (PWC_PRAMLPC_PRAMPDC11)
#define PWC_RAM_PD_ALL                  (PWC_RAM_PD_CACHE | PWC_RAM_PD_FMAC0 | \
                                         PWC_RAM_PD_FMAC1 | PWC_RAM_PD_MCAN)
/**
 * @}
 */

/**
 * @defgroup PWC_PD_Ram  Peripheral ram to power down
 * @{
 */
#define PWC_RAM_PD_SRAM1_1              (PWC_RAMPC0_RAMPDC0)    /*< 0x20000000 ~ 0x20007FFF */
#define PWC_RAM_PD_SRAM1_2              (PWC_RAMPC0_RAMPDC1)    /*< 0x20008000 ~ 0x2000FFFF */
#define PWC_RAM_PD_SRAM1_3              (PWC_RAMPC0_RAMPDC2)    /*< 0x20010000 ~ 0x20017FFF */
#define PWC_RAM_PD_SRAMH_1              (PWC_RAMPC0_RAMPDC7)    /*< 0x1FFF8000 ~ 0x1FFFBFFF */
#define PWC_RAM_PD_SRAMH_2              (PWC_RAMPC0_RAMPDC8)    /*< 0x1FFEC000 ~ 0x1FFFFFFF */
/**
 * @}
 */

/**
 * @defgroup PWC_LVD_Channel PWC LVD channel
 * @{
 */
#define PWC_LVD_CH1                     (0x00U)
#define PWC_LVD_CH2                     (0x01U)

/**
 * @}
 */

/**
 * @defgroup PWC_LVD_Config PWC LVD Config
 * @{
 */
#define PWC_LVD_ON                      (PWC_PVDCR0_PVD1EN)
#define PWC_LVD_OFF                     (0x00U)
/**
 * @}
 */

/**
 * @defgroup PWC_LVD_Exception_Type_Sel PWC LVD Exception Type Select
 * @{
 */
#define PWC_LVD_EXP_TYPE_NONE           (0x00U)
#define PWC_LVD_EXP_TYPE_INT            (0x0101U)
#define PWC_LVD_EXP_TYPE_NMI            (0x0001U)
#define PWC_LVD_EXP_TYPE_RST            (PWC_PVDCR1_PVD1IRE | PWC_PVDCR1_PVD1IRS)

/**
 * @}
 */

/**
 * @defgroup PWC_LVD_CMP_Config PWC LVD Compare Config
 * @{
 */
#define PWC_LVD_CMP_OFF                 (0x00U)
#define PWC_LVD_CMP_ON                  (PWC_PVDCR1_PVD1CMPOE)
/**
 * @}
 */

/**
 * @defgroup PWC_LVD_DF_Config LVD digital filter ON or OFF
 * @{
 */
#define PWC_LVD_FILTER_ON               (0x00U)
#define PWC_LVD_FILTER_OFF              (0x01U)
/**
 * @}
 */

/**
 * @defgroup PWC_LVD_DFS_Clk_Sel LVD digital filter sample ability
 * @note     modified this value must when PWC_LVD_FILTER_OFF
 * @{
 */
#define PWC_LVD_FILTER_LRC_DIV4        (0x00UL << PWC_PVDFCR_PVD1NFCKS_POS)     /*!< 0.25 LRC cycle */
#define PWC_LVD_FILTER_LRC_DIV2        (0x01UL << PWC_PVDFCR_PVD1NFCKS_POS)     /*!< 0.5 LRC cycle  */
#define PWC_LVD_FILTER_LRC_DIV1        (0x02UL << PWC_PVDFCR_PVD1NFCKS_POS)     /*!< 1 LRC cycle    */
#define PWC_LVD_FILTER_LRC_MUL2        (0x03UL << PWC_PVDFCR_PVD1NFCKS_POS)     /*!< 2 LRC cycles   */

/**
 * @}
 */

/**
 * @defgroup PWC_LVD_Detection_Voltage_Sel PWC LVD Detection voltage
 * @{
 * @note
 * @verbatim
 *       |  LVL0   |  LVL1   |  LVL2   |  LVL3   |  LVL4   |  LVL5   |  LVL6   |  LVL7   |  EXVCC |
 * LVD1  |  2.00V  |  2.10V  |  2.30V  |  2.50V  |  2.60V  |  2.70V  |  2.80V  |  2.90V  |   --   |
 * LVD2  |  2.10V  |  2.30V  |  2.50V  |  2.60V  |  2.70V  |  2.80V  |  2.90V  |  1.10V  |  EXVCC |
 * @endverbatim
 */
#define PWC_LVD_THRESHOLD_LVL0          (0x00U)
#define PWC_LVD_THRESHOLD_LVL1          (0x01U)
#define PWC_LVD_THRESHOLD_LVL2          (0x02U)
#define PWC_LVD_THRESHOLD_LVL3          (0x03U)
#define PWC_LVD_THRESHOLD_LVL4          (0x04U)
#define PWC_LVD_THRESHOLD_LVL5          (0x05U)
#define PWC_LVD_THRESHOLD_LVL6          (0x06U)
#define PWC_LVD_THRESHOLD_LVL7          (0x07U)
#define PWC_LVD_EXTVCC                  (0x07U)

/**
 * @}
 */

/**
 * @defgroup PWC_LVD_TRIG_Sel LVD trigger setting
 * @{
 */
#define PWC_LVD_TRIG_FALLING            (0x00UL << PWC_PVDICR_PVD1EDGS_POS)
#define PWC_LVD_TRIG_RISING             (0x01UL << PWC_PVDICR_PVD1EDGS_POS)
#define PWC_LVD_TRIG_BOTH               (0x02UL << PWC_PVDICR_PVD1EDGS_POS)

/**
 * @}
 */

/**
 * @defgroup PWC_LVD_Flag LVD flag
 * @{
 */
#define PWC_LVD1_FLAG_DETECT            (PWC_PVDDSR_PVD1DETFLG)          /*!< VCC across VLVD1   */
#define PWC_LVD2_FLAG_DETECT            (PWC_PVDDSR_PVD2DETFLG)          /*!< VCC across VLVD2   */
#define PWC_LVD1_FLAG_MON               (PWC_PVDDSR_PVD1MON)             /*!< VCC > VLVD1        */
#define PWC_LVD2_FLAG_MON               (PWC_PVDDSR_PVD2MON)             /*!< VCC > VLVD2        */

/**
 * @}
 */

/**
 * @defgroup PWC_Monitor_Power PWC Power Monitor voltage definition
 * @{
 */
#define PWC_PWR_MON_IREF                (0x00U)                 /*!< Internal reference voltage */
#define PWC_PWR_MON_VDD                 (PWC_PWRC4_ADBUFS)
/**
 * @}
 */

/**
 * @defgroup PWC_Ldo_Sel PWC LDO Selection
 * @{
 */
#define PWC_LDO_PLL                     (PWC_PWRC1_VPLLSD)
#define PWC_LDO_MASK                    (PWC_LDO_PLL)
/**
 * @}
 */

/**
 * @defgroup PWC_Port_Reset_Sel PWC Port reset selection
 * @{
 */
#define PWC_PORT_RST_WDT                (PWC_PWRC6_WDRTNE)
#define PWC_PORT_RST_SW                 (PWC_PWRC6_SWRTNE)
#define PWC_PORT_RST_ALL                (PWC_PORT_RST_WDT | PWC_PORT_RST_SW)
/**
 * @}
 */

/**
 * @defgroup PWC_Dac_Reset_Sel PWC DAC reset selection
 * @{
 */
#define PWC_DAC_RST_WDT                 (PWC_PWRC6_WDRDAC)
#define PWC_DAC_RST_SW                  (PWC_PWRC6_SWRDAC)
#define PWC_DAC_RST_ALL                 (PWC_DAC_RST_WDT | PWC_DAC_RST_SW)
/**
 * @}
 */

/**
 * @defgroup PWC_Cmp_Reset_Sel PWC CMP reset selection
 * @{
 */
#define PWC_CMP_RST_WDT                 (PWC_PWRC6_WDRCMP)
#define PWC_CMP_RST_SW                  (PWC_PWRC6_SWRCMP)
#define PWC_CMP_RST_ALL                 (PWC_CMP_RST_WDT | PWC_CMP_RST_SW)
/**
 * @}
 */

/**
 * @defgroup PWC_Port_Release_Sel PWC Port release selection
 * @{
 */
#define PWC_PORT_RELEASE_WDT                (PWC_PWRC6_WDRIOCLR)
#define PWC_PORT_RELEASE_SW                 (PWC_PWRC6_SWRIOCLR)
#define PWC_PORT_RELEASE_ALL                (PWC_PORT_RELEASE_WDT | PWC_PORT_RELEASE_SW)
/**
 * @}
 */

/**
 * @defgroup PWC_REG_Write_Unlock_Code PWC Register Unlock Code
 * @brief Lock/unlock Code for each module
@verbatim
 *        PWC_UNLOCK_CODE0:
 *          Below registers are locked in CLK module.
 *              XTALCFGR, XTALSTBCR, XTALCR, XTALSTDCR, XTALSTDSR,
 *              HRCTRM, HRCCR, MRCTRM, MRCCR, PLLHCFGR, PLLHCR, OSCSTBSR, CKSWR, SCFGR, LRCCR, LRCTRM,
 *              CANCKCFGR, TPIUCKCFGR, MCO1CFGR, MCO2CFGR
 *        PWC_UNLOCK_CODE1:
 *          Below registers are locked in PWC module.
 *              PWRC1, PWRC4, PWRC6, PERICKSEL, STPMCR, RAMPC0, RAMOPM, PRAMLPC
 *          Below register is locked in RMU module.
 *              RSTF0, RSTF1, RSTF2, RSTF3, PRSTCR0
 *        PWC_UNLOCK_CODE2:
 *          Below registers are locked in PWC module.
 *              PVDCR0, PVDCR1, PVDFCR, PVDLCR, PVDICR, PVDDSR
@endverbatim
 * @{
 */
#define PWC_WRITE_ENABLE                (0xA500U)
#define PWC_UNLOCK_CODE0                (0xA501U)
#define PWC_UNLOCK_CODE1                (0xA502U)
#define PWC_UNLOCK_CODE2                (0xA508U)
/**
 * @}
 */

/**
 * @defgroup PWC_FCG0_REG_Key PWC FCG0 Register Key
 * @{
 */
#define PWC_FCG0_REG_UNLOCK_KEY          (0xA5A50001UL)
#define PWC_FCG0_REG_LOCK_KEY            (0xA5A50000UL)
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
 * @addtogroup PWC_Global_Functions
 * @{
 */
/**
 * @brief  Lock PWC, CLK, RMU register.
 * @param  [in] u16Module Lock code for each module.
 *   @arg  PWC_UNLOCK_CODE0
 *   @arg  PWC_UNLOCK_CODE1
 *   @arg  PWC_UNLOCK_CODE2
 * @retval None
 */
__STATIC_INLINE void PWC_REG_Lock(uint16_t u16Module)
{
    CM_PWC->FPRC = (PWC_WRITE_ENABLE | (uint16_t)((uint16_t)(~u16Module) & (CM_PWC->FPRC)));
}

/**
 * @brief  Unlock PWC, CLK, RMU register.
 * @param  [in] u16Module Unlock code for each module.
 *   @arg  PWC_UNLOCK_CODE0
 *   @arg  PWC_UNLOCK_CODE1
 *   @arg  PWC_UNLOCK_CODE2
 * @retval None
 */
__STATIC_INLINE void PWC_REG_Unlock(uint16_t u16Module)
{
    SET_REG16_BIT(CM_PWC->FPRC, u16Module);
}

/**
 * @brief  Lock PWC_FCG0 register.
 * @param  None
 * @retval None
 */
__STATIC_INLINE void PWC_FCG0_REG_Lock(void)
{
    WRITE_REG32(CM_PWC->FCG0PC, PWC_FCG0_REG_LOCK_KEY);
}

/**
 * @brief  Unlock PWR_FCG0 register.
 * @param  None
 * @retval None
 * @note Call this function before FCG_Fcg0PeriphClockCmd()
 */
__STATIC_INLINE void PWC_FCG0_REG_Unlock(void)
{
    WRITE_REG32(CM_PWC->FCG0PC, PWC_FCG0_REG_UNLOCK_KEY);
}

void PWC_PD_RamCmd(uint32_t u32Ram, en_functional_state_t enNewState);

void PWC_SleepOnExitCmd(en_functional_state_t enNewState);
void PWC_SevOnPendCmd(en_functional_state_t enNewState);

/* PWC Sleep Function */
void PWC_SLEEP_Enter(uint8_t u8SleepType);

/* PWC Stop Function */
int32_t PWC_STOP_Enter(uint8_t u8StopType);
int32_t PWC_STOP_StructInit(stc_pwc_stop_mode_config_t *pstcStopConfig);
int32_t PWC_STOP_Config(const stc_pwc_stop_mode_config_t *pstcStopConfig);
void PWC_STOP_ClockSelect(uint8_t u8Clock);
void PWC_STOP_FlashWaitCmd(en_functional_state_t enNewState);

/* PWC LDO Function */
void PWC_LDO_Cmd(uint16_t u16Ldo, en_functional_state_t enNewState);

/* PWC LVD/PVD Function */
int32_t PWC_LVD_Init(uint8_t u8Ch, const stc_pwc_lvd_init_t *pstcLvdInit);
void PWC_LVD_DeInit(uint8_t u8Ch);
int32_t PWC_LVD_StructInit(stc_pwc_lvd_init_t *pstcLvdInit);
void PWC_LVD_Cmd(uint8_t u8Ch, en_functional_state_t enNewState);
void PWC_LVD_ExtInputCmd(en_functional_state_t enNewState);
void PWC_LVD_CompareOutputCmd(uint8_t u8Ch, en_functional_state_t enNewState);
void PWC_LVD_DigitalFilterCmd(uint8_t u8Ch, en_functional_state_t enNewState);
void PWC_LVD_SetFilterClock(uint8_t u8Ch, uint32_t u32Clock);
void PWC_LVD_SetThresholdVoltage(uint8_t u8Ch, uint32_t u32Voltage);
void PWC_LVD_ClearStatus(uint8_t u8Flag);
en_flag_status_t PWC_LVD_GetStatus(uint8_t u8Flag);

/* PWC Power Monitor Function */
void PWC_PowerMonitorCmd(en_functional_state_t enNewState);
void PWC_SetPowerMonitorVoltageSrc(uint8_t u8VoltageSrc);

/* PWC RAM Function */

void PWC_PortResetCmd(uint8_t u8Event, en_functional_state_t enNewState);
void PWC_DacResetCmd(uint8_t u8Event, en_functional_state_t enNewState);
void PWC_CmpResetCmd(uint8_t u8Event, en_functional_state_t enNewState);
void PWC_ReleasePort(uint8_t u8Event);

/**
 * @}
 */

#endif /* LL_PWC_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_PWC_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
