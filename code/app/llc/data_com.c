#include "data_com.h"
#include "section.h"
#include "adc_chk.h"
#include "usart.h"
#include "time_share.h"
#include "my_math.h"
#include "string.h"
#include "llc_fsm.h"
#include "gpio.h"
#include "pwr_on.h"

static llc_to_pfc_info_t llc_to_pfc_info;
static pfc_to_llc_info_t pfc_to_llc_info;
static bms_to_llc_info_t bms_to_llc_info;
static time_share_t data_com_time_share;

static uint8_t pfc_is_alive = 0;

DATA_COM_REG_ALIVE(pfc_to_llc_info_act, TIME_CNT_5S_IN_1MS, pfc_is_alive)

static void pfc_to_llc_info_act(section_packform_t *p_pack, __attribute__((unused)) DEC_MY_PRINTF)
{
    uint32_t len_min = 0;
    MIN(len_min, p_pack->len, sizeof(pfc_to_llc_info));
    memcpy((uint8_t *)&pfc_to_llc_info, p_pack->p_data, len_min);
    DATA_COM_ALIVE_RELOAD(pfc_to_llc_info_act);
}

REG_COMM(CMD_SET_PFC_TO_LLC_INFO, CMD_WORD_PFC_TO_LLC_INFO, pfc_to_llc_info_act)

static void llc_to_pfc_info_act(void)
{
    llc_to_pfc_info.v_bat = v_bat;
    llc_to_pfc_info.aux_is_on = pwr_on_get_aux_is_on();

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_LLC_TO_PFC_INFO;
    packform.cmd_word = CMD_WORD_LLC_TO_PFC_INFO;
    packform.src = LLC_ADDR;
    packform.dst = PFC_ADDR;
    packform.len = sizeof(llc_to_pfc_info_t);
    packform.p_data = (uint8_t *)&llc_to_pfc_info;
    comm_send_data(&packform, LINK_PRINTF(USART1_LINK));
}

uint8_t data_com_get_bus_is_ok(void)
{
    return pfc_to_llc_info.bus_is_ok;
}

uint8_t data_com_get_ac_is_ok(void)
{
    return pfc_to_llc_info.ac_is_ok;
}

uint8_t data_com_get_pfc_is_dsg(void)
{
    return pfc_to_llc_info.is_dsg;
}

time_share_func_table_t data_com_func_table[] = {
    TIME_SHARE_REG_FUNC(llc_to_pfc_info_act, 300),
};

static void data_com_init(void)
{
    time_share_init(&data_com_time_share,
                    data_com_func_table,
                    sizeof(data_com_func_table) / sizeof(data_com_func_table[0]));
}

REG_INIT(1, data_com_init)

static void data_com_chk_alive_func(void)
{
    DATA_COM_ALIVE_DN_CNT(pfc_to_llc_info_act);
}

uint8_t data_com_get_pfc_is_alive(void)
{
    return pfc_is_alive;
}

static void data_com_task(void)
{
    data_com_chk_alive_func();
    time_share_func(&data_com_time_share);
}

REG_TASK_MS(1, data_com_task)

// BMS
static void bms_to_llc_info_act(void)
{
    bms_to_llc_info.allow_ac_chg = 0;
    bms_to_llc_info.allow_pv_chg = 0;
    bms_to_llc_info.allow_ac_dsg = 0;
}

static void data_com_bms_task(void)
{
    bms_to_llc_info_act();
}

REG_TASK_MS(1, data_com_bms_task)
