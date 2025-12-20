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
#include "my_math.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"

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
#define SYSTEM_RESET NVIC_SystemReset()          ///< 系统复位
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#endif

// 全局链表头指针
reg_task_t *p_task_first = NULL;                   ///< 任务链表头指针
reg_interrupt_t *p_interrupt_first = NULL;         ///< 中断链表头指针
section_shell_t *p_shell_first = NULL;             ///< Shell命令链表头指针
section_link_t *p_link_first = NULL;               ///< 链路链表头指针
section_perf_record_t *p_perf_record_first = NULL; ///< 性能计数器链表头指针
uint32_t *p_perf_cnt = NULL;                       /// 性能计数器一级指针
section_com_t *p_com_first = NULL;                 ///< COMM链表头指针
reg_init_t *p_init_first = NULL;                   ///< INIT链表头指针
comm_route_t *p_comm_route_first = NULL;           /// 路由链表

static uint32_t shell_data_num = 0;

// 预计算的CRC查找表
static uint16_t crc16_table[256];

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
    else if (is_string_number(param))
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
void shell_run(uint8_t data, DEC_MY_PRINTF, void *p)
{
    comm_ctx_t *p_comm_ctx = (comm_ctx_t *)p;

    // 溢出保护
    if (p_comm_ctx->shell_ctx.shell_index >= sizeof(p_comm_ctx->shell_ctx.shell_buffer) - 1)
    {
        p_comm_ctx->shell_ctx.shell_index = 0;
    }
    p_comm_ctx->shell_ctx.shell_buffer[p_comm_ctx->shell_ctx.shell_index++] = data;

    // 检测命令结束（兼容 '\n' 或 "\r\n"）
    if ((data == '\n' && p_comm_ctx->shell_ctx.shell_index > 1 && p_comm_ctx->shell_ctx.shell_buffer[p_comm_ctx->shell_ctx.shell_index - 2] == '\r') ||
        (data == '\n' && p_comm_ctx->shell_ctx.shell_index > 0))
    {
        // 计算有效命令长度
        uint8_t end = p_comm_ctx->shell_ctx.shell_index;
        if (p_comm_ctx->shell_ctx.shell_index > 1 && p_comm_ctx->shell_ctx.shell_buffer[p_comm_ctx->shell_ctx.shell_index - 2] == '\r')
            end = p_comm_ctx->shell_ctx.shell_index - 2;
        else
            end = p_comm_ctx->shell_ctx.shell_index - 1;
        p_comm_ctx->shell_ctx.shell_buffer[end] = '\0';

        // 处理内置命令
        if (strcmp((char *)p_comm_ctx->shell_ctx.shell_buffer, "time") == 0)
        {
            // 显示系统时间
            my_printf->my_printf("time = %us.%03ums\r\n",
                                 SECTION_SYS_TICK / 10000,
                                 SECTION_SYS_TICK % 10000 / 10);
            goto shell_done;
        }
        else if (strcmp((char *)p_comm_ctx->shell_ctx.shell_buffer, "reset") == 0)
        {
            // 系统复位
            SYSTEM_RESET;
            goto shell_done;
        }
        else if (strcmp((char *)p_comm_ctx->shell_ctx.shell_buffer, "help") == 0)
        {
            // 显示帮助信息
            section_shell_t *s = p_shell_first;
            while (s)
            {
                my_printf->my_printf("%s\t%s\r\n", s->p_name, s->type == SHELL_CMD ? "CMD" : "VAR");
                s = s->p_next;
            }
            goto shell_done;
        }

        // 查找匹配的Shell命令或变量
        section_shell_t *p = p_shell_first;
        while (p)
        {
            if (strncmp((char *)p_comm_ctx->shell_ctx.shell_buffer, p->p_name, p->p_name_size) == 0 &&
                (p_comm_ctx->shell_ctx.shell_buffer[p->p_name_size] == ':' || p_comm_ctx->shell_ctx.shell_buffer[p->p_name_size] == '\0'))
            {
                char *param = NULL;
                if (p_comm_ctx->shell_ctx.shell_buffer[p->p_name_size] == ':')
                {
                    // 提取参数
                    param = (char *)&p_comm_ctx->shell_ctx.shell_buffer[p->p_name_size + 1];
                    while (*param == ' ')
                        param++;
                }

                int32_t intval = 0;

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
                    {
                        val = (uint8_t)intval;
                        SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
                    }
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf->my_printf("%s = %u\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_INT8:
                {
                    int8_t val = *(int8_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                    {
                        val = (int8_t)intval;
                        SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
                    }
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf->my_printf("%s = %d\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_UINT16:
                {
                    uint16_t val = *(uint16_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                    {
                        val = (uint16_t)intval;
                        SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
                    }
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf->my_printf("%s = %u\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_INT16:
                {
                    int16_t val = *(int16_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                    {
                        val = (int16_t)intval;
                        SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
                    }
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf->my_printf("%s = %d\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_UINT32:
                {
                    uint32_t val = *(uint32_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                    {
                        val = (uint32_t)intval;
                        SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
                    }
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf->my_printf("%s = %lu\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_INT32:
                {
                    int32_t val = *(int32_t *)p->p_var;
                    if (param && parse_integer(param, &intval))
                    {
                        val = (int32_t)intval;
                        SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
                    }
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf->my_printf("%s = %ld\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                case SHELL_FP32:
                {
                    float val = *(float *)p->p_var;
                    if (param && is_string_number(param))
                    {
                        val = eval_expr(param);
                        SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
                    }
                    memcpy(p->p_var, &val, sizeof(val));
                    my_printf->my_printf("%s = %f\r\n", p->p_name, val);
                    if (p->func)
                        p->func(my_printf);
                    break;
                }
                }

                int status_set = -1; // -1:未指定, 0:仅-s, 1:有-s 1
                if (param)
                {
                    // 查找-s
                    char *ps = strstr(param, "-s");
                    if (ps)
                    {
                        // 判断-s后是否有数字
                        char *ps_num = ps + 2;
                        while (*ps_num == ' ')
                            ps_num++;
                        if (isdigit((unsigned char)*ps_num))
                        {
                            status_set = atoi(ps_num);
                        }
                        else
                        {
                            status_set = 0;
                        }
                    }
                    if (status_set == -1)
                    {
                        // 仅-s，不改变status
                    }
                    else
                    {
                        p->status = status_set;
                        p->my_printf = my_printf; // 更新打印函数指针
                    }
                }

                goto shell_done;
            }
            p = p->p_next;
        }

    shell_done:
        // 清空缓冲区
        memset(p_comm_ctx->shell_ctx.shell_buffer, 0, sizeof(p_comm_ctx->shell_ctx.shell_buffer));
        p_comm_ctx->shell_ctx.shell_index = 0;
    }
}

void shell_status_run(void)
{
    // 遍历所有Shell命令，打印状态
    section_shell_t *p = p_shell_first;
    while (p)
    {
        if (p->status)
        {
            if (p->status & (1 << 0))
            {
                switch (p->type)
                {
                case SHELL_CMD:
                    // 执行命令
                    break;
                case SHELL_UINT8:
                {
                    uint8_t val = *(uint8_t *)p->p_var;
                    p->my_printf->my_printf("%s = %u\r\n", p->p_name, val);
                    break;
                }
                case SHELL_INT8:
                {
                    int8_t val = *(int8_t *)p->p_var;
                    p->my_printf->my_printf("%s = %d\r\n", p->p_name, val);
                    break;
                }
                case SHELL_UINT16:
                {
                    uint16_t val = *(uint16_t *)p->p_var;
                    p->my_printf->my_printf("%s = %u\r\n", p->p_name, val);
                    break;
                }
                case SHELL_INT16:
                {
                    int16_t val = *(int16_t *)p->p_var;
                    p->my_printf->my_printf("%s = %d\r\n", p->p_name, val);
                    break;
                }
                case SHELL_UINT32:
                {
                    uint32_t val = *(uint32_t *)p->p_var;
                    p->my_printf->my_printf("%s = %lu\r\n", p->p_name, val);
                    break;
                }
                case SHELL_INT32:
                {
                    int32_t val = *(int32_t *)p->p_var;
                    p->my_printf->my_printf("%s = %ld\r\n", p->p_name, val);
                    break;
                }
                case SHELL_FP32:
                {
                    float val = *(float *)p->p_var;
                    p->my_printf->my_printf("%s = %f\r\n", p->p_name, val);
                    break;
                }
                }
            }
            if (p->status & (1 << 1))
            {
                // 执行状态更新函数
                if (p->func)
                    p->func(p->my_printf);
            }
        }
        p = p->p_next;
    }
}

REG_TASK_MS(1000, shell_status_run) // 每200ms检查一次Shell状态

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
    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        if (p == shell)
            return;
    }

    shell->p_next = NULL;
    if (p_shell_first)
    {
        shell->p_next = p_shell_first;
        p_shell_first = shell;
        shell_data_num++;
    }
    else
    {
        p_shell_first = shell;
        shell_data_num++;
    }
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

static void comm_route_insert(comm_route_t *com)
{
    if (!com)
        return;

    com->p_next = NULL;
    if (!p_comm_route_first)
    {
        p_comm_route_first = com;
    }
    else
    {
        comm_route_t *curr = p_comm_route_first;
        while (curr->p_next)
            curr = curr->p_next;
        curr->p_next = com;
    }
}

static void init_insert(reg_init_t *init)
{
    if (!init)
        return;

    init->p_next = NULL;

    // 插入到链表头或按优先级顺序插入
    if (!p_init_first || init->priority < p_init_first->priority)
    {
        init->p_next = p_init_first;
        p_init_first = init;
    }
    else
    {
        reg_init_t *prev = p_init_first;
        while (prev->p_next && prev->p_next->priority <= init->priority)
        {
            prev = prev->p_next;
        }
        init->p_next = prev->p_next;
        prev->p_next = init;
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
            init_insert((reg_init_t *)p->p_str);
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
        case SECTION_COMM_ROUTE:
            // 生成路由链表
            comm_route_insert((comm_route_t *)p->p_str);
            break;
        }
    }
    // 执行所有初始化函数
    for (reg_init_t *init = p_init_first; init != NULL; init = (reg_init_t *)init->p_next)
    {
        if (init->p_func)
            init->p_func();
    }
}

static float task_run_time = 0.0f;

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
                task_run_time += t->p_perf_record->time;
            }
            if (likely((now - t->time_last) < (t->t_period << 2)))
            {
                t->time_last += t->t_period; // 更新最后执行时间
            }
            else
            {
                t->time_last = now; // 要是上次时间与当前时间间隔太大直接更新为当前时间
            }
        }
    }
}

float task_metric = 0.0f;
float task_metric_max = 0.0f;

REG_SHELL_VAR(TASK_METRIC, task_metric, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TASK_METRIC_MAX, task_metric_max, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)

static void task_metric_calculate(void)
{
    task_metric = task_run_time * 500e-6f;
    if (task_metric > task_metric_max)
    {
        task_metric_max = task_metric;
    }
    task_run_time = 0.0f;
}

REG_TASK_MS(100, task_metric_calculate)

static void CPU_Utilization(DEC_MY_PRINTF)
{
    my_printf->my_printf("CPU利用率:%f%%,CPU峰值:%f%%\n", task_metric, task_metric_max);
}

REG_SHELL_CMD(CPU_Utilization, CPU_Utilization)

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
                PLECS_LOG("%s\n", entry->p_name);
                entry->func_in();
            }

            // 执行状态处理函数
            entry->func_exe();

            // 检查是否有事件需要处理
            if (*fsm->p_fsm_ev)
            {
                uint32_t next = entry->func_chk(*fsm->p_fsm_ev);

                // 状态切换
                if (next && next != entry->fsm_sta)
                {
                    PLECS_LOG("%s-chk_ev:%d\n", entry->p_name, *fsm->p_fsm_ev);
                    entry->func_out();          // 执行出口函数
                    fsm->fsm_sta = next;        // 切换状态
                    fsm->fsm_sta_is_change = 1; // 标记状态已改变
                }
                *fsm->p_fsm_ev = 0;
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
    if (!link || !link->dma_cnt || !link->func_arr)
        return;

    uint32_t buff_size = link->buff_size;
    uint32_t dma_cnt = *link->dma_cnt;
    uint32_t cnt = buff_size - dma_cnt;

    // 处理缓冲区中的新数据
    while (link->pos != cnt)
    {
        uint8_t data = link->rx_buff[link->pos];

        // 调用所有注册的处理函数
        for (uint32_t i = 0; i < link->func_num; ++i)
        {
            if (link->func_arr[i])
            {
                link->p_comm_ctx->link_id = link->link_id;
                link->func_arr[i](data, link->my_printf, link->p_comm_ctx);
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

// 注册链路处理任务，每1ms执行一次
REG_TASK(10, section_link_task)

/**
 * @brief list分时打印上下文
 */
typedef struct
{
    section_shell_t *cur;
    DEC_MY_PRINTF;
    uint8_t active;
    int max_name_len; // 记录最长变量名长度
    int tab_count;
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
    my_printf->my_printf("\r\n==================== SHELL COMMANDS AND VARIABLES ====================\r\n");
}

REG_SHELL_CMD(list, list_print_start)

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

        // g_list_print_ctx.my_printf->my_printf("%s", s->p_name);
        // for (int i = 0; i < g_list_print_ctx.tab_count; i++)
        //     g_list_print_ctx.my_printf->my_printf("\t");

        switch (s->type)
        {
        case SHELL_CMD:
            g_list_print_ctx.my_printf->my_printf("%s\tCMD\r\n", s->p_name);
            break;
        case SHELL_UINT8:
            g_list_print_ctx.my_printf->my_printf("%s\tU8\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  *(uint8_t *)s->p_max,
                                                  *(uint8_t *)s->p_min,
                                                  *(uint8_t *)s->p_var);
            break;
        case SHELL_UINT16:
            g_list_print_ctx.my_printf->my_printf("%s\tU16\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  *(uint16_t *)s->p_max,
                                                  *(uint16_t *)s->p_min,
                                                  *(uint16_t *)s->p_var);
            break;
        case SHELL_UINT32:
            g_list_print_ctx.my_printf->my_printf("%s\tU32\t(%u)\t(%u)\t%u\r\n",
                                                  s->p_name,
                                                  *(uint32_t *)s->p_max,
                                                  *(uint32_t *)s->p_min,
                                                  *(uint32_t *)s->p_var);
            break;
        case SHELL_INT8:
            g_list_print_ctx.my_printf->my_printf("%s\tI8\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  *(int8_t *)s->p_max,
                                                  *(int8_t *)s->p_min,
                                                  *(int8_t *)s->p_var);
            break;
        case SHELL_INT16:
            g_list_print_ctx.my_printf->my_printf("%s\tI16\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  *(int16_t *)s->p_max,
                                                  *(int16_t *)s->p_min,
                                                  *(int16_t *)s->p_var);
            break;
        case SHELL_INT32:
            g_list_print_ctx.my_printf->my_printf("%s\tI32\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  *(int32_t *)s->p_max,
                                                  *(int32_t *)s->p_min,
                                                  *(int32_t *)s->p_var);
            break;
        case SHELL_FP32:
            g_list_print_ctx.my_printf->my_printf("%s\tFP32\t(%f)\t(%f)\t%f\r\n",
                                                  s->p_name,
                                                  *(float *)s->p_max,
                                                  *(float *)s->p_min,
                                                  *(float *)s->p_var);
            break;
        }
        g_list_print_ctx.cur = s->p_next;
        print_flag = 1;
        return 1;
    }
    // 打印分隔线
    else
    {
        g_list_print_ctx.my_printf->my_printf("-----------------------------------------\r\n");
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
REG_TASK_MS(10, list_print_task)

/**
 * @brief 打印所有性能统计结果
 * @param my_printf 打印函数指针
 */
void print_perf_record(DEC_MY_PRINTF)
{
    if (!my_printf)
        return;
    section_perf_record_t *p = p_perf_record_first;
    my_printf->my_printf("Perf Name\tTime(us)\tMax(us)\r\n");
    while (p)
    {
        my_printf->my_printf("%s\t%u\t%u\r\n", p->p_name, (unsigned)(p->time * 0.5f), (unsigned)(p->max_time * 0.5f));
        p = (section_perf_record_t *)p->p_next;
    }
}

typedef struct
{
    section_perf_record_t *cur;
    DEC_MY_PRINTF;
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
    my_printf->my_printf("Perf Name");
    for (int i = 0; i < tab_count; i++)
        my_printf->my_printf("\t");
    my_printf->my_printf("Time\tMax\r\n");
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
    g_perf_print_ctx.my_printf->my_printf("%s", p->p_name);
    for (int i = 0; i < tab_count; i++)
        g_perf_print_ctx.my_printf->my_printf("\t");
    g_perf_print_ctx.my_printf->my_printf("%u\t%u\r\n", (unsigned)(p->time * 0.5f), (unsigned)(p->max_time * 0.5f));
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
REG_TASK_MS(5, perf_print_task)

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
    my_printf->my_printf("Performance max time cleared.\r\n");
}

REG_SHELL_CMD(perf_clear_max_time, perf_clear_max_time)

/**
 * @brief 根据cmd查找COMM链表，返回对应的处理函数指针
 * @param cmd 命令字
 * @return 对应的处理函数指针，未找到返回NULL
 */
void (*find_comm_func(uint8_t cmd_set, uint8_t cmd_word))(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_com_t *p = p_com_first;
    section_com_t *p_last = NULL;
    while (p)
    {
        if ((p->cmd_set == cmd_set) &&
            (p->cmd_word == cmd_word))
        {
            if (p_last)
            {
                p_last->p_next = p->p_next; // 断开前一个节点的链接
                p->p_next = p_com_first;    // 将当前节点移到链表头部
                p_com_first = p;            // 更新链表头指针
            }
            return p->func;
        }
        p_last = p;
        p = (section_com_t *)p->p_next;
    }
    return NULL;
}

// 初始化CRC表（系统启动时执行一次）
static void crc16_init_table(void)
{
    for (int i = 0; i < 256; i++)
    {
        uint16_t crc = 0;
        uint16_t c = i << 8;

        for (int j = 0; j < 8; j++)
        {
            if ((crc ^ c) & 0x8000)
            {
                crc = (crc << 1) ^ CRC16_CCITT_POLY;
            }
            else
            {
                crc = crc << 1;
            }
            c = c << 1;
        }

        crc16_table[i] = crc;
    }
}

REG_INIT(0, crc16_init_table)

// CRC初始化 - 开始新的CRC计算
uint16_t crc16_init(void)
{
    return CRC16_CCITT_INIT;
}

// 更新CRC值 - 每次接收一个字节时调用
uint16_t crc16_update(uint16_t crc, uint8_t data)
{
    uint8_t table_index = (crc >> 8) ^ data;
    return (crc << 8) ^ crc16_table[table_index];
}

// 获取最终CRC结果（可选，有些标准需要异或输出）
uint16_t crc16_final(uint16_t crc)
{
    // return crc ^ 0x0000;  // 如果需要异或输出，可以在这里处理
    return crc;
}

uint16_t section_crc16(uint8_t *p_data, uint32_t len)
{
    uint16_t crc = crc16_init();
    for (uint32_t i = 0; i < len; i++)
    {
        crc = crc16_update(crc, *(p_data + i));
    }
    return crc16_final(crc);
}

uint16_t section_crc16_with_crc(uint8_t *p_data, uint32_t len, uint16_t crc_in)
{
    uint16_t crc = crc_in;
    for (uint32_t i = 0; i < len; i++)
    {
        crc = crc16_update(crc, *(p_data + i));
    }
    return crc;
}

void comm_route_run(comm_ctx_t *p_comm_ctx)
{
    comm_route_t *p_comm_route = p_comm_route_first;

    while (p_comm_route)
    {
        if ((p_comm_ctx->link_id == p_comm_route->src_link_id) &&
            (p_comm_ctx->pack.dst == p_comm_route->dst_addr))
        {
            section_link_t *p_link = p_link_first;
            while (p_link)
            {
                if (p_comm_route->dst_link_id == p_link->link_id)
                {
                    comm_send_data(&p_comm_ctx->pack, p_link->my_printf);
                }
                p_link = p_link->p_next;
            }
        }
        p_comm_route = p_comm_route->p_next;
    }
}

void comm_run(uint8_t data, DEC_MY_PRINTF, void *p)
{
    comm_ctx_t *p_comm_ctx = (comm_ctx_t *)p;
    switch (p_comm_ctx->status)
    {
    case SECTION_PACKFORM_STA_SOP: // 等待开始字符
        if (data == 0xE8)          // 假设'C'为开始字符
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_VER;                  // 切换到接收状态
            p_comm_ctx->crc = crc16_init();                                 // 重置CRC
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);          // 累加CRC
            p_comm_ctx->pack.sop = data;                                    // 记录开始字符
            p_comm_ctx->pack.p_data = (uint8_t *)p_comm_ctx->p_data_buffer; // 指向数据缓
            p_comm_ctx->is_route = 0;                                       // 清除路由标志
        }
        else
        {
            return; // 非开始字符
        }
        break;
    case SECTION_PACKFORM_STA_VER:
        p_comm_ctx->pack.version = data;
        p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
        if (p_comm_ctx->pack.version == 0x01)
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_SRC;
            p_comm_ctx->src_flag = 0;
        }
        else
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 未找到对应的处理函数，重置状态
            return;                                        // 未找到对应的处理函数，直接返回
        }
        break;
    case SECTION_PACKFORM_STA_SRC:
        if (p_comm_ctx->src_flag == 0)
        {
            p_comm_ctx->pack.src = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
            p_comm_ctx->src_flag = 1;
        }
        else
        {
            p_comm_ctx->pack.d_src = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
            p_comm_ctx->dst_flag = 0;
            p_comm_ctx->status = SECTION_PACKFORM_STA_DST;
        }
        break;
    case SECTION_PACKFORM_STA_DST:
        if (p_comm_ctx->dst_flag == 0)
        {
            p_comm_ctx->pack.dst = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
            if ((p_comm_ctx->pack.dst == 0x00) ||
                (p_comm_ctx->pack.dst == p_comm_ctx->src))
            {
                p_comm_ctx->dst_flag = 1;
            }
            else
            {
                p_comm_ctx->is_route = 1;
                p_comm_ctx->dst_flag = 1;
            }
        }
        else
        {
            p_comm_ctx->pack.d_dst = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
            if ((p_comm_ctx->pack.d_dst == 0x00) ||
                (p_comm_ctx->pack.d_dst == p_comm_ctx->d_src))
            {
                p_comm_ctx->cmd_flag = 0;
                p_comm_ctx->status = SECTION_PACKFORM_STA_CMD;
            }
            else if (p_comm_ctx->is_route == 1)
            {
                p_comm_ctx->cmd_flag = 0;
                p_comm_ctx->status = SECTION_PACKFORM_STA_CMD;
            }
            else
            {
                p_comm_ctx->status = SECTION_PACKFORM_STA_SOP;
                return;
            }
        }
        break;
    case SECTION_PACKFORM_STA_CMD: // 接收数据状态
        if (p_comm_ctx->cmd_flag == 0)
        {
            p_comm_ctx->pack.cmd_set = data;                       // 记录命令字
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // 累加CRC
            p_comm_ctx->cmd_flag = 1;
        }
        else
        {
            p_comm_ctx->pack.cmd_word = data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // 累加CRC
            if (p_comm_ctx->is_route == 0)
            {
                p_comm_ctx->func = find_comm_func(p_comm_ctx->pack.cmd_set,
                                                  p_comm_ctx->pack.cmd_word);
                if (p_comm_ctx->func == NULL)
                {
                    p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 未找到对应的处理函数，重置状态
                    return;                                        // 未找到对应的处理函数，直接返回
                }
                else
                {
                    p_comm_ctx->status = SECTION_PACKFORM_STA_ACK; // 切换到长度接收状态
                }
            }
            else
            {
                p_comm_ctx->status = SECTION_PACKFORM_STA_ACK; // 切换到长度接收状态
            }
        }
        break;
    case SECTION_PACKFORM_STA_ACK:
        p_comm_ctx->pack.is_ack = data;
        p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);
        p_comm_ctx->pack.len = 0;                      // 重置长度
        p_comm_ctx->len_flag = 0;                      // 重置长度标志
        p_comm_ctx->status = SECTION_PACKFORM_STA_LEN; // 切换到长度接收状态
        break;
    case SECTION_PACKFORM_STA_LEN: // 接收长度状态
        if (p_comm_ctx->len_flag == 0)
        {
            p_comm_ctx->pack.len += data;
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // 累加CRC
            p_comm_ctx->len_flag = 1;
        }
        else
        {
            p_comm_ctx->pack.len += (data << 8);                            // 累加长度
            p_comm_ctx->len = p_comm_ctx->pack.len;                         // 设置数据长度
            p_comm_ctx->len_flag = 0;                                       // 重置长度标志
            p_comm_ctx->status = SECTION_PACKFORM_STA_DATA;                 // 切换到数据接收状态
            p_comm_ctx->index = 0;                                          // 重置索引
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data);          // 累加CRC
            p_comm_ctx->pack.p_data = (uint8_t *)p_comm_ctx->p_data_buffer; // 指向数据缓冲区
            // 检查长度是否超出缓冲区
            if (p_comm_ctx->len > p_comm_ctx->buffer_size)
            {
                p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
                return;
            }
        }
        break;
    case SECTION_PACKFORM_STA_DATA: // 接收数据状态
        if (p_comm_ctx->len != 0)
        {
            p_comm_ctx->len--;
            uint8_t *p_data = (uint8_t *)*p_comm_ctx->p_data_buffer + p_comm_ctx->index;
            *p_data = data;                                        // 存储数据
            p_comm_ctx->crc = crc16_update(p_comm_ctx->crc, data); // CRC
            p_comm_ctx->index++;
        }
        else
        {
            p_comm_ctx->pack.crc = 0;                      // 接收到数据长度为0，准备接收CRC
            p_comm_ctx->pack.crc += data;                  // 接收CRC
            p_comm_ctx->status = SECTION_PACKFORM_STA_CRC; // 切换到CRC接收状态
        }
        break;
    case SECTION_PACKFORM_STA_CRC:           // 接收CRC状态
        p_comm_ctx->pack.crc += (data << 8); // 累加CRC
        p_comm_ctx->crc = crc16_final(p_comm_ctx->crc);
        if (p_comm_ctx->crc == p_comm_ctx->pack.crc) // 匹配
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_EOP; // 切换到结束状态
            p_comm_ctx->eop_flag = 0;
            p_comm_ctx->pack.eop = 0; // 记录结束字符
        }
        else
        {
            p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 不匹配，重置状态为等待开始字符
            return;
        }
        break;
    case SECTION_PACKFORM_STA_EOP:
        if (p_comm_ctx->eop_flag == 0)
        {
            p_comm_ctx->pack.eop += data; // 记录结束字符
            p_comm_ctx->eop_flag = 1;     // 设置结束标志
        }
        else if (p_comm_ctx->is_route == 1)
        {
            comm_route_run(p_comm_ctx);
            p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
        }
        else
        {
            p_comm_ctx->eop_flag = 0;            // 设置结束标志
            p_comm_ctx->pack.eop += (data << 8); // 记录结束字符
            if (p_comm_ctx->pack.eop == 0x0A0D)
            {
                // 调用处理函数
                if (p_comm_ctx->func)
                {
                    p_comm_ctx->func(&p_comm_ctx->pack, my_printf);
                }
            }
            p_comm_ctx->status = SECTION_PACKFORM_STA_SOP; // 重置状态为等待开始字符
        }
        break;
    case SECTION_PACKFORM_STA_ROUTE:
        break;
    }
}

static uint8_t tx_buffer[512] = {0};
static int tx_buffer_index = 0;

static void write_tx_buffer(uint8_t data)
{
    if (tx_buffer_index >= 512)
    {
        return;
    }
    tx_buffer[tx_buffer_index] = data;
    tx_buffer_index++;
}

void comm_send_data(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (!p_pack)
        return;

    tx_buffer_index = 0;

    p_pack->version = 0x01;
    p_pack->crc = crc16_init();

    // 发送开始字符
    write_tx_buffer(0xE8);
    p_pack->crc = crc16_update(p_pack->crc, 0xE8);

    // 发送版本
    write_tx_buffer(p_pack->version);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->version);

    // 发送源地址
    write_tx_buffer(p_pack->src);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->src);

    // 发送动态源地址
    write_tx_buffer(p_pack->d_src);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->d_src);

    // 发送目的地址
    write_tx_buffer(p_pack->dst);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->dst);

    // 发送动态目的地址
    write_tx_buffer(p_pack->d_dst);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->d_dst);

    // 发送命令集
    write_tx_buffer(p_pack->cmd_set);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->cmd_set);

    // 发送命令字
    write_tx_buffer(p_pack->cmd_word);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->cmd_word);

    // 发送是响应
    write_tx_buffer(p_pack->is_ack);
    p_pack->crc = crc16_update(p_pack->crc, p_pack->is_ack);

    // 发送长度
    write_tx_buffer((uint8_t)(p_pack->len & 0xFF));
    write_tx_buffer((uint8_t)((p_pack->len >> 8) & 0xFF));
    p_pack->crc = crc16_update(p_pack->crc, p_pack->len & 0xFF);
    p_pack->crc = crc16_update(p_pack->crc, (uint8_t)((p_pack->len >> 8) & 0xFF));

    // 发送数据
    if (p_pack->p_data)
    {
        for (uint32_t i = 0; i < p_pack->len; i++)
        {
            write_tx_buffer(p_pack->p_data[i]);
            p_pack->crc = crc16_update(p_pack->crc, p_pack->p_data[i]);
        }
    }

    // 发送CRC
    uint16_t crc = crc16_final(p_pack->crc);

    write_tx_buffer((uint8_t)(crc & 0xFF));
    write_tx_buffer((uint8_t)((crc >> 8) & 0xFF));

    write_tx_buffer(0x0D);
    write_tx_buffer(0x0A);

    if (my_printf)
        my_printf->tx_by_dma((char *)tx_buffer, tx_buffer_index);
}
