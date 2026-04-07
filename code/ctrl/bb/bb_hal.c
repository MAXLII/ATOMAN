/* bb_hal.c
 * Buck-boost HAL glue: bind runtime IO and expose FSM run hooks.
 */

#include "bb_hal.h"
#include "bb_cfg.h"
#include "bb_ctrl.h"
#include "bb_fsm.h"

static void bb_hal_enter_run(void);
static void bb_hal_exit_run(void);
static uint8_t hard_protect_latched;
static uint8_t bb_hal_binding_locked = 1U;

static bb_ctrl_hal_t bb_ctrl_hal = {0}; /* bb_ctrl_hal: control HAL instance populated by platform bindings */

/**
 * @brief Enter-run callback used by the FSM.
 * @param None.
 * @return None. Controller states are prepared and run permission is published.
 */
static void bb_hal_enter_run(void)
{
    PLECS_LOG("bb_hal enter run\n");
    bb_ctrl_prepare_run();

    bb_cfg_set_run_allowed(1U);
    bb_cfg_publish_building();
}

/**
 * @brief Exit-run callback used by the FSM.
 * @param None.
 * @return None. PWM is disabled and run permission is cleared.
 */
static void bb_hal_exit_run(void)
{
    PLECS_LOG("bb_hal exit run\n");

    if (bb_ctrl_hal.p_pwm_disable != NULL)
    {
        bb_ctrl_hal.p_pwm_disable();
    }

    bb_cfg_set_run_allowed(0U);
    bb_cfg_publish_building();
}

static bb_fsm_hal_t bb_fsm_hal = { /* bb_fsm_hal: FSM callbacks for run enter/exit */
    .p_enter_run_func = bb_hal_enter_run,
    .p_exit_run_func = bb_hal_exit_run,
    .p_latched = &hard_protect_latched,
};

bb_ctrl_hal_t *bb_hal_get_ctrl(void)
{
    return &bb_ctrl_hal;
}

bb_fsm_hal_t *bb_hal_get_fsm(void)
{
    return &bb_fsm_hal;
}

void bb_hal_hard_protect_trip(void)
{
    if (bb_ctrl_hal.p_pwm_disable != NULL)
    {
        bb_ctrl_hal.p_pwm_disable();
    }

    if (*bb_fsm_hal.p_latched == 0U)
    {
        *bb_fsm_hal.p_latched = 1U;
    }

    bb_cfg_set_run_allowed(0U);
    bb_cfg_publish_building();
}

void bb_hal_hard_protect_clear(void)
{
    *bb_fsm_hal.p_latched = 0U;
}

uint8_t bb_hal_is_ready(void)
{
    return (uint8_t)((STRUCT_ALL_PTR_VALID(bb_ctrl_hal) != 0) &&
                     (STRUCT_ALL_PTR_VALID(bb_fsm_hal) != 0));
}

void bb_hal_lock_binding(void)
{
    bb_hal_binding_locked = 1U;
}

void bb_hal_unlock_binding(void)
{
    bb_hal_binding_locked = 0U;
}

void bb_hal_set_v_in_ptr(float *p)
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_ctrl_hal.p_v_in = p;
}

void bb_hal_set_i_in_ptr(float *p)
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_ctrl_hal.p_i_in = p;
}

void bb_hal_set_v_out_ptr(float *p)
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_ctrl_hal.p_v_out = p;
}

void bb_hal_set_i_out_ptr(float *p)
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_ctrl_hal.p_i_out = p;
}

void bb_hal_set_i_l_ptr(float *p)
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_ctrl_hal.p_i_l = p;
}

void bb_hal_set_pwm_setter(void (*p)(float buck_duty,
                                     uint8_t buck_up_en,
                                     uint8_t buck_dn_en,
                                     float boost_duty,
                                     uint8_t boost_up_en,
                                     uint8_t boost_dn_en))
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_ctrl_hal.p_set_pwm_func = p;
}

void bb_hal_set_pwm_disable(void (*p)(void))
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_ctrl_hal.p_pwm_disable = p;
}

void bb_hal_set_enter_run_func(void (*p)(void))
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_fsm_hal.p_enter_run_func = p;
}

void bb_hal_set_exit_run_func(void (*p)(void))
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_fsm_hal.p_exit_run_func = p;
}

void bb_hal_set_latched_ptr(uint8_t *p)
{
    if (bb_hal_binding_locked != 0U)
    {
        return;
    }
    bb_fsm_hal.p_latched = p;
}
