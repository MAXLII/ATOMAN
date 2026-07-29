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

static update_info_t update_info;
static UPDATE_FSM_E fsm_sta;
static void (*app_run)(void) __attribute__((unused));
static uint8_t update_ready;
static FLASH_ZONE_E write_target_zone;
static update_fw_pack_t update_fw_pack = {0};
static update_fw_pack_ack_t update_fw_pack_ack = {0};
static uint8_t write_flash_flg = 0;
static uint8_t fw_is_ok = 0; // 1:数据OK , 2:数据异常
static uint8_t host_addr = 0;
static uint16_t fw_crc = 0;
static section_packform_t update_fw_receive_packform = {0};
static section_link_tx_func_t update_fw_receive_printf = {0};
static uint8_t read_flash_flg = 0;
static uint8_t cal_fw_crc_flg = 0;
static uint8_t overwrite_buffer[FMC_PAGE_SIZE] = {0};
static uint8_t to_overwrite = 0;
static uint8_t to_readflash = 0;
static uint8_t to_reset = 0;
static uint8_t to_stack = 0;
static uint32_t overwrite_offset = 0;
static const uint32_t stack_data = 0x20008000;
static uint32_t version = COMPOSE_VERSION(HARD_VER, DEVICE_VENDOR, RELEASE_VER, DEBUG_VER);

REG_SHELL_VAR(version, version, SHELL_UINT32, 0xFFFFFFFF, 0x0000000, NULL, SHELL_STA_NULL)

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
    packform.cmd_word = CMD_WROD_UPDATE_FW;
    packform.dst = update_fw_receive_packform.src;
    packform.d_dst = update_fw_receive_packform.d_src;
    packform.src = update_fw_receive_packform.dst;
    packform.d_src = update_fw_receive_packform.d_dst;
    packform.is_ack = 1;
    packform.len = sizeof(update_fw_pack_ack_t);
    packform.p_data = (uint8_t *)&update_fw_pack_ack;
    comm_send_data(&packform, &update_fw_receive_printf);
}

