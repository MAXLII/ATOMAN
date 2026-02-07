/**
 * @file section.h
 * @brief “段(section)”自动注册 + 运行期链表管理框架（INIT/TASK/INT/SHELL/LINK/PERF/COMM/ROUTE/FSM）
 *
 * 设计要点：
 * 1) 编译期：通过 REG_SECTION_FUNC() 把“注册目录项 reg_section_t”放进同一个链接段 "section"
 * 2) 运行期：section_init() 扫描段首尾（SECTION_START/SECTION_STOP），按类型插入各自链表
 * 3) 调度：
 *    - TASK：run_task() 根据 tick + period 执行任务
 *    - INT ：section_interrupt() 在中断上下文按优先级执行回调
 *    - LINK：section_link_task() 轮询 DMA 新字节并分发给 handler_arr
 *    - FSM ：REG_FSM() 把状态机运行函数注册成周期任务
 *
 * 说明：
 * - 本文件只放“框架层接口/宏/数据结构”；具体实现（shell_run/comm_run/link_process等）应放在 .c 文件中
 */

#ifndef __SECTION_H_
#define __SECTION_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>

/* =============================================================================
 * SECTION 基础：注册类型 + 段目录项
 * =============================================================================
 */

/**
 * @enum SECTION_E
 * @brief 可放入自动注册段的对象类型
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

/**
 * @struct reg_section_t
 * @brief 链接段中的“目录项”
 *
 * 链接器把所有 reg_section_t 聚合到同一段中；运行期遍历后插入各自链表。
 */
typedef struct
{
    uint32_t section_type; ///< SECTION_E
    void *p_str;           ///< 指向具体注册结构（reg_task_t / section_link_t / ...）
} reg_section_t;

/* =============================================================================
 * 编译器属性：把目录项放进同名段，避免被链接器裁剪
 * =============================================================================
 */
#ifdef IS_PLECS
#define AUTO_REG_SECTION __attribute__((__section__("section")))
#define FUNC_RAM
#else
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#define FUNC_RAM __attribute__((section(".func_ram")))
#endif

/**
 * @brief 注册一个对象到“section”段（目录项）
 * @param _section_type SECTION_E
 * @param _p_str        具体对象（结构体变量名）
 */
#define REG_SECTION_FUNC(_section_type, _p_str)                   \
    const reg_section_t reg_section_##_p_str AUTO_REG_SECTION = { \
        .section_type = (_section_type),                          \
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

/**
 * @brief 注册初始化函数
 * @note section_init() 会扫描段并按 priority 插入链表，随后逐个调用
 */
