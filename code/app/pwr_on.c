#include "pwr_on.h"
#include "stdint.h"
#include "section.h"
#include "adc_chk.h"
#include "key.h"
#include "gpio.h"
#include "pwr_on.h"
#include "fault.h"
#include "stdint.h"
#include "ctrl_buck.h"
#include "ctrl_llc_chg.h"
#include "ctrl_llc_dsg.h"
#include "fal.h"
#include "data_com.h"

static uint8_t aux_on = 0;
static uint8_t fault_dly_ok = 0;
static uint32_t fault_dly_cnt = 0;

static void fault_dly_ok_task(void)
{
    if (fault_dly_ok == 0)
    {
        if (fault_check() == 1)
        {
            if (fault_dly_cnt < TIME_CNT_10S_IN_1MS)
            {
                fault_dly_cnt++;
            }
            else
            {
                fault_dly_ok = 1;
            }
        }
        else
        {
            fault_dly_cnt = 0;
        }
    }
}

REG_TASK_MS(1, fault_dly_ok_task)

static void pwr_on_task(void)
{
    if (aux_on == 0)
    {
        if ((adc_chk_get_pv_in_is_ok() == 1) ||
            (key_get_status(KEY_STA_LONG_PRESS) == 1) ||
            (data_com_get_ac_is_ok() == 1))
        {
            aux_on = 1;
            gpio_set_en_pwr(1);
        }
    }
    else
    {
        if (((key_get_status(KEY_STA_LONG_PRESS) == 1) &&
             (adc_chk_get_pv_in_is_ok() == 0)) ||
            (fault_dly_ok == 1))
        {
            aux_on = 0;
            gpio_set_en_pwr(0);
        }
    }
}

REG_TASK_MS(1, pwr_on_task)

uint8_t pwr_on_get_aux_is_on(void)
{
    return aux_on;
}
