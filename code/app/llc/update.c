#include "update.h"
#include "section.h"
#include "fal.h"
#include "inter_flash.h"
#include "string.h"
#include "device_info.h"
#include "inter_flash.h"
#include "gd25q32e.h"
#include "apm32f402_403.h"
#include "device_info.h"
#include "data_com.h"

EXT_LINK(USART1_LINK);
EXT_LINK(UART4_LINK);

static update_info_t update_info;
static UPDATE_FSM_E fsm_sta;
static void (*app_run)(void) __attribute__((unused));
static uint8_t update_ready;
static FLASH_ZONE_E write_target_zone;
static update_fw_pack_t update_fw_pack = {0};
static update_fw_pack_ack_t update_fw_pack_ack = {0};
static uint8_t write_flash_flg = 0;
static uint8_t fw_is_ok = 0; // 1:数据OK , 2:数据异常
static uint16_t fw_crc = 0;
static section_packform_t update_fw_receive_packform = {0};
static section_link_tx_func_t update_fw_receive_printf = {0};
static uint8_t to_reset = 0;
static uint32_t version = COMPOSE_VERSION(HARD_VER, DEVICE_VENDOR, RELEASE_VER, DEBUG_VER);
static uint8_t pfc_update_step_is_ok = 0;
static uint32_t pfc_update_timeout = 0;
static uint8_t pfc_update_err_cnt = 0;
static UPDATE_PFC_FSM_E update_pfc_fsm_sta = UPDATE_PFC_FSM_IDLE;
static uint32_t read_pfc_fw_offset = 0;
static uint8_t pfc_is_update = 1;

REG_SHELL_VAR(version, version, SHELL_UINT32, 0xFFFFFFFF, 0x0000000, NULL, SHELL_STA_NULL)

static void update_to_pfc_update_info_act(void)
{
    section_packform_t packform = {0};

    update_fw_info_t update_fw_info = {0};
    if (update_info.module_id == PFC_ADDR)
    {
        update_fw_info.file_size = update_info.file_size;
        update_fw_info.module_id = update_info.module_id;
        update_fw_info.update_type = update_info.update_type;
        update_fw_info.version.raw = update_info.version;
        packform.cmd_set = CMD_SET_UPDATE_INFO;
        packform.cmd_word = CMD_WORD_UPDATE_INFO;
        packform.d_dst = 0;
        packform.d_src = 0;
        packform.dst = PFC_ADDR;
        packform.src = HOST_ADDR;
        packform.is_ack = 0;
        packform.len = sizeof(update_fw_info_t);
        packform.p_data = (uint8_t *)&update_fw_info;
        comm_send_data(&packform, LINK_PRINTF(USART1_LINK));
    }
}

static void update_to_pfc_update_ready_act(void)
{
    section_packform_t packform = {0};

    packform.cmd_set = CMD_SET_UPDATE_READY;
    packform.cmd_word = CMD_WORD_UPDATE_READY;
    packform.d_dst = 0;
    packform.d_src = 0;
    packform.dst = PFC_ADDR;
    packform.src = HOST_ADDR;
    packform.is_ack = 0;
    packform.len = 0;
    packform.p_data = NULL;
    comm_send_data(&packform, LINK_PRINTF(USART1_LINK));
}

static void update_to_pfc_fw_act(void)
{
    section_packform_t packform = {0};
    pfc_is_update = 1;

    packform.cmd_set = CMD_SET_UPDATE_FW;
    packform.cmd_word = CMD_WORD_UPDATE_FW;
    packform.d_dst = 0;
    packform.d_src = 0;
    packform.dst = PFC_ADDR;
    packform.src = HOST_ADDR;
    packform.is_ack = 0;
    packform.len = sizeof(update_fw_pack_t);
    packform.p_data = (uint8_t *)&update_fw_pack;
    comm_send_data(&packform, LINK_PRINTF(USART1_LINK));
}

