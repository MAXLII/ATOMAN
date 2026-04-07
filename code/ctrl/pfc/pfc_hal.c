#include "pfc_hal.h"
#include "section.h"
#include "my_math.h"
#include "pfc_ctrl.h"
#include "pfc_fsm.h"
#include "adc_proc.h"
#include "pwm.h"
#include "pfc_cfg.h"

static void pfc_hal_enter_run(void);
static void pfc_hal_exit_run(void);
static uint8_t hard_protect_latched;

static void pfc_hal_enter_run(void)
{
    PLECS_LOG("pfc_hal enter run\n");
    pfc_ctrl_prepare_run();
    pfc_cfg_set_run_allowed(1);
    pfc_cfg_publish_building();
}

static void pfc_hal_exit_run(void)
{
    PLECS_LOG("pfc_hal exit run\n");
    pfc_cfg_set_run_allowed(0);
    pfc_cfg_publish_building();
}

pfc_ctrl_hal_t pfc_ctrl_hal = {
    .p_i_l = &i_l,
    .p_v_bus = &v_bus,
    .p_v_cap = &v_cap,
    .p_v_g = &v_g,
    .p_v_rms = &v_g_rms,
    .p_main_rly_is_closed = &main_rly_is_closed,
    .p_pwm_enable = pwm_enable,
    .p_pwm_disable = pwm_disable,
    .p_set_pwm_func = pwm_set_pfc,
};

pfc_fsm_hal_t pfc_fsm_hal = {
    .p_vbus_sta = &vbus_sta,
    .p_main_rly_is_closed = &main_rly_is_closed,
    .p_enter_run_func = pfc_hal_enter_run,
    .p_exit_run_func = pfc_hal_exit_run,
    .p_main_rly_on_func = adc_proc_main_rly_on,
    .p_main_rly_off_func = adc_proc_main_rly_off,
    .p_latched = &hard_protect_latched,
};

static void pfc_hal_init(void)
{
    PLECS_LOG("pfc_hal init\n");

    if (STRUCT_ALL_PTR_VALID(pfc_ctrl_hal))
    {
        pfc_ctrl_set_p_hal(&pfc_ctrl_hal);
        PLECS_LOG("pfc_hal ctrl hal ready\n");
    }
    else
    {
        pfc_ctrl_set_p_hal(NULL);
        PLECS_LOG("pfc_hal ctrl hal invalid\n");
    }

    if (STRUCT_ALL_PTR_VALID(pfc_fsm_hal))
    {
        pfc_fsm_set_p_hal(&pfc_fsm_hal);
        PLECS_LOG("pfc_hal fsm hal ready\n");
    }
    else
    {
        pfc_fsm_set_p_hal(NULL);
        PLECS_LOG("pfc_hal fsm hal invalid\n");
    }
}

REG_INIT(1, pfc_hal_init)

void pfc_hal_hard_protect_trip(void)
{
    if (pfc_ctrl_hal.p_pwm_disable != NULL)
    {
        pfc_ctrl_hal.p_pwm_disable();
    }

    if (*pfc_fsm_hal.p_latched == 0U)
    {
        *pfc_fsm_hal.p_latched = 1U;
        PLECS_LOG("pfc_hal hard protect latched\n");
    }

    pfc_cfg_set_run_allowed(0U);
    pfc_cfg_publish_building();
}

void pfc_hal_hard_protect_clear(void)
{
    *pfc_fsm_hal.p_latched = 0U;
    PLECS_LOG("pfc_hal hard protect cleared\n");
}
