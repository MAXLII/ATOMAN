#ifndef __COMM_H__
#define __COMM_H__

#include <stdint.h>
#include <stddef.h>

#include "section.h"   // 仅依赖：REG_SECTION_FUNC / SECTION_COMM / SECTION_COMM_ROUTE / DEC_MY_PRINTF

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
    uint8_t  sop;      ///< 0xE8
    uint8_t  version;  ///< 协议版本
    uint8_t  src;      ///< 源地址
    uint8_t  d_src;    ///< 动态源地址
    uint8_t  dst;      ///< 目的地址
    uint8_t  d_dst;    ///< 动态目的地址
    uint8_t  cmd_set;  ///< 命令集
    uint8_t  cmd_word; ///< 命令字
    uint8_t  is_ack;   ///< 是否响应帧
    uint16_t len;      ///< payload 长度
    uint8_t *p_data;   ///< payload 指针
    uint16_t crc;      ///< CRC16（sop..payload）
    uint16_t eop;      ///< 0x0A0D
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
    uint8_t  *p_data_buffer; ///< payload 缓冲区首地址
    uint16_t  buffer_size;   ///< payload 缓冲区长度

    uint16_t  index;         ///< payload 写入索引
    uint8_t   status;        ///< SECTION_PACKFORM_STA_E
    uint16_t  crc;           ///< 运行中 CRC
    section_packform_t pack; ///< 当前帧缓存
    void (*func)(section_packform_t *p_pack, DEC_MY_PRINTF);

    uint16_t len;            ///< 剩余 payload 字节数
    const uint8_t src;       ///< 本机地址
    uint8_t d_src;           ///< 动态源地址

    uint8_t src_flag : 1;
    uint8_t dst_flag : 1;
    uint8_t cmd_flag : 1;
    uint8_t len_flag : 1;
    uint8_t eop_flag : 1;
    uint8_t is_route : 1;

    uint8_t link_id;         ///< 所属链路ID
} comm_ctx_t;

/**
 * @brief 一句宏声明 payload buffer + comm_ctx_t
 */
#define DECLARE_COMM_CTX(name, payload_size, _src, _link_id) \
    static uint8_t name##_payload_buf[(payload_size)] = {0}; \
    static comm_ctx_t name = {                               \
        .p_data_buffer = name##_payload_buf,                 \
        .buffer_size   = (uint16_t)sizeof(name##_payload_buf), \
        .index         = 0,                                  \
        .status        = SECTION_PACKFORM_STA_SOP,           \
        .crc           = 0,                                  \
        .pack          = (section_packform_t){0},            \
        .func          = NULL,                               \
        .len           = 0,                                  \
        .src           = (uint8_t)(_src),                    \
        .d_src         = 0,                                  \
        .src_flag      = 0,                                  \
        .dst_flag      = 0,                                  \
        .cmd_flag      = 0,                                  \
        .len_flag      = 0,                                  \
        .eop_flag      = 0,                                  \
        .is_route      = 0,                                  \
        .link_id       = (uint8_t)(_link_id),                \
    }

/* =============================================================================
 * COMM 命令表注册（保持你原来的 token-paste 版本）
 * =============================================================================
 */

typedef struct section_com_t
{
    uint8_t cmd_set;
    uint8_t cmd_word;
    void (*func)(section_packform_t *p_pack, DEC_MY_PRINTF);
    struct section_com_t *p_next;
} section_com_t;

/**
 * @brief 注册 COMM 命令处理项
 *
 * 说明：
 * - 变量名使用 token paste：section_com_<cmd_set>_<cmd_word>
 * - 你已经自行解决了数值 token paste 的问题，这里保持原样
 */
#define _REG_COMM(_cmd_set, _cmd_word, _func)              \
    section_com_t section_com_##_cmd_set##_##_cmd_word = { \
        .cmd_set = (_cmd_set),                             \
        .cmd_word = (_cmd_word),                           \
        .func = (_func),                                   \
        .p_next = NULL,                                    \
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
    struct comm_route_t *p_next;
} comm_route_t;

#define _REG_COMM_ROUTE(_src_link_id, _dst_link_id, _dst_addr)          \
    comm_route_t comm_route_##_src_link_id##_dst_link_id##_dst_addr = { \
        .src_link_id = (_src_link_id),                                  \
        .dst_link_id = (_dst_link_id),                                  \
        .dst_addr    = (_dst_addr),                                     \
        .p_next      = NULL,                                            \
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
void comm_send_data(section_packform_t *p_pack, DEC_MY_PRINTF);

#endif /* __COMM_H__ */