static void update_to_pfc_end_act(void)
{
    section_packform_t packform = {0};

    update_end_t update_end = {0};
    update_end.fw_crc = fw_crc;
    packform.cmd_set = CMD_SET_UPDATE_END;
    packform.cmd_word = CMD_WROD_UPDATE_END;
    packform.d_dst = 0;
    packform.d_src = 0;
    packform.dst = PFC_ADDR;
    packform.src = HOST_ADDR;
    packform.is_ack = 0;
    packform.len = sizeof(update_end_t);
    packform.p_data = (uint8_t *)&update_end;
    comm_send_data(&packform, LINK_PRINTF(USART1_LINK));
}

void update_init(void)
{
    fal_read(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t), (uint8_t *)&update_info);
}

REG_INIT(2, update_init)

static void update_fw_pack_act_func(uint8_t val)
{
    update_fw_pack_ack.data_is_ok = val;
    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_UPDATE_FW;
    packform.cmd_word = CMD_WORD_UPDATE_FW;
    packform.dst = update_fw_receive_packform.src;
    packform.d_dst = update_fw_receive_packform.d_src;
    packform.src = update_fw_receive_packform.dst;
    packform.d_src = update_fw_receive_packform.d_dst;
    packform.is_ack = 1;
    packform.len = sizeof(update_fw_pack_ack_t);
    packform.p_data = (uint8_t *)&update_fw_pack_ack;
    comm_send_data(&packform, &update_fw_receive_printf);
}

static UPDATE_FSM_E update_pfc_func(void)
{
    switch (update_pfc_fsm_sta)
    {
    case UPDATE_PFC_FSM_IDLE:
        break;
    case UPDATE_PFC_FSM_INFO:
        DN_CNT(pfc_update_timeout);
        if (pfc_update_step_is_ok == 1)
        {
            update_pfc_fsm_sta = UPDATE_PFC_FSM_READY;
            pfc_update_timeout = 0;
            pfc_update_err_cnt = 0;
            pfc_update_step_is_ok = 0;
        }
        else if (pfc_update_timeout == 0)
        {
            pfc_update_err_cnt++;
            if (pfc_update_err_cnt > UPDATE_PFC_ERR_CNT)
            {
                return UPDATE_FSM_FAULT;
            }
            else
            {
                update_to_pfc_update_info_act();
                pfc_update_timeout = TIME_CNT_1S_IN_1MS;
            }
        }
        break;
    case UPDATE_PFC_FSM_READY:
        DN_CNT(pfc_update_timeout);
        if (pfc_update_step_is_ok == 1)
        {
            pfc_update_step_is_ok = 0;
            update_pfc_fsm_sta = UPDATE_PFC_FSM_READ_FW;
            read_pfc_fw_offset = 0;
            fw_crc = crc16_init();
        }
        else if (pfc_update_timeout == 0)
        {
            pfc_update_err_cnt++;
            if (pfc_update_err_cnt > UPDATE_PFC_ERR_CNT)
            {
                return UPDATE_FSM_FAULT;
            }
            else
            {
                pfc_update_timeout = TIME_CNT_1S_IN_1MS;
                update_to_pfc_update_ready_act();
            }
        }
        break;
    case UPDATE_PFC_FSM_READ_FW:
        fal_read(FLASH_ZONE_UPDATE_PFC_FW, read_pfc_fw_offset, UPDATE_PFC_FW_SIZE, &update_fw_pack.packet_data[0]);
        update_fw_pack.data_length = UPDATE_PFC_FW_SIZE;
        update_fw_pack.module_id = PFC_ADDR;
        update_fw_pack.offset = read_pfc_fw_offset;
        update_pfc_fsm_sta = UPDATE_PFC_FSM_WAIT_READ_FW;
        break;
    case UPDATE_PFC_FSM_WAIT_READ_FW:
        if (fal_busy(FLASH_ZONE_UPDATE_PFC_FW) == 0)
        {
            update_fw_pack.packet_crc = section_crc16((uint8_t *)&update_fw_pack,
                                                      sizeof(update_fw_pack_t) -
                                                          sizeof(update_fw_pack.packet_crc));
            fw_crc = section_crc16_with_crc(update_fw_pack.packet_data,
                                            update_fw_pack.data_length,
                                            fw_crc);
            update_pfc_fsm_sta = UPDATE_PFC_FSM_SEND_FW;
            pfc_update_timeout = 0;
        }
        break;
    case UPDATE_PFC_FSM_SEND_FW:
        pfc_update_err_cnt++;
        if (pfc_update_err_cnt > UPDATE_PFC_ERR_CNT)
        {
            return UPDATE_FSM_FAULT;
        }
        else
        {
            update_to_pfc_fw_act();
            pfc_update_timeout = TIME_CNT_1S_IN_1MS;
            update_pfc_fsm_sta = UPDATE_PFC_FSM_WAIT_FW_OK;
            pfc_update_step_is_ok = 0;
        }
        break;
    case UPDATE_PFC_FSM_WAIT_FW_OK:
        DN_CNT(pfc_update_timeout);
        if ((pfc_update_step_is_ok == 2) ||
            (pfc_update_timeout == 0))
        {
            pfc_update_step_is_ok = 0;
            update_pfc_fsm_sta = UPDATE_PFC_FSM_SEND_FW;
        }
        else if (pfc_update_step_is_ok == 1)
        {
            pfc_update_step_is_ok = 0;
            pfc_update_err_cnt = 0;
            read_pfc_fw_offset += UPDATE_PFC_FW_SIZE;
            if (read_pfc_fw_offset >= update_info.file_size)
            {
                update_pfc_fsm_sta = UPDATE_PFC_FSM_SEND_END;
            }
            else
            {
                update_pfc_fsm_sta = UPDATE_PFC_FSM_READ_FW;
            }
        }
        break;
    case UPDATE_PFC_FSM_SEND_END:
        update_to_pfc_end_act();
        update_pfc_fsm_sta = UPDATE_PFC_FSM_IDLE;
        memset((uint8_t *)&update_info, 0, sizeof(update_info_t));
        fal_erase(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t));
        return UPDATE_FSM_WAIT_UPDATE;
        break;
    }
    return UPDATE_FSM_UPDATE_PFC;
}

