/**
 * @file shell.c
 * @brief Shell 命令系统（协议/具体实现层）
 */

#include "shell.h"
#include "section.h"
#include "platform.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"

section_shell_t *p_shell_first;

/* ------------------------ Shell 插入 ------------------------ */
uint32_t shell_data_num = 0;

static void shell_insert(section_shell_t *shell)
{
    if (!shell)
        return;

    /* 防止重复插入（理论上不会发生，属于防御性代码） */
    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        if (p == shell)
            return;
    }

    shell->p_next = p_shell_first;
    p_shell_first = shell;
    shell_data_num++;
}

void shell_init(void)
{
    for (reg_section_t *p = (reg_section_t *)&SECTION_START;
         p < (reg_section_t *)&SECTION_STOP;
         ++p)
    {
        switch (p->section_type)
        {
        case SECTION_SHELL:
            shell_insert((section_shell_t *)p->p_str);
            break;
        default:
            break;
        }
    }
}

REG_INIT(0, shell_init)

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

/* ------------------------ 小工具：字符串修剪/选项解析 ------------------------ */
static inline char *ltrim(char *s)
{
    while (s && *s && isspace((unsigned char)*s))
        s++;
    return s;
}

static inline void rtrim_inplace(char *s)
{
    if (!s)
        return;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1]))
    {
        s[n - 1] = '\0';
        n--;
    }
}

/* 从 param 中解析可选的 "-s[ N]"，并把值表达式部分（可能含空格）截出来
 * - value_begin/value_end: 指向 param 内部（会就地写 '\0' 分割），调用前确保 param 可写
 * - status_set: -1 表示未指定；>=0 表示用户指定的状态值
 */
static void parse_param_value_and_status(char *param, char **value_str, int32_t *status_set)
{
    *value_str = NULL;
    *status_set = -1;
    if (!param)
        return;

    char *p = ltrim(param);

    /* 找到 token 边界上的 "-s" */
    char *opt = strstr(p, "-s");
    while (opt)
    {
        if (opt == p || isspace((unsigned char)opt[-1]))
            break;
        opt = strstr(opt + 2, "-s");
    }

    if (opt)
    {
        /* value 部分：p ... opt-1 */
        *opt = '\0';
        char *v = ltrim(p);
        rtrim_inplace(v);
        *value_str = (*v) ? v : NULL;

        /* status 部分：从 opt+2 开始 */
        char *ps = opt + 2;
        ps = ltrim(ps);
        if (*ps == '\0')
        {
            *status_set = 0; /* 仅 "-s"：默认关 */
        }
        else
        {
            char *endp = NULL;
            long s = strtol(ps, &endp, 10);
            if (endp != ps)
                *status_set = (int32_t)s;
            else
                *status_set = 0;
        }
        return;
    }

    /* 没有 -s：整个 param 当 value（允许空格，右侧修剪） */
    char *v = ltrim(p);
    rtrim_inplace(v);
    *value_str = (*v) ? v : NULL;
}

/* ------------------------ 小工具：按类型打印/写入 ------------------------ */
static void shell_print_item(section_shell_t *p, DEC_MY_PRINTF)
{
    if (!p || !my_printf || !my_printf->my_printf)
        return;

    switch (p->type)
    {
    case SHELL_CMD:
        my_printf->my_printf("%s\tCMD\r\n", p->p_name);
        break;
    case SHELL_UINT8:
        my_printf->my_printf("%s = %u\r\n", p->p_name, (unsigned)(*(uint8_t *)p->p_var));
        break;
    case SHELL_INT8:
        my_printf->my_printf("%s = %d\r\n", p->p_name, (int)(*(int8_t *)p->p_var));
        break;
    case SHELL_UINT16:
        my_printf->my_printf("%s = %u\r\n", p->p_name, (unsigned)(*(uint16_t *)p->p_var));
        break;
    case SHELL_INT16:
        my_printf->my_printf("%s = %d\r\n", p->p_name, (int)(*(int16_t *)p->p_var));
        break;
    case SHELL_UINT32:
        my_printf->my_printf("%s = %lu\r\n", p->p_name, (unsigned long)(*(uint32_t *)p->p_var));
        break;
    case SHELL_INT32:
        my_printf->my_printf("%s = %ld\r\n", p->p_name, (long)(*(int32_t *)p->p_var));
        break;
    case SHELL_FP32:
        my_printf->my_printf("%s = %f\r\n", p->p_name, (double)(*(float *)p->p_var));
        break;
    default:
        break;
    }
}

