#include "pwr_cmd.h"
#include "stdint.h"
#include "section.h"
#include "bms_logic.h"
#include "adc_chk.h"
#include "pwr_on.h"
#include "data_com.h"
#include "gpio.h"
#include "key.h"
#include "fault.h"

enum
{
    NO_CHG_CMD,
    PV_CHG_CMD,
    AC_CHG_CMD,
};

enum
{
    NO_DSG_CMD,
    AC_DSG_CMD,
};

static uint8_t chg_cmd = NO_CHG_CMD;
static uint8_t dsg_cmd = NO_DSG_CMD;

static uint8_t buck_singleboard_en = 0;
REG_SHELL_VAR(BUCK_SGBD_EN, buck_singleboard_en, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)

static uint8_t llc_chg_singleboard_en = 0;
REG_SHELL_VAR(LLC_CHG_SGBD_EN, llc_chg_singleboard_en, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)

static uint8_t llc_dsg_singleboard_en = 0;
REG_SHELL_VAR(LLC_DSG_SGBD_EN, llc_dsg_singleboard_en, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)

static void pwr_cmd_task(void)
{
    if ((bms_logic_bat_is_dsg() == 0) ||
        (pwr_on_get_aux_is_on() == 0))
    {
        key_get_status(KEY_STA_LONG_TO_INV_PRESS);
        key_get_status(KEY_STA_SHORT_TO_INV_PRESS);
        chg_cmd = NO_CHG_CMD;
        dsg_cmd = NO_DSG_CMD;
        return;
    }

    if (adc_chk_get_pv_in_is_ok() == 1)
    {
        chg_cmd = PV_CHG_CMD;
    }
    else if (data_com_get_ac_is_ok() == 1)
    {
        chg_cmd = AC_CHG_CMD;
    }
    else
    {
        chg_cmd = NO_CHG_CMD;
    }

    if (dsg_cmd == NO_DSG_CMD)
    {
        if (pwr_on_get_aux_is_on() == 1)
        {
            if ((key_get_status(KEY_STA_SHORT_TO_INV_PRESS) == 1) ||
                (key_get_status(KEY_STA_LONG_TO_INV_PRESS) == 1) ||
                (data_com_get_ac_is_ok() == 1))
            {
                dsg_cmd = AC_DSG_CMD;
            }
        }
    }
    else
    {
        if (pwr_on_get_aux_is_on() == 1)
        {
            if ((key_get_status(KEY_STA_SHORT_TO_INV_PRESS) == 1) ||
                ((data_com_get_ac_is_ok() == 0) &&
                 (bms_logic_bat_is_dsg() == 0)) ||
                (fault_check() == 1))
            {
                dsg_cmd = NO_DSG_CMD;
            }
        }
        else
        {
            dsg_cmd = NO_DSG_CMD;
        }
    }
    gpio_set_to_pfc((dsg_cmd == AC_DSG_CMD) ? 0 : 1);
}

REG_TASK(1, pwr_cmd_task)

uint8_t pwr_cmd_get_pv_chg(void)
{
    return (chg_cmd == PV_CHG_CMD) ? 1 : 0;
}

uint8_t pwr_cmd_get_ac_chg(void)
{
    return (chg_cmd == AC_CHG_CMD) ? 1 : 0;
}

uint8_t pwr_cmd_get_inv_dsg(void)
{
    return (dsg_cmd == AC_DSG_CMD) ? 1 : 0;
}
