/**
 *******************************************************************************
 * @file  hc32_ll_pwc.c
 * @brief This file provides firmware functions to manage the Power Control(PWC).
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
#include "hc32_ll_pwc.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_PWC PWC
 * @brief Power Control Driver Library
 * @{
 */

#if (LL_PWC_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup PWC_Local_Macros PWC Local Macros
 * @{
 */

/* Get the backup register address of PWC */

#define PWC_MD_SWITCH_TIMEOUT           (30UL)
#define PWC_MD_SWITCH_TIMEOUT2          (0x1000UL)

#define PWC_LVD_FLAG_CLR_MSK            (PWC_LVD1_FLAG_DETECT | PWC_LVD2_FLAG_DETECT)

#define PWC_LVD_EN_REG                  (CM_PWC->PVDCR0)
#define PWC_LVD_EN_BIT                  (PWC_PVDCR0_PVD1EN)
#define PWC_LVD_FILTER_EN_REG           (CM_PWC->PVDFCR)
#define PWC_LVD_FILTER_EN_BIT           (PWC_PVDFCR_PVD1NFDIS)
#define PWC_LVD_STATUS_REG              (CM_PWC->PVDDSR)

#define PWC_LVD2_POS                    (4U)
#define PWC_LVD_BIT_OFFSET(x)           ((uint8_t)((x) * PWC_LVD2_POS))
#define PWC_LVD_EN_BIT_OFFSET(x)        (x)

#define PWC_RAM_MASK                    (PWC_RAM_PD_SRAM1_1 | PWC_RAM_PD_SRAM1_2 | \
                                         PWC_RAM_PD_SRAM1_3 | PWC_RAM_PD_SRAMH_1 | \
                                         PWC_RAM_PD_SRAMH_2)

#define PWC_PRAM_MASK                   (PWC_RAM_PD_ALL)

#define PWC_LVD_FLAG_MASK               (PWC_LVD1_FLAG_MON | PWC_LVD1_FLAG_DETECT | \
                                         PWC_LVD2_FLAG_MON | PWC_LVD2_FLAG_DETECT)

#define PWC_LVD_EXP_NMI_POS             (8U)

/**
 * @defgroup PWC_Check_Parameters_Validity PWC Check Parameters Validity
 * @{
 */

/* Check PWC register lock status. */
#define IS_PWC_UNLOCKED()               ((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1)
/* Check PWC LVD register lock status. */
#define IS_PWC_LVD_UNLOCKED()           ((CM_PWC->FPRC & PWC_FPRC_FPRCB3) == PWC_FPRC_FPRCB3)
/* Check PWC LVD digital filter enable status. */
#define IS_PWC_LVD_DIG_FILTER_DISABLED(x)                                       \
(   (PWC_LVD_FILTER_EN_BIT << PWC_LVD_BIT_OFFSET(x)) ==                         \
    (PWC_LVD_FILTER_EN_REG & (PWC_LVD_FILTER_EN_BIT << PWC_LVD_BIT_OFFSET(x))))

/* Parameter validity check for stop type */
#define IS_PWC_STOP_TYPE(x)                                                     \
(   ((x) == PWC_STOP_WFI)                           ||                          \
    ((x) == PWC_STOP_WFE))

/* Parameter validity check for sleep type */
#define IS_PWC_SLEEP_TYPE(x)                                                    \
(   ((x) == PWC_SLEEP_WFI)                          ||                          \
    ((x) == PWC_SLEEP_WFE))

/* Parameter validity check for internal RAM setting of power mode control */
#define IS_PWC_RAM_CONTROL(x)                                                   \
(   ((x) != 0x00UL)                                 &&                          \
    (((x) | PWC_RAM_MASK) == PWC_RAM_MASK))

/* Parameter validity check for LVD channel. */
#define IS_PWC_LVD_CH(x)                                                        \
(   ((x) == PWC_LVD_CH1)                            ||                          \
    ((x) == PWC_LVD_CH2))

/* Parameter validity check for LVD function setting. */
#define IS_PWC_LVD_EN(x)                                                        \
(   ((x) == PWC_LVD_ON)                             ||                          \
    ((x) == PWC_LVD_OFF))

/* Parameter validity check for LVD compare output setting. */
#define IS_PWC_LVD_CMP_EN(x)                                                    \
(   ((x) == PWC_LVD_CMP_ON)                         ||                          \
    ((x) == PWC_LVD_CMP_OFF))

