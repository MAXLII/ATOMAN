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

#endif /* __SHELL_H__ */
