#include "pfc_hal.h"
#include "my_math.h"
#include "pfc_ctrl.h"
#include "pfc_fsm.h"
#include "pfc_cfg.h"

static void pfc_hal_enter_run(void);
static void pfc_hal_exit_run(void);
static uint8_t hard_protect_latched;
static uint8_t pfc_hal_binding_locked = 1U;

static pfc_ctrl_hal_t pfc_ctrl_hal = {0};

static pfc_fsm_hal_t pfc_fsm_hal = {
    .p_enter_run_func = pfc_hal_enter_run,
    .p_exit_run_func = pfc_hal_exit_run,
    .p_latched = &hard_protect_latched,
};

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

pfc_ctrl_hal_t *pfc_hal_get_ctrl(void)
{
    return &pfc_ctrl_hal;
}

pfc_fsm_hal_t *pfc_hal_get_fsm(void)
{
    return &pfc_fsm_hal;
}

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

uint8_t pfc_hal_is_ready(void)
{
    uint8_t is_ready = 1U;

    if (pfc_ctrl_hal.p_v_g == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_v_g is null\n");
        is_ready = 0U;
    }
    if (pfc_ctrl_hal.p_v_cap == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_v_cap is null\n");
        is_ready = 0U;
    }
    if (pfc_ctrl_hal.p_i_l == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_i_l is null\n");
        is_ready = 0U;
    }
    if (pfc_ctrl_hal.p_v_bus == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_v_bus is null\n");
        is_ready = 0U;
    }
    if (pfc_ctrl_hal.p_v_rms == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_v_rms is null\n");
        is_ready = 0U;
    }
    if (pfc_ctrl_hal.p_main_rly_is_closed == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_main_rly_is_closed is null\n");
        is_ready = 0U;
    }
    if (pfc_ctrl_hal.p_set_pwm_func == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_set_pwm_func is null\n");
        is_ready = 0U;
    }
    if (pfc_ctrl_hal.p_pwm_enable == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_pwm_enable is null\n");
        is_ready = 0U;
    }
    if (pfc_ctrl_hal.p_pwm_disable == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_pwm_disable is null\n");
        is_ready = 0U;
    }

    if (pfc_fsm_hal.p_vbus_sta == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_vbus_sta is null\n");
        is_ready = 0U;
    }
    if (pfc_fsm_hal.p_main_rly_is_closed == NULL)
    {
        PLECS_LOG("pfc_hal not ready: fsm p_main_rly_is_closed is null\n");
        is_ready = 0U;
    }
    if (pfc_fsm_hal.p_enter_run_func == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_enter_run_func is null\n");
        is_ready = 0U;
    }
    if (pfc_fsm_hal.p_exit_run_func == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_exit_run_func is null\n");
        is_ready = 0U;
    }
    if (pfc_fsm_hal.p_main_rly_on_func == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_main_rly_on_func is null\n");
        is_ready = 0U;
    }
    if (pfc_fsm_hal.p_main_rly_off_func == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_main_rly_off_func is null\n");
        is_ready = 0U;
    }
    if (pfc_fsm_hal.p_latched == NULL)
    {
        PLECS_LOG("pfc_hal not ready: p_latched is null\n");
        is_ready = 0U;
    }

    if (is_ready != 0U)
    {
        PLECS_LOG("pfc_hal ready\n");
    }

    return is_ready;
}

void pfc_hal_lock_binding(void)
{
    pfc_hal_binding_locked = 1U;
}

void pfc_hal_unlock_binding(void)
{
    pfc_hal_binding_locked = 0U;
}

void pfc_hal_set_v_g_ptr(float *p)
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_v_g = p;
}

void pfc_hal_set_v_cap_ptr(float *p)
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_v_cap = p;
}

void pfc_hal_set_i_l_ptr(float *p)
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_i_l = p;
}

void pfc_hal_set_v_bus_ptr(float *p)
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_v_bus = p;
}

void pfc_hal_set_v_rms_ptr(float *p)
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_v_rms = p;
}

void pfc_hal_set_main_rly_is_closed_ptr(uint8_t *p)
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_main_rly_is_closed = p;
    pfc_fsm_hal.p_main_rly_is_closed = p;
}

void pfc_hal_set_pwm_setter(void (*p)(float v_pwm, float v_bus))
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_set_pwm_func = p;
}

void pfc_hal_set_pwm_enable(void (*p)(void))
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_pwm_enable = p;
}

void pfc_hal_set_pwm_disable(void (*p)(void))
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_ctrl_hal.p_pwm_disable = p;
}

void pfc_hal_set_vbus_sta_ptr(pfc_vbus_sta_e *p)
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_fsm_hal.p_vbus_sta = p;
}

void pfc_hal_set_enter_run_func(void (*p)(void))
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_fsm_hal.p_enter_run_func = p;
}

void pfc_hal_set_exit_run_func(void (*p)(void))
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_fsm_hal.p_exit_run_func = p;
}

void pfc_hal_set_main_rly_on_func(void (*p)(void))
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_fsm_hal.p_main_rly_on_func = p;
}

void pfc_hal_set_main_rly_off_func(void (*p)(void))
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_fsm_hal.p_main_rly_off_func = p;
}

void pfc_hal_set_latched_ptr(uint8_t *p)
{
    if (pfc_hal_binding_locked != 0U)
    {
        return;
    }
    pfc_fsm_hal.p_latched = p;
}