/*  Parameter validity check for PWC LVD exception type. */
#define IS_PWC_LVD_EXP_TYPE(x)                                                  \
(   ((x) == PWC_LVD_EXP_TYPE_NONE)                  ||                          \
    ((x) == PWC_LVD_EXP_TYPE_INT)                   ||                          \
    ((x) == PWC_LVD_EXP_TYPE_NMI)                   ||                          \
    ((x) == PWC_LVD_EXP_TYPE_RST))

/* Parameter validity check for LVD digital noise filter function setting. */
#define IS_PWC_LVD_FILTER_EN(x)                                                 \
(   ((x) == PWC_LVD_FILTER_ON)                      ||                          \
    ((x) == PWC_LVD_FILTER_OFF))

/* Parameter validity check for LVD digital noise filter clock setting. */
#define IS_PWC_LVD_FILTER_CLK(x)                                                \
(   ((x) == PWC_LVD_FILTER_LRC_DIV1)                ||                          \
    ((x) == PWC_LVD_FILTER_LRC_DIV2)                ||                          \
    ((x) == PWC_LVD_FILTER_LRC_DIV4)                ||                          \
    ((x) == PWC_LVD_FILTER_LRC_MUL2))

/* Parameter validity check for LVD detect voltage setting. */
#define IS_PWC_LVD_THRESHOLD_VOLTAGE(x)                                         \
(   ((x) == PWC_LVD_THRESHOLD_LVL0)                 ||                          \
    ((x) == PWC_LVD_THRESHOLD_LVL1)                 ||                          \
    ((x) == PWC_LVD_THRESHOLD_LVL2)                 ||                          \
    ((x) == PWC_LVD_THRESHOLD_LVL3)                 ||                          \
    ((x) == PWC_LVD_THRESHOLD_LVL4)                 ||                          \
    ((x) == PWC_LVD_THRESHOLD_LVL5)                 ||                          \
    ((x) == PWC_LVD_THRESHOLD_LVL6)                 ||                          \
    ((x) == PWC_LVD_THRESHOLD_LVL7))

/* Parameter validity check for LVD trigger setting. */
#define IS_PWC_LVD_TRIG_EDGE(x)                                                 \
(   ((x) == PWC_LVD_TRIG_FALLING)                   ||                          \
    ((x) == PWC_LVD_TRIG_RISING)                    ||                          \
    ((x) == PWC_LVD_TRIG_BOTH))

/* Parameter validity check for LVD trigger setting. */
#define IS_PWC_LVD_CLR_FLAG(x)                                                  \
(   ((x) == PWC_LVD1_FLAG_DETECT)                   ||                          \
    ((x) == PWC_LVD2_FLAG_DETECT))

/* Parameter validity check for LVD flag. */
#define IS_PWC_LVD_GET_FLAG(x)                                                  \
(   ((x) != 0x00U)                                  &&                          \
    (((x) | PWC_LVD_FLAG_MASK) == PWC_LVD_FLAG_MASK))

/* Parameter validity check for clock setting after wake-up from stop mode. */
#define IS_PWC_STOP_CLK(x)                                                      \
(   ((x) == PWC_STOP_CLK_KEEP)                      ||                          \
    ((x) == PWC_STOP_CLK_MRC))

/* Parameter validity check for flash wait setting after wake-up from stop mode. */
#define IS_PWC_STOP_FLASH_WAIT(x)                                               \
(   ((x)== PWC_STOP_FLASH_WAIT_ON)                  ||                          \
    ((x)== PWC_STOP_FLASH_WAIT_OFF))

/*  Parameter validity check for power monitor sel. */
#define IS_PWC_PWR_MON_SEL(x)                                                   \
(   ((x) == PWC_PWR_MON_IREF)                       ||                          \
    ((x) == PWC_PWR_MON_VDD))

#define IS_PWC_LDO_SEL(x)                                                       \
(   ((x) != 0x00U)                                  &&                          \
    (((x) | PWC_LDO_MASK) == PWC_LDO_MASK))

/* Parameter validity check for port reset event. */
#define IS_PWC_PORT_RST_EVT(x)                                                  \
(   ((x) != 0x00U)                                  &&                          \
    (((x) | PWC_PORT_RST_ALL) == PWC_PORT_RST_ALL))

/* Parameter validity check for DAC reset event. */
#define IS_PWC_DAC_RST_EVT(x)                                                   \
(   ((x) != 0x00U)                                  &&                          \
    (((x) | PWC_DAC_RST_ALL) == PWC_DAC_RST_ALL))

