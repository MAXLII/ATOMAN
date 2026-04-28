#include "bsp_clk.h"

#define BSP_XTAL_PORT                   (GPIO_PORT_F)
#define BSP_XTAL_IN_PIN                 (GPIO_PIN_00)
#define BSP_XTAL_OUT_PIN                (GPIO_PIN_01)

void BSP_CLK_Init(void)
{
    stc_clock_xtal_init_t stcXtalInit;
    stc_clock_pll_init_t stcPLLHInit;

    CLK_SetClockDiv(CLK_BUS_CLK_ALL,
                    (CLK_PCLK0_DIV1 | CLK_PCLK1_DIV2 | CLK_PCLK2_DIV2 |
                     CLK_PCLK3_DIV2 | CLK_PCLK4_DIV2 | CLK_HCLK_DIV1));

    GPIO_AnalogCmd(BSP_XTAL_PORT, BSP_XTAL_IN_PIN | BSP_XTAL_OUT_PIN, ENABLE);

    (void)CLK_XtalStructInit(&stcXtalInit);
    stcXtalInit.u8Mode = CLK_XTAL_MD_OSC;
    stcXtalInit.u8Drv = CLK_XTAL_DRV_ULOW;
    stcXtalInit.u8State = CLK_XTAL_ON;
    stcXtalInit.u8StableTime = CLK_XTAL_STB_2MS;
    (void)CLK_XtalInit(&stcXtalInit);

    (void)CLK_PLLStructInit(&stcPLLHInit);
    stcPLLHInit.u8PLLState = CLK_PLL_ON;
    stcPLLHInit.PLLCFGR = 0UL;
    stcPLLHInit.PLLCFGR_f.PLLM = 1UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLN = 60UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLP = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLQ = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLR = 4UL - 1UL;
    stcPLLHInit.PLLCFGR_f.PLLSRC = CLK_PLL_SRC_XTAL;
    (void)CLK_PLLInit(&stcPLLHInit);

    (void)EFM_SetWaitCycle(EFM_WAIT_CYCLE2);
    GPIO_SetReadWaitCycle(GPIO_RD_WAIT2);
    CLK_SetSysClockSrc(CLK_SYSCLK_SRC_PLL);

    EFM_CacheRamReset(ENABLE);
    EFM_CacheRamReset(DISABLE);
    EFM_CacheCmd(EFM_CACHE_ALL, ENABLE);

    SystemCoreClockUpdate();
}