static uint8_t pfc_update_dbg = 1;
REG_SHELL_VAR(pfc_update_dbg, pfc_update_dbg, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)

void update_task(void)
{
    switch (fsm_sta)
    {
    case UPDATE_FSM_IDLE:
        if (fal_busy(FLASH_ZONE_UPDATE_INFO) == 0)
        {
            if (update_info.update_pfc_ready == 1)
            {
                fsm_sta = UPDATE_FSM_CHK_PFC_ALIVE;
            }
            else
            {
                fsm_sta = UPDATE_FSM_WAIT_UPDATE;
            }
        }
        break;
    case UPDATE_FSM_WAIT_UPDATE:
        if ((update_info.ready_update == 1) &&
            ((update_info.module_id == LLC_ADDR) ||
             (update_info.module_id == PFC_ADDR)))
        {
            fal_erase(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t));
            fal_write(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t), (uint8_t *)&update_info);
            to_reset = 1;
            fsm_sta = UPDATE_FSM_FLASH_BUSY;
        }
        break;
    case UPDATE_FSM_FLASH_BUSY:
        if (fal_busy(FLASH_ZONE_UPDATE_INFO) == 0)
        {
            to_reset = 0;
            fsm_sta = UPDATE_FSM_RESET;
        }
        break;
    case UPDATE_FSM_RESET:
        NVIC_SystemReset();
        break;
    case UPDATE_FSM_FAULT:
        fal_erase(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t));
        memset((uint8_t *)&update_info, 0, sizeof(update_info_t));
        fsm_sta = UPDATE_FSM_WAIT_UPDATE;
        break;
    case UPDATE_FSM_CHK_PFC_ALIVE:
        if (pfc_update_dbg == 1)
        {
            pfc_update_err_cnt = 0;
            pfc_update_timeout = 0;
            pfc_update_step_is_ok = 0;
            fsm_sta = UPDATE_FSM_UPDATE_PFC;
            update_pfc_fsm_sta = UPDATE_PFC_FSM_INFO;
        }
        break;
    case UPDATE_FSM_LLC_DSG:
        break;
    case UPDATE_FSM_WAIT_PFC_ALIVE:
        break;
    case UPDATE_FSM_UPDATE_PFC:
        fsm_sta = update_pfc_func();
        break;
    }
}