/* Parameter validity check for CMP reset event. */
#define IS_PWC_CMP_RST_EVT(x)                                                   \
(   ((x) != 0x00U)                                  &&                          \
    (((x) | PWC_CMP_RST_ALL) == PWC_CMP_RST_ALL))

#define IS_PWC_PORT_RELEASE_EVT(x)                                                  \
(   ((x) != 0x00U)                                  &&                          \
    (((x) | PWC_PORT_RELEASE_ALL) == PWC_PORT_RELEASE_ALL))

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
 * @defgroup PWC_Global_Functions PWC Global Functions
 * @{
 */

/**
 * @brief Sets whether the processor enter sleep state on an exit from an ISR
 * @param  [in] enNewState        An @ref en_functional_state_t enumeration value
 *   @arg  ENABLE   Sets SLEEPONEXIT bit of SCR register: Enter sleep state on an exit from ISR
 *   @arg  DISABLE  Clears SLEEPONEXIT bit of SCR register: Do not enter sleep state on an exit from ISR
 * @retval None
 */
void PWC_SleepOnExitCmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        /* Set SLEEPONEXIT bit of Cortex System Control Register */
        SET_REG32_BIT(SCB->SCR, SCB_SCR_SLEEPONEXIT_Msk);
    } else {
        /* Clear SLEEPONEXIT bit of Cortex System Control Register */
        CLR_REG32_BIT(SCB->SCR, SCB_SCR_SLEEPONEXIT_Msk);
    }
}

/**
 * @brief Sets whether an interrupt transition from inactive state to pending state is a wakeup event
 * @param  [in] enNewState        An @ref en_functional_state_t enumeration value
 *   @arg  ENABLE   Sets SEVONPEND bit of SCR register: an interrupt transition from inactive to pending are wakeup events
 *   @arg  DISABLE  Clears SEVONPEND bit of SCR register: an interrupt transition from inactive to pending are not wakeup events
 * @retval None
 */
void PWC_SevOnPendCmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        /* Set SEVONPEND bit of Cortex System Control Register */
        SET_REG32_BIT(SCB->SCR, SCB_SCR_SEVONPEND_Msk);
    } else {
        /* Clear SEVONPEND bit of Cortex System Control Register */
        CLR_REG32_BIT(SCB->SCR, SCB_SCR_SEVONPEND_Msk);
    }
}

/**
 * @brief  Enter stop mode.
 * @param  [in] u8StopType specifies the type of enter stop's command.
 *   @arg  PWC_STOP_WFI             Enter stop mode by WFI instruction.
 *   @arg  PWC_STOP_WFE             Enter stop mode by WFE instruction.
 * @retval int32_t:
 *          - LL_OK:                Enter stop mode OK, and has woken up.
 *          - LL_ERR_TIMEOUT:       Enter stop mode timeout
 * @note   Make sure XTALSTD is disabled before call this function.
 */
int32_t PWC_STOP_Enter(uint8_t u8StopType)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32Timeout = 0UL;

    DDL_ASSERT(IS_PWC_UNLOCKED());
    DDL_ASSERT(IS_PWC_STOP_TYPE(u8StopType));
    /* Clear lpm flag */
    WRITE_REG16(CM_PWC->LPMCSCR, PWC_LPMCSCR_LPMFC);
    while (0UL == READ_REG32(CM_PWC->LPMCSR)) {
        SET_REG16_BIT(CM_PWC->STPMCR, PWC_STPMCR_STOP);
        if (PWC_STOP_WFI == u8StopType) {
            /* Request Wait For Interrupt */
            __WFI();
        } else {
            /* Request Wait For Event */
            __SEV();
            __WFE();
            __WFE();
        }
        u32Timeout++;
        if (u32Timeout > 1000UL) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }
    return i32Ret;
}

/**
 * @brief  Enter sleep mode.
 * @param  [in] u8SleepType specifies the type of enter sleep's command.
 *   @arg  PWC_SLEEP_WFI            Enter sleep mode by WFI instruction.
 *   @arg  PWC_SLEEP_WFE            Enter sleep mode by WFE instruction.
 * @retval None
 */
