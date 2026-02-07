#ifndef __SECTION_H__
#define __SECTION_H__

#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include "platform.h"

/* =============================================================================
 * section.h —— “段(section)”自动注册 + 运行期链表管理（纯框架层）
 *
 * 包含：
 * - SECTION 基础：注册目录项 reg_section_t + 段属性 + REG_SECTION_FUNC
 * - INIT：初始化函数注册与按优先级执行
 * - TASK：周期任务注册与 run_task 调度
 * - INTERRUPT：中断回调注册与 section_interrupt 执行
 * - PERF：性能计数器基址注册 + 记录项注册（不耦合 shell）
 * - FSM：表驱动状态机（REG_FSM 生成周期任务驱动）
 * - LINK：DMA ring 缓冲取新字节并分发 handler_arr（保留 DEC_MY_PRINTF 输出抽象）
 *
 * 不包含（完全解耦）：
 * - Shell（命令/变量/上下文/注册宏）
 * - Comm（packform/ctx/CRC/route/注册宏）
 * =============================================================================
 */

/* =============================================================================
 * LINK 输出抽象（保留 DEC_MY_PRINTF）
 * =============================================================================
 *
 * 说明：
 * - 这是链路层“输出能力”的抽象，不绑定 shell/comm。
 * - my_printf：格式化输出（通常最终走 vprintf 或 UART printf）
 * - tx_by_dma：可选的二进制/字符串块发送（例如 DMA 发送）
 */
typedef struct
{
    void (*my_printf)(const char *__format, ...);
    void (*tx_by_dma)(char *ptr, int len);
} section_link_tx_func_t;

/* 统一形参写法（保持你现有 handler 签名） */
#define DEC_MY_PRINTF section_link_tx_func_t *my_printf

/* =============================================================================
 * SECTION 基础：注册类型 + 段目录项
 * =============================================================================
 */

typedef enum
{
    SECTION_INIT = 0,   ///< 模块初始化函数
    SECTION_TASK,       ///< 周期任务
    SECTION_INTERRUPT,  ///< 中断回调
    SECTION_SHELL,      ///< Shell 命令/变量
    SECTION_LINK,       ///< 链路（字节分发入口）
    SECTION_PERF,       ///< 性能计数/记录
    SECTION_COMM,       ///< 协议处理单元（cmd_set/cmd_word -> callback）
    SECTION_COMM_ROUTE, ///< 路由表
} SECTION_E;

/* 链接段中的“目录项” */
typedef struct
{
    uint32_t section_type; ///< SECTION_E
    void *p_str;           ///< 指向具体注册结构体
} reg_section_t;

/* =============================================================================
 * 段属性：把目录项放进同名段，避免被链接器裁剪
 * =============================================================================
 */
#ifdef IS_PLECS
#define AUTO_REG_SECTION __attribute__((__section__("section")))
#define FUNC_RAM
#else
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#define FUNC_RAM __attribute__((section(".func_ram")))
#endif

#define REG_SECTION_FUNC(_section_type, _p_str)                   \
    const reg_section_t reg_section_##_p_str AUTO_REG_SECTION = { \
        .section_type = (uint32_t)(_section_type),                \
        .p_str = (void *)&(_p_str),                               \
    };

/* likely/unlikely */
#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

/* =============================================================================
 * INIT：初始化函数注册（按优先级排序）
 * =============================================================================
 */

typedef struct reg_init
{
    int8_t priority; ///< 越小越先执行
    void (*p_func)(void);
    struct reg_init *p_next;
} reg_init_t;