REG_TASK_MS(1, update_task)

static void update_info_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->is_ack == 1)
    {
        update_fw_info_ack_t *p_update_fw_info_ack = (update_fw_info_ack_t *)p_pack->p_data;
        if (p_update_fw_info_ack->allow_update == 1)
        {
            pfc_update_err_cnt = 0;
            pfc_update_step_is_ok = 1;
            pfc_update_timeout = 0;
        }
        else
        {
            pfc_update_step_is_ok = 0;
            pfc_update_err_cnt = UPDATE_PFC_ERR_CNT;
        }
        return;
    }

    if (sizeof(update_fw_info_t) != p_pack->len)
    {
        return;
    }
    update_fw_info_t *p_update_fw_info = (update_fw_info_t *)p_pack->p_data;
    update_fw_info_ack_t update_fw_info_ack = {0};
    update_fw_info_ack.allow_update = UPDATE_ACK_ALLOW;
    if (p_update_fw_info->file_size > flash_zone[FLASH_ZONE_APP_FW].zone_size)
    {
        update_fw_info_ack.allow_update = UPDATE_ACK_REJECT;
        update_fw_info_ack.reject_reason.bit.oversize = 1;
    }
    if (p_update_fw_info->update_type == UPDATE_TYPE_NORMAL)
    {
        if (((p_update_fw_info->version.byte.hard_ver != HARD_VER) &&
             (HARD_VER != 255)) ||
            ((p_update_fw_info->version.byte.device_vendor != DEVICE_VENDOR) &&
             (DEVICE_VENDOR != 255)))
        {
            update_fw_info_ack.allow_update = UPDATE_ACK_REJECT;
            update_fw_info_ack.reject_reason.bit.version_err = 1;
        }
        else
        {
            update_fw_info_ack.allow_update = UPDATE_ACK_ALLOW;
        }
    }
    if ((p_update_fw_info->module_id != LLC_ADDR) &&
        (p_update_fw_info->module_id != PFC_ADDR))
    {
        update_fw_info_ack.allow_update = UPDATE_ACK_REJECT;
        update_fw_info_ack.reject_reason.bit.module_err = 1;
    }

    if (update_fw_info_ack.allow_update == UPDATE_ACK_ALLOW)
    {
        update_info.ready_update = 1;
        update_info.file_size = p_update_fw_info->file_size;
        update_info.module_id = p_update_fw_info->module_id;
        update_info.version = p_update_fw_info->version.raw;
        update_info.update_type = p_update_fw_info->update_type;
    }
    else
    {
        update_info.ready_update = 0;
    }

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_UPDATE_INFO;
    packform.cmd_word = CMD_WORD_UPDATE_INFO;
    packform.dst = p_pack->src;
    packform.d_dst = p_pack->d_src;
    packform.src = p_pack->dst;
    packform.d_src = p_pack->d_dst;
    packform.is_ack = 1;
    packform.len = sizeof(update_fw_info_ack);
    packform.p_data = (uint8_t *)&update_fw_info_ack;
    comm_send_data(&packform, my_printf);
}

REG_COMM(CMD_SET_UPDATE_INFO, CMD_WORD_UPDATE_INFO, update_info_act)

