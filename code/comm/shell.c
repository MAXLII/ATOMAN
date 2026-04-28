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

/* Head of the runtime shell registration list built from SECTION_SHELL items. */
section_shell_t *p_shell_first;

/* Number of shell entries inserted into the runtime list. */
uint32_t shell_data_num = 0;

static void shell_insert(section_shell_t *shell)
{
    /* Reject NULL input so the caller can scan sections without extra guards. */
    if (!shell)
        return;

    /* Avoid inserting the same registration twice if init is called again. */
    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        if (p == shell)
            return;
    }

    /* Insert at the head because ordering is not performance critical here. */
    shell->p_next = p_shell_first;
    p_shell_first = shell;
    shell_data_num++;
}

void shell_init(void)
{
    /* Scan the linker section and collect every shell command/variable entry. */
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

static int is_string_number(const char *str)
{
    /* Accept plain numbers and simple expressions such as "1+2*3" or "-0.5". */
    if (!str || !*str)
        return 0;

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
        /* Skip whitespace between tokens so the parser is user friendly. */
        while (*expr == ' ')
            expr++;

        /* Collapse repeated leading +/- signs into a final sign. */
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

        if (*expr == '(')
        {
            /* Parentheses recurse into the same precedence parser. */
            expr++;
            number = eval_expr_inner(&expr);
            if (*expr == ')')
                expr++;
        }
        else
        {
            /* strtof advances expr to the first non-numeric character. */
            number = strtof(expr, (char **)&expr);
        }
        number *= sign;

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

            while (*expr == '+' || *expr == '-')
            {
                if (*expr == '-')
                    next_sign *= -1;
                expr++;
            }
            while (*expr == ' ')
                expr++;

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

            if (muldiv == '*')
                number *= next;
            else if (muldiv == '/' && next != 0)
                number /= next;
        }

        /* Merge the resolved term into the accumulated result. */
        if (op == '+')
            result += number;
        else if (op == '-')
            result -= number;

        while (*expr == ' ')
            expr++;
        if (*expr == '+' || *expr == '-')
            op = *expr++;
        else if (*expr == ')')
            break;
        else if (*expr == '\0')
            break;
        else
            op = '+';
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
        /* Hex format: 0x1234 */
        *out = (int32_t)strtol(param, NULL, 16);
        return 1;
    }
    else if (strncmp(param, "0b", 2) == 0)
    {
        /* Binary format: 0b1010 */
        *out = (int32_t)strtol(param + 2, NULL, 2);
        return 1;
    }
    else if (is_string_number(param))
    {
        /* Decimal and simple expression format. */
        *out = (int32_t)eval_expr(param);
        return 1;
    }
    return 0;
}

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

static void parse_param_value_and_status(char *param, char **value_str, int32_t *status_set)
{
    /* Parse "value -s N" from one buffer in place to avoid extra allocation. */
    *value_str = NULL;
    *status_set = -1;
    if (!param)
        return;

    char *p = ltrim(param);

    char *opt = strstr(p, "-s");
    while (opt)
    {
        if (opt == p || isspace((unsigned char)opt[-1]))
            break;
        opt = strstr(opt + 2, "-s");
    }

    if (opt)
    {
        /* Split the original string into payload part and optional status part. */
        *opt = '\0';
        char *v = ltrim(p);
        rtrim_inplace(v);
        *value_str = (*v) ? v : NULL;

        char *ps = opt + 2;
        ps = ltrim(ps);
        if (*ps == '\0')
        {
            *status_set = 0;
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

    char *v = ltrim(p);
    rtrim_inplace(v);
    *value_str = (*v) ? v : NULL;
}

static void shell_print_item(section_shell_t *p, DEC_MY_PRINTF)
{
    if (!p || !my_printf || !my_printf->my_printf)
        return;

    /* Print every shell item with a type-specific formatter. */
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
        /* Commands do not own writable storage, so nothing is updated here. */
        break;

    case SHELL_UINT8:
    {
        uint8_t val = *(uint8_t *)p->p_var;
        if (parse_integer(value_str, &intval))
        {
            val = (uint8_t)intval;
            /* Clamp the requested value to the registered shell limits. */
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
            /* Float shell variables reuse the same limit mechanism. */
            SHELL_UP_DN_LMT(val, p->p_max, p->p_min);
            memcpy(p->p_var, &val, sizeof(val));
        }
        break;
    }
    default:
        break;
    }

    if (p->func)
        /* Notify the owner after the value is updated. */
        p->func(my_printf);
}

void shell_run(uint8_t data, DEC_MY_PRINTF, void *p_ctx)
{
    if (!my_printf || !my_printf->my_printf)
        return;
    if (!p_ctx)
        return;

    shell_ctx_t *ctx = (shell_ctx_t *)p_ctx;

    /* Reset on overflow so a malformed stream cannot overrun the line buffer. */
    if (ctx->shell_index >= (uint8_t)(sizeof(ctx->shell_buffer) - 1u))
    {
        ctx->shell_index = 0;
    }

    ctx->shell_buffer[ctx->shell_index++] = data;

    if (data != '\n')
        return;

    /* Convert CRLF or LF line endings into one C string in place. */
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

    /* Handle built-in commands first before walking the registered shell list. */
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

    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        /* Match exact command/variable name, then accept optional ":payload". */
        if (strncmp(line, p->p_name, p->p_name_size) != 0)
            continue;

        char c = line[p->p_name_size];
        if (!(c == ':' || c == '\0'))
            continue;

        char *param = NULL;
        if (c == ':')
            param = (char *)&line[p->p_name_size + 1u];

        char *value_str = NULL;
        int32_t status_set = -1;
        if (param)
        {
            parse_param_value_and_status(param, &value_str, &status_set);
        }

        if (p->type == SHELL_CMD)
        {
            /* Commands execute immediately; "-s" only affects periodic follow-up. */
            if (p->func)
                p->func(my_printf);

            if (status_set >= 0)
            {
                p->status = (uint32_t)status_set;
                p->my_printf = my_printf;
            }
            goto shell_done;
        }

        if (value_str)
        {
            /* Variables optionally accept a write value before the echo print. */
            shell_write_item_if_needed(p, value_str, my_printf);
        }

        shell_print_item(p, my_printf);

        if (status_set >= 0)
        {
            p->status = (uint32_t)status_set;
            p->my_printf = my_printf;
        }

        goto shell_done;
    }

shell_done:
    /* Always reset the line parser so the next command starts from a clean state. */
    ctx->shell_index = 0;
    ctx->shell_buffer[0] = 0;
}