void update_task(void)
{
    switch (fsm_sta)
    {
    case UPDATE_FSM_IDLE:
        if (fal_busy(FLASH_ZONE_UPDATE_INFO) == 0)
        {
            if ((update_info.ready_update == 1) &&
                (update_info.module_id == LLC_ADDR))
            {
                write_target_zone = FLASH_ZONE_UPDATE_LLC_FW; // 往LLC固件区域写代码
                host_addr = LLC_ADDR;
                fal_erase(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t));
                fsm_sta = UPDATE_FSM_ERASE_EXTER_FLASH;
            }
            else if ((update_info.ready_update == 1) &&
                     (update_info.module_id == PFC_ADDR))
            {
                write_target_zone = FLASH_ZONE_UPDATE_PFC_FW; // 往PFC固件区域写代码
                host_addr = PFC_ADDR;
                fal_erase(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t));
                fsm_sta = UPDATE_FSM_ERASE_EXTER_FLASH;
            }
            else
            {
                if (IS_VALID_STACK_POINTER(STACK_DATA) == 1)
                {
                    app_run = JUMP_APP_PC_ADDR;
                    SysTick->CTRL &= ~(SysTick_CTRL_CLKSOURCE_Msk |
                                       SysTick_CTRL_TICKINT_Msk |
                                       SysTick_CTRL_ENABLE_Msk);
                    app_run();
                }
                else
                {
                    fsm_sta = UPDATE_FSM_WAIT_UPDATE;
                }
            }
        }
        break;
    case UPDATE_FSM_WAIT_UPDATE:
        if ((update_info.ready_update == 1) &&
            (update_info.module_id == LLC_ADDR))
        {
            fal_erase(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t));
            write_target_zone = FLASH_ZONE_UPDATE_LLC_FW;
            fsm_sta = UPDATE_FSM_ERASE_EXTER_FLASH;
        }
        else if ((update_info.ready_update == 1) &&
                 (update_info.module_id == PFC_ADDR))
        {
            fal_erase(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t));
            write_target_zone = FLASH_ZONE_UPDATE_PFC_FW;
            fsm_sta = UPDATE_FSM_ERASE_EXTER_FLASH;
        }
        break;
    case UPDATE_FSM_ERASE_EXTER_FLASH:
        fal_erase(write_target_zone, 0, flash_zone[write_target_zone].zone_size); // 整个区域全擦除
        fsm_sta = UPDATE_FSM_READY_UPDATE;
        break;
    case UPDATE_FSM_READY_UPDATE:
        if (fal_busy(write_target_zone) == 0)
        {
            update_ready = 1;
            fw_crc = crc16_init();
            fsm_sta = UPDATE_FSM_RECEIVE_FW;
        }
        else
        {
            update_ready = 0;
        }
        break;
    case UPDATE_FSM_RECEIVE_FW:
        if (write_flash_flg == 1)
        {
            write_flash_flg = 0;
            fal_write(write_target_zone,
                      update_fw_pack.offset,
                      update_fw_pack.data_length,
                      update_fw_pack.packet_data);
            read_flash_flg = 1;
        }
        else if ((read_flash_flg == 1) &&
                 (fal_busy(write_target_zone) == 0))
        {
            read_flash_flg = 0;
            fal_read(write_target_zone,
                     update_fw_pack.offset,
                     update_fw_pack.data_length,
                     update_fw_pack.packet_data);
            cal_fw_crc_flg = 1;
        }
        else if ((cal_fw_crc_flg == 1) &&
                 (fal_busy(write_target_zone) == 0))
        {
            cal_fw_crc_flg = 0;
            fw_crc = section_crc16_with_crc(update_fw_pack.packet_data,
                                            update_fw_pack.data_length,
                                            fw_crc);
            update_fw_pack_act_func(1);
        }
        if (fw_is_ok == 1)
        {
            if (update_info.module_id == LLC_ADDR)
            {
                fsm_sta = UPDATE_FSM_ERASE_INTER_FLASH;
                fal_erase(FLASH_ZONE_APP_FW, 0, flash_zone[FLASH_ZONE_APP_FW].zone_size);
            }
            else if (update_info.module_id == PFC_ADDR)
            {
                update_info.update_pfc_ready = 1;
                update_info.ready_update = 0;
                fal_erase(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t));
                fal_write(FLASH_ZONE_UPDATE_INFO, 0, sizeof(update_info_t), (uint8_t *)&update_info);
                to_reset = 1;
                fsm_sta = UPDATE_FSM_FLASH_BUSY;
            }
        }
        else if (fw_is_ok == 2)
        {
            fsm_sta = UPDATE_FSM_FAULT;
        }
        break;
    case UPDATE_FSM_ERASE_INTER_FLASH:
        if (fal_busy(FLASH_ZONE_APP_FW))
        {
            fsm_sta = UPDATE_FSM_READ_FLASH;
            overwrite_offset = 0;
        }
        break;
    case UPDATE_FSM_READ_FLASH:
        fal_read(write_target_zone, overwrite_offset, sizeof(overwrite_buffer), overwrite_buffer);
        to_overwrite = 1;
        fsm_sta = UPDATE_FSM_FLASH_BUSY;
        break;
    case UPDATE_FSM_OVERWRITE:
        if (overwrite_offset == 0)
        {
            overwrite_buffer[0] = 0xFF;
            overwrite_buffer[1] = 0xFF;
            overwrite_buffer[2] = 0xFF;
            overwrite_buffer[3] = 0xFF;
        }
        fal_write(FLASH_ZONE_APP_FW, overwrite_offset, sizeof(overwrite_buffer), overwrite_buffer);
        overwrite_offset += sizeof(overwrite_buffer);
        if (overwrite_offset >= update_info.file_size)
        {
            to_stack = 1;
            fsm_sta = UPDATE_FSM_FLASH_BUSY;
        }
        else
        {
            to_readflash = 1;
            fsm_sta = UPDATE_FSM_FLASH_BUSY;
        }
        break;
    case UPDATE_FSM_FLASH_BUSY:
        if ((to_overwrite == 1) &&
            (fal_busy(write_target_zone) == 0))
        {
            to_overwrite = 0;
            fsm_sta = UPDATE_FSM_OVERWRITE;
        }
        else if ((to_readflash == 1) &&
                 fal_busy(FLASH_ZONE_APP_FW) == 0)
        {
            to_readflash = 0;
            fsm_sta = UPDATE_FSM_READ_FLASH;
        }
        else if ((to_reset == 1) &&
                 (fal_busy(FLASH_ZONE_APP_FW) == 0) &&
                 (fal_busy(FLASH_ZONE_UPDATE_INFO) == 0))
        {
            to_reset = 0;
            fsm_sta = UPDATE_FSM_RESET;
        }
        else if ((to_stack == 1) &&
                 (fal_busy(FLASH_ZONE_APP_FW) == 0))
        {
            to_stack = 0;
            fsm_sta = UPDATE_FSM_STACK;
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
    case UPDATE_FSM_STACK:
        fal_write(FLASH_ZONE_APP_FW, 0, 4, (uint8_t *)&stack_data);
        to_reset = 1;
        fsm_sta = UPDATE_FSM_FLASH_BUSY;
        break;
    }
}

REG_TASK_MS(1, update_task)

static void update_info_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
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

REG_COMM(CMD_SET_UPDATE_FW, CMD_WROD_UPDATE_FW, update_fw_pack_act)

static void update_end_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
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