void PWC_SLEEP_Enter(uint8_t u8SleepType)
{
    DDL_ASSERT(IS_PWC_UNLOCKED());
    DDL_ASSERT(IS_PWC_SLEEP_TYPE(u8SleepType));

    CLR_REG16_BIT(CM_PWC->STPMCR, PWC_STPMCR_STOP);

    if (PWC_SLEEP_WFI == u8SleepType) {
        /* Request Wait For Interrupt */
        __WFI();
    } else {
        /* Request Wait For Event */
        __SEV();
        __WFE();
        __WFE();
    }
}

/**
 * @brief  Initialize LVD config structure. Fill each pstcLvdInit with default value
 * @param  [in] pstcLvdInit Pointer to a stc_pwc_lvd_init_t structure that contains configuration information.
 * @retval int32_t:
 *          - LL_OK: LVD structure initialize successful
 *          - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PWC_LVD_StructInit(stc_pwc_lvd_init_t *pstcLvdInit)
{
    int32_t i32Ret = LL_OK;
    /* Check if pointer is NULL */
    if (NULL == pstcLvdInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        /* RESET LVD init structure parameters values */
        pstcLvdInit->u32State              = PWC_LVD_OFF;
        pstcLvdInit->u32CompareOutputState = PWC_LVD_CMP_OFF;
        pstcLvdInit->u32ExceptionType      = PWC_LVD_EXP_TYPE_NONE;
        pstcLvdInit->u32Filter             = PWC_LVD_FILTER_OFF;
        pstcLvdInit->u32FilterClock        = PWC_LVD_FILTER_LRC_MUL2;
        pstcLvdInit->u32ThresholdVoltage   = PWC_LVD_THRESHOLD_LVL0;
        pstcLvdInit->u32TriggerEdge        = PWC_LVD_TRIG_FALLING;
    }
    return i32Ret;
}

/**
 * @brief LVD configuration.
 * @param [in] u8Ch LVD channel @ref PWC_LVD_Channel.
 * @param [in] pstcLvdInit Pointer to a stc_pwc_lvd_init_t structure that contains configuration information.
 * @retval int32_t:
 *          - LL_OK: LVD initialize successful
 *          - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PWC_LVD_Init(uint8_t u8Ch, const stc_pwc_lvd_init_t *pstcLvdInit)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcLvdInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_PWC_LVD_UNLOCKED());
        DDL_ASSERT(IS_PWC_LVD_CH(u8Ch));
        DDL_ASSERT(IS_PWC_LVD_EN(pstcLvdInit->u32State));
        DDL_ASSERT(IS_PWC_LVD_EXP_TYPE(pstcLvdInit->u32ExceptionType));
        DDL_ASSERT(IS_PWC_LVD_CMP_EN(pstcLvdInit->u32CompareOutputState));
        DDL_ASSERT(IS_PWC_LVD_FILTER_EN(pstcLvdInit->u32Filter));
        DDL_ASSERT(IS_PWC_LVD_FILTER_CLK(pstcLvdInit->u32FilterClock));
        DDL_ASSERT(IS_PWC_LVD_THRESHOLD_VOLTAGE(pstcLvdInit->u32ThresholdVoltage));
        DDL_ASSERT(IS_PWC_LVD_TRIG_EDGE(pstcLvdInit->u32TriggerEdge));

        /* disable filter function in advance */
        SET_REG8_BIT(CM_PWC->PVDFCR, (PWC_PVDFCR_PVD1NFDIS << PWC_LVD_BIT_OFFSET(u8Ch)));
        /* Enable LVD */
        MODIFY_REG8(CM_PWC->PVDCR0, PWC_PVDCR0_PVD1EN << u8Ch, pstcLvdInit->u32State << u8Ch);
        /* Delay 10us */
        DDL_DelayUS(10U);
        /* Configure the filter */
        MODIFY_REG8(CM_PWC->PVDFCR, (PWC_PVDFCR_PVD1NFDIS | PWC_PVDFCR_PVD1NFCKS) << PWC_LVD_BIT_OFFSET(u8Ch), \
                    (pstcLvdInit->u32Filter | pstcLvdInit->u32FilterClock) << PWC_LVD_BIT_OFFSET(u8Ch));
        /* Config LVD threshold voltage */
        MODIFY_REG8(CM_PWC->PVDLCR, PWC_PVDLCR_PVD1LVL << PWC_LVD_BIT_OFFSET(u8Ch), \
                    pstcLvdInit->u32ThresholdVoltage << PWC_LVD_BIT_OFFSET(u8Ch));
        /* Config the trigger edge */
        MODIFY_REG8(CM_PWC->PVDICR, PWC_PVDICR_PVD1EDGS <<  PWC_LVD_BIT_OFFSET(u8Ch), \
                    pstcLvdInit->u32TriggerEdge << PWC_LVD_BIT_OFFSET(u8Ch));
        /* Enable compare output */
        MODIFY_REG8(CM_PWC->PVDCR1, PWC_PVDCR1_PVD1CMPOE << PWC_LVD_BIT_OFFSET(u8Ch), \
                    pstcLvdInit->u32CompareOutputState << PWC_LVD_BIT_OFFSET(u8Ch));
        /* config exception type while PVDEN & PVDCMPOE enable */
        MODIFY_REG8(CM_PWC->PVDCR1, PWC_PVDCR1_PVD1IRS << PWC_LVD_BIT_OFFSET(u8Ch), \
                    (pstcLvdInit->u32ExceptionType & 0xFFU) << PWC_LVD_BIT_OFFSET(u8Ch));
        MODIFY_REG8(CM_PWC->PVDICR, PWC_PVDICR_PVD1NMIS <<  PWC_LVD_BIT_OFFSET(u8Ch), \
                    ((pstcLvdInit->u32ExceptionType >> PWC_LVD_EXP_NMI_POS) & 0xFFU) << PWC_LVD_BIT_OFFSET(u8Ch));
        /* Clear the flag */
        CLR_REG8_BIT(CM_PWC->PVDDSR, PWC_PVDDSR_PVD1DETFLG << PWC_LVD_BIT_OFFSET(u8Ch));
        /* Enable exception */
        MODIFY_REG8(CM_PWC->PVDCR1, PWC_PVDCR1_PVD1IRE << PWC_LVD_BIT_OFFSET(u8Ch), \
                    (pstcLvdInit->u32ExceptionType & 0xFFU) << PWC_LVD_BIT_OFFSET(u8Ch));

    }
    return i32Ret;
}

