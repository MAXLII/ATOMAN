#include "main.h"
#include "bsp_clk.h"
#include "systick.h"
#include "section.h"
#include "hc32_ll.h"
#include "gpio.h"
#include "my_math.h"

#define LL_PERIPH_SEL (LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU | \
                       LL_PERIPH_EFM | LL_PERIPH_SRAM)

int main(void)
{
    LL_PERIPH_WE(LL_PERIPH_SEL);
    BSP_CLK_Init();
    systick_config();
    section_init();
    while (1)
    {
        run_task();
    }
}

void HRPWM_1_Ovf_Udf_Handler(void)
{
    gpio_set_test1(1);
    if (CM_HRPWM1->STFLR1 & (1 << 7))
    {
        section_interrupt();
        CM_HRPWM1->STFLR1 &= ~(1 << 7);
    }
    gpio_set_test1(0);
}