#define REG_INIT(prio, func)       \
    reg_init_t reg_init_##func = { \
        .priority = (prio),        \
        .p_func = (func),          \
        .p_next = NULL,            \
    };                             \
    REG_SECTION_FUNC(SECTION_INIT, reg_init_##func)

void section_init(void);

/* =============================================================================
 * PERF：计数器基址注册 + 记录项注册
 * =============================================================================
 */

typedef enum
{
    SECTION_PERF_RECORD = 0, ///< 运行时间记录项
    SECTION_PERF_BASE,       ///< 计数器基址（timer cnt）
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
    char *p_name;
    uint16_t start;
    uint16_t end;
    uint16_t time; ///< 计数值差（单位由外部计数器决定）
    uint16_t reserved;
    uint32_t max_time;
    uint32_t **p_cnt; ///< 二级指针：指向“全局计数器指针 p_perf_cnt”
    void *p_next;
} section_perf_record_t;

/**
 * @brief 注册计数器基址（全局 p_perf_cnt 指向它）
 */
#define REG_PERF_BASE_CNT(timer_cnt)                \
    section_perf_base_t section_perf_base_timer = { \
        .p_cnt = (uint32_t *)&(timer_cnt),          \
    };                                              \
    section_perf_t section_timer_cnt_perf = {       \
        .perf_type = SECTION_PERF_BASE,             \
        .p_perf = (void *)&section_perf_base_timer, \
    };                                              \
    REG_SECTION_FUNC(SECTION_PERF, section_timer_cnt_perf)

/* 是否启用 PERF 记录宏 */
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

/**
 * @brief 注册一个性能记录项（并自动挂到 SECTION_PERF 段）
 * 注意：这里保留了你原来的“同时生成 shell 变量”的写法，依赖 REG_SHELL_VAR。
 *       如果你后续想把 PERF 与 SHELL 完全解耦，可把这段 shell 注册挪到别处。
 */
#define REG_PERF_RECORD(name)                                                                                                                         \
    section_perf_record_t section_perf_record_##name = {                                                                                              \
        .p_name = #name,                                                                                                                              \
        .start = 0,                                                                                                                                   \
        .end = 0,                                                                                                                                     \
        .max_time = 0,                                                                                                                                \
        .p_cnt = NULL,                                                                                                                                \
        .p_next = NULL,                                                                                                                               \
    };                                                                                                                                                \
    section_perf_t section_perf_record_##name##_perf = {                                                                                              \
        .perf_type = SECTION_PERF_RECORD,                                                                                                             \
        .p_perf = (void *)&section_perf_record_##name,                                                                                                \
    };                                                                                                                                                \
    REG_SECTION_FUNC(SECTION_PERF, section_perf_record_##name##_perf)                                                                                 \
    static uint32_t section_perf_record_##name##_time = 0;                                                                                            \
    static uint32_t section_perf_record_##name##_max_time = 0;                                                                                        \
    static void section_perf_record_##name##_time_cal(DEC_MY_PRINTF)                                                                                  \
    {                                                                                                                                                 \
        (void)my_printf;                                                                                                                              \
        section_perf_record_##name##_time = (uint32_t)(section_perf_record_##name.time * 0.5f);                                                       \
        section_perf_record_##name##_max_time = (uint32_t)(section_perf_record_##name.max_time * 0.5f);                                               \
    }                                                                                                                                                 \
    REG_SHELL_VAR(PE##name##T, section_perf_record_##name##_time, SHELL_UINT32, 0xFFFFFFFF, 0, section_perf_record_##name##_time_cal, SHELL_STA_NULL) \
    REG_SHELL_VAR(PE##name##MT, section_perf_record_##name##_max_time, SHELL_UINT32, 0xFFFFFFFF, 0, section_perf_record_##name##_time_cal, SHELL_STA_NULL)

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
#define REG_TASK_MS(period, func) REG_TASK(((period) * 10), func)

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
 * FSM：表驱动状态机
 * =============================================================================
 */

typedef struct
{
    char *p_name;
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

/**
 * @brief 注册状态机，并自动注册一个 1ms 周期任务驱动它
 */
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
 * SHELL：命令/变量注册 + 输入处理接口
 * =============================================================================
 */

typedef struct
{
    void (*my_printf)(const char *__format, ...);
    void (*tx_by_dma)(char *ptr, int len);
} section_link_tx_func_t;

/**
 * @brief 统一打印接口的形参写法（保持你现有签名）
 */
#define DEC_MY_PRINTF section_link_tx_func_t *my_printf

/**
 * @brief Shell 输入处理（具体实现应在 shell.c）
 * @param data  单字节输入
 * @param my_printf 打印接口（可为空）
 * @param p     handler 私有上下文
 */
void shell_run(uint8_t data, DEC_MY_PRINTF, void *p);

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

typedef enum
{
    SHELL_INT8,
    SHELL_UINT8,
    SHELL_INT16,
    SHELL_UINT16,
    SHELL_INT32,
    SHELL_UINT32,
    SHELL_FP32,
    SHELL_CMD,
} SHELL_TYPE_E;

typedef struct
{
    uint8_t shell_buffer[128]; ///< Shell缓冲区
    uint8_t shell_index;       ///< 缓冲区索引
} shell_ctx_t;

#define SHELL_STR_SIZE_MAX 40
#define SHELL_STA_NULL (0)
#define SHELL_STA_AUTO (1u << 2)

typedef struct section_shell_t
{
    const char *p_name;
    uint32_t p_name_size;
    void *p_var;   ///< 变量指针（命令为 NULL）
    uint32_t type; ///< SHELL_TYPE_E
    void *p_max;
    void *p_min;
    void (*func)(DEC_MY_PRINTF);
    uint32_t status;
    DEC_MY_PRINTF; ///< 运行期可被框架填入/透传（可选使用）
    struct section_shell_t *p_next;
} section_shell_t;

#define REG_SHELL_VAR(_name, _var, _type, _max, _min, _func, _status)                      \
    static __typeof__(_var) _name##_##max = (__typeof__(_var))(_max);                      \
    static __typeof__(_var) _name##_##min = (__typeof__(_var))(_min);                      \
    section_shell_t section_shell_##_name = {                                              \
        .p_name = #_name,                                                                  \
        .p_name_size = (sizeof(#_name) - 1),                                               \
        .p_var = (void *)&(_var),                                                          \
        .type = (_type),                                                                   \
        .p_max = (void *)&_name##_##max,                                                   \
        .p_min = (void *)&_name##_##min,                                                   \
        .func = (_func),                                                                   \
        .p_next = NULL,                                                                    \
        .status = (_status),                                                               \
    };                                                                                     \
    static_assert(sizeof(#_name) <= (SHELL_STR_SIZE_MAX + 1), #_name " String too long!"); \
    REG_SECTION_FUNC(SECTION_SHELL, section_shell_##_name)

#define REG_SHELL_CMD(_name, _func)                                                        \
    section_shell_t section_shell_##_name = {                                              \
        .p_name = #_name,                                                                  \
        .p_name_size = (sizeof(#_name) - 1),                                               \
        .p_var = NULL,                                                                     \
        .type = SHELL_CMD,                                                                 \
        .func = (_func),                                                                   \
        .p_next = NULL,                                                                    \
    };                                                                                     \
    static_assert(sizeof(#_name) <= (SHELL_STR_SIZE_MAX + 1), #_name " String too long!"); \
    REG_SECTION_FUNC(SECTION_SHELL, section_shell_##_name)

/* =============================================================================
 * COMM：协议/路由/CRC + comm_ctx_t（已与 LINK 解耦）
 * =============================================================================
 */

/* CRC-16-CCITT */
#define CRC16_CCITT_POLY 0x1021
#define CRC16_CCITT_INIT 0xFFFF

uint16_t crc16_init(void);
uint16_t crc16_update(uint16_t crc, uint8_t data);
uint16_t crc16_final(uint16_t crc);

uint16_t section_crc16(uint8_t *p_data, uint32_t len);
uint16_t section_crc16_with_crc(uint8_t *p_data, uint32_t len, uint16_t crc_in);

#pragma pack(1)
typedef struct
{
    uint8_t sop; ///< 0xE8
    uint8_t version;
    uint8_t src;
    uint8_t d_src;
    uint8_t dst;
    uint8_t d_dst;
    uint8_t cmd_set;
    uint8_t cmd_word;
    uint8_t is_ack;
    uint16_t len;
    uint8_t *p_data;
    uint16_t crc; ///< sop..p_data
    uint16_t eop; ///< 0x0A0D
} section_packform_t;
#pragma pack()

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

/**
 * @struct comm_ctx_t
 * @brief COMM 解析上下文（与 LINK 解耦）
 *
 */
typedef struct
{
    uint8_t *p_data_buffer; ///< payload 缓冲区（由用户/宏创建）
    uint16_t buffer_size;   ///< payload 缓冲长度
    uint16_t index;
    uint8_t status;
    uint16_t crc;
    section_packform_t pack;
    void (*func)(section_packform_t *p_pack, DEC_MY_PRINTF);

    uint16_t len;
    const uint8_t src;
    uint8_t d_src;

    uint8_t src_flag : 1;
    uint8_t dst_flag : 1;
    uint8_t cmd_flag : 1;
    uint8_t len_flag : 1;
    uint8_t eop_flag : 1;
    uint8_t is_route : 1;

    uint8_t link_id;
} comm_ctx_t;

/**
 * @brief 一句宏把 comm_ctx_t + payload buffer 声明好（放在业务文件里用）
 */
#define DECLARE_COMM_CTX(name, payload_size, _src, _link_id) \
    static uint8_t name##_payload_buf[(payload_size)] = {0}; \
    static comm_ctx_t name = {                               \
        .p_data_buffer = name##_payload_buf,                 \
        .buffer_size = (uint16_t)sizeof(name##_payload_buf), \
        .index = 0,                                          \
        .status = SECTION_PACKFORM_STA_SOP,                  \
        .crc = 0,                                            \
        .pack = (section_packform_t){0},                     \
        .func = NULL,                                        \
        .len = 0,                                            \
        .src = (uint8_t)(_src),                              \
        .d_src = 0,                                          \
        .src_flag = 0,                                       \
        .dst_flag = 0,                                       \
        .cmd_flag = 0,                                       \
        .len_flag = 0,                                       \
        .eop_flag = 0,                                       \
        .is_route = 0,                                       \
        .link_id = (uint8_t)(_link_id),                      \
    }

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
        .dst_addr = (_dst_addr),                                        \
        .p_next = NULL,                                                 \
    };                                                                  \
    REG_SECTION_FUNC(SECTION_COMM_ROUTE, comm_route_##_src_link_id##_dst_link_id##_dst_addr)

#define REG_COMM_ROUTE(_src_link_id, _dst_link_id, _dst_addr) \
    _REG_COMM_ROUTE((_src_link_id), (_dst_link_id), (_dst_addr))

typedef struct section_com_t
{
    uint8_t cmd_set;
    uint8_t cmd_word;
    void (*func)(section_packform_t *p_pack, DEC_MY_PRINTF);
    struct section_com_t *p_next;
} section_com_t;

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

/* COMM 输入/发送接口（实现放 comm.c） */
void comm_run(uint8_t data, DEC_MY_PRINTF, void *p);
void comm_send_data(section_packform_t *p_pack, DEC_MY_PRINTF);

/* =============================================================================
 * LINK：字节分发层（仅负责“从 DMA ring 缓冲取新字节 -> 分发 handler”）
 * =============================================================================
 */

/**
 * @brief LINK handler 统一签名：每字节回调
 * @param data 单字节
 * @param my_printf 输出接口
 * @param ctx handler 私有上下文（可传 comm_ctx_t* / shell_ctx_t* / NULL）
 */
typedef void (*section_link_handler_f)(uint8_t data, DEC_MY_PRINTF, void *ctx);

typedef struct
{
    section_link_handler_f func;
    void *ctx;
} section_link_handler_item_t;

/**
 * @struct section_link_t
 * @brief 链路运行期对象（被 section.c 扫描后插入 link 链表）
 *
 * 说明：
 * - rx_buff：链路层 ring 缓冲（通常由 DMA 填充）
 * - dma_cnt：DMA 当前剩余计数（或其它“写指针”信息），由 link_process 计算新字节数量
 * - handler_arr：分发目标数组（每个元素 = func + ctx）
 */
typedef struct section_link_t
{
    uint8_t rx_buff[128];
    uint32_t buff_size;
    uint32_t pos;

    uint32_t *dma_cnt;
    DEC_MY_PRINTF;

    struct section_link_t *p_next;

    const section_link_handler_item_t *handler_arr;
    uint32_t handler_num;

    uint8_t link_id;
} section_link_t;

/**
 * @brief 注册一个 LINK（把 section_link_##link 放入 SECTION_LINK 段）
 *
 * 注意：
 * - 本版 REG_LINK 不再生成 comm_ctx_t；COMM/SHELL 的 ctx 由你在业务文件里自行 DECLARE_*_CTX 后传入 handler_arr
 * - rx_buff 固定 128（与 section_link_t 定义一致）；若要可变长度，需要把 rx_buff 改成柔性数组并重写宏
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

#endif /* __SECTION_H_ */
