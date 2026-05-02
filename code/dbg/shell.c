// SPDX-License-Identifier: MIT
/**
 * @file    shell.c
 * @brief   shell core module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Parse text shell input into command names, arguments, and variable operations
 *          - Maintain the section-registered shell command and variable registry
 *          - Dispatch shell commands and read/write variables through caller-provided transport callbacks
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "shell.h"
#include "section.h"
#include "platform.h"

#include <stddef.h>
#include <string.h>
#include <ctype.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"

/* Head of the runtime shell registration list built from SECTION_SHELL items. */
section_shell_t *p_shell_first;

/* Number of shell entries inserted into the runtime list. */
uint32_t shell_data_num = 0u;

static void shell_insert(section_shell_t *shell)
{
    /* Reject NULL input so the caller can scan sections without extra guards. */
    if (shell == NULL)
    {
        return;
    }

    /* Avoid inserting the same registration twice if init is called again. */
    for (section_shell_t *p = p_shell_first; p != NULL; p = p->p_next)
    {
        if (p == shell)
        {
            return;
        }
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

/* --- string parsing & variable writing (gated by SHELL_STRING_PARSE) ------ */

#if SHELL_STRING_PARSE == 1

/* --- string helpers (always available) ----------------------------------- */

static inline char *ltrim(char *s)
{
    while (s && *s && isspace((unsigned char)*s))
        s++;
    return s;
}

static inline void rtrim_inplace(char *s)
{
    if (s == NULL)
        return;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1]))
    {
        s[n - 1] = '\0';
        n--;
    }
}

/* --- shell item print (always available) --------------------------------- */

void shell_item_print(section_shell_t *p, DEC_MY_PRINTF)
{
    if ((p == NULL) || (my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

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


#include <stdlib.h>

static int is_string_number(const char *str)
{
    /* Accept plain numbers and simple expressions such as "1+2*3" or "-0.5". */
    if ((str == NULL) || (*str == '\0'))
        return 0;

    if ((*str == '-') || (*str == '+'))
    {
        str++;
    }

    int has_digit = 0, has_dot = 0;
    while (*str != '\0')
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
    float result = 0.0f;
    char op = '+';

    while (*expr != '\0')
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

        float number = 0.0f;

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

        while (1u)
        {
            while (*expr == ' ')
                expr++;
            if (*expr != '*' && *expr != '/')
                break;

            char muldiv = *expr++;
            while (*expr == ' ')
                expr++;

            float next = 0.0f;
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
            else if (muldiv == '/' && next != 0.0f)
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

static void parse_param_value_and_status(char *param, char **value_str, int32_t *status_set)
{
    /* Parse "value -s N" from one buffer in place to avoid extra allocation. */
    *value_str = NULL;
    *status_set = -1;
    if (param == NULL)
        return;

    char *p = ltrim(param);

    char *opt = strstr(p, "-s");
    while (opt != NULL)
    {
        if ((opt == p) || isspace((unsigned char)opt[-1]))
        {
            break;
        }
        opt = strstr(opt + 2, "-s");
    }

    if (opt != NULL)
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

static void shell_write_item_if_needed(section_shell_t *p, const char *value_str, DEC_MY_PRINTF)
{
    if ((p == NULL) || (value_str == NULL))
    {
        return;
    }

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

/**
 * @brief Handle parameter parsing, value writing, and status update after a
 *        command/variable name match in shell_run.
 *
 * When SHELL_STRING_PARSE is defined this is the full implementation.
 * Otherwise a minimal stub dispatches commands and prints variables read-only.
 */
static void shell_handle_param(section_shell_t *p, char c, char *line, DEC_MY_PRINTF)
{
    char *param = NULL;
    if (c == ':')
        param = (char *)&line[p->p_name_size + 1u];

    char *value_str = NULL;
    int32_t status_set = -1;
    if (param != NULL)
    {
        parse_param_value_and_status(param, &value_str, &status_set);
    }

    if (p->type == SHELL_CMD)
    {
        if (p->func)
            p->func(my_printf);

        if (status_set >= 0)
        {
            p->status = (uint32_t)status_set;
            p->my_printf = my_printf;
        }
    }
    else
    {
        if (value_str != NULL)
        {
            shell_write_item_if_needed(p, value_str, my_printf);
        }

        shell_item_print(p, my_printf);

        if (status_set >= 0)
        {
            p->status = (uint32_t)status_set;
            p->my_printf = my_printf;
        }
    }
}

/* --- shell line dispatcher ------------------------------------------------ */

void shell_run(uint8_t data, DEC_MY_PRINTF, void *p_ctx)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }
    if (p_ctx == NULL)
    {
        return;
    }

    shell_ctx_t *ctx = (shell_ctx_t *)p_ctx;

    /* Reset on overflow so a malformed stream cannot overrun the line buffer. */
    if (ctx->shell_index >= (uint8_t)(sizeof(ctx->shell_buffer) - 1u))
    {
        ctx->shell_index = 0u;
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
        for (section_shell_t *s = p_shell_first; s != NULL; s = s->p_next)
        {
            my_printf->my_printf("%s\t%s\r\n", s->p_name, s->type == SHELL_CMD ? "CMD" : "VAR");
        }
        goto shell_done;
    }

    for (section_shell_t *p = p_shell_first; p != NULL; p = p->p_next)
    {
        /* Match exact command/variable name, then accept optional ":payload". */
        if (strncmp(line, p->p_name, p->p_name_size) != 0)
            continue;

        char c = line[p->p_name_size];
        if (c != ':' && c != '\0')
            continue;

        shell_handle_param(p, c, line, my_printf);
        goto shell_done;
    }

shell_done:
    /* Always reset the line parser so the next command starts from a clean state. */
    ctx->shell_index = 0u;
    ctx->shell_buffer[0] = 0u;
}

#else /* !SHELL_STRING_PARSE — minimal stubs */

void shell_run(uint8_t data, DEC_MY_PRINTF, void *p_ctx)
{
    (void)data;
    (void)my_printf;
    (void)p_ctx;
}

#endif /* SHELL_STRING_PARSE */

section_shell_t *shell_first_get(void)
{
    return p_shell_first;
}

uint32_t shell_count_get(void)
{
    return shell_data_num;
}

section_shell_t *shell_find(const char *p_name, uint8_t len)
{
    if (p_name == NULL)
    {
        return NULL;
    }

    for (section_shell_t *p = p_shell_first; p != NULL; p = p->p_next)
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

#pragma GCC diagnostic pop
