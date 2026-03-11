/**
 * @file shell.c
 * @brief Shell 命令系统（协议/具体实现层）
 */

#include "shell.h"
#include "section.h"
#include "platform.h"
#include "comm.h"
#include "my_math.h"

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


static shell_report_ctx_t shell_report_ctx = {0};

void shell_data_num_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if ((p_pack->is_ack == 1) ||
        (shell_report_ctx.active == 1))
    {
        return;
    }
    section_packform_t pack_ret = {0};
    pack_ret.src = p_pack->dst;
    pack_ret.d_src = p_pack->d_dst;
    pack_ret.dst = p_pack->src;
    pack_ret.d_dst = p_pack->d_src;
    pack_ret.cmd_set = CMD_SET_SHELL_DATA_NUM;
    pack_ret.cmd_word = CMD_WORD_SHELL_DATA_NUM;
    pack_ret.is_ack = 1;
    pack_ret.len = sizeof(uint32_t);
    pack_ret.p_data = (uint8_t *)&shell_data_num;

    shell_report_ctx.active = 1;
    shell_report_ctx.my_printf = my_printf;
    shell_report_ctx.p_shell = p_shell_first;
    shell_report_ctx.src = pack_ret.src;
    shell_report_ctx.d_src = pack_ret.d_src;
    shell_report_ctx.dst = pack_ret.dst;
    shell_report_ctx.d_dst = pack_ret.d_dst;

    comm_send_data(&pack_ret, my_printf);
}

REG_COMM(CMD_SET_SHELL_DATA_NUM, CMD_WORD_SHELL_DATA_NUM, shell_data_num_act)

static void shell_data_report_act(void)
{
    if (shell_report_ctx.active == 1)
    {
        if (shell_report_ctx.p_shell == NULL)
        {
            shell_report_ctx.active = 0;
        }
        else
        {
            section_packform_t packform = {0};

            shell_report_list_t shell_report_list;
            shell_report_list.name_len = shell_report_ctx.p_shell->p_name_size;
            shell_report_list.type = shell_report_ctx.p_shell->type;
            shell_report_list.data = *(uint32_t *)shell_report_ctx.p_shell->p_var;
            shell_report_list.data_max = *(uint32_t *)shell_report_ctx.p_shell->p_max;
            shell_report_list.data_min = *(uint32_t *)shell_report_ctx.p_shell->p_min;
            memcpy(shell_report_list.name, shell_report_ctx.p_shell->p_name, shell_report_ctx.p_shell->p_name_size);
            shell_report_list.auto_report = (shell_report_ctx.p_shell->status & (1 << 2)) ? 1 : 0;

            packform.src = shell_report_ctx.src;
            packform.d_src = shell_report_ctx.d_src;
            packform.dst = shell_report_ctx.dst;
            packform.d_dst = shell_report_ctx.d_dst;
            packform.cmd_set = CMD_SET_SHELL_REPORT_LIST;
            packform.cmd_word = CMD_WORD_SHELL_REPORT_LIST;
            packform.is_ack = 0;
            packform.len = sizeof(shell_report_list_t) - SHELL_STR_SIZE_MAX + shell_report_ctx.p_shell->p_name_size;
            packform.p_data = (uint8_t *)&shell_report_list;
            comm_send_data(&packform, shell_report_ctx.my_printf);
            shell_report_ctx.p_shell = shell_report_ctx.p_shell->p_next;
        }
    }
}

REG_TASK_MS(50, shell_data_report_act)

static section_shell_t *find_shell(char *p_name, uint8_t len)
{
    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        if (p->p_name_size != len)
        {
            continue;
        }
        if (memcmp(p->p_name, p_name, len) == 0)
        {
            return p;
        }
    }
    return NULL;
}

static void shell_read_data_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    shell_read_data_t *p_shell_read_data;
    p_shell_read_data = (shell_read_data_t *)p_pack->p_data;
    if (p_pack->len != sizeof(shell_read_data_t) - SHELL_STR_SIZE_MAX + p_shell_read_data->name_len)
    {
        return;
    }
    section_shell_t *p = find_shell(p_shell_read_data->name, p_shell_read_data->name_len);
    if (p)
    {
        if (p->func)
        {
            p->func(my_printf);
        }
        shell_read_data_ret_t shell_read_data_ret = {0};
        shell_read_data_ret.name_len = p->p_name_size;
        shell_read_data_ret.type = p->type;
        shell_read_data_ret.data = *(uint32_t *)p->p_var;
        memcpy(shell_read_data_ret.name, p->p_name, p->p_name_size);

        section_packform_t packform = {0};
        packform.src = p_pack->dst;
        packform.d_src = p_pack->d_dst;
        packform.dst = p_pack->src;
        packform.d_dst = p_pack->d_src;
        packform.cmd_set = CMD_SET_SHELL_READ_DATA;
        packform.cmd_word = CMD_WORD_SHELL_READ_DATA;
        packform.is_ack = 1;
        packform.len = sizeof(shell_read_data_ret_t) - SHELL_STR_SIZE_MAX + p->p_name_size;
        packform.p_data = (uint8_t *)&shell_read_data_ret;

        comm_send_data(&packform, my_printf);
    }
}

