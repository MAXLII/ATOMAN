// SPDX-License-Identifier: MIT
/**
 * @file    bsp_pwm.c
 * @brief   HC32F558 HRPWM BSP implementation.
 * @details
 *          This file is part of the HC32F558 AC project.
 *
 *          Module responsibilities:
 *          - Configure HRPWM1 as the master timing and ADC trigger unit
 *          - Configure HRPWM2/HRPWM3 complementary outputs on PA8/PA9 and PA10/PA11
 *          - Dispatch the HRPWM1 interrupt into the section interrupt scheduler
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Duty update path is suitable for interrupt context
 *          - Hardware access is abstracted through HC32 LL HRPWM/GPIO APIs
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

#include "bsp_pwm.h"
#include "bsp_clk.h"
#include "hc32_ll.h"
#include "my_math.h"
#include "section.h"

#define BSP_PWM_MASTER_UNIT       CM_HRPWM1
#define BSP_PWM_FAST_UNIT         CM_HRPWM2
#define BSP_PWM_SLOW_UNIT         CM_HRPWM3
#define BSP_PWM_OUTPUT_UNITS      (HRPWM_UNIT2 | HRPWM_UNIT3)
#define BSP_PWM_OUTPUT_SYNC_UNITS (HRPWM_SW_SYNC_UNIT2 | HRPWM_SW_SYNC_UNIT3)

#define BSP_PWM_MASTER_IRQ      HRPWM1_IRQn
#define BSP_PWM_MASTER_INT_SRC  INT_SRC_HRPWM1
#define BSP_PWM_MASTER_IRQ_PRIO DDL_IRQ_PRIO_01

#define BSP_PWM_CLK_HZ          ((float)BSP_CLK_PCLK0_HZ * 64.0f)
#define BSP_PWM_PERIOD_REG      ((uint32_t)((BSP_PWM_CLK_HZ / PFC_PWM_FREQ / 2.0f) + 0.5f))
#define BSP_PWM_COMPARE_MIN_REG (0x000000C0UL)
#define BSP_PWM_DEADTIME_REG    (60UL * 64UL)
#define BSP_PWM_ADC_TRIG_ADV    (24UL << 6)

static float bsp_pwm_clamp(float value, float min, float max)
{
    if (value < min) {
        return min;
    }
    if (value > max) {
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
    uint32_t bpcnar1;

    duty = bsp_pwm_clamp(duty, 0.0f, 1.0f);
    compare = (uint32_t)((float)BSP_PWM_PERIOD_REG * duty);

    if (compare <= BSP_PWM_COMPARE_MIN_REG) {
        compare = BSP_PWM_COMPARE_MIN_REG + 1U;
        cmauca_cfg = 2UL;
        cmadca_cfg = 2UL;
        forca_cfg = 2UL;
    } else if (compare >= (BSP_PWM_PERIOD_REG - 0x40UL)) {
        compare = BSP_PWM_PERIOD_REG - 0x41UL;
        cmauca_cfg = 2UL;
        cmadca_cfg = 2UL;
        forca_cfg = 3UL;
    } else {
        cmauca_cfg = 0UL;
        cmadca_cfg = 1UL;
        forca_cfg = 0UL;
    }

    unit->HRGCMCR = compare;

    bpcnar1 = (unit->BPCNAR1 & ~(HRPWM_BPCNAR1_CMAUCA |
                                 HRPWM_BPCNAR1_CMADCA |
                                 HRPWM_BPCNAR1_FORCA |
                                 HRPWM_BPCNAR1_OUTENA)) |
              (cmauca_cfg << HRPWM_BPCNAR1_CMAUCA_POS) |
              (cmadca_cfg << HRPWM_BPCNAR1_CMADCA_POS) |
              (forca_cfg << HRPWM_BPCNAR1_FORCA_POS) |
              (((0U != up_en) ? 1UL : 0UL) << HRPWM_BPCNAR1_OUTENA_POS);

    unit->BPCNAR1 = bpcnar1;
    unit->BPCNBR1 = (unit->BPCNBR1 & ~HRPWM_BPCNBR1_OUTENB) |
                    (((0U != dn_en) ? 1UL : 0UL) << HRPWM_BPCNBR1_OUTENB_POS);
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
    unit->BCONR1 |= (1UL << 0) | (1UL << 3);
    unit->BCONR2 = (1UL << 12) | (1UL << 14);
}

static void bsp_pwm_config_idle(CM_HRPWM_TypeDef *unit)
{
    HRPWM_IDLE_DELAY_Enable(unit);
    HRPWM_IDLE_DELAY_SetTriggerSrc(unit, HRPWM_IDLE_DELAY_TRIG_SW);
    HRPWM_IDLE_DELAY_SetOutputChAStatus(unit, HRPWM_IDLE_OUTPUT_CHA_ON);
    HRPWM_IDLE_DELAY_SetOutputChBStatus(unit, HRPWM_IDLE_OUTPUT_CHB_ON);
    HRPWM_IDLE_SetChAIdleLevel(unit, HRPWM_IDLE_CHA_LVL_LOW);
    HRPWM_IDLE_SetChBIdleLevel(unit, HRPWM_IDLE_CHB_LVL_LOW);
}

static void bsp_pwm_config_output_unit(CM_HRPWM_TypeDef *unit)
{
    stc_hrpwm_init_t base;

    (void)HRPWM_StructInit(&base);
    base.u32CountMode = HRPWM_MD_TRIANGLE;
    base.u32PeriodValue = BSP_PWM_PERIOD_REG;
    base.u32CountReload = HRPWM_CNT_RELOAD_ON;
    (void)HRPWM_Init(unit, &base);

    HRPWM_Enable(unit);
    bsp_pwm_config_channel_waveform(unit);
    bsp_pwm_config_buffer(unit);
    bsp_pwm_config_idle(unit);

    unit->HRDTUAR = BSP_PWM_DEADTIME_REG;
    unit->HRDTDAR = BSP_PWM_DEADTIME_REG;
    unit->HRDTUBR = BSP_PWM_DEADTIME_REG;
    unit->HRDTDBR = BSP_PWM_DEADTIME_REG;
    unit->DCONR = (1UL << HRPWM_DCONR_DTCEN_POS);
    unit->GCONR = (1UL << 2);

    HRPWM_ClearStatus(unit, HRPWM_FLAG_CNT_PEAK | HRPWM_FLAG_CNT_VALLEY);
}

static void bsp_pwm_master_irq_callback(void)
{
    if (SET == HRPWM_GetStatus(BSP_PWM_MASTER_UNIT, HRPWM_FLAG_CNT_VALLEY)) {
        HRPWM_ClearStatus(BSP_PWM_MASTER_UNIT, HRPWM_FLAG_CNT_VALLEY);
        section_interrupt();
    }
}

static void bsp_pwm_irq_init(void)
{
    stc_irq_signin_config_t irq_cfg;

    irq_cfg.enIntSrc = BSP_PWM_MASTER_INT_SRC;
    irq_cfg.enIRQn = BSP_PWM_MASTER_IRQ;
    irq_cfg.pfnCallback = bsp_pwm_master_irq_callback;
    (void)INTC_IrqSignIn(&irq_cfg);

    NVIC_ClearPendingIRQ(BSP_PWM_MASTER_IRQ);
    NVIC_SetPriority(BSP_PWM_MASTER_IRQ, BSP_PWM_MASTER_IRQ_PRIO);
    NVIC_EnableIRQ(BSP_PWM_MASTER_IRQ);
}

static void bsp_pwm_config_master(void)
{
    uint32_t trig;

    HRPWM_Enable(BSP_PWM_MASTER_UNIT);
    BSP_PWM_MASTER_UNIT->CR |= (1UL << 3);
    BSP_PWM_MASTER_UNIT->CNTER = 0UL;
    BSP_PWM_MASTER_UNIT->UPDAR = 0UL;
    BSP_PWM_MASTER_UNIT->HRPERAR = (0xFFFFUL << 6);
    BSP_PWM_MASTER_UNIT->HRPERBR = (0xFFFFUL << 6);

    BSP_PWM_MASTER_UNIT->HRGCMAR = BSP_PWM_PERIOD_REG * 2UL;
    trig = BSP_PWM_MASTER_UNIT->HRGCMAR;
    if (trig > BSP_PWM_ADC_TRIG_ADV) {
        trig -= BSP_PWM_ADC_TRIG_ADV;
    }
    HRPWM_SetSpecialCompareAValue(BSP_PWM_MASTER_UNIT, trig);
    HRPWM_MatchSpecialEventEnable(BSP_PWM_MASTER_UNIT, HRPWM_EVT_UP_MATCH_SPECIAL_A);
    HRPWM_AosEventEnable(BSP_PWM_MASTER_UNIT, HRPWM_AOS_EVT_MATCH_SPECIAL_A);

    HRPWM_IntEnable(BSP_PWM_MASTER_UNIT, HRPWM_INT_CNT_VALLEY);
    HRPWM_ClearStatus(BSP_PWM_MASTER_UNIT, HRPWM_FLAG_CNT_PEAK | HRPWM_FLAG_CNT_VALLEY);
}

static void bsp_pwm_init(void)
{
    LL_PERIPH_WE(LL_PERIPH_FCG | LL_PERIPH_GPIO);

    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_HRPWM_1 |
                               FCG2_PERIPH_HRPWM_2 |
                               FCG2_PERIPH_HRPWM_3,
                           ENABLE);

    GPIO_HrpwmPinCmd(GPIO_HRPWM2_PWMA |
                         GPIO_HRPWM2_PWMB |
                         GPIO_HRPWM3_PWMA |
                         GPIO_HRPWM3_PWMB,
                     ENABLE);

    LL_PERIPH_WP(LL_PERIPH_FCG | LL_PERIPH_GPIO);

    HRPWM_CommonDeInit();
    (void)HRPWM_CALIB_PeriodInit(HRPWM_CALIB_PERIOD_8P7_MS);

    bsp_pwm_config_master();
    bsp_pwm_config_output_unit(BSP_PWM_FAST_UNIT);
    bsp_pwm_config_output_unit(BSP_PWM_SLOW_UNIT);
    bsp_pwm_set_duty(0.0f, 0.0f, 0U, 0U, 0U, 0U);

    CM_HRPWM_COMMON->SSTAIDLR |= (BSP_PWM_OUTPUT_SYNC_UNITS & HRPWM_SW_SYNC_CH_ALL);
    bsp_pwm_irq_init();
    CM_HRPWM_COMMON->SSTAR = (HRPWM_UNIT1 | HRPWM_UNIT2 | HRPWM_UNIT3);
}

REG_INIT(0, bsp_pwm_init)

void bsp_pwm_enable(void)
{
    HRPWM_IDLE_Exit(BSP_PWM_OUTPUT_SYNC_UNITS, HRPWM_SW_SYNC_CH_ALL);
}

void bsp_pwm_disable(void)
{
    bsp_pwm_set_duty(0.0f, 0.0f, 0U, 0U, 0U, 0U);
    HRPWM_IDLE_DELAY_MultiUnitSWTrigger(BSP_PWM_OUTPUT_UNITS);
}

void bsp_pwm_set_duty(float duty_fast,
                      float duty_slow,
                      uint8_t up_en_fast,
                      uint8_t dn_en_fast,
                      uint8_t up_en_slow,
                      uint8_t dn_en_slow)
{
    bsp_pwm_write_unit_buffer(BSP_PWM_FAST_UNIT,
                              duty_fast,
                              (0U != up_en_fast) ? 1U : 0U,
                              (0U != dn_en_fast) ? 1U : 0U);
    bsp_pwm_write_unit_buffer(BSP_PWM_SLOW_UNIT,
                              duty_slow,
                              (0U != up_en_slow) ? 1U : 0U,
                              (0U != dn_en_slow) ? 1U : 0U);
}
