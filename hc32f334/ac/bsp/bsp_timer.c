#include "bsp_timer.h"
#include "section.h"

#define BSP_TIMER_PERF_UNIT              (CM_TMRA_1)
#define BSP_TIMER_PERF_FCG               (FCG2_PERIPH_TMRA_1)
#define BSP_TIMER_PERF_CLK_DIV           (TMRA_CLK_DIV64)
#define BSP_TIMER_PERF_PERIOD_VALUE      (0xFFFFFFFFUL)

REG_PERF_BASE_CNT((uint32_t *)&BSP_TIMER_PERF_UNIT->CNTER)

int32_t bsp_timer_init(void)
{
    stc_tmra_init_t stcTmraInit;

    LL_PERIPH_WE(LL_PERIPH_FCG);

    FCG_Fcg2PeriphClockCmd(BSP_TIMER_PERF_FCG, ENABLE);

    (void)TMRA_StructInit(&stcTmraInit);
    stcTmraInit.u8CountSrc = TMRA_CNT_SRC_SW;
    stcTmraInit.sw_count.u8ClockDiv = BSP_TIMER_PERF_CLK_DIV;
    stcTmraInit.sw_count.u8CountMode = TMRA_MD_SAWTOOTH;
    stcTmraInit.sw_count.u8CountDir = TMRA_DIR_UP;
    stcTmraInit.u8CountReload = TMRA_CNT_RELOAD_ENABLE;
    stcTmraInit.u32PeriodValue = BSP_TIMER_PERF_PERIOD_VALUE;

    TMRA_Stop(BSP_TIMER_PERF_UNIT);
    TMRA_SetCountValue(BSP_TIMER_PERF_UNIT, 0UL);
    if (LL_OK != TMRA_Init(BSP_TIMER_PERF_UNIT, &stcTmraInit))
    {
        LL_PERIPH_WP(LL_PERIPH_FCG);
        return LL_ERR;
    }

    TMRA_Start(BSP_TIMER_PERF_UNIT);

    LL_PERIPH_WP(LL_PERIPH_FCG);
    return LL_OK;
}

uint32_t bsp_timer_get_perf_cnt(void)
{
    return TMRA_GetCountValue(BSP_TIMER_PERF_UNIT);
}

static void bsp_timer_init_entry(void)
{
    (void)bsp_timer_init();
}

REG_INIT(0, bsp_timer_init_entry)
