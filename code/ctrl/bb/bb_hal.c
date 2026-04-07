/* bb_hal.c
 * Buck-boost HAL glue: bind runtime IO and expose FSM run hooks.
 */

#include "bb_hal.h"
#include "bb_cfg.h"
#include "bb_ctrl.h"
#include "bb_fsm.h"
#include "section.h"

static void bb_hal_enter_run(void);
static void bb_hal_exit_run(void);
static uint8_t hard_protect_latched;

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

/**
 * @brief HAL initialization hook that binds control and FSM HAL objects.
 * @param None.
 * @return None.
 */
static void bb_hal_init(void)
{
    bb_ctrl_set_p_hal(&bb_ctrl_hal);
    bb_fsm_set_p_hal(&bb_fsm_hal);
}

REG_INIT(1, bb_hal_init)

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