void shell_status_run(void)
{
    /* Periodically service status-triggered shell items. */
    for (section_shell_t *p = p_shell_first; p; p = p->p_next)
    {
        if (!p->status)
            continue;

        if (!p->my_printf || !p->my_printf->my_printf)
            continue;

        if (p->status & (1u << 0))
        {
            /* Bit0 means "print me periodically" for variable-style entries. */
            if (p->type != SHELL_CMD)
            {
                shell_print_item(p, p->my_printf);
            }
        }

        if (p->status & (1u << 1))
        {
            /* Bit1 means "run callback periodically". */
            if (p->func)
                p->func(p->my_printf);
        }
    }
}

REG_TASK_MS(1000, shell_status_run)

typedef struct
{
    /* Current shell item being printed by the non-blocking list command. */
    section_shell_t *cur;
    DEC_MY_PRINTF;
    /* Active flag lets the task print one item per tick instead of blocking. */
    uint8_t active;
    int max_name_len;
    int tab_count;
} list_print_ctx_t;

/* Shared context for the staged "list" output task. */
static list_print_ctx_t g_list_print_ctx = {0};

void list_print_start(DEC_MY_PRINTF)
{
    if (!my_printf || g_list_print_ctx.active)
        return;
    /* Prime the asynchronous printer so a long list does not block the shell. */
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
    /* print_flag inserts a separator line between two consecutive entries. */
    static uint8_t print_flag = 0;

    if (!g_list_print_ctx.active || !g_list_print_ctx.my_printf || !g_list_print_ctx.my_printf->my_printf)
        return 0;

    if (print_flag == 0)
    {
        section_shell_t *s = g_list_print_ctx.cur;
        if (!s)
        {
            /* The whole list has been drained. */
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
    /* Service at most one slice per task tick to keep timing predictable. */
    if (g_list_print_ctx.active)
    {
        list_print_step();
    }
}

REG_TASK_MS(10, list_print_task)

extern float task_run_time;
/* Latest coarse CPU utilization estimation derived from accumulated task time. */
float task_metric = 0.0f;
/* Peak CPU utilization estimation since boot or last reset. */
float task_metric_max = 0.0f;

static void task_metric_calculate(void)
{
    /* task_run_time is accumulated in 0.5 us ticks, so scale it to percent here. */
    task_metric = task_run_time * 500e-6f;
    if (task_metric > task_metric_max)
    {
        task_metric_max = task_metric;
    }
    task_run_time = 0.0f;
    task_metric /= 100.0f;
    task_metric_max /= 100.0f;
}

REG_TASK_MS(100, task_metric_calculate)

static void CPU_Utilization(DEC_MY_PRINTF)
{
    my_printf->my_printf("CPU Load:%f%%,CPU Peak:%f%%\n", task_metric * 100.0f, task_metric_max * 100.0f);
}
REG_SHELL_CMD(CPU_Utilization, CPU_Utilization)
REG_SHELL_VAR(TASK_METRIC, task_metric, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TASK_METRIC_MAX, task_metric_max, SHELL_FP32, 100.0f, 0.0f, NULL, SHELL_STA_NULL)

/* Context used when a remote peer requests a full shell item enumeration. */
static shell_report_ctx_t shell_report_ctx = {0};

void shell_data_num_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    /* Ignore replies. A fresh host request always restarts the report cursor. */
    if (p_pack->is_ack == 1)
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

    /* Save the routing information so the follow-up report task can stream data. */
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
            /* End the report session when every shell item has been emitted. */
            shell_report_ctx.active = 0;
        }
        else
        {
            section_packform_t packform = {0};

            /* Convert the current shell entry into one compact report frame. */
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
    /* Name lookup is linear because the shell registry is expected to stay small. */
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
    /* Validate the variable-length payload before dereferencing its name field. */
    if (p_pack->len != sizeof(shell_read_data_t) - SHELL_STR_SIZE_MAX + p_shell_read_data->name_len)
    {
        return;
    }
    section_shell_t *p = find_shell(p_shell_read_data->name, p_shell_read_data->name_len);
    if (p)
    {
        if (p->func)
        {
            /* Let the owner refresh its value before it is reported. */
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
        /* Remote write shares the same data model as the local shell entry. */
        switch (p->type)
        {
        case SHELL_CMD:
            /* Commands are not writable via the generic write-data path. */
            break;
        case SHELL_UINT8:
        {
            memcpy(p->p_var, (uint8_t *)&p_shell_write_data->data, sizeof(uint8_t));
            uint8_t val = *(uint8_t *)p->p_var;
            memcpy(p->p_max, (uint8_t *)&p_shell_write_data->data_max, sizeof(uint8_t));
            memcpy(p->p_min, (uint8_t *)&p_shell_write_data->data_min, sizeof(uint8_t));
            /* Re-apply range clamp after the new remote limits are installed. */
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
            /* Notify the owner after a successful remote update. */
            p->func(my_printf);
    }
}

REG_COMM(CMD_SET_SHELL_WRITE_DATA, CMD_WORD_SHELL_WRITE_DATA, shell_write_data_act)

static void shell_wave_param_enable_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    shell_wave_enable_param_t *p_shell_wave_enable_param;
    p_shell_wave_enable_param = (shell_wave_enable_param_t *)p_pack->p_data;
    /* Validate variable-length payload before touching the embedded name field. */
    if (p_pack->len != sizeof(shell_wave_enable_param_t) - SHELL_STR_SIZE_MAX + p_shell_wave_enable_param->name_len)
    {
        return;
    }
    section_shell_t *p = find_shell(p_shell_wave_enable_param->name, p_shell_wave_enable_param->name_len);

    shell_wave_enable_param_ack_t shell_wave_enable_param_ack;

    if (p)
    {
        shell_wave_enable_param_ack.ok = 1;
        /* Bit2 is reserved for wave auto-report selection. */
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

/* Global enable for the wave streaming state machine. */
static uint8_t shell_wave_report_flg = 0;
/* Delay between two wave frames, expressed in shell task ticks. */
static uint32_t shell_wave_report_period = 300;
/* Down-counter used by the WAIT state. */
static uint32_t shell_wave_report_dn_cnt = 0;
/* Cached output path used to send wave data back to the requester. */
static section_link_tx_func_t *p_shell_wave_report_printf;
/* Cached source address for wave packets. */
static uint8_t shell_wave_src = 0;
/* Cached destination address for wave packets. */
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
    /* Cache the response route so the periodic task can keep streaming later. */
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

    /* Send one wave sample or one frame marker packet. */
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
    /* Idle until a remote peer enables wave streaming. */
    SHELL_WAVE_FSM_IDLE,
    /* Emit the start marker of one wave frame. */
    SHELL_WAVE_FSM_START,
    /* Walk through all auto-report shell variables. */
    SHELL_WAVE_FSM_DATA,
    /* Emit the end marker of one wave frame. */
    SHELL_WAVE_FSM_END,
    /* Wait the requested inter-frame gap before the next round. */
    SHELL_WAVE_FSM_WAIT,
} SHELL_WAVE_FSM_E;

/* Current state of the shell wave report finite-state machine. */
static SHELL_WAVE_FSM_E shell_wave_fsm = 0;

static void shell_wave_report_task(void)
{
    /* delay_cnt throttles packet rate inside the DATA state. */
    static uint8_t delay_cnt = 0;
    shell_wave_param_t shell_wave_param = {0};
    /* Cursor used to resume variable scanning across task ticks. */
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
        /* 0x55555555 marks the beginning of one streamed frame. */
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
                /* Bit2-selected shell variables are streamed one by one. */
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
        /* 0xAAAAAAAA marks the end of one streamed frame. */
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

#pragma GCC diagnostic pop
