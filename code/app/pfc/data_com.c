#include "data_com.h"
#include "section.h"
#include "adc_chk.h"
#include "time_share.h"
#include "adc_chk.h"
#include "usart.h"
#include "string.h"
#include "gpio.h"

static time_share_t data_com_time_share;
static llc_to_pfc_info_t llc_to_pfc_info = {0};
static pfc_to_llc_info_t pfc_to_llc_info = {0};
static uint8_t llc_is_alive;

DATA_COM_REG_ALIVE(llc_to_pfc_info_act, TIME_CNT_5S_IN_1MS, llc_is_alive)

extern uint8_t single_board_en;

static void llc_to_pfc_info_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    (void)my_printf;
    uint32_t len_min = 0;
    MIN(len_min, p_pack->len, sizeof(llc_to_pfc_info_t));
    memcpy((uint8_t *)&llc_to_pfc_info, p_pack->p_data, len_min);

    DATA_COM_ALIVE_RELOAD(llc_to_pfc_info_act);
}

REG_COMM(CMD_SET_LLC_TO_PFC_INFO, CMD_WORD_LLC_TO_PFC_INFO, llc_to_pfc_info_act)

static void pfc_to_llc_info_act(void)
{
    pfc_to_llc_info.ac_is_ok = adc_chk_get_ac_is_ok();
    pfc_to_llc_info.bus_is_ok = adc_chk_get_bus_is_ok();
    section_packform_t packform = {0};

    packform.cmd_set = CMD_SET_PFC_TO_LLC_INFO;
    packform.cmd_word = CMD_WORD_PFC_TO_LLC_INFO;
    packform.src = PFC_ADDR;
    packform.dst = LLC_ADDR;
    packform.len = sizeof(pfc_to_llc_info_t);
    packform.p_data = (uint8_t *)&pfc_to_llc_info;

    comm_send_data(&packform, LINK_PRINTF(USART1_LINK));
}

static void data_com_chk_alive_func(void)
{
    DATA_COM_ALIVE_DN_CNT(llc_to_pfc_info_act);
}

uint8_t data_com_get_llc_is_alive(void)
{
    if (single_board_en == 1)
    {
        return 1;
    }
    return llc_is_alive;
}

float data_com_get_v_bat(void)
{
    return llc_to_pfc_info.v_bat;
}

time_share_func_table_t data_com_func_table[] = {
    TIME_SHARE_REG_FUNC(pfc_to_llc_info_act, 300),
};

static void data_com_init(void)
{
    time_share_init(&data_com_time_share,
                    data_com_func_table,
                    sizeof(data_com_func_table) / sizeof(data_com_func_table[0]));
}

REG_INIT(1, data_com_init)

static void data_com_task(void)
{
    time_share_func(&data_com_time_share);
    data_com_chk_alive_func();
    if (llc_to_pfc_info.aux_is_on == 1) // 低侧辅源锁定
    {
        gpio_set_pwr(1); // 高侧关闭辅源使能
    }
}

REG_TASK_MS(1, data_com_task)
