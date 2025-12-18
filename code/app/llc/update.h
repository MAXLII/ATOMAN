#ifndef __UPDATE_H
#define __UPDATE_H

#include "stdint.h"
#include "inter_flash.h"
#include "fw_info.h"
#include "fal.h"

#define UPDATE_PFC_ERR_CNT 5
#define UPDATE_PFC_FW_SIZE 256

#define CMD_SET_UPDATE_INFO 0x01
#define CMD_WORD_UPDATE_INFO 0x08

#define CMD_SET_UPDATE_READY 0x01
#define CMD_WORD_UPDATE_READY 0x09

#define CMD_SET_UPDATE_FW 0x01
#define CMD_WORD_UPDATE_FW 0x0A

#define CMD_SET_UPDATE_END 0x01
#define CMD_WROD_UPDATE_END 0x0B

// RAM 范围定义
#define RAM_START 0x20000000
#define RAM_SIZE (32 * 1024) // 64KB
#define RAM_END (RAM_START + RAM_SIZE - 1)

// 栈指针有效性检查宏
#define IS_VALID_STACK_POINTER(sp) \
    (sp == (RAM_START + RAM_SIZE)) // 4字节对齐检查

#define STACK_DATA (*(uint32_t *)0x08006000)

#define JUMP_APP_PC_ADDR (void (*)(void))(*(uint32_t *)(FMC_START_ADDR + 0x6000 + 4))

#ifdef IS_BOOTLOADER
#undef HARD_VER
#define HARD_VER 255 // 硬件版本

#undef DEVICE_VENDOR
#define DEVICE_VENDOR 255 // 设备商代码

#undef RELEASE_VER
#define RELEASE_VER 255 // 发布版本

#undef DEBUG_VER
#define DEBUG_VER 1 // 调试版本
#endif

typedef enum
{
    UPDATE_FSM_IDLE,
    UPDATE_FSM_WAIT_UPDATE,
    UPDATE_FSM_FLASH_BUSY,
    UPDATE_FSM_RESET,
    UPDATE_FSM_FAULT,
    UPDATE_FSM_CHK_PFC_ALIVE,
    UPDATE_FSM_LLC_DSG,
    UPDATE_FSM_WAIT_PFC_ALIVE,
    UPDATE_FSM_UPDATE_PFC,
} UPDATE_FSM_E;

typedef enum
{
    UPDATE_PFC_FSM_IDLE,
    UPDATE_PFC_FSM_INFO,
    UPDATE_PFC_FSM_READY,
    UPDATE_PFC_FSM_READ_FW,
    UPDATE_PFC_FSM_WAIT_READ_FW,
    UPDATE_PFC_FSM_SEND_FW,
    UPDATE_PFC_FSM_WAIT_FW_OK,
    UPDATE_PFC_FSM_SEND_END,
} UPDATE_PFC_FSM_E;

enum
{
    UPDATE_TYPE_NULL,
    UPDATE_TYPE_NORMAL,
    UPDATE_TYPE_FORCE,
};

enum
{
    UPDATE_ACK_NULL,
    UPDATE_ACK_ALLOW,
    UPDATE_ACK_REJECT,
};

typedef union
{
    uint16_t raw;
    struct
    {
        uint16_t oversize : 1;
        uint16_t version_err : 1;
        uint16_t module_err : 1;
    } bit;
} reject_reason_u;

typedef union
{
    uint32_t raw;
    struct
    {
        uint8_t debug_ver;     // 调试版本
        uint8_t release_ver;   // 发布版本
        uint8_t device_vendor; // 设备商代码
        uint8_t hard_ver;      // 硬件版本
    } byte;
} version_u;

#pragma pack(1)

/* 以下所有的CRC校验与通信格式的CRC校验算法一样 */
/* 上位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x08
// IS_ACK:0
// 数据
typedef struct
{
    uint8_t module_id;   // 模块ID
    version_u version;   // 固件版本
    uint32_t file_size;  // 最终固件大小 包含footer信息
    uint8_t update_type; // 其它:不升级 1:正常升级 2:强制升级
} update_fw_info_t;

/* 下位机返回 */
// CMD_SET:0x01
// CMD_WORD:0x08
// IS_ACK:1
// 数据
typedef struct
{
    uint8_t allow_update;          // 1：允许升级，2：不允许升级，其他：无效
    reject_reason_u reject_reason; // 拒绝原因代码
} update_fw_info_ack_t;

typedef struct
{
    uint8_t module_id;        // 模块ID
    uint32_t version;         // 固件版本
    uint32_t file_size;       // 最终固件大小 包含footer信息
    uint8_t ready_update;     // 准备升级
    uint8_t update_pfc_ready; // 通过LLC的APP升级PFC
    uint8_t update_type;      // 其它:不升级 1:正常升级 2:强制升级
} update_info_t;

/* 上位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x0A
// IS_ACK:0
// 数据
typedef struct
{
    uint32_t offset;                         // 在固件中的偏移量
    uint8_t module_id;                       // 模块ID
    uint16_t data_length;                    // 数据长度 实际有效的数据长度
    uint8_t packet_data[UPDATE_PFC_FW_SIZE]; // 小包数据 数据不够时用0xFF来凑齐
    uint16_t packet_crc;                     // 本数据包的CRC16
} update_fw_pack_t;

/* 下位机返回 */
// CMD_SET:0x01
// CMD_WORD:0x0A
// IS_ACK:1
// 数据
typedef struct
{
    uint8_t data_is_ok; // 1:数据正确，其他：数据错误
} update_fw_pack_ack_t;

/* 上位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x0B
// IS_ACK:0
// 数据
typedef struct
{
    uint16_t fw_crc; // 整包固件包含footer的CRC校验
} update_end_t;

/* 下位机返回 */
// CMD_SET:0x01
// CMD_WORD:0x0B
// IS_ACK:1
// 数据
typedef struct
{
    uint8_t success_flg; // 1:升级成功 其他:升级失败
} update_end_ack_t;

#pragma pack()

#endif