REG_COMM(CMD_SET_SHELL_READ_DATA, CMD_WORD_SHELL_READ_DATA, shell_read_data_act)

static void shell_write_data_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    shell_write_data_t *p_shell_write_data;
    p_shell_write_data = (shell_write_data_t *)p_pack->p_data;
    section_shell_t *p;
    p = find_shell(p_shell_write_data->name, p_shell_write_data->name_len);
    if (p)
    {
        // 根据变量类型进行处理
        switch (p->type)
        {
        case SHELL_CMD:
            // 执行命令

            break;
        case SHELL_UINT8:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(uint8_t));
            uint8_t val = *(uint8_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint8_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint8_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_INT8:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(int8_t));
            int8_t val = *(int8_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int8_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int8_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_UINT16:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(uint16_t));
            uint16_t val = *(uint16_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint16_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint16_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_INT16:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(int16_t));
            int16_t val = *(int16_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int16_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int16_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_UINT32:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(uint32_t));
            uint32_t val = *(uint32_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint32_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint32_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_INT32:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(int32_t));
            int32_t val = *(int32_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(int32_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(int32_t));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        case SHELL_FP32:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(float));
            float val = *(float *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(float));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(float));
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);

            break;
        }
        }
        shell_write_data_ret_t shell_write_data_ret = {0};
        shell_write_data_ret.data = *(uint32_t *)p->p_var;
        shell_write_data_ret.data_max = *(uint32_t *)p->p_max;
        shell_write_data_ret.data_min = *(uint32_t *)p->p_min;
        memcpy(shell_write_data_ret.name, p->p_name, p->p_name_size);
        shell_write_data_ret.name_len = p->p_name_size;
        shell_write_data_ret.type = p->type;

        section_packform_t packform = {0};
        packform.src = p_pack->dst;
        packform.d_src = p_pack->d_dst;
        packform.dst = p_pack->src;
        packform.d_dst = p_pack->d_src;
        packform.cmd_set = CMD_SET_SHELL_WRITE_DATA;
        packform.cmd_word = CMD_WORD_SHELL_WRITE_DATA;
        packform.is_ack = 1;
        packform.len = sizeof(shell_write_data_ret_t) - SHELL_STR_SIZE_MAX + p->p_name_size;
        packform.p_data = (uint8_t *)&shell_write_data_ret;

        comm_send_data(&packform, my_printf);

        if (p->func)
            p->func(my_printf);
    }
}

REG_COMM(CMD_SET_SHELL_WRITE_DATA, CMD_WORD_SHELL_WRITE_DATA, shell_write_data_act)

static void shell_wave_param_enable_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    shell_wave_enable_param_t *p_shell_wave_enable_param;
    p_shell_wave_enable_param = (shell_wave_enable_param_t *)p_pack->p_data;
    if (p_pack->len != sizeof(shell_wave_enable_param_t) - SHELL_STR_SIZE_MAX + p_shell_wave_enable_param->name_len)
    {
        return;
    }
    section_shell_t *p = find_shell(p_shell_wave_enable_param->name, p_shell_wave_enable_param->name_len);

    shell_wave_enable_param_ack_t shell_wave_enable_param_ack;

    if (p)
    {
        shell_wave_enable_param_ack.ok = 1;
        if (p_shell_wave_enable_param->auto_report == 1)
        {
            p->status |= 1 << 2;
        }
        else
        {
            p->status &= ~(1 << 2);
        }
    }

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_SHELL_WAVE_ENABLE_PARAM;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_ENABLE_PARAM;
    packform.src = p_pack->dst;
    packform.dst = p_pack->src;
    packform.is_ack = 1;
    packform.len = sizeof(shell_wave_enable_param_ack_t);
    packform.p_data = (uint8_t *)&shell_wave_enable_param_ack;
    comm_send_data(&packform, my_printf);
}

REG_COMM(CMD_SET_SHELL_WAVE_ENABLE_PARAM, CMD_WORD_SHELL_WAVE_ENABLE_PARAM, shell_wave_param_enable_act)

static uint8_t shell_wave_report_flg = 0;
static uint32_t shell_wave_report_period = 300;
static uint32_t shell_wave_report_dn_cnt = 0;
static section_link_tx_func_t *p_shell_wave_report_printf;
static uint8_t shell_wave_src = 0;
static uint8_t shell_wave_dst = 0;

