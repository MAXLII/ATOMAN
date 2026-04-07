#include "app.h"
#include "section.h"
#include "plecs.h"
#include "pfc_fsm.h"
#include "inv_fsm.h"
#include "pfc_hal.h"
#include "inv_hal.h"
#include "adc_proc.h"
#include "gpio.h"
#include "pwm.h"

typedef enum
{
    APP_MODE_IDLE = 0,
    APP_MODE_AC_CHARGE = 1,
    APP_MODE_AC_DISCHARGE = 2,
} app_mode_e;

#define APP_INV_START_VBUS_MIN_V PFC_VBUS_NOM_ENTER

static void app_inv_rly_on(void)
{
    gpio_set_ac_out_rly_sta(1U);
}

static void app_inv_rly_off(void)
{
    gpio_set_ac_out_rly_sta(0U);
}

static void app_bind_pfc_hal(void)
{
    pfc_hal_set_v_g_ptr(&v_g);
    pfc_hal_set_v_cap_ptr(&v_cap);
    pfc_hal_set_i_l_ptr(&i_l);
    pfc_hal_set_v_bus_ptr(&v_bus);
    pfc_hal_set_v_rms_ptr(&v_g_rms);
    pfc_hal_set_vbus_sta_ptr(&vbus_sta);
    pfc_hal_set_main_rly_is_closed_ptr(&main_rly_is_closed);
    pfc_hal_set_pwm_setter(pwm_set_pfc);
    pfc_hal_set_pwm_enable(pwm_enable);
    pfc_hal_set_pwm_disable(pwm_disable);
    pfc_hal_set_main_rly_on_func(adc_proc_main_rly_on);
    pfc_hal_set_main_rly_off_func(adc_proc_main_rly_off);
}

static void app_bind_inv_hal(void)
{
    inv_hal_set_v_cap_ptr(&v_cap);
    inv_hal_set_i_l_ptr(&i_l);
    inv_hal_set_v_bus_ptr(&v_bus);
    inv_hal_set_pwm_setter(pwm_set_inv);
    inv_hal_set_pwm_enable(pwm_enable);
    inv_hal_set_pwm_disable(pwm_disable);
    inv_hal_set_inv_rly_on_func(app_inv_rly_on);
    inv_hal_set_inv_rly_off_func(app_inv_rly_off);
}

static void app_task(void)
{
    app_mode_e mode = (app_mode_e)((uint8_t)plecs_get_input(PLECS_INPUT_MODE));

    switch (mode)
    {
    case APP_MODE_AC_CHARGE:
        if ((adc_proc_get_ac_is_ok() == 1U) &&
            (pfc_fsm_get_run_sta() == pfc_run_sta_idle))
        {
            app_bind_pfc_hal();
            pfc_fsm_set_cmd(pfc_fsm_cmd_start);
        }

        inv_fsm_set_cmd(inv_fsm_cmd_stop);
        if ((adc_proc_get_ac_is_ok() == 0U) &&
            (pfc_fsm_get_run_sta() != pfc_run_sta_idle))
        {
            pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
        }
        break;

    case APP_MODE_AC_DISCHARGE:
        pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
        if ((v_bus >= APP_INV_START_VBUS_MIN_V) &&
            (inv_fsm_get_run_sta() == inv_run_sta_idle))
        {
            app_bind_inv_hal();
            inv_fsm_set_cmd(inv_fsm_cmd_start);
        }
        else if ((v_bus < APP_INV_START_VBUS_MIN_V) &&
                 (inv_fsm_get_run_sta() != inv_run_sta_idle))
        {
            inv_fsm_set_cmd(inv_fsm_cmd_stop);
        }
        break;

    case APP_MODE_IDLE:
    default:
        pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
        inv_fsm_set_cmd(inv_fsm_cmd_stop);
        break;
    }
}

REG_TASK_MS(1, app_task);