/**
 * @brief De-initialize PWC LVD.
 * @param [in] u8Ch LVD channel @ref PWC_LVD_Channel.
 * @retval None
 */
void PWC_LVD_DeInit(uint8_t u8Ch)
{
    DDL_ASSERT(IS_PWC_LVD_CH(u8Ch));
    DDL_ASSERT(IS_PWC_LVD_UNLOCKED());

    /* Disable LVD */
    CLR_REG_BIT(CM_PWC->PVDCR0, PWC_PVDCR0_PVD1EN << u8Ch);
    /* Disable Ext-Vcc */
    if (PWC_LVD_CH2 == u8Ch) {
        CLR_REG8_BIT(CM_PWC->PVDCR0, PWC_PVDCR0_EXVCCINEN);
    } else {
        /* rsvd */
    }
    /* Reset filter */
    MODIFY_REG8(CM_PWC->PVDFCR, (PWC_PVDFCR_PVD1NFDIS | PWC_PVDFCR_PVD1NFCKS) << PWC_LVD_BIT_OFFSET(u8Ch), \
                (PWC_PVDFCR_PVD1NFDIS) << PWC_LVD_BIT_OFFSET(u8Ch));
    /* Reset configure */
    CLR_REG8_BIT(CM_PWC->PVDCR1, (PWC_PVDCR1_PVD1IRE | PWC_PVDCR1_PVD1IRS | PWC_PVDCR1_PVD1CMPOE) << \
                 PWC_LVD_BIT_OFFSET(u8Ch));
    CLR_REG8_BIT(CM_PWC->PVDICR, (PWC_PVDICR_PVD1NMIS | PWC_PVDICR_PVD1EDGS) << PWC_LVD_BIT_OFFSET(u8Ch));
    /* Reset threshold voltage */
    CLR_REG8_BIT(CM_PWC->PVDLCR, PWC_PVDLCR_PVD1LVL << PWC_LVD_BIT_OFFSET(u8Ch));
    /* Clear the flag */
    CLR_REG8_BIT(CM_PWC->PVDDSR, PWC_PVDDSR_PVD1DETFLG << PWC_LVD_BIT_OFFSET(u8Ch));
}

