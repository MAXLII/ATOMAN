#include "perf.h"

#include "section.h"
#include "shell.h"

#include <string.h>

section_perf_record_t *p_perf_record_first = NULL;
static uint32_t *s_perf_cnt = NULL;

static void perf_insert(section_perf_t *perf)
{
    section_perf_base_t *base;
    section_perf_record_t *rec;
    static section_perf_record_t *s_perf_record_tail = NULL;

    if (!perf)
        return;

    switch (perf->perf_type)
    {
    case SECTION_PERF_BASE:
        base = (section_perf_base_t *)perf->p_perf;
        if (base && base->p_cnt)
            s_perf_cnt = base->p_cnt;
        break;

    case SECTION_PERF_RECORD:
        rec = (section_perf_record_t *)perf->p_perf;
        if (rec)
        {
            rec->p_cnt = &s_perf_cnt;
            rec->p_next = NULL;

            if (!p_perf_record_first)
            {
                p_perf_record_first = rec;
                s_perf_record_tail = rec;
            }
            else
            {
                s_perf_record_tail->p_next = rec;
                s_perf_record_tail = rec;
            }
        }
        break;

    default:
        break;
    }
}

static void perf_init(void)
{
    p_perf_record_first = NULL;
    s_perf_cnt = NULL;

    for (reg_section_t *p = (reg_section_t *)&SECTION_START;
         p < (reg_section_t *)&SECTION_STOP;
         ++p)
    {
        switch (p->section_type)
        {
        case SECTION_PERF:
            perf_insert((section_perf_t *)p->p_str);
            break;
        default:
            break;
        }
    }
}

void print_perf_record(DEC_MY_PRINTF)
{
    if (!my_printf)
        return;

    section_perf_record_t *p = p_perf_record_first;
    my_printf->my_printf("Perf Name\tTime(us)\tMax(us)\r\n");
    while (p)
    {
        my_printf->my_printf("%s\t%u\t%u\r\n",
                             p->p_name,
                             (unsigned)(p->time * 0.5f),
                             (unsigned)(p->max_time * 0.5f));
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
        int len = (int)strlen(s->p_name);
        if (len > max_len)
            max_len = len;
    }

    g_perf_print_ctx.perf_max_name_len = (uint32_t)max_len;

    int name_len = (int)strlen("Perf Name");
    int tab_size = 8;
    int tab_count = ((int)g_perf_print_ctx.perf_max_name_len / tab_size) - (name_len / tab_size) + 1;

    my_printf->my_printf("Perf Name");
    for (int i = 0; i < tab_count; i++)
        my_printf->my_printf("\t");
    my_printf->my_printf("Time(us)\tMax(us)\r\n");
}

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

    int name_len = (int)strlen(p->p_name);
    int tab_size = 8;
    int tab_count = ((int)g_perf_print_ctx.perf_max_name_len / tab_size) - (name_len / tab_size) + 1;

    g_perf_print_ctx.my_printf->my_printf("%s", p->p_name);
    for (int i = 0; i < tab_count; i++)
        g_perf_print_ctx.my_printf->my_printf("\t");
    g_perf_print_ctx.my_printf->my_printf("%u\t%u\r\n",
                                          (unsigned)(p->time * 0.5f),
                                          (unsigned)(p->max_time * 0.5f));
    g_perf_print_ctx.cur = (section_perf_record_t *)p->p_next;
    return 1;
}

static void perf_print_task(void)
{
    if (g_perf_print_ctx.active)
        (void)print_perf_record_step();
}

REG_INIT(0, perf_init)
REG_TASK_MS(5, perf_print_task)
REG_SHELL_CMD(perf_print_record, print_perf_record_start)
