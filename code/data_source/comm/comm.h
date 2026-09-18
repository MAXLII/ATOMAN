// SPDX-License-Identifier: MIT
/**
 * @file    comm.h
 * @brief   comm communication public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define the 0xE8 frame layout, parser states, link descriptors, and routing descriptors
 *          - Provide REG_COMM, REG_COMM_LINK, and REG_COMM_ROUTE macros for section-based registration
 *          - Expose CRC helpers, parser initialization, byte input, and frame send APIs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __COMM_H__
#define __COMM_H__

#include <stdint.h>
#include <stddef.h>

#include "section.h" // 仅依赖：REG_SECTION_FUNC / SECTION_COMM / SECTION_COMM_ROUTE / DEC_MY_PRINTF

/* =============================================================================
 * COMM 协议基础：CRC / 帧格式 / 解析状态
 * =============================================================================
 */

/* CRC-16-CCITT 参数（与原逻辑一致） */
#define CRC16_CCITT_POLY 0x1021u
#define CRC16_CCITT_INIT 0xFFFFu

uint16_t crc16_init(void);
uint16_t crc16_update(uint16_t crc, uint8_t data);
uint16_t crc16_final(uint16_t crc);

uint16_t section_crc16(uint8_t *p_data, uint32_t len);
uint16_t section_crc16_with_crc(uint8_t *p_data, uint32_t len, uint16_t crc_in);

#pragma pack(push, 1)
/**
 * @brief 通信帧格式（解析与发送共用）
 *
 * 说明：
 * - crc 覆盖范围：sop...p_data（不含 eop）
 * - eop 固定 0x0A0D（低字节 0x0D， 高字节 0x0A）
 */
typedef struct
{
    uint8_t sop;      ///< 0xE8
    uint8_t version;  ///< 协议版本
    uint8_t src;      ///< 源地址
    uint8_t d_src;    ///< 动态源地址
    uint8_t dst;      ///< 目的地址
    uint8_t d_dst;    ///< 动态目的地址
    uint8_t cmd_set;  ///< 命令集
    uint8_t cmd_word; ///< 命令字
    uint8_t is_ack;   ///< 是否响应帧
    uint16_t len;     ///< payload 长度
    uint8_t *p_data;  ///< payload 指针
    uint16_t crc;     ///< CRC16（sop..payload）
    uint16_t eop;     ///< 0x0A0D
    uint8_t seq;      ///< COMM v1 frame sequence 0..7; zero for legacy 0xE8 frames
} section_packform_t;
#pragma pack(pop)

/**
 * @brief COMM 解析状态机状态定义
 */
typedef enum
{
    SECTION_PACKFORM_STA_SOP = 0,
    SECTION_PACKFORM_STA_VER,
    SECTION_PACKFORM_STA_SRC,
    SECTION_PACKFORM_STA_DST,
    SECTION_PACKFORM_STA_CMD,
    SECTION_PACKFORM_STA_ACK,
    SECTION_PACKFORM_STA_LEN,
    SECTION_PACKFORM_STA_DATA,
    SECTION_PACKFORM_STA_CRC,
    SECTION_PACKFORM_STA_EOP,
    SECTION_PACKFORM_STA_ROUTE,
} SECTION_PACKFORM_STA_E;

/* =============================================================================
 * COMM 上下文（与 LINK 解耦）
 * =============================================================================
 *
 * 约定：
 * - p_data_buffer / buffer_size 仅用于 payload 存储与越界检查
 * - link_id 用于路由：标记“此帧来自哪条链路”
 */
