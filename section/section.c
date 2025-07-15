/**
 * @file section.c
 * @brief 段管理系统实现文件
 * @author WeiXin.Li
 * @version 1.0
 * @date 2025-07-09
 * @copyright Copyright (c) 2025
 *
 * @details
 * 本文件实现了基于段的自动注册管理系统，支持以下功能：
 * - 任务管理：定时任务的注册和执行
 * - 初始化管理：初始化函数的自动调用
 * - 中断管理：中断处理函数的优先级管理
 * - Shell命令：交互式命令行接口
 * - 状态机：FSM状态机管理
 * - 链路管理：数据链路处理
 */

#include "section.h"
#include "stddef.h"
#include "string.h"
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

// 平台相关配置
#ifdef IS_PLECS
#include "plecs.h"
extern uint32_t plecs_time_100us;
#define SECTION_SYS_TICK plecs_time_100us ///< 系统时钟（100us单位）
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section ///< 段起始地址
#define SECTION_STOP __stop_section   ///< 段结束地址
#define SYSTEM_RESET ;                ///< 系统复位（PLECS中为空）
#else
#include "systick.h"
#include "gd32g5x3.h"
#define SECTION_SYS_TICK systick_gettime_100us() ///< 系统时钟（100us单位）
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start            ///< 段起始地址
#define SECTION_STOP __section_end               ///< 段结束地址
#define SYSTEM_RESET nvic_system_reset()         ///< 系统复位
#endif

// 全局链表头指针
reg_task_t *p_task_first = NULL;                   ///< 任务链表头指针
reg_interrupt_t *p_interrupt_first = NULL;         ///< 中断链表头指针
section_shell_t *p_shell_first = NULL;             ///< Shell命令链表头指针
section_link_t *p_link_first = NULL;               ///< 链路链表头指针
section_perf_record_t *p_perf_record_first = NULL; ///< 性能计数器链表头指针
uint32_t *p_perf_cnt = NULL;                       /// 性能计数器一级指针
section_com_t *p_com_first = NULL;                 ///< COMM链表头指针

/**
 * @brief 检查字符串是否为有效数字表达式
 * @param str 待检查的字符串
 * @return 1: 有效数字表达式, 0: 无效
 * @note 支持小数、负数、以及简单的四则运算符
 */
static int is_string_number(const char *str)
{
    if (!str || !*str)
        return 0;

    // 处理正负号
    if (*str == '-' || *str == '+')
        str++;

    int has_digit = 0, has_dot = 0;
    while (*str)
    {
        if (isdigit((unsigned char)*str))
        {
            has_digit = 1;
        }
        else if (*str == '.' && !has_dot)
        {
            has_dot = 1;
        }
        else if (*str == '*' || *str == '/' || *str == '+' || *str == '-')
        {
            // 允许表达式字符
        }
        else
        {
            return 0;
        }
        str++;
    }
    return has_digit;
}

// 表达式求值函数声明
static float eval_expr_inner(const char **p);

/**
 * @brief 简单表达式求值（支持 + - * / 和括号）
 * @param expr 表达式字符串
 * @return 计算结果
 * @note 支持运算符优先级和括号，考虑负号处理
 */
static float eval_expr(const char *expr)
{
    return eval_expr_inner(&expr);
}

/**
 * @brief 递归解析表达式内部实现
 * @param p 指向表达式字符串指针的指针
 * @return 计算结果
 * @note 支持括号和运算符优先级，乘除法优先于加减法
 */
static float eval_expr_inner(const char **p)
{
    const char *expr = *p;
    float result = 0;
    char op = '+';

    while (*expr)
    {
        // 跳过空格
        while (*expr == ' ')
            expr++;

        // 处理连续的正负号
        int sign = 1;
        while (*expr == '+' || *expr == '-')
        {
            if (*expr == '-')
                sign *= -1;
            expr++;
        }
        while (*expr == ' ')
            expr++;

        float number = 0;

        // 处理括号
        if (*expr == '(')
        {
            expr++; // 跳过 '('
            number = eval_expr_inner(&expr);
            if (*expr == ')')
                expr++; // 跳过 ')'
        }
        else
        {
            // 解析数字
            number = strtof(expr, (char **)&expr);
        }
        number *= sign;

        // 处理连续的乘除运算（优先级高）
        while (1)
        {
            while (*expr == ' ')
                expr++;
            if (*expr != '*' && *expr != '/')
                break;

            char muldiv = *expr++;
            while (*expr == ' ')
                expr++;

            float next = 0;
            int next_sign = 1;

            // 处理乘除运算后的正负号
            while (*expr == '+' || *expr == '-')
            {
                if (*expr == '-')
                    next_sign *= -1;
                expr++;
            }
            while (*expr == ' ')
                expr++;

            // 处理括号或数字
            if (*expr == '(')
            {
                expr++;
                next = eval_expr_inner(&expr);
                if (*expr == ')')
                    expr++;
            }
            else
            {
                next = strtof(expr, (char **)&expr);
            }
            next *= next_sign;

            // 执行乘除运算
            if (muldiv == '*')
                number *= next;
            else if (muldiv == '/' && next != 0)
                number /= next;
        }

        // 执行加减运算
        if (op == '+')
            result += number;
        else if (op == '-')
            result -= number;

        // 获取下一个运算符
        while (*expr == ' ')
            expr++;
        if (*expr == '+' || *expr == '-')
            op = *expr++;
        else if (*expr == ')')
            break;
        else if (*expr == '\0')
            break;
        else
            op = '+'; // 容错处理
    }
    *p = expr;
    return result;
}

