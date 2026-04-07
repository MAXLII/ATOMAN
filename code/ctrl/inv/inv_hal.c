#include "inv_hal.h"
#include "inv_ctrl.h"
#include "inv_fsm.h"
#include "inv_cfg.h"
#include "my_math.h"

static void inv_hal_enter_run(void);
static void inv_hal_exit_run(void);
static void inv_hal_rly_on(void);
static void inv_hal_rly_off(void);
static uint8_t hard_protect_latched;
static uint8_t inv_hal_binding_locked = 1U;

static inv_ctrl_hal_t inv_ctrl_hal = {0};

static inv_fsm_hal_t inv_fsm_hal = {
    .p_enter_run_func = inv_hal_enter_run,
    .p_exit_run_func = inv_hal_exit_run,
    .p_inv_rly_on_func = inv_hal_rly_on,
    .p_inv_rly_off_func = inv_hal_rly_off,
    .p_latched = &hard_protect_latched,
};

static void inv_hal_enter_run(void)
{
    PLECS_LOG("inv_hal enter run\n");
    inv_ctrl_prepare_run();
    if (inv_ctrl_hal.p_pwm_enable != NULL)
    {
        inv_ctrl_hal.p_pwm_enable();
    }
    inv_cfg_set_run_allowed(1U);
    inv_cfg_publish_building();
}

static void inv_hal_exit_run(void)
{
    PLECS_LOG("inv_hal exit run\n");
    if (inv_ctrl_hal.p_pwm_disable != NULL)
    {
        inv_ctrl_hal.p_pwm_disable();
    }
    inv_cfg_set_run_allowed(0U);
    inv_cfg_publish_building();
}

static void inv_hal_rly_on(void)
{
    PLECS_LOG("inv_hal relay on hook is not bound\n");
}

static void inv_hal_rly_off(void)
{
    PLECS_LOG("inv_hal relay off hook is not bound\n");
}

inv_ctrl_hal_t *inv_hal_get_ctrl(void)
{
    return &inv_ctrl_hal;
}

inv_fsm_hal_t *inv_hal_get_fsm(void)
{
    return &inv_fsm_hal;
}

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

uint8_t inv_hal_is_ready(void)
{
    return (uint8_t)((STRUCT_ALL_PTR_VALID(inv_ctrl_hal) != 0) &&
                     (STRUCT_ALL_PTR_VALID(inv_fsm_hal) != 0));
}

void inv_hal_lock_binding(void)
{
    inv_hal_binding_locked = 1U;
}

void inv_hal_unlock_binding(void)
{
    inv_hal_binding_locked = 0U;
}

void inv_hal_set_v_cap_ptr(float *p)
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_ctrl_hal.p_v_cap = p;
}

void inv_hal_set_i_l_ptr(float *p)
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_ctrl_hal.p_i_l = p;
}

void inv_hal_set_v_bus_ptr(float *p)
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_ctrl_hal.p_v_bus = p;
}

void inv_hal_set_pwm_setter(void (*p)(float v_pwm, float v_bus))
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_ctrl_hal.p_set_pwm_func = p;
}

void inv_hal_set_pwm_enable(void (*p)(void))
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_ctrl_hal.p_pwm_enable = p;
}

void inv_hal_set_pwm_disable(void (*p)(void))
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_ctrl_hal.p_pwm_disable = p;
}

void inv_hal_set_enter_run_func(void (*p)(void))
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_fsm_hal.p_enter_run_func = p;
}

void inv_hal_set_exit_run_func(void (*p)(void))
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_fsm_hal.p_exit_run_func = p;
}

void inv_hal_set_inv_rly_on_func(void (*p)(void))
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_fsm_hal.p_inv_rly_on_func = p;
}

void inv_hal_set_inv_rly_off_func(void (*p)(void))
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_fsm_hal.p_inv_rly_off_func = p;
}

void inv_hal_set_latched_ptr(uint8_t *p)
{
    if (inv_hal_binding_locked != 0U)
    {
        return;
    }
    inv_fsm_hal.p_latched = p;
}
