#ifndef __SHELL_H__
#define __SHELL_H__

#include <stdint.h>
#include <stddef.h>

#include "section.h" // 只用 DEC_MY_PRINTF、REG_SECTION_FUNC、SECTION_SHELL 等

/* =============================================================================
 * Shell：输入解析上下文（仅供 shell_run 使用）
 * =============================================================================
 *
 * 说明：
 * - shell_ctx_t 是每条链路/每个 shell handler 的私有上下文
 * - link 层只把 ctx 当 void* 透传，不需要认识它
 */
typedef struct
{
    uint8_t shell_buffer[128];
    uint8_t shell_index;
} shell_ctx_t;

/**
 * @brief 在业务文件里一行声明一个 shell ctx
 * @note 该 ctx 通常作为 handler_arr 中 shell_run 的 ctx
 */
#define DECLARE_SHELL_CTX(name) \
    static shell_ctx_t name = {0}

/* =============================================================================
 * Shell：变量/命令注册（放入 SECTION_SHELL 段，运行期 section_init() 扫描插链表）
 * =============================================================================
 */

typedef enum
{
    SHELL_INT8 = 0,
    SHELL_UINT8,
    SHELL_INT16,
    SHELL_UINT16,
    SHELL_INT32,
    SHELL_UINT32,
    SHELL_FP32,
    SHELL_CMD,
} SHELL_TYPE_E;

#define SHELL_STR_SIZE_MAX 40

#define SHELL_STA_NULL (0u)
#define SHELL_STA_AUTO (1u << 2)

/**
 * @brief Shell 注册项（命令/变量）
 *
 * 说明：
 * - section_init() 会把所有 SECTION_SHELL 项插入 p_shell_first 链表
 * - shell_run() 在解析命令时遍历该链表并执行/读写变量
 *
 * 注意：
 * - 这里保留 DEC_MY_PRINTF 字段是为了让某些实现选择缓存/透传输出接口；
 *   但它不是必须使用的字段（可以为空）。
 */
typedef struct section_shell_t
{
    const char *p_name;
    uint32_t p_name_size;

    void *p_var;                 ///< 变量地址（命令则为 NULL）
    uint32_t type;               ///< SHELL_TYPE_E
    void *p_max;                 ///< 上限指针（可选）
    void *p_min;                 ///< 下限指针（可选）
    void (*func)(DEC_MY_PRINTF); ///< 回调（命令执行、变量变更通知等）
    uint32_t status;

    DEC_MY_PRINTF; ///< 可选：运行期透传/缓存输出接口
    struct section_shell_t *p_next;
} section_shell_t;

/**
 * @brief 变量限幅宏（沿用你的写法）
 */
#define SHELL_UP_DN_LMT(var, p_up_lmt, p_dn_lmt)         \
    do                                                   \
    {                                                    \
        if ((var) > *(__typeof__(var) *)(p_up_lmt))      \
        {                                                \
            (var) = *(__typeof__(var) *)(p_up_lmt);      \
        }                                                \
        else if ((var) < *(__typeof__(var) *)(p_dn_lmt)) \
        {                                                \
            (var) = *(__typeof__(var) *)(p_dn_lmt);      \
        }                                                \
    } while (0)

/**
 * @brief 注册 Shell 变量
 *
 * @param _name   变量名（会转成字符串作为命令名）
 * @param _var    变量实体
 * @param _type   SHELL_TYPE_E
 * @param _max    最大值（字面量或同类型值）
 * @param _min    最小值（字面量或同类型值）
 * @param _func   可选回调（变量被写入后触发）
 * @param _status 状态位（如 SHELL_STA_AUTO）
 */
