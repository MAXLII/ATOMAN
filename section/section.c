#include "section.h"
#include "stddef.h"
#include "string.h"
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#ifdef IS_PLECS
#include "plecs.h"
extern uint32_t plecs_time_100us;
#define SECTION_SYS_TICK plecs_time_100us
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section
#define SECTION_STOP __stop_section
#define SYSTEM_RESET ;
#else
#include "systick.h"
#include "gd32g5x3.h"
#define SECTION_SYS_TICK systick_gettime_100us()
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end
#define SYSTEM_RESET nvic_system_reset()
#endif

reg_task_t *p_task_first = NULL;
reg_interrupt_t *p_interrupt_first = NULL;
section_shell_t *p_shell_first = NULL;
section_link_t *p_link_first = NULL;

static int is_string_number(const char *str)
{
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
            // allow expression characters
        }
        else
        {
            return 0;
        }
        str++;
    }
    return has_digit;
}

// 简单表达式求值（支持 + - * /）
// 改进版本：考虑运算符优先级和负号处理
// static声明提前，且只声明一次，避免重复声明冲突
static float eval_expr_inner(const char **p);

static float eval_expr(const char *expr)
{
    // 主入口
    return eval_expr_inner(&expr);
}

// 递归解析表达式，支持括号
static float eval_expr_inner(const char **p)
{
    const char *expr = *p;
    float result = 0;
    char op = '+';

    while (*expr)
    {
        while (*expr == ' ') expr++;

        int sign = 1;
        while (*expr == '+' || *expr == '-')
        {
            if (*expr == '-') sign *= -1;
            expr++;
        }
        while (*expr == ' ') expr++;

        float number = 0;
        if (*expr == '(')
        {
            expr++; // 跳过 '('
            number = eval_expr_inner(&expr);
            if (*expr == ')') expr++; // 跳过 ')'
        }
        else
        {
            number = strtof(expr, (char **)&expr);
        }
        number *= sign;

        // 连续乘除优先处理
        while (1)
        {
            while (*expr == ' ') expr++;
            if (*expr != '*' && *expr != '/') break;

            char muldiv = *expr++;
            while (*expr == ' ') expr++;
            float next = 0;
            int next_sign = 1;
            while (*expr == '+' || *expr == '-')
            {
                if (*expr == '-') next_sign *= -1;
                expr++;
            }
            while (*expr == ' ') expr++;
            if (*expr == '(')
            {
                expr++;
                next = eval_expr_inner(&expr);
                if (*expr == ')') expr++;
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

        if (op == '+')
            result += number;
        else if (op == '-')
            result -= number;

        while (*expr == ' ') expr++;
        if (*expr == '+' || *expr == '-')
            op = *expr++;
        else if (*expr == ')')
            break;
        else if (*expr == '\0')
            break;
        else
            op = '+'; // 容错
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
        *out = (int32_t)strtol(param, NULL, 16);
        return 1;
    }
    else if (strncmp(param, "0b", 2) == 0)
    {
        *out = (int32_t)strtol(param + 2, NULL, 2);
        return 1;
    }
    else
    {
        *out = (int32_t)eval_expr(param);
        return 1;
    }
    return 0;
}

void shell_run(char data, DEC_MY_PRINTF)
{
    static uint8_t shell_buffer[128];
    static uint8_t shell_index = 0;

    // 溢出保护
    if (shell_index >= sizeof(shell_buffer) - 1) {
        shell_index = 0;
    }
    shell_buffer[shell_index++] = data;

    // 兼容 '\n' 或 "\r\n"
    if ((data == '\n' && shell_index > 1 && shell_buffer[shell_index - 2] == '\r') ||
        (data == '\n' && shell_index > 0))
    {
        uint8_t end = shell_index;
        if (shell_index > 1 && shell_buffer[shell_index - 2] == '\r')
            end = shell_index - 2;
        else
            end = shell_index - 1;
        shell_buffer[end] = '\0';

        // 特殊命令处理
        if (strcmp((char *)shell_buffer, "time") == 0)
        {
            my_printf("time = %us.%03ums\r\n",
                      SECTION_SYS_TICK / 10000,
                      SECTION_SYS_TICK % 10000 / 10);
            goto shell_done;
        }
        else if (strcmp((char *)shell_buffer, "reset") == 0)
        {
            SYSTEM_RESET;
            goto shell_done;
        }
        else if (strcmp((char *)shell_buffer, "help") == 0)
        {
            section_shell_t *s = p_shell_first;
            while (s)
            {
                my_printf("%s\t%s\r\n", s->p_name, s->type == SHELL_CMD ? "CMD" : "VAR");
                s = s->p_next;
            }
            goto shell_done;
        }

        section_shell_t *p = p_shell_first;
        while (p)
        {
            if (strncmp((char *)shell_buffer, p->p_name, p->p_name_size) == 0 &&
                (shell_buffer[p->p_name_size] == ':' || shell_buffer[p->p_name_size] == '\0'))
            {

                char *param = NULL;
                if (shell_buffer[p->p_name_size] == ':')
                {
                    param = (char *)&shell_buffer[p->p_name_size + 1];
                    while (*param == ' ')
                        param++;
                }

                int32_t intval = 0;
                float floatval = 0.0f;

                switch (p->type)
                {
                case SHELL_CMD:
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
            p = p->p_next;
        }
        my_printf("Unknown command\r\n");
    shell_done:
        memset(shell_buffer, 0, sizeof(shell_buffer));
        shell_index = 0;
    }
}

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

static void interrupt_insert(reg_interrupt_t *intr)
{
    if (!intr) return;
    intr->p_next = NULL;
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

static void shell_insert(section_shell_t *shell)
{
    if (!shell) return;
    // 防止重复插入
    section_shell_t *last = NULL;
    for (section_shell_t *p = p_shell_first; p; last = p, p = p->p_next) {
        if (p == shell) return;
    }
    shell->p_next = NULL;
    if (last)
        last->p_next = shell;
    else
        p_shell_first = shell;
}

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

void section_init(void)
{
    for (reg_section_t *p = (reg_section_t *)&SECTION_START; p < (reg_section_t *)&SECTION_STOP; ++p)
    {
        switch (p->section_type)
        {
        case SECTION_INIT:
            ((reg_init_t *)p->p_str)->p_func();
            break;
        case SECTION_TASK:
            task_insert((reg_task_t *)p->p_str);
            break;
        case SECTION_INTERRUPT:
            interrupt_insert((reg_interrupt_t *)p->p_str);
            break;
        case SECTION_SHELL:
            shell_insert((section_shell_t *)p->p_str);
            break;
        case SECTION_LINK:
            link_insert((section_link_t *)p->p_str);
            break;
        }
    }
}

void run_task(void)
{
    uint32_t now = SECTION_SYS_TICK;
    for (reg_task_t *t = p_task_first; t != NULL; t = (reg_task_t *)t->p_next)
    {
        if (now - t->time_last >= t->t_period)
        {
            t->p_func();
            t->time_last = now;
        }
    }
}

void section_interrupt(void)
{
    for (reg_interrupt_t *p = p_interrupt_first; p != NULL; p = (reg_interrupt_t *)p->p_next)
    {
        p->p_func();
    }
}

void section_fsm_func(reg_fsm_t *fsm)
{
    if (!fsm->p_fsm_func_table || !fsm->p_fsm_ev)
        return;
    for (uint32_t i = 0; i < fsm->fsm_table_size; ++i)
    {
        reg_fsm_func_t *entry = &fsm->p_fsm_func_table[i];
        if (fsm->fsm_sta == entry->fsm_sta)
        {
            if (fsm->fsm_sta_is_change)
            {
                fsm->fsm_sta_is_change = 0;
                entry->func_in();
            }
            entry->func_exe();
            if (*fsm->p_fsm_ev)
            {
                uint32_t next = entry->func_chk(*fsm->p_fsm_ev);
                *fsm->p_fsm_ev = 0;
                if (next && next != entry->fsm_sta)
                {
                    entry->func_out();
                    fsm->fsm_sta = next;
                    fsm->fsm_sta_is_change = 1;
                }
            }
            break;
        }
    }
}

static void link_process(section_link_t *link)
{
    if (!link || !link->p_buff || !link->dma_cnt || !link->func_arr)
        return;

    uint32_t buff_size = link->buff_size;
    uint32_t dma_cnt = *link->dma_cnt;
    uint32_t cnt = buff_size - dma_cnt;

    while (link->pos != cnt)
    {
        uint8_t data = (*link->p_buff)[link->pos];
        for (uint32_t i = 0; i < link->func_num; ++i)
        {
            if (link->func_arr[i])
            {
                link->func_arr[i](data, link->my_printf);
            }
        }
        link->pos = (link->pos + 1) % buff_size;
    }
}

void section_link_task(void)
{
    for (section_link_t *p = p_link_first; p != NULL; p = (section_link_t *)p->p_next)
    {
        link_process(p);
    }
}

REG_TASK(10, section_link_task);

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

REG_SHELL_CMD(list, list);
