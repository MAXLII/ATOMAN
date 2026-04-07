#include "inv_hal.h"
#include "inv_ctrl.h"
#include "inv_fsm.h"
#include "inv_cfg.h"
#include "adc_proc.h"
#include "gpio.h"
#include "pwm.h"
#include "section.h"
#include "my_math.h"

static void inv_hal_enter_run(void);
static void inv_hal_exit_run(void);
static void inv_hal_rly_on(void);
static void inv_hal_rly_off(void);
static uint8_t hard_protect_latched;

static void inv_hal_enter_run(void)
{
    PLECS_LOG("inv_hal enter run\n");
    inv_ctrl_prepare_run();
    pwm_enable();
    inv_cfg_set_run_allowed(1U);
    inv_cfg_publish_building();
}

static void inv_hal_exit_run(void)
{
    PLECS_LOG("inv_hal exit run\n");
    pwm_disable();
    inv_cfg_set_run_allowed(0U);
    inv_cfg_publish_building();
}

static void inv_hal_rly_on(void)
{
    gpio_set_ac_out_rly_sta(1U);
    PLECS_LOG("inv_hal relay on\n");
}

static void inv_hal_rly_off(void)
{
    gpio_set_ac_out_rly_sta(0U);
    PLECS_LOG("inv_hal relay off\n");
}

static inv_ctrl_hal_t inv_ctrl_hal = {
    .p_v_cap = &v_cap,
    .p_i_l = &i_l,
    .p_v_bus = &v_bus,
    .p_set_pwm_func = pwm_set_inv,
    .p_pwm_enable = pwm_enable,
    .p_pwm_disable = pwm_disable,
};

static inv_fsm_hal_t inv_fsm_hal = {
    .p_enter_run_func = inv_hal_enter_run,
    .p_exit_run_func = inv_hal_exit_run,
    .p_inv_rly_on_func = inv_hal_rly_on,
    .p_inv_rly_off_func = inv_hal_rly_off,
    .p_latched = &hard_protect_latched,
};

static void inv_hal_init(void)
{
    PLECS_LOG("inv_hal init\n");

    if (STRUCT_ALL_PTR_VALID(inv_ctrl_hal))
    {
        inv_ctrl_set_p_hal(&inv_ctrl_hal);
        PLECS_LOG("inv_hal ctrl hal ready\n");
    }
    else
    {
        inv_ctrl_set_p_hal(NULL);
        PLECS_LOG("inv_hal ctrl hal invalid\n");
    }

    if (STRUCT_ALL_PTR_VALID(inv_fsm_hal))
    {
        inv_fsm_set_p_hal(&inv_fsm_hal);
        PLECS_LOG("inv_hal fsm hal ready\n");
    }
    else
    {
        inv_fsm_set_p_hal(NULL);
        PLECS_LOG("inv_hal fsm hal invalid\n");
    }
}

REG_INIT(1, inv_hal_init)

void inv_hal_hard_protect_trip(void)
{

    if (inv_ctrl_hal.p_pwm_disable != NULL)
    {
        inv_ctrl_hal.p_pwm_disable();
    }

    if (*inv_fsm_hal.p_latched == 0U)
    {
        *inv_fsm_hal.p_latched = 1U;
        PLECS_LOG("inv_hal hard protect latched\n");
    }

    inv_cfg_set_run_allowed(0U);
    inv_cfg_publish_building();
}

void inv_hal_hard_protect_clear(void)
{
    *inv_fsm_hal.p_latched = 0U;
    PLECS_LOG("inv_hal hard protect cleared\n");
}
