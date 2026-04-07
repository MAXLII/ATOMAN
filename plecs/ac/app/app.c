#include "app.h"
#include "section.h"
#include "plecs.h"
#include "pfc_fsm.h"
#include "inv_fsm.h"
#include "adc_proc.h"

typedef enum
{
    APP_MODE_IDLE = 0,
    APP_MODE_AC_CHARGE = 1,
    APP_MODE_AC_DISCHARGE = 2,
} app_mode_e;

#define APP_INV_START_VBUS_MIN_V PFC_VBUS_NOM_ENTER

static void app_task(void)
{
    app_mode_e mode = (app_mode_e)((uint8_t)plecs_get_input(PLECS_INPUT_MODE));

    switch (mode)
    {
    case APP_MODE_AC_CHARGE:
        if ((adc_proc_get_ac_is_ok() == 1U) &&
            (pfc_fsm_get_run_sta() == pfc_run_sta_idle))
        {
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
