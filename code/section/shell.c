/**
 * @file shell.c
 * @brief Shell 命令系统（协议/具体实现层）
 */

#include "section.h"
#include "platform.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"

// 使用方案A：直接 extern 全局链表头
extern section_shell_t *p_shell_first;

// ------------------------ 原 section.c 的 shell 相关静态变量/工具函数迁入 ------------------------
extern uint32_t shell_data_num;

// 你原来的：is_string_number / eval_expr / eval_expr_inner / parse_integer
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
static float eval_expr_inner(const char **p);
static float eval_expr(const char *expr) { return eval_expr_inner(&expr); }
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

// ------------------------ 你原来的 shell_run 迁入 ------------------------
void shell_run(uint8_t data, DEC_MY_PRINTF, void *p_ctx)
{
    shell_ctx_t *p_shell_ctx = (shell_ctx_t *)p_ctx;

    // 溢出保护
    if (p_shell_ctx->shell_index >= sizeof(p_shell_ctx->shell_buffer) - 1)
    {
        p_shell_ctx->shell_index = 0;
    }
    p_shell_ctx->shell_buffer[p_shell_ctx->shell_index++] = data;

    // 检测命令结束（兼容 '\n' 或 "\r\n"）
    if ((data == '\n' && p_shell_ctx->shell_index > 1 && p_shell_ctx->shell_buffer[p_shell_ctx->shell_index - 2] == '\r') ||
        (data == '\n' && p_shell_ctx->shell_index > 0))
    {
        // 计算有效命令长度
        uint8_t end = p_shell_ctx->shell_index;
        if (p_shell_ctx->shell_index > 1 && p_shell_ctx->shell_buffer[p_shell_ctx->shell_index - 2] == '\r')
            end = p_shell_ctx->shell_index - 2;
        else
            end = p_shell_ctx->shell_index - 1;
        p_shell_ctx->shell_buffer[end] = '\0';

        // 处理内置命令
        if (strcmp((char *)p_shell_ctx->shell_buffer, "time") == 0)
        {
            // 显示系统时间
            my_printf->my_printf("time = %us.%03ums\r\n",
                                 SECTION_SYS_TICK / 10000,
                                 SECTION_SYS_TICK % 10000 / 10);
            goto shell_done;
        }
        else if (strcmp((char *)p_shell_ctx->shell_buffer, "reset") == 0)
        {
            // 系统复位
            SYSTEM_RESET;
            goto shell_done;
        }
        else if (strcmp((char *)p_shell_ctx->shell_buffer, "help") == 0)
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
            if (strncmp((char *)p_shell_ctx->shell_buffer, p->p_name, p->p_name_size) == 0 &&
                (p_shell_ctx->shell_buffer[p->p_name_size] == ':' || p_shell_ctx->shell_buffer[p->p_name_size] == '\0'))
            {
                char *param = NULL;
                if (p_shell_ctx->shell_buffer[p->p_name_size] == ':')
                {
                    // 提取参数
                    param = (char *)&p_shell_ctx->shell_buffer[p->p_name_size + 1];
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
        memset(p_shell_ctx->shell_buffer, 0, sizeof(p_shell_ctx->shell_buffer));
        p_shell_ctx->shell_index = 0;
    }
}

// ------------------------ shell 状态轮询（你原来的迁入） ------------------------
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

// 原来的定时注册也搬到这里
REG_TASK_MS(1000, shell_status_run)

// ------------------------ list 分时打印（你原来的迁入） ------------------------
typedef struct
{
    section_shell_t *cur;
    DEC_MY_PRINTF;
    uint8_t active;
    int max_name_len;
    int tab_count;
} list_print_ctx_t;

static list_print_ctx_t g_list_print_ctx = {0};

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

static void list_print_task(void)
{
    if (g_list_print_ctx.active)
    {
        list_print_step();
    }
}

REG_TASK_MS(10, list_print_task)

// ------------------------ （可选）CPU利用率命令如果你觉得属于shell，就也搬进来 ------------------------
extern float task_metric, task_metric_max; // 如果变量在别处定义
static void CPU_Utilization(DEC_MY_PRINTF)
{
    my_printf->my_printf("CPU利用率:%f%%,CPU峰值:%f%%\n", task_metric, task_metric_max);
}
REG_SHELL_CMD(CPU_Utilization, CPU_Utilization)
REG_SHELL_VAR(TASK_METRIC, task_metric, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TASK_METRIC_MAX, task_metric_max, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)

#pragma GCC diagnostic pop