static void shell_wave_start_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->len != sizeof(shell_wave_start_t))
    {
        return;
    }
    shell_wave_start_t *p_shell_wave_start = (shell_wave_start_t *)p_pack->p_data;
    shell_wave_report_flg = p_shell_wave_start->start_report;

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_SHELL_WAVE_START;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_START;
    packform.src = p_pack->dst;
    packform.dst = p_pack->src;
    packform.is_ack = 1;
    packform.len = 0;
    packform.p_data = NULL;
    comm_send_data(&packform, my_printf);
    p_shell_wave_report_printf = my_printf;
    shell_wave_src = packform.src;
    shell_wave_dst = packform.dst;
}
REG_COMM(CMD_SET_SHELL_WAVE_START, CMD_WORD_SHELL_WAVE_START, shell_wave_start_act)

static void shell_wave_period_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->len != sizeof(shell_wave_period_t))
    {
        return;
    }
    shell_wave_period_t *p_shell_wave_period = (shell_wave_period_t *)p_pack->p_data;
    shell_wave_report_period = p_shell_wave_period->reprot_period;

    shell_wave_period_ack_t shell_wave_period_ack = {.reprot_period = shell_wave_report_period};

    section_packform_t packform = {0};
    packform.cmd_set = CMD_SET_SHELL_WAVE_PERIOD;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_PERIOD;
    packform.src = p_pack->dst;
    packform.dst = p_pack->src;
    packform.is_ack = 1;
    packform.len = sizeof(shell_wave_period_ack_t);
    packform.p_data = (uint8_t *)&shell_wave_period_ack;
    comm_send_data(&packform, my_printf);
}
REG_COMM(CMD_SET_SHELL_WAVE_PERIOD, CMD_WORD_SHELL_WAVE_PERIOD, shell_wave_period_act)

static void shell_wave_param_act(shell_wave_param_t *p, DEC_MY_PRINTF)
{
    section_packform_t packform = {0};

    packform.cmd_set = CMD_SET_SHELL_WAVE_PARAM;
    packform.cmd_word = CMD_WORD_SHELL_WAVE_PARAM;
    packform.src = shell_wave_src;
    packform.dst = shell_wave_dst;
    packform.len = sizeof(shell_wave_param_t) - SHELL_STR_SIZE_MAX + p->name_len;
    packform.p_data = (uint8_t *)p;

    comm_send_data(&packform, my_printf);
}

typedef enum
{
    SHELL_WAVE_FSM_IDLE,
    SHELL_WAVE_FSM_START,
    SHELL_WAVE_FSM_DATA,
    SHELL_WAVE_FSM_END,
    SHELL_WAVE_FSM_WAIT,
} SHELL_WAVE_FSM_E;

static SHELL_WAVE_FSM_E shell_wave_fsm = 0;

static void shell_wave_report_task(void)
{
    static uint8_t delay_cnt = 0;
    shell_wave_param_t shell_wave_param = {0};
    static section_shell_t *p = NULL;
    switch (shell_wave_fsm)
    {
    case SHELL_WAVE_FSM_IDLE:
        if (shell_wave_report_flg == 1)
        {
            shell_wave_fsm = SHELL_WAVE_FSM_START;
        }
        break;
    case SHELL_WAVE_FSM_START:
        shell_wave_param.data = 0x55555555;
        shell_wave_param_act(&shell_wave_param, p_shell_wave_report_printf);
        p = p_shell_first;
        shell_wave_fsm = SHELL_WAVE_FSM_DATA;
        delay_cnt = 10;
        break;
    case SHELL_WAVE_FSM_DATA:
        if (delay_cnt)
        {
            delay_cnt--;
            break;
        }
        else
        {
            delay_cnt = 10;
        }
        while (p)
        {
            if (p->status & (1 << 2))
            {
                shell_wave_param.data = (uint32_t)*(uint32_t *)p->p_var;
                shell_wave_param.name_len = p->p_name_size;
                shell_wave_param.type = p->type;
                memcpy((uint8_t *)shell_wave_param.name, (uint8_t *)p->p_name, shell_wave_param.name_len);
                p = p->p_next;
                shell_wave_param_act(&shell_wave_param, p_shell_wave_report_printf);
                break;
            }
            p = p->p_next;
        }
        if (p == NULL)
        {
            shell_wave_fsm = SHELL_WAVE_FSM_END;
        }
        break;
    case SHELL_WAVE_FSM_END:
        shell_wave_param.data = 0xAAAAAAAA;
        shell_wave_param_act(&shell_wave_param, p_shell_wave_report_printf);
        shell_wave_fsm = SHELL_WAVE_FSM_WAIT;
        shell_wave_report_dn_cnt = shell_wave_report_period;
        break;
    case SHELL_WAVE_FSM_WAIT:
        DN_CNT(shell_wave_report_dn_cnt);
        if (shell_wave_report_dn_cnt == 0)
        {
            shell_wave_fsm = SHELL_WAVE_FSM_START;
        }
        if (shell_wave_report_flg == 0)
        {
            shell_wave_fsm = SHELL_WAVE_FSM_IDLE;
        }
        break;
    }
}

REG_TASK_MS(1, shell_wave_report_task)