typedef struct
{
    uint8_t *p_data_buffer; ///< payload 缓冲区首地址
    uint16_t buffer_size;   ///< payload 缓冲区长度

    uint16_t index;          ///< payload 写入索引
    uint8_t status;          ///< SECTION_PACKFORM_STA_E
    uint16_t crc;            ///< 运行中 CRC
    section_packform_t pack; ///< 当前帧缓存
    void (*func)(void *p_pack, DEC_MY_PRINTF);

    uint16_t len;          ///< 剩余 payload 字节数
    const uint8_t src;     ///< 本机地址
    uint8_t d_src;         ///< 动态源地址
    uint32_t last_rx_tick; ///< Last byte time for parser timeout recovery

    uint8_t src_flag : 1;
    uint8_t dst_flag : 1;
    uint8_t cmd_flag : 1;
    uint8_t len_flag : 1;
    uint8_t eop_flag : 1;
    uint8_t is_route : 1;

    uint8_t link_id; ///< 所属链路ID
} comm_ctx_t;

/**
 * @brief 一句宏声明 payload buffer + comm_ctx_t
 */
#define DECLARE_COMM_CTX(name, payload_size, _src, _link_id) \
    static uint8_t name##_payload_buf[(payload_size)] = {0}; \
    static comm_ctx_t name = {                               \
        .p_data_buffer = name##_payload_buf,                 \
        .buffer_size = (uint16_t)sizeof(name##_payload_buf), \
        .index = 0,                                          \
        .status = SECTION_PACKFORM_STA_SOP,                  \
        .crc = 0,                                            \
        .pack = {0},                                         \
        .func = NULL,                                        \
        .len = 0,                                            \
        .src = (uint8_t)(_src),                              \
        .d_src = 0,                                          \
        .last_rx_tick = 0,                                   \
        .src_flag = 0,                                       \
        .dst_flag = 0,                                       \
        .cmd_flag = 0,                                       \
        .len_flag = 0,                                       \
        .eop_flag = 0,                                       \
        .is_route = 0,                                       \
        .link_id = (uint8_t)(_link_id),                      \
    }

/* =============================================================================
 * COMM 命令表注册（保持你原来的 token-paste 版本）
 * =============================================================================
 */

typedef struct section_com_t
{
    uint8_t cmd_set;
    uint8_t cmd_word;
    void (*func)(void *p_pack, DEC_MY_PRINTF);
} section_com_t;

/**
 * @brief 注册 COMM 命令处理项
 *
 * 说明：
 * - 变量名使用 token paste：section_com_<cmd_set>_<cmd_word>
 * - cmd_set 和 cmd_word 的十六进制定义或直接实参禁止添加 u/U 后缀
 * - 你已经自行解决了数值 token paste 的问题，这里保持原样
 */
#define _REG_COMM(_cmd_set, _cmd_word, _func)              \
    section_com_t section_com_##_cmd_set##_##_cmd_word = { \
        .cmd_set = (_cmd_set),                             \
        .cmd_word = (_cmd_word),                           \
        .func = (_func),                                   \
    };                                                     \
    REG_SECTION_FUNC(SECTION_COMM, section_com_##_cmd_set##_##_cmd_word)

#define REG_COMM(_cmd_set, _cmd_word, _func) \
    _REG_COMM(_cmd_set, _cmd_word, _func)

/* =============================================================================
 * COMM 路由表注册
 * =============================================================================
 */

typedef struct comm_route_t
{
    uint8_t src_link_id;
    uint8_t dst_link_id;
    uint8_t dst_addr;
} comm_route_t;

extern section_item_t *p_comm_command_first;
extern section_item_t *p_comm_route_first;

#define _REG_COMM_ROUTE(_src_link_id, _dst_link_id, _dst_addr)          \
    comm_route_t comm_route_##_src_link_id##_dst_link_id##_dst_addr = { \
        .src_link_id = (_src_link_id),                                  \
        .dst_link_id = (_dst_link_id),                                  \
        .dst_addr = (_dst_addr),                                        \
    };                                                                  \
    REG_SECTION_FUNC(SECTION_COMM_ROUTE, comm_route_##_src_link_id##_dst_link_id##_dst_addr)

#define REG_COMM_ROUTE(_src_link_id, _dst_link_id, _dst_addr) \
    _REG_COMM_ROUTE(_src_link_id, _dst_link_id, _dst_addr)

/* =============================================================================
 * COMM 对外接口（实现位于 comm.c）
 * =============================================================================
 *
 * 约定：
 * - comm_run 作为 link handler 被调用：ctx 指向 comm_ctx_t
 */
void comm_run(uint8_t data, DEC_MY_PRINTF, void *ctx);

/**
 * @brief Feed a contiguous transport block into one communication parser context.
 * @param[in] p_data Contiguous bytes received from the transport.
 * @param[in] length Number of bytes available in p_data.
 * @param[in] my_printf Output interface associated with the source link.
 * @param[in,out] p_context Parser context created by DECLARE_COMM_CTX.
 */
void comm_run_buffer(const uint8_t *p_data, uint32_t length, DEC_MY_PRINTF, void *p_context);
void comm_send_data(void *p_pack, DEC_MY_PRINTF);

/* =============================================================================
 * COMM v1（0xE9）协议：帧格式 / MSG / SUM / 解析状态
 * =============================================================================
 *
 * 约定：
 * - v1 与旧 0xE8 协议共存，两套解析状态完全独立
 * - comm_send_data 根据输入对象的第一个字节选择编码方式，业务始终使用同一入口
 * - DATA 长度为 1~256 B，线上 LEN 用 0x00 表示 256 B
 */

#define COMM_V1_SOP 0xE9u       ///< v1 帧起始字节
#define COMM_V1_VERSION 0x01u   ///< v1 本地协议标识，不占线上字节
#define COMM_V1_MAX_DATA 256u   ///< 数据帧 DATA 最大长度
#define COMM_V1_PURE_CMD_LEN 6u ///< 纯命令帧固定长度（SOP + MSG + SUM）
#define COMM_V1_FRAME_MAX 263u  ///< 最大 v1 数据帧长度（7 + 256）
#define COMM_V1_SEQ_MASK 0x07u  ///< MSG SEQ 字段掩码
#define COMM_V1_ADDR_MAX 0x0Fu  ///< 静态地址最大有效值
#define COMM_V1_DADDR_MAX 0x07u ///< 动态地址最大有效值
#define COMM_V1_CMD_SET_MAX 0x0Fu  ///< 命令集最大有效值
#define COMM_V1_CMD_WORD_MAX 0x3Fu ///< 命令字最大有效值

/* CODEC 取值（MSG bit29:27） */
#define COMM_V1_CODEC_RAW 0u  ///< DATA 为原始字节流
#define COMM_V1_CODEC_DICT 1u ///< 静态字典压缩
#define COMM_V1_CODEC_RLE 2u  ///< PackBits / 重复字节压缩
#define COMM_V1_CODEC_LZSS 3u ///< 块内 LZSS-256 压缩
#define COMM_V1_CODEC_ZERO 4u ///< 数零法 nibble 变长编码（小值密集数据）

/* 码本协商命令：CMD_SET=0、CMD_WORD=0 保留为 CODEC_SELECT */
#define COMM_V1_CODEC_SELECT_SET 0x0u
#define COMM_V1_CODEC_SELECT_WORD 0x00u
/* CODEC_SELECT 响应 result 值 */
#define COMM_V1_CODEC_SELECT_OK 0u

/* 接收半帧超时（SECTION_SYS_TICK 单位，与旧协议一致） */
#define COMM_V1_FRAME_TIMEOUT_TICK 1000u

/**
 * @brief COMM v1 解析状态机状态定义
 */
typedef enum
{
    COMM_V1_STA_IDLE = 0, ///< 等待 SOP(0xE9)，丢弃其余字节
    COMM_V1_STA_MSG_L0,   ///< 接收 MSG 第 0 字节（SEQ、DST、D_DST bit0）
    COMM_V1_STA_MSG_L1,   ///< 接收 MSG 第 1 字节（D_DST、SRC、D_SRC）
    COMM_V1_STA_MSG_L2,   ///< 接收 MSG 第 2 字节（D_SRC、CMD_SET、CMD_WORD）
    COMM_V1_STA_MSG_L3,   ///< 接收 MSG 第 3 字节（CMD_WORD、CODEC、CMD、ACK）
    COMM_V1_STA_LEN,      ///< 数据帧接收 LEN 字节
    COMM_V1_STA_DATA,     ///< 数据帧接收 DATA 字节
    COMM_V1_STA_SUM,      ///< 接收 SUM 字节并校验
} COMM_V1_STA_E;

/**
 * @brief COMM v1 MSG 本地联合体
 *
 * 仅用于本地字段访问；线上格式始终由显式移位和字节序列化保证。
 * 位域布局依赖 LSB-first 编译器，跨编译器不得作为存储格式。
 */
typedef union
{
    uint32_t raw;     ///< 32 bit 原始值
    uint8_t bytes[4]; ///< 按小端顺序的线上字节视图

    struct
    {
        uint32_t seq      : 3; ///< 序号 0..7
        uint32_t dst      : 4; ///< 静态目的地址 0..15
        uint32_t d_dst    : 3; ///< 动态目的地址 0..7
        uint32_t src      : 4; ///< 静态源地址 0..15
        uint32_t d_src    : 3; ///< 动态源地址 0..7
        uint32_t cmd_set  : 4; ///< 指令集 0..15
        uint32_t cmd_word : 6; ///< 子命令 0..63
        uint32_t codec    : 3; ///< 数据编码类型
        uint32_t cmd      : 1; ///< 1=纯命令帧；0=数据帧
        uint32_t ack      : 1; ///< 1=直接响应；0=请求或主动上报
    } bits_lsb;
} comm_msg_t;

/**
 * @brief COMM v1 解析上下文（与 v1 LINK 挂接）
 *
 * 约定：
 * - p_data_buffer 保存接收的 DATA 或解压结果，容量 256 B
 * - p_candidate_buffer 保存压缩候选输出，容量 256 B
 * - p_wire_buffer 保存完整接收帧（SOP..SUM），用于路由转发
 * - p_tx_buffer 为发送组包工作区，容量 263 B
 * - 只保存上一有效包的 SEQ，用于上一包去重
 */
typedef struct
{
    uint8_t *p_data_buffer;      ///< DATA / 解压输出缓冲区首地址
    uint16_t buffer_size;        ///< DATA 缓冲区长度，固定 256
    uint8_t *p_candidate_buffer; ///< 压缩候选临时缓冲区首地址
    uint16_t candidate_size;     ///< 压缩候选缓冲区长度，固定 256
    uint8_t *p_wire_buffer;      ///< 完整接收帧捕获缓冲区首地址
    uint16_t wire_size;          ///< 完整接收帧捕获缓冲区长度，固定 263
    uint8_t *p_tx_buffer;        ///< 发送组包工作区首地址
    uint16_t tx_size;            ///< 发送组包工作区长度，固定 263

    uint8_t status;    ///< COMM_V1_STA_E
    uint16_t index;    ///< DATA 写入索引
    uint16_t len;      ///< 剩余 DATA 字节数
    uint8_t sum;       ///< 运行中 SUM（覆盖 SOP..DATA）
    uint32_t msg;      ///< 已累积的 MSG 字节（低字节优先）
    uint8_t msg_cnt;   ///< 已接收 MSG 字节数 0..4
    uint16_t wire_len; ///< 已捕获的线上帧长度
    uint8_t is_route;  ///< 此帧是否为路由转发帧
    uint8_t is_local;  ///< 此帧是否投递给本机

    section_packform_t pack;                                  ///< 当前帧解码缓存
    void (*func)(void *p_pack, DEC_MY_PRINTF);                ///< 命中的命令回调

    uint8_t last_seq;       ///< 上一有效包的 SEQ
    uint8_t last_seq_valid; ///< 上一有效包标志，0 表示无历史
    uint32_t last_rx_tick;  ///< 最后一个接收字节的 tick，用于半帧超时

    const uint8_t src;  ///< 本机静态地址
    uint8_t d_src;      ///< 本机动态地址
    uint8_t link_id;    ///< 所属链路 ID
} comm_v1_ctx_t;

/**
 * @brief 一句宏声明 v1 DATA / 候选 / 线上 / 发送缓冲区 + comm_v1_ctx_t
 */
#define DECLARE_COMM_V1_CTX(name, _src, _link_id)              \
    static uint8_t name##_data_buf[COMM_V1_MAX_DATA] = {0};    \
    static uint8_t name##_candidate_buf[COMM_V1_MAX_DATA] = {0}; \
    static uint8_t name##_wire_buf[COMM_V1_FRAME_MAX] = {0};   \
    static uint8_t name##_tx_buf[COMM_V1_FRAME_MAX] = {0};     \
    static comm_v1_ctx_t name = {                              \
        .p_data_buffer = name##_data_buf,                      \
        .buffer_size = (uint16_t)sizeof(name##_data_buf),      \
        .p_candidate_buffer = name##_candidate_buf,            \
        .candidate_size = (uint16_t)sizeof(name##_candidate_buf), \
        .p_wire_buffer = name##_wire_buf,                      \
        .wire_size = (uint16_t)sizeof(name##_wire_buf),        \
        .p_tx_buffer = name##_tx_buf,                          \
        .tx_size = (uint16_t)sizeof(name##_tx_buf),            \
        .status = COMM_V1_STA_IDLE,                            \
        .index = 0,                                            \
        .len = 0,                                              \
        .sum = 0,                                              \
        .msg = 0,                                              \
        .msg_cnt = 0,                                          \
        .wire_len = 0,                                         \
        .is_route = 0,                                         \
        .is_local = 0,                                         \
        .pack = {0},                                           \
        .func = NULL,                                          \
        .last_seq = 0,                                         \
        .last_seq_valid = 0,                                   \
        .last_rx_tick = 0,                                     \
        .src = (uint8_t)(_src),                                \
        .d_src = 0,                                            \
        .link_id = (uint8_t)(_link_id),                        \
    }

/**
 * @brief COMM v1 SUM 计算：覆盖 SOP 至 DATA 的最后一个字节，不含 SUM 自身。
 * @param p_data 待校验数据首地址，不得为 NULL。
 * @param length 待校验字节数。
 * @return 编码后 SUM：普通和值，0x00 替换为 0x55，0xFF 替换为 0xAA。
 */
uint8_t comm_sum_encode(const uint8_t *p_data, uint16_t length);

/**
 * @brief COMM v1 逐字节解析入口，作为 link handler 被调用：ctx 指向 comm_v1_ctx_t。
 */
void comm_v1_run(uint8_t data, DEC_MY_PRINTF, void *ctx);

/**
 * @brief Reset a COMM v1 parser to idle while keeping the previous valid SEQ.
 * @param[in,out] p_ctx Parser context created by DECLARE_COMM_V1_CTX.
 */
void comm_v1_reset_ctx(comm_v1_ctx_t *p_ctx);

/**
 * @brief Reset a COMM v1 parser for a new transport session and clear the
 *        previous valid SEQ as well.
 * @param[in,out] p_ctx Parser context created by DECLARE_COMM_V1_CTX.
 */
void comm_v1_reset_session(comm_v1_ctx_t *p_ctx);

/**
 * @brief Feed a contiguous transport block into one COMM v1 parser context.
 * @param[in] p_data Contiguous bytes received from the transport.
 * @param[in] length Number of bytes available in p_data.
 * @param[in] my_printf Output interface associated with the source link.
 * @param[in,out] p_context Parser context created by DECLARE_COMM_V1_CTX.
 */
void comm_v1_run_buffer(const uint8_t *p_data, uint32_t length, DEC_MY_PRINTF, void *p_context);

/**
 * @brief COMM v1 发送组包入口，由 comm_send_data 在 SOP=0xE9 时调用。
 * @param[in] p_pack 指向有效的 section_packform_t，sop 必须为 0xE9、version 为 0x01。
 * @param[in] my_printf 目标链路的输出接口。
 */
void comm_v1_send(void *p_pack, DEC_MY_PRINTF);

#endif /* __COMM_H__ */