/**
 * @brief  Enable or disable LVD.
 * @param  [in] u8Ch Specifies which channel to operate. @ref PWC_LVD_Channel.
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_LVD_Cmd(uint8_t u8Ch, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_LVD_CH(u8Ch));
    DDL_ASSERT(IS_PWC_LVD_UNLOCKED());

    if (ENABLE == enNewState) {
        SET_REG_BIT(PWC_LVD_EN_REG, PWC_LVD_EN_BIT << PWC_LVD_EN_BIT_OFFSET(u8Ch));
    } else {
        CLR_REG_BIT(PWC_LVD_EN_REG, PWC_LVD_EN_BIT << PWC_LVD_EN_BIT_OFFSET(u8Ch));
    }
}

/**
 * @brief  Enable or disable LVD external input.
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_LVD_ExtInputCmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_LVD_UNLOCKED());

    if (ENABLE == enNewState) {
        SET_REG8_BIT(CM_PWC->PVDCR0, PWC_PVDCR0_EXVCCINEN);
    } else {
        CLR_REG8_BIT(CM_PWC->PVDCR0, PWC_PVDCR0_EXVCCINEN);
    }
}

/**
 * @brief  Enable or disable LVD compare output.
 * @param  [in] u8Ch Specifies which channel to operate. @ref PWC_LVD_Channel.
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_LVD_CompareOutputCmd(uint8_t u8Ch, en_functional_state_t enNewState)
{

    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_LVD_CH(u8Ch));
    DDL_ASSERT(IS_PWC_LVD_UNLOCKED());

    if (ENABLE == enNewState) {
        SET_REG8_BIT(CM_PWC->PVDCR1, PWC_PVDCR1_PVD1CMPOE << PWC_LVD_BIT_OFFSET(u8Ch));
    } else {
        CLR_REG8_BIT(CM_PWC->PVDCR1, PWC_PVDCR1_PVD1CMPOE << PWC_LVD_BIT_OFFSET(u8Ch));
    }
}

/**
 * @brief  Enable or disable LVD digital filter.
 * @param  [in] u8Ch Specifies which channel to operate. @ref PWC_LVD_Channel.
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_LVD_DigitalFilterCmd(uint8_t u8Ch, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_LVD_CH(u8Ch));
    DDL_ASSERT(IS_PWC_LVD_UNLOCKED());

    if (ENABLE == enNewState) {
        CLR_REG_BIT(PWC_LVD_FILTER_EN_REG, PWC_LVD_FILTER_EN_BIT << PWC_LVD_BIT_OFFSET(u8Ch));
    } else {
        SET_REG_BIT(PWC_LVD_FILTER_EN_REG, PWC_LVD_FILTER_EN_BIT << PWC_LVD_BIT_OFFSET(u8Ch));
    }
}

/**
 * @brief  Enable or disable LVD compare output.
 * @param  [in] u8Ch Specifies which channel to operate. @ref PWC_LVD_Channel.
 * @param  [in] u32Clock Specifies filter clock. @ref PWC_LVD_DFS_Clk_Sel
 * @retval None
 */
void PWC_LVD_SetFilterClock(uint8_t u8Ch, uint32_t u32Clock)
{
    DDL_ASSERT(IS_PWC_LVD_CH(u8Ch));
    DDL_ASSERT(IS_PWC_LVD_FILTER_CLK(u32Clock));
    DDL_ASSERT(IS_PWC_LVD_UNLOCKED());
    DDL_ASSERT(IS_PWC_LVD_DIG_FILTER_DISABLED(u8Ch));

    MODIFY_REG8(CM_PWC->PVDFCR, PWC_PVDFCR_PVD1NFCKS << PWC_LVD_BIT_OFFSET(u8Ch), \
                u32Clock << PWC_LVD_BIT_OFFSET(u8Ch));
}

/**
 * @brief Set LVD threshold voltage.
 * @param  [in] u8Ch        Specifies which channel to operate. @ref PWC_LVD_Channel.
 * @param  [in] u32Voltage  Specifies threshold voltage. @ref PWC_LVD_Detection_Voltage_Sel
 * @retval None
 */
void PWC_LVD_SetThresholdVoltage(uint8_t u8Ch, uint32_t u32Voltage)
{
    DDL_ASSERT(IS_PWC_LVD_CH(u8Ch));
    DDL_ASSERT(IS_PWC_LVD_THRESHOLD_VOLTAGE(u32Voltage));
    DDL_ASSERT(IS_PWC_LVD_UNLOCKED());

    MODIFY_REG8(CM_PWC->PVDLCR, (PWC_PVDLCR_PVD1LVL << PWC_LVD_BIT_OFFSET(u8Ch)), \
                u32Voltage << PWC_LVD_BIT_OFFSET(u8Ch));
}

