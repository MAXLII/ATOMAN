#include "bsp_pwm.h"
#include "hc32_ll.h"
#include "section.h"
#include "my_math.h"

#define BSP_PWM_MASTER_UNIT CM_HRPWM1
#define BSP_PWM_FAST_UNIT CM_HRPWM2
#define BSP_PWM_SLOW_UNIT CM_HRPWM3

#define BSP_PWM_MASTER_IRQn HRPWM_1_OVF_UDF_IRQn
#define BSP_PWM_MASTER_IRQ_PRIO DDL_IRQ_PRIO_01

#define BSP_PWM_CLK_HZ (120000000.0f * 64.0f)
#define BSP_PWM_PERIOD_REG ((uint32_t)((BSP_PWM_CLK_HZ / PFC_PWM_FREQ / 2.0f) + 0.5f))
#define BSP_PWM_COMPARE_MIN_REG (0x000000C0UL)
#define BSP_PWM_DEADTIME_REG (60UL * 64UL)

#define BSP_PWM_POL_LOW (0UL)
#define BSP_PWM_POL_HIGH (1UL)
#define BSP_PWM_POL_HOLD (2UL)

typedef struct
{
    uint8_t reserved;
} bsp_pwm_req_t;

static volatile bsp_pwm_req_t s_pwm_req = {
    .reserved = 0U,
};

static float bsp_pwm_clamp(float value, float min, float max)
{
    if (value < min)
    {
        return min;
    }
    if (value > max)
    {
        return max;
    }
    return value;
}

static void bsp_pwm_write_unit_buffer(CM_HRPWM_TypeDef *unit, float duty, uint8_t up_en, uint8_t dn_en)
{
    uint32_t compare;
    uint32_t forca_cfg;
    uint32_t cmauca_cfg;
    uint32_t cmadca_cfg;

    if (duty < 0.0f)
    {
        duty = 0.0f;
    }
    else if (duty > 1.0f)
    {
        duty = 1.0f;
    }

    compare = (uint32_t)((float)BSP_PWM_PERIOD_REG * duty);

    if (compare <= BSP_PWM_COMPARE_MIN_REG)
    {
        compare = BSP_PWM_COMPARE_MIN_REG + 1U;
        cmauca_cfg = 2UL;
        cmadca_cfg = 2UL;
        forca_cfg = 2UL;
    }
    else if (compare >= (BSP_PWM_PERIOD_REG - 0x40))
    {
        compare = BSP_PWM_PERIOD_REG - 0x41U;
        cmauca_cfg = 2UL;
        cmadca_cfg = 2UL;
        forca_cfg = 3UL;
    }
    else
    {
        cmauca_cfg = 0UL;
        cmadca_cfg = 1UL;
        forca_cfg = 0UL;
    }

    if (compare >= BSP_PWM_PERIOD_REG)
    {
        compare = BSP_PWM_PERIOD_REG;
    }

    unit->HRGCMCR = compare;

    uint32_t bpcnxr1 = 0x0000A0AAUL;

    if (up_en == 0)
    {
        bpcnxr1 |= (unit->BPCNAR1 & ~(HRPWM_BPCNAR1_CMAUCA | HRPWM_BPCNAR1_CMADCA |
                                      HRPWM_BPCNAR1_FORCA | HRPWM_BPCNAR1_OUTENA)) |
                   (cmauca_cfg << HRPWM_BPCNAR1_CMAUCA_POS) |
                   (cmadca_cfg << HRPWM_BPCNAR1_CMADCA_POS) |
                   (forca_cfg << HRPWM_BPCNAR1_FORCA_POS) |
                   (0 << HRPWM_BPCNAR1_OUTENA_POS);
    }
    else
    {
        bpcnxr1 |= (unit->BPCNAR1 & ~(HRPWM_BPCNAR1_CMAUCA | HRPWM_BPCNAR1_CMADCA |
                                      HRPWM_BPCNAR1_FORCA | HRPWM_BPCNAR1_OUTENA)) |
                   (cmauca_cfg << HRPWM_BPCNAR1_CMAUCA_POS) |
                   (cmadca_cfg << HRPWM_BPCNAR1_CMADCA_POS) |
                   (forca_cfg << HRPWM_BPCNAR1_FORCA_POS) |
                   (1 << HRPWM_BPCNAR1_OUTENA_POS);
    }

    unit->BPCNAR1 = bpcnxr1;

    unit->BPCNBR1 = (unit->BPCNBR1 & ~HRPWM_BPCNBR1_OUTENB) |
                    (((dn_en != 0U) ? 1UL : 0UL) << HRPWM_BPCNBR1_OUTENB_POS);
}

static void bsp_pwm_config_channel_waveform(CM_HRPWM_TypeDef *unit)
{
    unit->HRGCMAR = BSP_PWM_COMPARE_MIN_REG;
    unit->PCNAR1 = 0x0000AAAAUL;
    unit->PCNAR2 = 0x0AAAAAAAUL;
    unit->PCNAR3 = 0x0AAAAAAAUL;
    unit->BPCNAR1 = 0x0000AAAAUL;
    unit->BPCNAR2 = 0x0AAAAAAAUL;
    unit->BPCNAR3 = 0x0AAAAAAAUL;
}