#define REG_INIT(prio, func)       \
    reg_init_t reg_init_##func = { \
        .priority = (prio),        \
        .p_func = (func),          \
        .p_next = NULL,            \
    };                             \
    REG_SECTION_FUNC(SECTION_INIT, reg_init_##func)

/* 扫描段并插入链表、执行 INIT */
void section_init(void);

/* =============================================================================
 * PERF：计数器基址注册 + 记录项注册（不耦合 shell）
 * =============================================================================
 */

typedef enum
{
    SECTION_PERF_RECORD = 0,
    SECTION_PERF_BASE,
} SECTION_PERF_E;

typedef struct
{
    uint32_t *p_cnt; ///< 计数器一级指针
} section_perf_base_t;

typedef struct
{
    uint32_t perf_type; ///< SECTION_PERF_E
    void *p_perf;       ///< 指向 section_perf_base_t 或 section_perf_record_t
} section_perf_t;

typedef struct
{
    const char *p_name;
    uint16_t start;
    uint16_t end;
    uint16_t time;
    uint16_t reserved;
    uint32_t max_time;
    uint32_t **p_cnt; ///< 二级指针：指向“全局计数器指针 p_perf_cnt”
    void *p_next;
} section_perf_record_t;

/* 注册计数器基址（section.c 会把全局 p_perf_cnt 指向它） */
#define REG_PERF_BASE_CNT(timer_cnt)                \
    section_perf_base_t section_perf_base_timer = { \
        .p_cnt = (uint32_t *)&(timer_cnt),          \
    };                                              \
    section_perf_t section_timer_cnt_perf = {       \
        .perf_type = SECTION_PERF_BASE,             \
        .p_perf = (void *)&section_perf_base_timer, \
    };                                              \
    REG_SECTION_FUNC(SECTION_PERF, section_timer_cnt_perf)

/* PERF 记录宏（不带 shell 变量导出） */
#define PERF_RECORD_ENABLE 1

#if (PERF_RECORD_ENABLE == 1)

#define PERF_START(name)                                                           \
    do                                                                             \
    {                                                                              \
        if (*section_perf_record_##name.p_cnt != NULL)                             \
        {                                                                          \
            section_perf_record_##name.start = **section_perf_record_##name.p_cnt; \
        }                                                                          \
    } while (0)

#define PERF_END(name)                                                                                                       \
    do                                                                                                                       \
    {                                                                                                                        \
        if (*section_perf_record_##name.p_cnt != NULL)                                                                       \
        {                                                                                                                    \
            section_perf_record_##name.end = **section_perf_record_##name.p_cnt;                                             \
            section_perf_record_##name.time = (uint16_t)(section_perf_record_##name.end - section_perf_record_##name.start); \
            if (section_perf_record_##name.time > section_perf_record_##name.max_time)                                       \
                section_perf_record_##name.max_time = section_perf_record_##name.time;                                       \
        }                                                                                                                    \
    } while (0)

#define P_RECORD_PERF(name) ((section_perf_record_t *)&section_perf_record_##name)

#define REG_PERF_RECORD(name)                            \
    section_perf_record_t section_perf_record_##name = { \
        .p_name = #name,                                 \
        .start = 0,                                      \
        .end = 0,                                        \
        .time = 0,                                       \
        .reserved = 0,                                   \
        .max_time = 0,                                   \
        .p_cnt = NULL,                                   \
        .p_next = NULL,                                  \
    };                                                   \
    section_perf_t section_perf_record_##name##_perf = { \
        .perf_type = SECTION_PERF_RECORD,                \
        .p_perf = (void *)&section_perf_record_##name,   \
    };                                                   \
    REG_SECTION_FUNC(SECTION_PERF, section_perf_record_##name##_perf)

#else
#define PERF_START(name)
#define PERF_END(name)
#define P_RECORD_PERF(name) NULL
#define REG_PERF_RECORD(name)
#endif

/* =============================================================================
 * TASK：定时任务注册与调度
 * =============================================================================
 */

typedef struct reg_task_t
{
    uint32_t t_period;                    ///< 100us 单位
    uint32_t time_last;                   ///< 上次执行 tick
    void (*p_func)(void);                 ///< 任务函数
    section_perf_record_t *p_perf_record; ///< 可选：性能记录
    struct reg_task_t *p_next;
} reg_task_t;

/* 是否给任务自动挂接 PERF_RECORD（0=关闭，1=开启） */
#define TASK_RECORD_PERF_ENABLE 0

#if (TASK_RECORD_PERF_ENABLE == 1)
#define REG_TASK_PERF_RECORD(name) REG_PERF_RECORD(name)
#define TASK_RECORD_PERF(name) P_RECORD_PERF(name)
#else
#define REG_TASK_PERF_RECORD(name)
#define TASK_RECORD_PERF(name) NULL
#endif

/**
 * @brief 注册定时任务（period 单位：100us）
 * @note 会生成 reg_task_##func，并注册到 SECTION_TASK 段
 */
#define REG_TASK(period, func)                   \
    REG_TASK_PERF_RECORD(func)                   \
    reg_task_t reg_task_##func = {               \
        .t_period = (period),                    \
        .p_func = (func),                        \
        .time_last = 0,                          \
        .p_perf_record = TASK_RECORD_PERF(func), \
        .p_next = NULL,                          \
    };                                           \
    REG_SECTION_FUNC(SECTION_TASK, reg_task_##func)

/**
 * @brief 注册定时任务（period 单位：ms）
 */
#define REG_TASK_MS(period, func) REG_TASK(((period) * 10u), func)

void run_task(void);

/* =============================================================================
 * INTERRUPT：中断回调注册
 * =============================================================================
 */

#define PRIORITY_NUM_MAX 16

typedef struct reg_interrupt
{
    uint8_t priority; ///< 0 最优先
    void (*p_func)(void);
    struct reg_interrupt *p_next;
} reg_interrupt_t;

#define REG_INTERRUPT(priority_num, func)    \
    reg_interrupt_t reg_interrupt_##func = { \
        .priority = (priority_num),          \
        .p_func = (func),                    \
        .p_next = NULL,                      \
    };                                       \
    REG_SECTION_FUNC(SECTION_INTERRUPT, reg_interrupt_##func)

void section_interrupt(void);

/* =============================================================================
 * FSM：表驱动状态机（REG_FSM 生成周期任务驱动）
 * =============================================================================
 */

typedef struct
{
    const char *p_name;
    uint32_t fsm_sta;
    void (*func_in)(void);
    void (*func_exe)(void);
    uint32_t (*func_chk)(uint32_t);
    void (*func_out)(void);
} reg_fsm_func_t;

typedef struct
{
    uint32_t fsm_sta;
    reg_fsm_func_t *p_fsm_func_table;
    uint32_t fsm_table_size;
    uint8_t fsm_sta_is_change;
    uint32_t *p_fsm_ev;
} reg_fsm_t;

#define FSM_ENTRY(sta, in, exe, chk, out) \
    {                                     \
        .p_name = #sta,                   \
        .fsm_sta = (sta),                 \
        .func_in = (in),                  \
        .func_exe = (exe),                \
        .func_chk = (chk),                \
        .func_out = (out),                \
    }

#define REG_FSM(name, init_sta, fsm_ev, ...)                                            \
    static reg_fsm_func_t reg_fsm_func_##name##_table[] = {__VA_ARGS__};                \
    static reg_fsm_t reg_fsm_##name = {                                                 \
        .fsm_sta = (init_sta),                                                          \
        .p_fsm_func_table = reg_fsm_func_##name##_table,                                \
        .fsm_table_size = sizeof(reg_fsm_func_##name##_table) / sizeof(reg_fsm_func_t), \
        .fsm_sta_is_change = 1,                                                         \
        .p_fsm_ev = &(fsm_ev),                                                          \
    };                                                                                  \
    static void fsm_##name##_run(void)                                                  \
    {                                                                                   \
        section_fsm_func(&reg_fsm_##name);                                              \
    }                                                                                   \
    REG_TASK_MS(1, fsm_##name##_run)

#define FSM_GET_STATE(name) (reg_fsm_##name.fsm_sta)

void section_fsm_func(reg_fsm_t *str);

/* =============================================================================
 * LINK：字节分发层（DMA ring -> handler_arr）
 * =============================================================================
 */

/* handler 签名保持不变：每字节回调 + DEC_MY_PRINTF + ctx */
typedef void (*section_link_handler_f)(uint8_t data, DEC_MY_PRINTF, void *ctx);

typedef struct
{
    section_link_handler_f func;
    void *ctx;
} section_link_handler_item_t;

typedef struct section_link_t
{
    uint8_t rx_buff[128];
    uint32_t buff_size;
    uint32_t pos;
    uint32_t *dma_cnt; ///< DMA 剩余计数（或等价写指针信息）
    DEC_MY_PRINTF;     ///< 链路输出接口（可为 NULL）
    struct section_link_t *p_next;
    const section_link_handler_item_t *handler_arr;
    uint32_t handler_num;

    uint8_t link_id;
} section_link_t;

/**
 * @brief 注册一个 LINK（放入 SECTION_LINK 段）
 *
 * 注意：
 * - REG_LINK 不生成任何 shell/comm ctx；ctx 由业务侧自己定义并填入 handler_arr
 * - rx_buff 固定 128（与结构体一致）
 */
#define REG_LINK(link, print, _dma_cnt, _handler_arr, _handler_num) \
    section_link_t section_link_##link = {                          \
        .rx_buff = {0},                                             \
        .buff_size = sizeof(section_link_##link.rx_buff),           \
        .pos = 0,                                                   \
        .dma_cnt = (_dma_cnt),                                      \
        .my_printf = &(print),                                      \
        .p_next = NULL,                                             \
        .handler_arr = (_handler_arr),                              \
        .handler_num = (uint32_t)(_handler_num),                    \
        .link_id = (uint8_t)(link),                                 \
    };                                                              \
    REG_SECTION_FUNC(SECTION_LINK, section_link_##link)

#define EXT_LINK(link) extern section_link_t section_link_##link
#define LINK_PRINTF(link) section_link_##link.my_printf
#define GET_LINK_BUFF(link) section_link_##link.rx_buff
#define GET_LINK_SIZE(link) section_link_##link.buff_size

#endif /* __SECTION_H__ */