/**
 * @brief  Get LVD flag.
 * @param  [in] u8Flag LVD flag to be get @ref PWC_LVD_Flag
 * @retval An @ref en_flag_status_t enumeration value
 * @note   PVDxDETFLG is available when PVDCR0.PVDxEN and PVDCR1.PVDxCMPOE are set to '1'
 */
en_flag_status_t PWC_LVD_GetStatus(uint8_t u8Flag)
{
    DDL_ASSERT(IS_PWC_LVD_GET_FLAG(u8Flag));
    return ((0x00U != READ_REG8_BIT(PWC_LVD_STATUS_REG, u8Flag)) ? SET : RESET);
}

/**
 * @brief  Clear LVD flag.
 * @param  [in] u8Flag LVD flag to be get @ref PWC_LVD_Flag
 *  @arg      PWC_LVD1_FLAG_DETECT
 *  @arg      PWC_LVD2_FLAG_DETECT
 * @retval None
 */
void PWC_LVD_ClearStatus(uint8_t u8Flag)
{

    DDL_ASSERT(IS_PWC_LVD_UNLOCKED());
    DDL_ASSERT(IS_PWC_LVD_CLR_FLAG(u8Flag));

    WRITE_REG8(PWC_LVD_STATUS_REG, (~u8Flag) & (uint8_t)PWC_LVD_FLAG_CLR_MSK);
}

/**
 * @brief  LDO(HRC & PLL) command.
 * @param  [in] u16Ldo  Specifies the LDO to command.
 *  @arg    PWC_LDO_HRC
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_LDO_Cmd(uint16_t u16Ldo, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_LDO_SEL(u16Ldo));
    DDL_ASSERT(IS_PWC_UNLOCKED());

    if (ENABLE == enNewState) {
        CLR_REG8_BIT(CM_PWC->PWRC1, u16Ldo);
    } else {
        SET_REG8_BIT(CM_PWC->PWRC1, u16Ldo);
    }
}

/**
 * @brief  Ram area power down command.
 * @param  [in] u32Ram Specifies which ram to operate. @ref PWC_PD_Ram
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 *  @arg    ENABLE:      Power down mode
 *  @arg    DISABLE:     Run mode
 * @retval None
 */
void PWC_PD_RamCmd(uint32_t u32Ram, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_RAM_CONTROL(u32Ram));
    DDL_ASSERT(IS_PWC_UNLOCKED());

    if (ENABLE == enNewState) {
        SET_REG8_BIT(CM_PWC->RAMPC0, u32Ram);
    } else {
        CLR_REG8_BIT(CM_PWC->RAMPC0, u32Ram);
    }
}

/**
 * @brief Stop mode config.
 * @param [in] pstcStopConfig Chip config before entry stop mode.
 *  @arg    u8StopDrv, MCU from which speed mode entry stop mode.
 *  @arg    u16Clock, System clock setting after wake-up from stop mode.
 *  @arg    u16FlashWait, Whether wait flash stable after wake-up from stop mode.
 *  @arg    u16ExBusHold, ExBus status in stop mode.
 * @retval int32_t:
 *          - LL_OK: Stop mode config successful
 *          - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PWC_STOP_Config(const stc_pwc_stop_mode_config_t *pstcStopConfig)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcStopConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {

        DDL_ASSERT(IS_PWC_UNLOCKED());

        DDL_ASSERT(IS_PWC_STOP_CLK(pstcStopConfig->u16Clock));
        DDL_ASSERT(IS_PWC_STOP_FLASH_WAIT(pstcStopConfig->u16FlashWait));
        MODIFY_REG16(CM_PWC->STPMCR, (PWC_STPMCR_CKSMRC | PWC_STPMCR_FLNWT), \
                     (pstcStopConfig->u16Clock | pstcStopConfig->u16FlashWait));

    }
    return i32Ret;
}

/**
 * @brief  Initialize stop mode config structure. Fill each pstcStopConfig with default value
 * @param  [in] pstcStopConfig Pointer to a stc_pwc_stop_mode_config_t structure that
 *                            contains configuration information.
 * @retval int32_t:
 *          - LL_OK: Stop down mode structure initialize successful
 *          - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t PWC_STOP_StructInit(stc_pwc_stop_mode_config_t *pstcStopConfig)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcStopConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcStopConfig->u16Clock = PWC_STOP_CLK_KEEP;
        pstcStopConfig->u16FlashWait = PWC_STOP_FLASH_WAIT_ON;
    }
    return i32Ret;
}

/**
 * @brief Stop mode wake up clock config.
 * @param [in] u8Clock System clock setting after wake-up from stop mode. @ref PWC_STOP_CLK_Sel
 * @retval None
 */