static void shell_write_item_if_needed(section_shell_t *p, const char *value_str, DEC_MY_PRINTF)
{
    if (!p || !value_str)
        return;

    int32_t intval = 0;

    switch (p->type)
    {
    case SHELL_CMD:
        /* CMD 不写入 */
        break;

    case SHELL_UINT8:
    {
        uint8_t val = *(uint8_t *)p->p_var;
        if (parse_integer(value_str, &intval))
        {
            val = (uint8_t)intval;
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));
        }
        break;
    }
    case SHELL_INT8:
    {
        int8_t val = *(int8_t *)p->p_var;
        if (parse_integer(value_str, &intval))
        {
            val = (int8_t)intval;
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));
        }
        break;
    }
    case SHELL_UINT16:
    {
        uint16_t val = *(uint16_t *)p->p_var;
        if (parse_integer(value_str, &intval))
        {
            val = (uint16_t)intval;
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));
        }
        break;
    }
    case SHELL_INT16:
    {
        int16_t val = *(int16_t *)p->p_var;
        if (parse_integer(value_str, &intval))
        {
            val = (int16_t)intval;
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));
        }
        break;
    }
    case SHELL_UINT32:
    {
        uint32_t val = *(uint32_t *)p->p_var;
        if (parse_integer(value_str, &intval))
        {
            val = (uint32_t)intval;
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));
        }
        break;
    }
    case SHELL_INT32:
    {
        int32_t val = *(int32_t *)p->p_var;
        if (parse_integer(value_str, &intval))
        {
            val = (int32_t)intval;
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));
        }
        break;
    }
    case SHELL_FP32:
    {
        float val = *(float *)p->p_var;
        if (is_string_number(value_str))
        {
            val = eval_expr(value_str);
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));
        }
        break;
    }
    default:
        break;
    }

    if (p->func)
        p->func(my_printf);
}

// ------------------------ 你原来的 shell_run 迁入 ------------------------
void shell_run(uint8_t data, DEC_MY_PRINTF, void *p_ctx)
{
    if (!my_printf || !my_printf->my_printf)
        return;
    if (!p_ctx)
        return;

    shell_ctx_t *ctx = (shell_ctx_t *)p_ctx;

    /* 溢出保护：永远预留 '\0' */
    if (ctx->shell_index >= (uint8_t)(sizeof(ctx->shell_buffer) - 1u))
    {
        ctx->shell_index = 0;
    }

    ctx->shell_buffer[ctx->shell_index++] = data;

    /* 仅在 '\n' 触发解析（兼容 "\r\n" 和 "\n"） */
    if (data != '\n')
        return;

    /* 计算有效长度并补 '\0' */
    uint8_t end = ctx->shell_index;
    if (end >= 2u && ctx->shell_buffer[end - 2u] == '\r')
        end = (uint8_t)(end - 2u);
    else
        end = (uint8_t)(end - 1u);

    ctx->shell_buffer[end] = '\0';

    char *line = (char *)ctx->shell_buffer;
    line = ltrim(line);
    rtrim_inplace(line);

    if (*line == '\0')
        goto shell_done;

    /* ---- 内置命令 ---- */
    if (strcmp(line, "time") == 0)
    {
        my_printf->my_printf("time = %us.%03ums\r\n",
                             (unsigned)(SECTION_SYS_TICK / 10000u),
                             (unsigned)((SECTION_SYS_TICK % 10000u) / 10u));
        goto shell_done;
    }
    if (strcmp(line, "reset") == 0)
    {
        SYSTEM_RESET;
        goto shell_done;
    }
    if (strcmp(line, "help") == 0)
    {
        for (section_shell_t *s = p_shell_first; s; s = s->p_next)
        {
            my_printf->my_printf("%s\t%s\r\n", s->p_name, s->type == SHELL_CMD ? "CMD" : "VAR");
        }
        goto shell_done;
    }

    /* ---- 查找命令/变量 ---- */
    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        if (strncmp(line, p->p_name, p->p_name_size) != 0)
            continue;

        char c = line[p->p_name_size];
        if (!(c == ':' || c == '\0'))
            continue;

        char *param = NULL;
        if (c == ':')
            param = (char *)&line[p->p_name_size + 1u];

        /* 参数可写：在 buffer 内就地分割 */
        char *value_str = NULL;
        int32_t status_set = -1;
        if (param)
        {
            parse_param_value_and_status(param, &value_str, &status_set);
        }

        /* 1) CMD：有参数也忽略；直接执行 */
        if (p->type == SHELL_CMD)
        {
            if (p->func)
                p->func(my_printf);

            /* 允许用 "CMD:-s 3" 绑定状态输出/回调（如果你希望支持） */
            if (status_set >= 0)
            {
                p->status = (uint32_t)status_set;
                p->my_printf = my_printf;
            }
            goto shell_done;
        }

        /* 2) VAR：若给了 value_str 则写入；不带 value 仅打印 */
        if (value_str)
        {
            shell_write_item_if_needed(p, value_str, my_printf);
        }

        shell_print_item(p, my_printf);

        /* 3) 解析 -s：解决 "VAR: 123 -s 3" 这种之前无法设置的问题 */
        if (status_set >= 0)
        {
            p->status = (uint32_t)status_set;
            p->my_printf = my_printf;
        }

        goto shell_done;
    }