static void update_ready_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->is_ack == 1)
    {
        uint8_t *p_update_ready = p_pack->p_data;
        if (*p_update_ready == 1)
        {
            pfc_update_err_cnt = 0;
            pfc_update_step_is_ok = 1;
        }
        else
        {
            pfc_update_step_is_ok = 0;
        }
        return;
    }

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_UPDATE_READY;
    packform.cmd_word = CMD_WORD_UPDATE_READY;
    packform.dst = p_pack->src;
    packform.d_dst = p_pack->d_src;
    packform.src = p_pack->dst;
    packform.d_src = p_pack->d_dst;
    packform.is_ack = 1;
    packform.len = sizeof(update_ready);
    packform.p_data = (uint8_t *)&update_ready;
    comm_send_data(&packform, my_printf);
}

REG_COMM(CMD_SET_UPDATE_READY, CMD_WORD_UPDATE_READY, update_ready_act)

static void update_fw_pack_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->is_ack == 1)
    {
        update_fw_pack_ack_t *p_update_fw_pack_ack = (update_fw_pack_ack_t *)p_pack->p_data;
        if (p_update_fw_pack_ack->data_is_ok == 1)
        {
            pfc_update_step_is_ok = 1;
        }
        else
        {
            pfc_update_step_is_ok = 2;
        }
        return;
    }

    memcpy((uint8_t *)&update_fw_receive_packform, p_pack, sizeof(section_packform_t));
    memcpy((uint8_t *)&update_fw_receive_printf, (uint8_t *)my_printf, sizeof(section_link_tx_func_t));
    if ((sizeof(update_fw_pack_t) == p_pack->len) &&
        (fal_busy(write_target_zone) == 0))
    {
        update_fw_pack_t *p_update_fw_pack = (update_fw_pack_t *)p_pack->p_data;
        if (p_update_fw_pack->packet_crc == section_crc16((uint8_t *)p_update_fw_pack,
                                                          sizeof(update_fw_pack_t) -
                                                              sizeof(update_fw_pack.packet_crc)))
        {
            memcpy((uint8_t *)&update_fw_pack, p_pack->p_data, sizeof(update_fw_pack_t));
            write_flash_flg = 1;
        }
        else
        {
            update_fw_pack_act_func(0);
            write_flash_flg = 0;
        }
    }
    else
    {
        update_fw_pack_act_func(0);
        write_flash_flg = 0;
    }
}

REG_COMM(CMD_SET_UPDATE_FW, CMD_WORD_UPDATE_FW, update_fw_pack_act)

static void update_end_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->is_ack == 1)
    {
        update_end_ack_t *p_update_end_ack = (update_end_ack_t *)p_pack->p_data;
        LINK_PRINTF(UART4_LINK)->my_printf(" success_flg:%d\r\n", p_update_end_ack->success_flg);
        return;
    }
    if (sizeof(update_end_t) != p_pack->len)
    {
        return;
    }
    update_end_t *p_update_end = (update_end_t *)p_pack->p_data;
    update_end_ack_t update_end_ack = {0};
    if (fw_crc == p_update_end->fw_crc)
    {
        update_end_ack.success_flg = 1;
        fw_is_ok = 1;
    }
    else
    {
        update_end_ack.success_flg = 0;
        fw_is_ok = 2;
    }

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_UPDATE_END;
    packform.cmd_word = CMD_WROD_UPDATE_END;
    packform.dst = p_pack->src;
    packform.d_dst = p_pack->d_src;
    packform.src = p_pack->dst;
    packform.d_src = p_pack->d_dst;
    packform.is_ack = 1;
    packform.len = sizeof(update_end_ack);
    packform.p_data = (uint8_t *)&update_end_ack;
    comm_send_data(&packform, my_printf);
}

REG_COMM(CMD_SET_UPDATE_END, CMD_WROD_UPDATE_END, update_end_act)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"

static void update_scope_task(void)
{
    if (pfc_is_update == 1)
    {
        pfc_is_update = 0;
    }
}

REG_TASK_MS(100, update_scope_task)

#pragma GCC diagnostic pop