void PWC_STOP_ClockSelect(uint8_t u8Clock)
{
    DDL_ASSERT(IS_PWC_STOP_CLK(u8Clock));
    DDL_ASSERT(IS_PWC_UNLOCKED());

    MODIFY_REG16(CM_PWC->STPMCR, PWC_STPMCR_CKSMRC, (uint16_t)u8Clock);

}

/**
 * @brief  Stop mode wake up flash wait config.
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_STOP_FlashWaitCmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_UNLOCKED());

    if (ENABLE == enNewState) {
        CLR_REG16_BIT(CM_PWC->STPMCR, PWC_STPMCR_FLNWT);
    } else {
        SET_REG16_BIT(CM_PWC->STPMCR, PWC_STPMCR_FLNWT);
    }
}

/**
 * @brief  PWC power monitor command.
 * @param  [in] enNewState      An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   This monitor power is used for ADC and output to REGC pin.
 */
void PWC_PowerMonitorCmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_UNLOCKED());
    if (ENABLE == enNewState) {
        SET_REG8_BIT(CM_PWC->PWRC4, PWC_PWRC4_ADBUFE);
    } else {
        CLR_REG8_BIT(CM_PWC->PWRC4, PWC_PWRC4_ADBUFE);
    }

}

/**
 * @brief  PWC power monitor voltage config.
 * @param  [in] u8VoltageSrc   PWC power monitor voltage config @ref PWC_Monitor_Power.
 *         This parameter can be one of the following values
 *  @arg    PWC_PWR_MON_IREF
 * @retval None
 * @note   This monitor power is used for ADC and output to REGC pin.
 */
void PWC_SetPowerMonitorVoltageSrc(uint8_t u8VoltageSrc)
{
    DDL_ASSERT(IS_PWC_PWR_MON_SEL(u8VoltageSrc));
    DDL_ASSERT(IS_PWC_UNLOCKED());

    MODIFY_REG8(CM_PWC->PWRC4, PWC_PWRC4_ADBUFS, u8VoltageSrc);
}

/**
 * @brief  Port reset event config.
 * @param  [in] u8Event Reset event. @ref PWC_Port_Reset_Sel
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_PortResetCmd(uint8_t u8Event, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_PWC_PORT_RST_EVT(u8Event));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_UNLOCKED());

    if (ENABLE == enNewState) {
        CLR_REG8_BIT(CM_PWC->PWRC6, u8Event);
    } else {
        SET_REG8_BIT(CM_PWC->PWRC6, u8Event);
    }
}

/**
 * @brief  DAC reset event config.
 * @param  [in] u8Event Reset event. @ref PWC_Dac_Reset_Sel
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_DacResetCmd(uint8_t u8Event, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_PWC_DAC_RST_EVT(u8Event));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_UNLOCKED());

    if (ENABLE == enNewState) {
        CLR_REG8_BIT(CM_PWC->PWRC6, u8Event);
    } else {
        SET_REG8_BIT(CM_PWC->PWRC6, u8Event);
    }
}

/**
 * @brief  CMP reset event config.
 * @param  [in] u8Event Reset event. @ref PWC_Cmp_Reset_Sel
 * @param  [in] enNewState An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void PWC_CmpResetCmd(uint8_t u8Event, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_PWC_CMP_RST_EVT(u8Event));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    DDL_ASSERT(IS_PWC_UNLOCKED());

    if (ENABLE == enNewState) {
        CLR_REG8_BIT(CM_PWC->PWRC6, u8Event);
    } else {
        SET_REG8_BIT(CM_PWC->PWRC6, u8Event);
    }
}

/**
 * @brief  Release port status.
 * @param  [in] u8Event Release event. @ref PWC_Port_Release_Sel
 * @retval None
 */
void PWC_ReleasePort(uint8_t u8Event)
{
    DDL_ASSERT(IS_PWC_UNLOCKED());
    DDL_ASSERT(IS_PWC_PORT_RELEASE_EVT(u8Event));

    SET_REG8_BIT(CM_PWC->PWRC6, u8Event);
}

/**
 * @}
 */

#endif  /* LL_PWC_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
