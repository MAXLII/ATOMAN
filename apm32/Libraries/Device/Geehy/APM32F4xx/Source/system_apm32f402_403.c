/*!
 * @file        system_apm32f402_403.c
 *
 * @brief       CMSIS Cortex-M4 Device Peripheral Access Layer System Source File
 *
 * @version     V1.0.1
 *
 * @date        2025-02-14
 *
 * @attention
 *
 *  Copyright (C) 2024-2025 Geehy Semiconductor
 *
 *  You may not use this file except in compliance with the
 *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
 *
 *  The program is only for reference, which is distributed in the hope
 *  that it will be useful and instructional for customers to develop
 *  their software. Unless required by applicable law or agreed to in
 *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
 *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
 *  and limitations under the License.
 */

#include "apm32f402_403.h"

/** @addtogroup CMSIS
  @{
*/

/** @addtogroup APM32F402_403_System
  @{
*/

/** @defgroup System_Macros
  @{
*/

/* #define VECT_TAB_SRAM */

#ifdef FIRMWARE_TYPE_IAP
// IAP版本，从0x08006000开始
#define VECT_TAB_OFFSET 0x6000
#else
// ISP版本，从0x08000000开始
#define VECT_TAB_OFFSET 0x0000
#endif

/**@} end of group System_Macros */

/** @defgroup System_Variables
  @{
*/

uint32_t SystemCoreClock = 16000000;

/**@} end of group System_Variables */

/** @defgroup System_Functions
  @{
*/

static void SystemClockConfig(void);

/*!
 * @brief       Setup the microcontroller system
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
void SystemInit (void)
{
    /* FPU settings */
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10 * 2)|(3UL << 11 * 2));  //!< set CP10 and CP11 Full Access
    #endif
    /** Set HSIEN bit */
    RCM->CTRL_B.HSIEN = BIT_SET;
    /** Reset SCLKSEL, AHBPSC, APB1PSC, APB2PSC, ADCPSC and MCOSEL bits */
    RCM->CFG &= (uint32_t)0xF8FF0000;
    /** Reset HSEEN, CSSEN and PLLEN bits */
    RCM->CTRL &= (uint32_t)0xFEF6FFFF;
    /** Reset HSEBCFG bit */
    RCM->CTRL_B.HSEBCFG = BIT_RESET;
    /** Reset PLLSRCSEL, PLLHSEPSC, PLLMULCFG and USBDIV bits */
    RCM->CFG &= (uint32_t)0xFF80FFFF;
    /** Disable all interrupts and clear pending bits */
    RCM->INT = 0x009F0000;

    SystemClockConfig();

#ifdef VECT_TAB_SRAM
    SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET;
#else
    SCB->VTOR = FMC_BASE | VECT_TAB_OFFSET;
#endif
}

/*!
 * @brief       Update SystemCoreClock variable according to Clock Register Values
 *              The SystemCoreClock variable contains the core clock (HCLK)
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
void SystemCoreClockUpdate (void)
{
    uint32_t sysClock, pllMull, pllSource, Prescaler;
    uint8_t AHBPrescTable[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 6, 7, 8, 9};

    sysClock = RCM->CFG_B.SCLKSELSTS;

    switch(sysClock)
    {
        /** sys clock is HSI */
        case 0:
            SystemCoreClock = HSI_VALUE;
        break;

        /** sys clock is HSE */
        case 1:
            SystemCoreClock = HSE_VALUE;
        break;

        /** sys clock is PLL */
        case 2:
            pllMull = RCM->CFG_B.PLLMULCFG + 2;
            pllSource = RCM->CFG_B.PLLSRCSEL;

            /** PLL entry clock source is HSE */
            if(pllSource == BIT_SET)
            {
                SystemCoreClock = HSE_VALUE * pllMull;

                /** HSE clock divided by 2 */
                if(pllSource == RCM->CFG_B.PLLHSEPSC)
                {
                    SystemCoreClock >>= 1;
                }
            }
            /** PLL entry clock source is HSI/2 */
            else
            {
                SystemCoreClock = (HSI_VALUE >> 1) * pllMull;
            }
            break;

        default:
            SystemCoreClock = HSI_VALUE;
        break;
    }

    Prescaler = AHBPrescTable[RCM->CFG_B.AHBPSC];
    SystemCoreClock >>= Prescaler;
}

/*!
 * @brief       Configures the System clock frequency, HCLK, PCLK2 and PCLK1 prescalers
 *
 * @param       None
 *
 * @retval      None
 *
 * @note
 */
static void SystemClockConfig(void)
{
    __IO uint32_t i;

    RCM->CTRL_B.HSEEN = BIT_SET;

    for (i = 0; i < HSE_STARTUP_TIMEOUT; i++)
    {
        if (RCM->CTRL_B.HSERDYFLG)
        {
            break;
        }
    }

    if (RCM->CTRL_B.HSERDYFLG)
    {
        /* Enable Prefetch Buffer */
        FMC->CTRL1_B.PBEN = BIT_SET;

        /* Enable Instruction Cache and Data Cache */
        FMC->CTRL1_B.ICACHEEN = BIT_SET;
        FMC->CTRL1_B.DCACHEEN = BIT_SET;

        /* Flash 3 wait state */
        FMC->CTRL1_B.WS2_0 = 3;

        /* HCLK = SYSCLK */
        RCM->CFG_B.AHBPSC = 0x00;
        /* PCLK2 = HCLK / 2 */
        RCM->CFG_B.APB2PSC = 4;
        /* PCLK1 = HCLK / 2 */
        RCM->CFG_B.APB1PSC = 4;

        /** PLL: HSE * 15 */
        RCM->CFG_B.PLLSRCSEL = 1;
        RCM->CFG_B.PLLMULCFG = 13;

        /** Enable PLL */
        RCM->CTRL_B.PLLEN = 1;
        /** Wait PLL Ready */
        while(RCM->CTRL_B.PLLRDYFLG == BIT_RESET);

        /* Select PLL as system clock source */
        RCM->CFG_B.SCLKSEL = 2;
        /* Wait till PLL is used as system clock source */
        while(RCM->CFG_B.SCLKSELSTS != 0x02);

        SystemCoreClockUpdate();
    }
}

/**@} end of group System_Functions */
/**@} end of group APM32F402_403_System */
/**@} end of group CMSIS */