static void bsp_pwm_config_buffer(CM_HRPWM_TypeDef *unit)
{
    unit->BCONR1 |= (1 << 0) |
                    (1 << 3);

    unit->BCONR2 = (1UL << 12) |
                   (1UL << 14);
}

static void bsp_pwm_config_output_unit(CM_HRPWM_TypeDef *unit)
{
    stc_hrpwm_init_t base;

    (void)HRPWM_StructInit(&base);
    base.u32CountMode = HRPWM_MD_TRIANGLE;
    base.u32PeriodValue = BSP_PWM_PERIOD_REG;
    base.u32CountReload = HRPWM_CNT_RELOAD_ON;
    (void)HRPWM_Init(unit, &base);

    bsp_pwm_config_channel_waveform(unit);
    bsp_pwm_config_buffer(unit);

    unit->HRDTUAR = BSP_PWM_DEADTIME_REG;
    unit->HRDTDAR = BSP_PWM_DEADTIME_REG;
    unit->HRDTUBR = BSP_PWM_DEADTIME_REG;
    unit->HRDTDBR = BSP_PWM_DEADTIME_REG;
    unit->DCONR = (1UL << HRPWM_DCONR_DTCEN_POS);

    unit->GCONR = 1 << 2;

    unit->HCLRR2 |= (1 << 10);

    unit->HCLRR1 |= (1 << 7);
}

void bsp_pwm_int_handler(void)
{
    s_pwm_req.reserved = s_pwm_req.reserved;
}

static void bsp_pwm_init(void)
{
    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_HRPWM_1 |
                               FCG2_PERIPH_HRPWM_2 |
                               FCG2_PERIPH_HRPWM_3,
                           ENABLE);

    GPIO_HrpwmPinCmd(GPIO_HRPWM2_PWMA |
                         GPIO_HRPWM2_PWMB |
                         GPIO_HRPWM3_PWMA |
                         GPIO_HRPWM3_PWMB,
                     ENABLE);

    HRPWM_CommonDeInit();
    HRPWM_CALIB_PeriodInit(HRPWM_CALIB_PERIOD_8P7_MS);

    /* HRPWM1 */
    CM_HRPWM1->CR |= 1 << 3;

    CM_HRPWM1->CNTER = 0;
    CM_HRPWM1->UPDAR = 0;
    CM_HRPWM1->HRPERAR = 0xFFFF << 6;
    CM_HRPWM1->HRPERBR = 0xFFFF << 6;
    bCM_HRPWM1->HCLRR2_b.HCLRGCMA1 = 1;

    bCM_HRPWM1->HCLRR1_b.CLES = 1;

    bCM_HRPWM1->BCONR1_b.BENAE = 1;
    bCM_HRPWM1->BCONR1_b.BTRDAE = 1;

    CM_HRPWM1->ICONR |= 1 << 7;

    CM_HRPWM1->HRGCMAR = BSP_PWM_PERIOD_REG * 2;

    bCM_HRPWM_COMMON->GBCONR_b.OSTENU1 = 1;

    bCM_HRPWM1->GCONR_b.START = 1;

    bsp_pwm_config_output_unit(BSP_PWM_FAST_UNIT);
    bsp_pwm_config_output_unit(BSP_PWM_SLOW_UNIT);

    CM_HRPWM_COMMON->SSTAIDLR |= ((HRPWM_SW_SYNC_UNIT2 | HRPWM_SW_SYNC_UNIT3) & HRPWM_SW_SYNC_CH_ALL);

    NVIC_ClearPendingIRQ(BSP_PWM_MASTER_IRQn);
    NVIC_SetPriority(BSP_PWM_MASTER_IRQn, BSP_PWM_MASTER_IRQ_PRIO);
    NVIC_EnableIRQ(BSP_PWM_MASTER_IRQn);

    CM_HRPWM_COMMON->SSTAR = (HRPWM_UNIT1 | HRPWM_UNIT2 | HRPWM_UNIT3);
}

REG_INIT(0, bsp_pwm_init)

void bsp_pwm_enable(void)
{
    HRPWM_IDLE_Exit(HRPWM_SW_SYNC_UNIT2 | HRPWM_SW_SYNC_UNIT3, HRPWM_SW_SYNC_CH_ALL);
}

void bsp_pwm_disable(void)
{
    bsp_pwm_set_duty(0.0f, 0.0f, 0, 0, 0, 0);
    HRPWM_IDLE_DELAY_MultiUnitSWTrigger(HRPWM_UNIT2 | HRPWM_UNIT3);
}

void bsp_pwm_set_duty(float duty_fast,
                      float duty_slow,
                      uint8_t up_en_fast,
                      uint8_t dn_en_fast,
                      uint8_t up_en_slow,
                      uint8_t dn_en_slow)
{
    bsp_pwm_write_unit_buffer(BSP_PWM_FAST_UNIT,
                              bsp_pwm_clamp(duty_fast, 0.0f, 1.0f),
                              (up_en_fast != 0U) ? 1U : 0U,
                              (dn_en_fast != 0U) ? 1U : 0U);
    bsp_pwm_write_unit_buffer(BSP_PWM_SLOW_UNIT,
                              bsp_pwm_clamp(duty_slow, 0.0f, 1.0f),
                              (up_en_slow != 0U) ? 1U : 0U,
                              (dn_en_slow != 0U) ? 1U : 0U);
}