shell_done:
    /* 只清索引即可（避免每次 memset 128 字节） */
    ctx->shell_index = 0;
    ctx->shell_buffer[0] = 0;
}

// ------------------------ shell 状态轮询（你原来的迁入） ------------------------
void shell_status_run(void)
{
    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        if (!p->status)
            continue;

        /* 没有绑定输出接口则跳过，避免空指针 */
        if (!p->my_printf || !p->my_printf->my_printf)
            continue;

        if (p->status & (1u << 0))
        {
            /* bit0：打印变量/值 */
            if (p->type != SHELL_CMD)
            {
                shell_print_item(p, p->my_printf);
            }
        }

        if (p->status & (1u << 1))
        {
            /* bit1：执行回调（通常用于采样/刷新/额外输出） */
            if (p->func)
                p->func(p->my_printf);
        }
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
    static uint8_t print_flag = 0; /* 0: data, 1: separator */

    if (!g_list_print_ctx.active || !g_list_print_ctx.my_printf || !g_list_print_ctx.my_printf->my_printf)
        return 0;

    if (print_flag == 0)
    {
        section_shell_t *s = g_list_print_ctx.cur;
        if (!s)
        {
            g_list_print_ctx.active = 0;
            print_flag = 0;
            return 0;
        }

        switch (s->type)
        {
        case SHELL_CMD:
            g_list_print_ctx.my_printf->my_printf("%s\tCMD\r\n", s->p_name);
            break;

        case SHELL_UINT8:
            g_list_print_ctx.my_printf->my_printf("%s\tU8\t(%u)\t(%u)\t%u\r\n",
                                                  s->p_name,
                                                  (unsigned)(*(uint8_t *)s->p_max),
                                                  (unsigned)(*(uint8_t *)s->p_min),
                                                  (unsigned)(*(uint8_t *)s->p_var));
            break;

        case SHELL_UINT16:
            g_list_print_ctx.my_printf->my_printf("%s\tU16\t(%u)\t(%u)\t%u\r\n",
                                                  s->p_name,
                                                  (unsigned)(*(uint16_t *)s->p_max),
                                                  (unsigned)(*(uint16_t *)s->p_min),
                                                  (unsigned)(*(uint16_t *)s->p_var));
            break;

        case SHELL_UINT32:
            g_list_print_ctx.my_printf->my_printf("%s\tU32\t(%lu)\t(%lu)\t%lu\r\n",
                                                  s->p_name,
                                                  (unsigned long)(*(uint32_t *)s->p_max),
                                                  (unsigned long)(*(uint32_t *)s->p_min),
                                                  (unsigned long)(*(uint32_t *)s->p_var));
            break;

        case SHELL_INT8:
            g_list_print_ctx.my_printf->my_printf("%s\tI8\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  (int)(*(int8_t *)s->p_max),
                                                  (int)(*(int8_t *)s->p_min),
                                                  (int)(*(int8_t *)s->p_var));
            break;

        case SHELL_INT16:
            g_list_print_ctx.my_printf->my_printf("%s\tI16\t(%d)\t(%d)\t%d\r\n",
                                                  s->p_name,
                                                  (int)(*(int16_t *)s->p_max),
                                                  (int)(*(int16_t *)s->p_min),
                                                  (int)(*(int16_t *)s->p_var));
            break;

        case SHELL_INT32:
            g_list_print_ctx.my_printf->my_printf("%s\tI32\t(%ld)\t(%ld)\t%ld\r\n",
                                                  s->p_name,
                                                  (long)(*(int32_t *)s->p_max),
                                                  (long)(*(int32_t *)s->p_min),
                                                  (long)(*(int32_t *)s->p_var));
            break;

        case SHELL_FP32:
            g_list_print_ctx.my_printf->my_printf("%s\tFP32\t(%f)\t(%f)\t%f\r\n",
                                                  s->p_name,
                                                  (double)(*(float *)s->p_max),
                                                  (double)(*(float *)s->p_min),
                                                  (double)(*(float *)s->p_var));
            break;
        default:
            break;
        }

        g_list_print_ctx.cur = s->p_next;
        print_flag = 1;
        return 1;
    }

    g_list_print_ctx.my_printf->my_printf("-----------------------------------------\r\n");
    print_flag = 0;
    return 1;
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

extern float task_run_time;
float task_metric = 0.0f;
float task_metric_max = 0.0f;

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
REG_SHELL_VAR(TASK_METRIC, task_metric, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TASK_METRIC_MAX, task_metric_max, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)

#pragma GCC diagnostic pop