/**
 * @brief 解析整数参数（支持十进制、十六进制、二进制和表达式）
 * @param param 参数字符串
 * @param out 输出的整数值
 * @return 1: 解析成功, 0: 解析失败
 * @note 支持0x前缀(十六进制)、0b前缀(二进制)和表达式计算
 */
static int parse_integer(const char *param, int32_t *out)
{
    if (param == NULL)
        return 0;

    if (strncmp(param, "0x", 2) == 0)
    {
        // 十六进制
        *out = (int32_t)strtol(param, NULL, 16);
        return 1;
    }
    else if (strncmp(param, "0b", 2) == 0)
    {
        // 二进制
        *out = (int32_t)strtol(param + 2, NULL, 2);
        return 1;
    }
    else
    {
        // 十进制或表达式
        *out = (int32_t)eval_expr(param);
        return 1;
    }
    return 0;
}

/**
 * @brief Shell命令处理函数
 * @param data 接收到的字符
 * @param my_printf 打印函数指针
 * @note 处理交互式命令行，支持变量查看/修改和命令执行
 */
void shell_run(uint8_t data, DEC_MY_PRINTF)
{
    static uint8_t shell_buffer[128]; ///< Shell缓冲区
    static uint8_t shell_index = 0;   ///< 缓冲区索引

    // 溢出保护
    if (shell_index >= sizeof(shell_buffer) - 1)
    {
        shell_index = 0;
    }
    shell_buffer[shell_index++] = data;

    // 检测命令结束（兼容 '\n' 或 "\r\n"）
    if ((data == '\n' && shell_index > 1 && shell_buffer[shell_index - 2] == '\r') ||
        (data == '\n' && shell_index > 0))
    {
        // 计算有效命令长度
        uint8_t end = shell_index;
        if (shell_index > 1 && shell_buffer[shell_index - 2] == '\r')
            end = shell_index - 2;
        else
            end = shell_index - 1;
        shell_buffer[end] = '\0';

        // 处理内置命令
        if (strcmp((char *)shell_buffer, "time") == 0)
        {
            // 显示系统时间
            my_printf("time = %us.%03ums\r\n",
                      SECTION_SYS_TICK / 10000,
                      SECTION_SYS_TICK % 10000 / 10);
            goto shell_done;
        }
        else if (strcmp((char *)shell_buffer, "reset") == 0)
        {
            // 系统复位
            SYSTEM_RESET;
            goto shell_done;
        }
        else if (strcmp((char *)shell_buffer, "help") == 0)
        {
            // 显示帮助信息
            section_shell_t *s = p_shell_first;
            while (s)
            {
                my_printf("%s\t%s\r\n", s->p_name, s->type == SHELL_CMD ? "CMD" : "VAR");
                s = s->p_next;
            }
            goto shell_done;
        }

        // 查找匹配的Shell命令或变量
        section_shell_t *p = p_shell_first;
        section_shell_t *p_last = NULL; // 用于记录上一个节点
        while (p)
        {
            if (strncmp((char *)shell_buffer, p->p_name, p->p_name_size) == 0 &&
                (shell_buffer[p->p_name_size] == ':' || shell_buffer[p->p_name_size] == '\0'))
            {
                if (p_last)
                {
                    p_last->p_next = p->p_next; // 断开前一个节点的链接
                    p->p_next = p_shell_first;  // 将当前节点移到链表头部
                    p_shell_first = p;          // 更新链表头指针
                }
                char *param = NULL;
                if (shell_buffer[p->p_name_size] == ':')
                {
                    // 提取参数
                    param = (char *)&shell_buffer[p->p_name_size + 1];
                    while (*param == ' ')
                        param++;
                }

                int32_t intval = 0;
                float floatval = 0.0f;

                // 根据变量类型进行处理
                switch (p->type)
                {
                case SHELL_CMD:
                    // 执行命令
                    if (p->func)
                        p->func(my_printf);
                    break;
                case SHELL_UINT8:
                {
                    uint8_t val = *(uint8_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                        val = (uint8_t)intval;
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf("%s = %u\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_INT8:
                {
                    int8_t val = *(int8_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                        val = (int8_t)intval;
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf("%s = %d\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_UINT16:
                {
                    uint16_t val = *(uint16_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                        val = (uint16_t)intval;
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf("%s = %u\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_INT16:
                {
                    int16_t val = *(int16_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                        val = (int16_t)intval;
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf("%s = %d\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_UINT32:
                {
                    uint32_t val = *(uint32_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                        val = (uint32_t)intval;
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf("%s = %lu\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_INT32:
                {
                    int32_t val = *(int32_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                        val = (int32_t)intval;
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf("%s = %ld\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_FP32:
                {
                    float val = *(float *)p->p_var;
                    if (param && is_string_number(param))
                        val = eval_expr(param);
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf("%s = %f\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                }
                goto shell_done;
            }
            p_last = p; // 记录上一个节点
            p = p->p_next;
        }

    shell_done:
        // 清空缓冲区
        memset(shell_buffer, 0, sizeof(shell_buffer));
        shell_index = 0;
    }
}

/**
 * @brief 将任务插入到任务链表中
 * @param task 要插入的任务
 * @note 任务按注册顺序排列，插入到链表尾部
 */
static void task_insert(reg_task_t *task)
{
    task->time_last = SECTION_SYS_TICK;
    task->p_next = NULL;

    if (p_task_first == NULL)
    {
        p_task_first = task;
    }
    else
    {
        reg_task_t *curr = p_task_first;
        while (curr->p_next)
            curr = curr->p_next;
        curr->p_next = task;
    }
}

/**
 * @brief 将中断处理函数插入到中断链表中
 * @param intr 要插入的中断处理函数
 * @note 中断按优先级排序，优先级数值越小优先级越高
 */
static void interrupt_insert(reg_interrupt_t *intr)
{
    if (!intr)
        return;

    intr->p_next = NULL;

    // 插入到链表头或按优先级顺序插入
    if (!p_interrupt_first || intr->priority < p_interrupt_first->priority)
    {
        intr->p_next = p_interrupt_first;
        p_interrupt_first = intr;
    }
    else
    {
        reg_interrupt_t *prev = p_interrupt_first;
        while (prev->p_next && prev->p_next->priority < intr->priority)
        {
            prev = prev->p_next;
        }
        intr->p_next = prev->p_next;
        prev->p_next = intr;
    }
}

/**
 * @brief 将Shell命令插入到Shell链表中
 * @param shell 要插入的Shell命令
 * @note 防止重复插入同一个命令
 */
static void shell_insert(section_shell_t *shell)
{
    if (!shell)
        return;

    // 防止重复插入
    section_shell_t *last = NULL;
    for (section_shell_t *p = p_shell_first; p; last = p, p = p->p_next)
    {
        if (p == shell)
            return;
    }

    shell->p_next = NULL;
    if (last)
        last->p_next = shell;
    else
        p_shell_first = shell;
}

/**
 * @brief 将链路插入到链路链表中
 * @param link 要插入的链路
 * @note 链路按注册顺序排列
 */
static void link_insert(section_link_t *link)
{
    link->p_next = NULL;

    if (!p_link_first)
        p_link_first = link;
    else
    {
        section_link_t *curr = p_link_first;
        while (curr->p_next)
            curr = (section_link_t *)curr->p_next;
        curr->p_next = link;
    }
}

/**
 * @brief 将性能计数器或记录插入到性能管理系统
 * @param perf 要插入的性能计数器或记录
 * @note 支持性能计数器基地址和性能记录的注册
 */
static void perf_insert(section_perf_t *perf)
{
    if (!perf)
        return;
    section_perf_base_t *base;
    section_perf_record_t *rec;
    switch (perf->perf_type)
    {
    case SECTION_PERF_BASE:
        // 性能计数器基地址注册
        base = (section_perf_base_t *)perf->p_perf;
        if (base && base->p_cnt)
            p_perf_cnt = base->p_cnt;
        break;
    case SECTION_PERF_RECORD:
        // 性能记录注册，链表管理
        rec = (section_perf_record_t *)perf->p_perf;
        if (rec)
        {
            rec->p_cnt = &p_perf_cnt; // 赋值为一级指针的地址
            rec->p_next = NULL;
            if (!p_perf_record_first)
                p_perf_record_first = rec;
            else
            {
                section_perf_record_t *cur = p_perf_record_first;
                while (cur->p_next)
                    cur = (section_perf_record_t *)cur->p_next;
                cur->p_next = rec;
            }
        }
        break;
    default:
        break;
    }
}

/**
 * @brief 将COMM命令插入到COMM链表中
 * @param com 要插入的COMM命令
 */
static void comm_insert(section_com_t *com)
{
    if (!com)
        return;
    com->p_next = NULL;
    if (!p_com_first)
        p_com_first = com;
    else
    {
        section_com_t *curr = p_com_first;
        while (curr->p_next)
            curr = (section_com_t *)curr->p_next;
        curr->p_next = com;
    }
}

/**
 * @brief 系统初始化函数
 * @note 遍历段中的所有注册项，根据类型进行相应的初始化操作
 */
void section_init(void)
{
    // 遍历段中的所有注册项
    for (reg_section_t *p = (reg_section_t *)&SECTION_START; p < (reg_section_t *)&SECTION_STOP; ++p)
    {
        switch (p->section_type)
        {
        case SECTION_INIT:
            // 执行初始化函数
            ((reg_init_t *)p->p_str)->p_func();
            break;
        case SECTION_TASK:
            // 插入任务到任务链表
            task_insert((reg_task_t *)p->p_str);
            break;
        case SECTION_INTERRUPT:
            // 插入中断到中断链表
            interrupt_insert((reg_interrupt_t *)p->p_str);
            break;
        case SECTION_SHELL:
            // 插入Shell命令到Shell链表
            shell_insert((section_shell_t *)p->p_str);
            break;
        case SECTION_LINK:
            // 插入链路到链路链表
            link_insert((section_link_t *)p->p_str);
            break;
        case SECTION_PERF:
            // 性能计数器初始化
            perf_insert((section_perf_t *)p->p_str);
            break;
        case SECTION_COMM:
            // 插入COMM命令到COMM链表
            comm_insert((section_com_t *)p->p_str);
            break;
        }
    }
}

/**
 * @brief 运行所有注册的定时任务
 * @note 检查每个任务的执行周期，到期则执行任务函数
 */
void run_task(void)
{
    uint32_t now = SECTION_SYS_TICK;

    // 遍历任务链表，检查是否到期
    for (reg_task_t *t = p_task_first; t != NULL; t = (reg_task_t *)t->p_next)
    {
        if (now - t->time_last >= t->t_period)
        {
            if ((t->p_perf_record == NULL) ||
                (*t->p_perf_record->p_cnt == NULL)) // 如果没有性能记录或计数器
            {
                t->p_func(); // 执行任务函数
            }
            else
            {
                t->p_perf_record->start = **t->p_perf_record->p_cnt;                      // 记录开始时间
                t->p_func();                                                              // 执行任务函数
                t->p_perf_record->end = **t->p_perf_record->p_cnt;                        // 记录结束时间
                t->p_perf_record->time = t->p_perf_record->end - t->p_perf_record->start; // 计算执行时间
                if (t->p_perf_record->time > t->p_perf_record->max_time)
                    t->p_perf_record->max_time = t->p_perf_record->time; // 更新最大时间
            }
            t->time_last = now; // 更新最后执行时间
        }
    }
}

/**
 * @brief 执行所有注册的中断处理函数
 * @note 按优先级顺序执行所有中断处理函数
 */
void section_interrupt(void)
{
    for (reg_interrupt_t *p = p_interrupt_first; p != NULL; p = (reg_interrupt_t *)p->p_next)
    {
        p->p_func();
    }
}

/**
 * @brief 状态机运行函数
 * @param fsm 状态机控制结构
 * @note 根据当前状态执行相应的状态处理函数，处理状态转换
 */
void section_fsm_func(reg_fsm_t *fsm)
{
    if (!fsm->p_fsm_func_table || !fsm->p_fsm_ev)
        return;

    // 查找当前状态对应的处理函数
    for (uint32_t i = 0; i < fsm->fsm_table_size; ++i)
    {
        reg_fsm_func_t *entry = &fsm->p_fsm_func_table[i];
        if (fsm->fsm_sta == entry->fsm_sta)
        {
            // 状态刚切换时执行入口函数
            if (fsm->fsm_sta_is_change)
            {
                fsm->fsm_sta_is_change = 0;
                entry->func_in();
            }

            // 执行状态处理函数
            entry->func_exe();

            // 检查是否有事件需要处理
            if (*fsm->p_fsm_ev)
            {
                uint32_t next = entry->func_chk(*fsm->p_fsm_ev);
                *fsm->p_fsm_ev = 0;

                // 状态切换
                if (next && next != entry->fsm_sta)
                {
                    entry->func_out();          // 执行出口函数
                    fsm->fsm_sta = next;        // 切换状态
                    fsm->fsm_sta_is_change = 1; // 标记状态已改变
                }
            }
            break;
        }
    }
}

/**
 * @brief 处理单个链路的数据
 * @param link 链路结构指针
 * @note 从DMA缓冲区读取数据，调用注册的处理函数
 */
static void link_process(section_link_t *link)
{
    if (!link || !link->p_buff || !link->dma_cnt || !link->func_arr)
        return;

    uint32_t buff_size = link->buff_size;
    uint32_t dma_cnt = *link->dma_cnt;
    uint32_t cnt = buff_size - dma_cnt;

    // 处理缓冲区中的新数据
    while (link->pos != cnt)
    {
        uint8_t data = (*link->p_buff)[link->pos];

        // 调用所有注册的处理函数
        for (uint32_t i = 0; i < link->func_num; ++i)
        {
            if (link->func_arr[i])
            {
                link->func_arr[i](data, link->my_printf);
            }
        }

        // 更新位置指针（环形缓冲区）
        link->pos = (link->pos + 1) % buff_size;
    }
}

/**
 * @brief 链路处理任务
 * @note 定期处理所有注册的链路数据
 */
void section_link_task(void)
{
    for (section_link_t *p = p_link_first; p != NULL; p = (section_link_t *)p->p_next)
    {
        link_process(p);
    }
}

// 注册链路处理任务，每100ms执行一次
REG_TASK(10, section_link_task);

/**
 * @brief 列出所有Shell命令和变量
 * @param my_printf 打印函数指针
 * @note 显示所有注册的Shell命令和变量及其当前值
 */
static void list(DEC_MY_PRINTF)
{
    for (section_shell_t *s = p_shell_first; s; s = s->p_next)
    {
        my_printf("%s\t", s->p_name);
        switch (s->type)
        {
        case SHELL_CMD:
            my_printf("CMD\r\n");
            break;
        case SHELL_UINT8:
            my_printf("U8\t%d\r\n", *(uint8_t *)s->p_var);
            break;
        case SHELL_UINT16:
            my_printf("U16\t%d\r\n", *(uint16_t *)s->p_var);
            break;
        case SHELL_UINT32:
            my_printf("U32\t%d\r\n", *(uint32_t *)s->p_var);
            break;
        case SHELL_INT8:
            my_printf("I8\t%d\r\n", *(int8_t *)s->p_var);
            break;
        case SHELL_INT16:
            my_printf("I16\t%d\r\n", *(int16_t *)s->p_var);
            break;
        case SHELL_INT32:
            my_printf("I32\t%d\r\n", *(int32_t *)s->p_var);
            break;
        case SHELL_FP32:
            my_printf("FP32\t%f\r\n", *(float *)s->p_var);
            break;
        }
    }
}

/**
 * @brief list分时打印上下文
 */
typedef struct
{
    section_shell_t *cur;
    void (*my_printf)(const char *__format, ...);
    uint8_t active;
    int max_name_len; // 记录最长变量名长度
} list_print_ctx_t;

static list_print_ctx_t g_list_print_ctx = {0};

/**
 * @brief 启动list分时打印
 * @param my_printf 打印函数指针
 */
void list_print_start(DEC_MY_PRINTF)
{
    if (!my_printf || g_list_print_ctx.active)
        return;
    g_list_print_ctx.cur = p_shell_first;
    g_list_print_ctx.my_printf = my_printf;
    g_list_print_ctx.active = 1;
    int max_len = 0;
    for (section_shell_t *s = p_shell_first; s; s = s->p_next)
    {
        int len = strlen(s->p_name);
        if (len > max_len)
            max_len = len;
    }
    g_list_print_ctx.max_name_len = max_len;
    my_printf("\r\n========== SHELL COMMANDS AND VARIABLES ==========\r\n");
}

REG_SHELL_CMD(list, list_print_start);

/**
 * @brief list分时打印，每次调用打印一项，并交替打印分隔线（自动对齐）
 * @return 1: 未完成，0: 已完成
 */
int list_print_step(void)
{
    static uint8_t print_flag = 0; // 0: 打印数据, 1: 打印分隔线

    if (!g_list_print_ctx.active)
        return 0;

    // 打印数据行
    if (print_flag == 0)
    {
        section_shell_t *s = g_list_print_ctx.cur;
        if (!s)
        {
            g_list_print_ctx.active = 0;
            print_flag = 0;
            return 0;
        }

        int name_len = strlen(s->p_name);
        int tab_size = 8; // 一个\t约4字符宽
        int tab_count = (g_list_print_ctx.max_name_len / tab_size) - (name_len / tab_size) + 1;

        g_list_print_ctx.my_printf("%s", s->p_name);
        for (int i = 0; i < tab_count; i++)
            g_list_print_ctx.my_printf("\t");

        switch (s->type)
        {
        case SHELL_CMD:
            g_list_print_ctx.my_printf("CMD\r\n");
            break;
        case SHELL_UINT8:
            g_list_print_ctx.my_printf("U8\t%d\r\n", *(uint8_t *)s->p_var);
            break;
        case SHELL_UINT16:
            g_list_print_ctx.my_printf("U16\t%d\r\n", *(uint16_t *)s->p_var);
            break;
        case SHELL_UINT32:
            g_list_print_ctx.my_printf("U32\t%d\r\n", *(uint32_t *)s->p_var);
            break;
        case SHELL_INT8:
            g_list_print_ctx.my_printf("I8\t%d\r\n", *(int8_t *)s->p_var);
            break;
        case SHELL_INT16:
            g_list_print_ctx.my_printf("I16\t%d\r\n", *(int16_t *)s->p_var);
            break;
        case SHELL_INT32:
            g_list_print_ctx.my_printf("I32\t%d\r\n", *(int32_t *)s->p_var);
            break;
        case SHELL_FP32:
            g_list_print_ctx.my_printf("FP32\t%f\r\n", *(float *)s->p_var);
            break;
        }
        g_list_print_ctx.cur = s->p_next;
        print_flag = 1;
        return 1;
    }
    // 打印分隔线
    else
    {
        g_list_print_ctx.my_printf("-----------------------------------------\r\n");
        print_flag = 0;
        return 1;
    }
}

/**
 * @brief 1ms分时任务，驱动list分时打印
 */
static void list_print_task(void)
{
    // 每次调用打印一项，直到完成
    if (g_list_print_ctx.active)
    {
        list_print_step();
    }
}

// 注册1ms分时任务
REG_TASK_MS(1, list_print_task);

/**
 * @brief 打印所有性能统计结果
 * @param my_printf 打印函数指针
 */
void print_perf_record(DEC_MY_PRINTF)
{
    if (!my_printf)
        return;
    section_perf_record_t *p = p_perf_record_first;
    my_printf("Perf Name\tTime(us)\tMax(us)\r\n");
    while (p)
    {
        my_printf("%s\t%u\t%u\r\n", p->p_name, (unsigned)(p->time * 1.0f / 216.0f * 21.0f), (unsigned)(p->max_time * 0.105));
        p = (section_perf_record_t *)p->p_next;
    }
}

typedef struct
{
    section_perf_record_t *cur;
    void (*my_printf)(const char *__format, ...);
    uint8_t active;
    uint32_t perf_max_name_len;
} perf_print_ctx_t;

static perf_print_ctx_t g_perf_print_ctx = {0};

/**
 * @brief 启动分时打印性能统计结果
 * @param my_printf 打印函数指针
 */
void print_perf_record_start(DEC_MY_PRINTF)
{
    if (!my_printf || g_perf_print_ctx.active)
        return;
    g_perf_print_ctx.cur = p_perf_record_first;
    g_perf_print_ctx.my_printf = my_printf;
    g_perf_print_ctx.active = 1;
    int max_len = 0;
    for (section_perf_record_t *s = p_perf_record_first; s; s = s->p_next)
    {
        int len = strlen(s->p_name);
        if (len > max_len)
            max_len = len;
    }
    g_perf_print_ctx.perf_max_name_len = max_len;
    int name_len = strlen("Perf Name");
    int tab_size = 8;
    int tab_count = (g_perf_print_ctx.perf_max_name_len / tab_size) - (name_len / tab_size) + 1;
    my_printf("Perf Name");
    for (int i = 0; i < tab_count; i++)
        my_printf("\t");
    my_printf("Time\tMax\r\n");
}

/**
 * @brief 分时打印性能统计结果，每次调用打印一项
 * @return 1: 未完成，0: 已完成
 */
int print_perf_record_step(void)
{
    if (!g_perf_print_ctx.active)
        return 0;
    section_perf_record_t *p = g_perf_print_ctx.cur;
    if (!p)
    {
        g_perf_print_ctx.active = 0;
        return 0;
    }
    int name_len = strlen(p->p_name);
    int tab_size = 8;
    int tab_count = (g_perf_print_ctx.perf_max_name_len / tab_size) - (name_len / tab_size) + 1;
    g_perf_print_ctx.my_printf("%s", p->p_name);
    for (int i = 0; i < tab_count; i++)
        g_perf_print_ctx.my_printf("\t");
    g_perf_print_ctx.my_printf("%u\t%u\r\n", (unsigned)(p->time * 0.105), (unsigned)(p->max_time * 0.105));
    g_perf_print_ctx.cur = (section_perf_record_t *)p->p_next;
    return 1;
}

/**
 * @brief 1ms分时任务，驱动性能统计分时打印
 */
static void perf_print_task(void)
{
    if (g_perf_print_ctx.active)
        print_perf_record_step();
}

// 注册分时打印性能统计任务
REG_TASK_MS(1, perf_print_task);

// 注册性能统计打印命令到Shell
REG_SHELL_CMD(perf_print_record, print_perf_record_start)

static void perf_clear_max_time(DEC_MY_PRINTF)
{
    section_perf_record_t *p = p_perf_record_first;
    while (p)
    {
        p->time = 0;     // 清除当前时间
        p->max_time = 0; // 清除最大时间
        p = (section_perf_record_t *)p->p_next;
    }
    my_printf("Performance max time cleared.\r\n");
}

REG_SHELL_CMD(perf_clear_max_time, perf_clear_max_time);

/**
 * @brief 根据cmd查找COMM链表，返回对应的处理函数指针
 * @param cmd 命令字
 * @return 对应的处理函数指针，未找到返回NULL
 */
void (*find_comm_func(uint8_t cmd))(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_com_t *p = p_com_first;
    section_com_t *p_last = NULL;
    while (p)
    {
        if (p->cmd == cmd)
        {
            if (p_last)
            {
                p_last->p_next = p->p_next; // 断开前一个节点的链接
                p->p_next = p_com_first;    // 将当前节点移到链表头部
                p_com_first = p;            // 更新链表头指针
            }
            void (*func)(section_packform_t *p_pack, DEC_MY_PRINTF);
            return p->func;
        }
        p_last = p;
        p = (section_com_t *)p->p_next;
    }
    return NULL;
}

static comm_ctx_t g_comm_ctx = {0};

/**
 * @brief 清理COMM相关静态数据
 */
void comm_clear(void)
{
    g_comm_ctx.index = 0;
    g_comm_ctx.status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
    g_comm_ctx.sum = 0;
    g_comm_ctx.pack.cmd = 0;
    g_comm_ctx.pack.len = 0;
    g_comm_ctx.pack.p_data = NULL;
    g_comm_ctx.pack.sum = 0;
    g_comm_ctx.func = NULL;
    g_comm_ctx.len = 0; // 重置长度
    g_comm_ctx.pack.sop = 0;
    g_comm_ctx.pack.eop = 0;
}

void comm_run(uint8_t data, DEC_MY_PRINTF)
{
    switch (g_comm_ctx.status)
    {
    case SECTION_PACKFORM_STA_SOP: // 等待开始字符
        if (data == 0xE8)          // 假设'C'为开始字符
        {
            g_comm_ctx.status = 1;                           // 切换到接收状态
            g_comm_ctx.sum = 0;                              // 重置校验和
            g_comm_ctx.sum += data;                          // 累加校验和
            g_comm_ctx.pack.sop = data;                      // 记录开始字符
            g_comm_ctx.pack.p_data = g_comm_ctx.data_buffer; // 指向数据缓
        }
        else
        {
            return; // 非开始字符
        }
        break;
    case SECTION_PACKFORM_STA_CMD: // 接收数据状态
        g_comm_ctx.func = find_comm_func(data);
        if (g_comm_ctx.func == NULL)
        {
            g_comm_ctx.status = SECTION_PACKFORM_STA_SOP; // 未找到对应的处理函数，重置状态
            return;                                       // 未找到对应的处理函数，直接返回
        }
        else
        {
            g_comm_ctx.pack.cmd = data;                   // 记录命令字
            g_comm_ctx.sum += data;                       // 累加校验和
            g_comm_ctx.pack.len = 0;                      // 重置长度
            g_comm_ctx.len_flag = 0;                      // 重置长度标志
            g_comm_ctx.status = SECTION_PACKFORM_STA_LEN; // 切换到长度接收状态
        }
        break;
    case SECTION_PACKFORM_STA_LEN: // 接收长度状态
        if (g_comm_ctx.len_flag == 0)
        {
            g_comm_ctx.pack.len += data;
            g_comm_ctx.len_flag = 1;
            g_comm_ctx.sum += data; // 累加校验和
        }
        else
        {
            g_comm_ctx.pack.len += (data << 8);              // 累加长度
            g_comm_ctx.len = g_comm_ctx.pack.len;            // 设置数据长度
            g_comm_ctx.len_flag = 0;                         // 重置长度标志
            g_comm_ctx.status = SECTION_PACKFORM_STA_DATA;   // 切换到数据接收状态
            g_comm_ctx.index = 0;                            // 重置索引
            g_comm_ctx.sum += data;                          // 累加校验和
            g_comm_ctx.pack.p_data = g_comm_ctx.data_buffer; // 指向数据缓冲区
            // 检查长度是否超出缓冲区
            if (g_comm_ctx.len > sizeof(g_comm_ctx.data_buffer))
            {
                g_comm_ctx.status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
                return;
            }
        }
        break;
    case SECTION_PACKFORM_STA_DATA: // 接收数据状态
        if (g_comm_ctx.len != 0)
        {
            g_comm_ctx.len--;
            g_comm_ctx.data_buffer[g_comm_ctx.index++] = data; // 存储数据
            g_comm_ctx.sum += data;                            // 累加校验和
        }
        else
        {
            g_comm_ctx.pack.sum = 0;                      // 接收到数据长度为0，准备接收CRC校验和
            g_comm_ctx.pack.sum += data;                  // 累加CRC校验和
            g_comm_ctx.status = SECTION_PACKFORM_STA_CRC; // 切换到CRC接收状态
        }
        break;
    case SECTION_PACKFORM_STA_CRC:                 // 接收CRC校验和状态
        g_comm_ctx.pack.sum += (data << 8);        // 累加CRC校验和
        if (g_comm_ctx.sum == g_comm_ctx.pack.sum) // 校验和匹配
        {
            g_comm_ctx.status = SECTION_PACKFORM_STA_EOP; // 切换到结束状态
            g_comm_ctx.eop_flag = 0;
            g_comm_ctx.pack.eop = 0; // 记录结束字符
        }
        else
        {
            g_comm_ctx.status = SECTION_PACKFORM_STA_SOP; // 校验和不匹配，重置状态为等待开始字符
        }
        break;
    case SECTION_PACKFORM_STA_EOP:
        if (g_comm_ctx.eop_flag == 0)
        {
            g_comm_ctx.pack.eop += data; // 记录结束字符
            g_comm_ctx.eop_flag = 1;     // 设置结束标志
        }
        else
        {
            g_comm_ctx.eop_flag = 0;            // 设置结束标志
            g_comm_ctx.pack.eop += (data << 8); // 记录结束字符
            if (g_comm_ctx.pack.eop == 0x0A0D)
            {
                // 调用处理函数
                if (g_comm_ctx.func)
                {
                    g_comm_ctx.func(&g_comm_ctx.pack, my_printf);
                }
            }
            g_comm_ctx.status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
        }
        break;
    }
}

void comm_send_data(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (!p_pack)
        return;

    p_pack->sum = 0;

    // 发送开始字符
    my_printf("%c", 0xE8);
    p_pack->sum += 0xE8;

    // 发送命令字
    my_printf("%c", p_pack->cmd);
    p_pack->sum += p_pack->cmd;

    // 发送长度
    my_printf("%c%c", (uint8_t)(p_pack->len & 0xFF), (uint8_t)((p_pack->len >> 8) & 0xFF));
    p_pack->sum += p_pack->len & 0xFF;
    p_pack->sum += (uint8_t)((p_pack->len >> 8) & 0xFF);

    // 发送数据
    if (p_pack->p_data)
    {
        for (uint32_t i = 0; i < p_pack->len; i++)
        {
            my_printf("%c", p_pack->p_data[i]);
            p_pack->sum += p_pack->p_data[i];
        }
    }

    // 发送CRC校验和
    uint16_t crc = p_pack->sum;
    my_printf("%c%c", (uint8_t)(crc & 0xFF), (uint8_t)((crc >> 8) & 0xFF));

    // 发送结束字符
    p_pack->eop = 0x0A0D;
    my_printf("%c%c", p_pack->eop & 0xFF, (p_pack->eop >> 8) & 0xFF);
}
