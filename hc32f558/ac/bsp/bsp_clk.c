// SPDX-License-Identifier: MIT
/**
 * @file    bsp_clk.c
 * @brief   HC32F558 system clock BSP module.
 * @details
 *          This file is part of the HC32F558 project.
 *
 *          Module responsibilities:
 *          - Configure XTAL and PLLH for the maximum 240 MHz system clock
 *          - Configure bus dividers and memory wait cycles for high-speed operation
 *          - Enable EFM Flash instruction/data cache and prefetch acceleration
 *          - Update CMSIS SystemCoreClock after switching the system clock source
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path is not required; initialization runs before scheduler start
 *          - Hardware access is abstracted through the HC32 LL driver
 *
 * @author  Max.Li
 * @date    2026-06-06
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_clk.h"

#include "hc32_ll.h"

#define BSP_CLK_XTAL_PORT (GPIO_PORT_F)
#define BSP_CLK_XTAL_PIN  (GPIO_PIN_00 | GPIO_PIN_01)

static void bsp_clk_flash_cache_enable(void)
{
    EFM_CacheRamReset(ENABLE);
    EFM_CacheRamReset(DISABLE);
    EFM_CacheCmd(EFM_CACHE_ALL, ENABLE);
}

int32_t bsp_clk_init(void)
{
    stc_clock_xtal_init_t xtal_init;
    stc_clock_pll_init_t pll_init;

    LL_PERIPH_WE(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO |
                 LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM);

    CLK_SetClockDiv(CLK_BUS_CLK_ALL,
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV4 |
                     CLK_PCLK3_DIV4 | CLK_PCLK4_DIV2 | CLK_HCLK_DIV1));

    GPIO_AnalogCmd(BSP_CLK_XTAL_PORT, BSP_CLK_XTAL_PIN, ENABLE);

    (void)CLK_XtalStructInit(&xtal_init);
    xtal_init.u8Mode = CLK_XTAL_MD_OSC;
    xtal_init.u8Drv = CLK_XTAL_DRV_ULOW;
    xtal_init.u8State = CLK_XTAL_ON;
    xtal_init.u8StableTime = CLK_XTAL_STB_2MS;
    if (LL_OK != CLK_XtalInit(&xtal_init)) {
        LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO |
                     LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM);
        return LL_ERR;
    }

    (void)CLK_PLLStructInit(&pll_init);
    pll_init.u8PLLState = CLK_PLL_ON;
    pll_init.PLLCFGR = 0UL;
    pll_init.PLLCFGR_f.PLLM = 1UL - 1UL;
    pll_init.PLLCFGR_f.PLLN = 120UL - 1UL;
    pll_init.PLLCFGR_f.PLLP = 4UL - 1UL;
    pll_init.PLLCFGR_f.PLLQ = 4UL - 1UL;
    pll_init.PLLCFGR_f.PLLR = 4UL - 1UL;
    pll_init.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    if (LL_OK != CLK_PLLInit(&pll_init)) {
        LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO |
                     LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM);
        return LL_ERR;
    }

    SRAM_SetWaitCycle(SRAM_SRAM_ALL, SRAM_WAIT_CYCLE1, SRAM_WAIT_CYCLE1);
    if (LL_OK != EFM_SetWaitCycle(EFM_WAIT_CYCLE5)) {
        LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO |
                     LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM);
        return LL_ERR;
    }
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT4);

    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);
    SystemCoreClockUpdate();
    bsp_clk_flash_cache_enable();

    LL_PERIPH_WP(LL_PERIPH_EFM | LL_PERIPH_FCG | LL_PERIPH_GPIO |
                 LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_SRAM);

    return (SystemCoreClock == BSP_CLK_SYSCLK_HZ) ? LL_OK : LL_ERR;
}