#define REG_SHELL_VAR(_name, _var, _type, _max, _min, _func, _status)                      \
    static __typeof__(_var) _name##_##max = (__typeof__(_var))(_max);                      \
    static __typeof__(_var) _name##_##min = (__typeof__(_var))(_min);                      \
    section_shell_t section_shell_##_name = {                                              \
        .p_name = #_name,                                                                  \
        .p_name_size = (uint32_t)(sizeof(#_name) - 1),                                     \
        .p_var = (void *)&(_var),                                                          \
        .type = (uint32_t)(_type),                                                         \
        .p_max = (void *)&_name##_##max,                                                   \
        .p_min = (void *)&_name##_##min,                                                   \
        .func = (_func),                                                                   \
        .status = (uint32_t)(_status),                                                     \
        .p_next = NULL,                                                                    \
    };                                                                                     \
    static_assert(sizeof(#_name) <= (SHELL_STR_SIZE_MAX + 1), #_name " String too long!"); \
    REG_SECTION_FUNC(SECTION_SHELL, section_shell_##_name)

/**
 * @brief 注册 Shell 命令
 */
#define REG_SHELL_CMD(_name, _func)                                                        \
    section_shell_t section_shell_##_name = {                                              \
        .p_name = #_name,                                                                  \
        .p_name_size = (uint32_t)(sizeof(#_name) - 1),                                     \
        .p_var = NULL,                                                                     \
        .type = (uint32_t)SHELL_CMD,                                                       \
        .func = (_func),                                                                   \
        .status = 0,                                                                       \
        .p_next = NULL,                                                                    \
    };                                                                                     \
    static_assert(sizeof(#_name) <= (SHELL_STR_SIZE_MAX + 1), #_name " String too long!"); \
    REG_SECTION_FUNC(SECTION_SHELL, section_shell_##_name)

/* =============================================================================
 * Shell：handler 接口（供 LINK 分发调用）
 * =============================================================================
 *
 * ctx 约定：ctx 指向 shell_ctx_t（由 DECLARE_SHELL_CTX 创建）
 */
void shell_run(uint8_t data, DEC_MY_PRINTF, void *ctx);

#define CMD_SET_SHELL_DATA_NUM 0x01
#define CMD_WORD_SHELL_DATA_NUM 0x01

#define CMD_SET_SHELL_REPORT_LIST 0x01
#define CMD_WORD_SHELL_REPORT_LIST 0x04

#define CMD_SET_SHELL_READ_DATA 0x01
#define CMD_WORD_SHELL_READ_DATA 0x02

#define CMD_SET_SHELL_WRITE_DATA 0x01
#define CMD_WORD_SHELL_WRITE_DATA 0x03

#define CMD_SET_SHELL_WAVE_ENABLE_PARAM 0x01
#define CMD_WORD_SHELL_WAVE_ENABLE_PARAM 0x05

#define CMD_SET_SHELL_WAVE_START 0x01
#define CMD_WORD_SHELL_WAVE_START 0x0C

#define CMD_SET_SHELL_WAVE_PERIOD 0x01
#define CMD_WORD_SHELL_WAVE_PERIOD 0x06

#define CMD_SET_SHELL_WAVE_PARAM 0x01
#define CMD_WORD_SHELL_WAVE_PARAM 0x07

typedef struct
{
    uint8_t active;
    DEC_MY_PRINTF;
    uint8_t src;
    uint8_t d_src;
    uint8_t dst;
    uint8_t d_dst;
    section_shell_t *p_shell;
} shell_report_ctx_t;

#pragma pack(1)
/* 下位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x04
// IS_ACK:0
// 数据
typedef struct
{
    uint8_t name_len;              // 名称长度
    uint8_t type;                  // 数据类型
    uint32_t data;                 // 数据，固定4byte
    uint32_t data_max;             // 数据最大值
    uint32_t data_min;             // 数据最小值
    uint8_t auto_report;           // 波形打印
    char name[SHELL_STR_SIZE_MAX]; // 名称
} shell_report_list_t;

/* 上位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x02
// IS_ACK:0
// 数据
typedef struct
{
    uint8_t name_len;              // 名称长度
    char name[SHELL_STR_SIZE_MAX]; // 名称
} shell_read_data_t;

/* 下位机返回 */
// CMD_SET:0x01
// CMD_WORD:0x02
// IS_ACK:1
// 数据
typedef struct
{
    uint8_t name_len;              // 名称长度
    uint8_t type;                  // 数据类型
    uint32_t data;                 // 数据
    char name[SHELL_STR_SIZE_MAX]; // 名称
} shell_read_data_ret_t;

/* 上位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x03
// IS_ACK:0
// 数据
typedef struct
{
    uint8_t name_len;              // 名称长度
    uint32_t data;                 // 数据
    uint32_t data_max;             // 数据最大值
    uint32_t data_min;             // 数据最小值
    char name[SHELL_STR_SIZE_MAX]; // 名称
} shell_write_data_t;

/* 下位机返回 */
// CMD_SET:0x01
// CMD_WORD:0x03
// IS_ACK:1
// 数据
typedef struct
{
    uint8_t name_len;              // 名称长度
    uint8_t type;                  // 数据类型
    uint32_t data;                 // 数据
    uint32_t data_max;             // 数据最大值
    uint32_t data_min;             // 数据最小值
    char name[SHELL_STR_SIZE_MAX]; // 名称
} shell_write_data_ret_t;

/* 上位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x05
// IS_ACK:0
// 数据
typedef struct
{
    uint8_t name_len;              // 名称长度
    uint8_t auto_report;           // 自动上报，1：打开自动上报，0：关闭自动上报
    char name[SHELL_STR_SIZE_MAX]; // 名称
} shell_wave_enable_param_t;

/* 下位机返回 */
// CMD_SET:0x01
// CMD_WORD:0x05
// IS_ACK:1
// 数据
typedef struct
{
    uint8_t ok; // 1:设置有效 其他:设置无效
} shell_wave_enable_param_ack_t;

/* 下位机自动上报 */
// CMD_SET:0x01
// CMD_WORD:0x07
// IS_ACK:1
// 数据
typedef struct
{
    uint8_t name_len;              // 名称长度
    uint8_t type;                  // 数据类型
    uint32_t data;                 // 数据
    char name[SHELL_STR_SIZE_MAX]; // 名称
} shell_wave_param_t;
// 当name_len = 0x00,type = 0x00,data = 0x55555555,name空时，表示一组数据的开始
// 当name_len = 0x00,type = 0x00,data = 0xAAAAAAAA,name空时，表示一组数据的结束

/* 上位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x0C
// IS_ACK:0
// 数据
typedef struct
{
    uint8_t start_report; // 1:开始自动上报，0:停止自动上报
} shell_wave_start_t;

/* 下位机返回 */
// CMD_SET:0x01
// CMD_WORD:0x0C
// IS_ACK:1
// 数据NULL

/* 上位机发送 */
// CMD_SET:0x01
// CMD_WORD:0x06
// IS_ACK:0
// 数据
typedef struct
{
    uint32_t reprot_period; // 单位为ms
} shell_wave_period_t;

/* 下位机返回 */
// CMD_SET:0x01
// CMD_WORD:0x06
// IS_ACK:1
// 数据
typedef struct
{
    uint32_t reprot_period; // 单位为ms
} shell_wave_period_ack_t;

#pragma pack()

#endif /* __SHELL_H__ */
