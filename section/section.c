#include "section.h"
#include "stddef.h"

#ifdef IS_PLECS

#include "plecs.h"
extern uint32_t plecs_time_100us;
#define SECTION_SYS_TICK plecs_time_100us

extern size_t __start_section;
extern size_t __stop_section;

#define SECTION_START __start_section
#define SECTION_STOP __stop_section

#else

#include "systick.h"

#define SECTION_SYS_TICK systick_gettime_100us()

/* 使用链接脚本中定义的符号 */
extern uint32_t __section_start;
extern uint32_t __section_end;

#define SECTION_START __section_start
#define SECTION_STOP __section_end

#endif

reg_task_t *p_task_first = NULL;
reg_interrupt_t *p_interrupt_first = NULL;
section_shell_t *p_shell_first = NULL;
section_link_t *p_link_first = NULL;

void run_task(void)
{
    uint32_t sys_tisk = SECTION_SYS_TICK; // 获取当前系统时间
    reg_task_t *p_task = p_task_first;    // 获取任务链表的第一个任务

    if (p_task == NULL)
    {
        return; // 如果没有任务，直接返回
    }

    while (p_task != NULL)
    { // 遍历链表，直到链表末尾
        // 检查任务是否到达执行时间
        if (sys_tisk - p_task->time_last >= p_task->t_period)
        {
            p_task->p_func();             // 执行任务
            p_task->time_last = sys_tisk; // 更新任务的上次执行时间
        }

        p_task = (reg_task_t *)p_task->p_next; // 移动到下一个任务
    }
}

void section_init(void)
{
    reg_section_t *p_section = (reg_section_t *)&SECTION_START; // 获取段的起始地址
    reg_task_t *p_task_last = NULL;                             // 上一个任务的指针
    reg_task_t *p_task_now = NULL;                              // 当前任务的指针
    reg_interrupt_t *p_interrupt_now = NULL;                    // 当前中断的指针
    section_shell_t *p_shell_last = NULL;                       // 上一个shell命令的指针
    section_link_t *p_link_last = NULL;                         // 上一个shell命令的指针

    for (; p_section < (reg_section_t *)&SECTION_STOP; p_section++)
    {
        if (p_section->section_type == SECTION_INIT)
        {
            // 如果是初始化段，调用初始化函数
            reg_init_t *p_init = (reg_init_t *)p_section->p_str;
            p_init->p_func();
        }
        else if (p_section->section_type == SECTION_TASK)
        {
            // 如果是任务段，将任务添加到链表中
            p_task_now = (reg_task_t *)p_section->p_str;
            p_task_now->time_last = SECTION_SYS_TICK; // 初始化任务的上次执行时间
            p_task_now->p_next = NULL;                // 新任务的 p_next 初始化为 NULL

            if (p_task_last != NULL)
            {
                p_task_last->p_next = p_task_now; // 将上一个任务的 p_next 指向当前任务
            }
            else
            {
                p_task_first = p_task_now; // 如果是第一个任务，设置为链表头
            }
            p_task_last = p_task_now; // 更新上一个任务的指针
        }
        else if (p_section->section_type == SECTION_INTERRUPT)
        {
            // 如果是中断段，按优先级插入链表
            p_interrupt_now = (reg_interrupt_t *)p_section->p_str;
            p_interrupt_now->p_next = NULL;

            if (p_interrupt_first == NULL || p_interrupt_now->priority < p_interrupt_first->priority)
            {
                p_interrupt_now->p_next = p_interrupt_first;
                p_interrupt_first = p_interrupt_now;
            }
            else
            {
                reg_interrupt_t *p_prev = p_interrupt_first;
                while (p_prev->p_next != NULL && p_prev->p_next->priority <= p_interrupt_now->priority)
                {
                    p_prev = p_prev->p_next;
                }
                p_interrupt_now->p_next = p_prev->p_next;
                p_prev->p_next = p_interrupt_now;
            }
        }
        else if (p_section->section_type == SECTION_SHELL)
        {
            // 如果是shell命令段，将命令添加到链表中
            section_shell_t *p_shell_now = (section_shell_t *)p_section->p_str;
            p_shell_now->p_next = NULL;

            if (p_shell_last != NULL)
            {
                p_shell_last->p_next = p_shell_now; // 将上一个命令的 p_next 指向当前命令
            }
            else
            {
                p_shell_first = p_shell_now; // 如果是第一个命令，设置为链表头
            }
            p_shell_last = p_shell_now; // 更新上一个命令的指针
        }
        else if (p_section->section_type == SECTION_LINK)
        {
            section_link_t *p_link_now = (section_link_t *)p_section->p_str;
            p_link_now->p_next = NULL; // 初始化当前链接的下一个指针
            if (p_link_last != NULL)
            {
                p_link_last->p_next = (void *)p_link_now; // 将上一个链接的 p_next 指向当前链接
            }
            else
            {
                p_link_first = p_link_now; // 如果是第一个链接，设置为链表头
            }
            p_link_last = p_link_now; // 更新上一个链接的指针
        }
    }
}

void section_interrupt(void)
{
    reg_interrupt_t *p_interrupt = p_interrupt_first;
    while (p_interrupt != NULL)
    {
        p_interrupt->p_func();
        p_interrupt = (reg_interrupt_t *)p_interrupt->p_next;
    }
}

void section_fsm_func(reg_fsm_t *str)
{
    if ((str->p_fsm_func_table == NULL) ||
        (str->p_fsm_ev == NULL))
    {
        return; // Exit the function if the table pointer or fsm_event is NULL
    }
    for (uint32_t i = 0; i < str->fsm_table_size; i++)
    {
        reg_fsm_func_t *p_func = &str->p_fsm_func_table[i];
        if (str->fsm_sta == p_func->fsm_sta)
        {
            if (str->fsm_sta_is_change != 0)
            {
                str->fsm_sta_is_change = 0; // 状态机状态改变标志清零
                p_func->func_in();          // 进入状态函数
            }
            p_func->func_exe(); // 执行状态函数
            if (*str->p_fsm_ev != 0)
            {
                uint32_t next_state = p_func->func_chk(*str->p_fsm_ev); // 检查状态函数
                *str->p_fsm_ev = 0;
                if ((next_state != p_func->fsm_sta) &&
                    (next_state != 0))
                {
                    p_func->func_out();         // 退出状态函数
                    str->fsm_sta = next_state;  // 更新状态机状态
                    str->fsm_sta_is_change = 1; // 状态机状态改变标志置位
                }
            }
            break;
        }
    }
}

void section_link_func(section_link_t *p_str)
{
    uint32_t dma_up_cnt = 0;
    dma_up_cnt = p_str->buff_size - *p_str->dma_cnt; // 计算DMA剩余数据量
    while (p_str->pos != dma_up_cnt)
    {
        for (uint32_t i = 0; i < p_str->func_num; i++)
        {
            if (p_str->func_arr[i] != NULL)
            {
                p_str->func_arr[i]((*p_str->p_buff)[p_str->pos], p_str->my_printf); // 调用函数指针数组中的函数
            }
        }
        p_str->pos++; // 移动到下一个数据位置
        if (p_str->pos >= p_str->buff_size)
        {
            p_str->pos = 0; // 如果到达缓冲区末尾，重置位置
        }
    }
}

void section_link_task(void)
{
    if (p_link_first == NULL)
    {
        return; // 如果没有链接段，直接返回
    }
    section_link_t *p_str = p_link_first; // 获取链接段的第一个元素
    while (p_str != NULL)
    {
        section_link_func(p_str);                // 执行链接段的函数
        p_str = (section_link_t *)p_str->p_next; // 移动到下一个链接段
    }
}

REG_TASK(10, section_link_task);

uint8_t shell_buffer[128];
uint8_t shell_index = 0;

#include "string.h"

void shell_run(char data, DEC_MY_PRINTF)
{
    uint8_t err = 0;
    uint8_t u8_temp = 0;
    uint16_t u16_temp = 0;
    uint32_t u32_temp = 0;
    int8_t i8_temp = 0;
    int16_t i16_temp = 0;
    int32_t i32_temp = 0;
    uint8_t data_is_negedge = 0;
    float f32_temp = 0.0f;
    float f32_integer = 0.0f;
    float f32_decimal = 0.0f;
    uint8_t is_decimal = 0;

    shell_buffer[shell_index++] = data;
    if (shell_index >= 128)
    {
        shell_index = 0; // Reset index if buffer is full
    }
    if ((data == '\n') &&
        (shell_index > 0) &&
        (shell_buffer[shell_index - 2] == '\r')) // 检测回车
    {
        err = 1;
        section_shell_t *p_shell = p_shell_first; // 获取第一个shell命令
        while (p_shell != NULL)
        {
            uint32_t is_cmp = 0;
            for (uint32_t i = 0; i < shell_index; i++)
            {
                if (i == p_shell->p_name_size)
                {
                    if ((shell_buffer[i] == ':') ||
                        (shell_buffer[i] == '\r'))
                    {
                        is_cmp = 1;
                        switch (p_shell->type)
                        {
                        case SHELL_CMD:
                            if (p_shell->func != NULL)
                            {
                                my_printf("Executing %s\r\n", p_shell->p_name);
                                p_shell->func(my_printf);
                            }
                            break;
                        case SHELL_UINT8:
                            u8_temp = 0;
                            for (; i < shell_index - 2; i++)
                            {
                                u8_temp *= 10;
                                if ((shell_buffer[i] >= '0') &&
                                    (shell_buffer[i] <= '9'))
                                {
                                    err = 0;
                                    u8_temp += shell_buffer[i] - '0';
                                }
                                else
                                {
                                    err = 1;
                                }
                            }
                            if (err == 0)
                            {
                                memcpy((void *)p_shell->p_var, &u8_temp, sizeof(uint8_t));
                                my_printf("%s = %d\r\n", p_shell->p_name, *(uint8_t *)p_shell->p_var);
                            }
                            else
                            {
                                my_printf("%s = %d\r\n", p_shell->p_name, *(uint8_t *)p_shell->p_var);
                            }
                            if (p_shell->func != NULL)
                            {
                                my_printf("Executing %s\r\n", p_shell->p_name);
                                p_shell->func(my_printf);
                            }
                            break;
                        case SHELL_UINT16:
                            u16_temp = 0;
                            for (; i < shell_index - 2; i++)
                            {
                                u16_temp *= 10;
                                if ((shell_buffer[i] >= '0') &&
                                    (shell_buffer[i] <= '9'))
                                {
                                    err = 0;
                                    u16_temp += shell_buffer[i] - '0';
                                }
                                else
                                {
                                    err = 1;
                                }
                            }
                            if (err == 0)
                            {
                                memcpy((void *)p_shell->p_var, &u16_temp, sizeof(uint16_t));
                                my_printf("%s = %d\r\n", p_shell->p_name, *(uint16_t *)p_shell->p_var);
                            }
                            else
                            {
                                my_printf("%s = %d\r\n", p_shell->p_name, *(uint16_t *)p_shell->p_var);
                            }
                            if (p_shell->func != NULL)
                            {
                                my_printf("Executing %s\r\n", p_shell->p_name);
                                p_shell->func(my_printf);
                            }
                            break;
                        case SHELL_UINT32:
                            u32_temp = 0;
                            for (; i < shell_index - 2; i++)
                            {
                                u32_temp *= 10;
                                if ((shell_buffer[i] >= '0') &&
                                    (shell_buffer[i] <= '9'))
                                {
                                    err = 0;
                                    u32_temp += shell_buffer[i] - '0';
                                }
                                else
                                {
                                    err = 1;
                                }
                            }
                            if (err == 0)
                            {
                                memcpy((void *)p_shell->p_var, &u32_temp, sizeof(uint32_t));
                                my_printf("%s = %d\r\n", p_shell->p_name, *(uint32_t *)p_shell->p_var);
                            }
                            else
                            {
                                my_printf("%s = %d\r\n", p_shell->p_name, *(uint32_t *)p_shell->p_var);
                            }
                            if (p_shell->func != NULL)
                            {
                                my_printf("Executing %s\r\n", p_shell->p_name);
                                p_shell->func(my_printf);
                            }
                            break;
                        case SHELL_INT8:
                            i8_temp = 0;
                            for (; i < shell_index - 2; i++)
                            {
                                i8_temp *= 10;
                                if ((shell_buffer[i] >= '0') &&
                                    (shell_buffer[i] <= '9'))
                                {
                                    err = 0;
                                    i8_temp += shell_buffer[i] - '0';
                                }
                                else if ((shell_buffer[i] == '-') &&
                                         (data_is_negedge == 0))
                                {
                                    data_is_negedge = 1;
                                }
                                else
                                {
                                    err = 1;
                                }
                            }
                            if (err == 0)
                            {
                                if (data_is_negedge == 1)
                                {
                                    i8_temp = -i8_temp;
                                }
                                memcpy((void *)p_shell->p_var, &i8_temp, sizeof(int8_t));
                                my_printf("%s = %d\r\n", p_shell->p_name, *(int8_t *)p_shell->p_var);
                            }
                            else
                            {
                                my_printf("%s = %d\r\n", p_shell->p_name, *(int8_t *)p_shell->p_var);
                            }
                            if (p_shell->func != NULL)
                            {
                                my_printf("Executing %s\r\n", p_shell->p_name);
                                p_shell->func(my_printf);
                            }
                            break;
                        case SHELL_INT16:
                            i16_temp = 0;
                            for (; i < shell_index - 2; i++)
                            {
                                i16_temp *= 10;
                                if ((shell_buffer[i] >= '0') &&
                                    (shell_buffer[i] <= '9'))
                                {
                                    err = 0;
                                    i16_temp += shell_buffer[i] - '0';
                                }
                                else if ((shell_buffer[i] == '-') &&
                                         (data_is_negedge == 0))
                                {
                                    data_is_negedge = 1;
                                }
                                else
                                {
                                    err = 1;
                                }
                            }
                            if (err == 0)
                            {
                                if (data_is_negedge == 1)
                                {
                                    i16_temp = -i16_temp;
                                }
                                memcpy((void *)p_shell->p_var, &i16_temp, sizeof(int16_t));
                                my_printf("%s = %d\r\n", p_shell->p_name, *(int16_t *)p_shell->p_var);
                            }
                            else
                            {
                                my_printf("%s = %d\r\n", p_shell->p_name, *(int16_t *)p_shell->p_var);
                            }
                            if (p_shell->func != NULL)
                            {
                                my_printf("Executing %s\r\n", p_shell->p_name);
                                p_shell->func(my_printf);
                            }
                            break;
                        case SHELL_INT32:
                            i32_temp = 0;
                            for (; i < shell_index - 2; i++)
                            {
                                i32_temp *= 10;
                                if ((shell_buffer[i] >= '0') &&
                                    (shell_buffer[i] <= '9'))
                                {
                                    err = 0;
                                    i32_temp += shell_buffer[i] - '0';
                                }
                                else if ((shell_buffer[i] == '-') &&
                                         (data_is_negedge == 0))
                                {
                                    data_is_negedge = 1;
                                }
                                else
                                {
                                    err = 1;
                                    break;
                                }
                            }
                            if (err == 0)
                            {
                                if (data_is_negedge == 1)
                                {
                                    i32_temp = -i32_temp;
                                }
                                memcpy((void *)p_shell->p_var, &i32_temp, sizeof(int32_t));
                                my_printf("%s = %d\r\n", p_shell->p_name, *(int32_t *)p_shell->p_var);
                            }
                            else
                            {
                                my_printf("%s = %d\r\n", p_shell->p_name, *(int32_t *)p_shell->p_var);
                            }
                            if (p_shell->func != NULL)
                            {
                                my_printf("Executing %s\r\n", p_shell->p_name);
                                p_shell->func(my_printf);
                            }
                            break;
                        case SHELL_FP32:
                            i++;
                            f32_temp = 0.0f;
                            f32_decimal = 1.0f;
                            f32_integer = 0.0f;
                            is_decimal = 0;
                            for (; i < shell_index - 2; i++)
                            {
                                if ((shell_buffer[i] >= '0') &&
                                    (shell_buffer[i] <= '9'))
                                {
                                    err = 0;
                                    f32_integer *= 10.0f;
                                    f32_integer += shell_buffer[i] - '0';
                                    if (is_decimal == 1)
                                    {
                                        f32_decimal /= 10.0f;
                                    }
                                }
                                else if ((shell_buffer[i] == '-') &&
                                         (data_is_negedge == 0))
                                {
                                    data_is_negedge = 1;
                                }
                                else if ((shell_buffer[i] == '.') &&
                                         (is_decimal == 0))
                                {
                                    is_decimal = 1;
                                }
                                else
                                {
                                    err = 1;
                                    break;
                                }
                            }
                            f32_temp = f32_integer * f32_decimal;
                            if (data_is_negedge == 1)
                            {
                                f32_temp = -f32_temp;
                            }
                            if (err == 0)
                            {
                                memcpy((void *)p_shell->p_var, &f32_temp, sizeof(float));
                                my_printf("%s = %f\r\n", p_shell->p_name, *(float *)p_shell->p_var);
                            }
                            else
                            {
                                my_printf("%s = %f\r\n", p_shell->p_name, *(float *)p_shell->p_var);
                            }
                            if (p_shell->func != NULL)
                            {
                                my_printf("Executing %s\r\n", p_shell->p_name);
                                p_shell->func(my_printf);
                            }
                            break;
                        }
                    }
                }
                else
                {
                    if (shell_buffer[i] != p_shell->p_name[i])
                    {
                        break; // 如果字符不匹配，跳出循环
                    }
                }
            }
            if (is_cmp == 1)
            {
                break;
            }
            p_shell = p_shell->p_next;
        }
        if (p_shell == NULL)
        {
            my_printf("Unknown command\r\n"); // 如果没有匹配的命令，打印错误信息
        }
        memset(shell_buffer, 0, sizeof(shell_buffer));
        shell_index = 0;
    }
}

static void list(DEC_MY_PRINTF)
{
    section_shell_t *p_shell = p_shell_first; // 获取第一个shell命令
    while (p_shell != NULL)
    {
        my_printf("%s\t", p_shell->p_name);
        switch (p_shell->type)
        {
        case SHELL_CMD:
            my_printf("SHELL_CMD\r\n");
            break;
        case SHELL_UINT8:
            my_printf("SHELL_DATA_UINT8_T\t");
            my_printf("%d\r\n", *(uint8_t *)p_shell->p_var);
            break;
        case SHELL_UINT16:
            my_printf("SHELL_DATA_UINT16_T\t");
            my_printf("%d\r\n", *(uint16_t *)p_shell->p_var);
            break;
        case SHELL_UINT32:
            my_printf("SHELL_DATA_UINT32_T\t");
            my_printf("%d\r\n", *(uint32_t *)p_shell->p_var);
            break;
        case SHELL_INT8:
            my_printf("SHELL_DATA_INT8_T\t");
            my_printf("%d\r\n", *(int8_t *)p_shell->p_var);
            break;
        case SHELL_INT16:
            my_printf("SHELL_DATA_INT16_T\t");
            my_printf("%d\r\n", *(int16_t *)p_shell->p_var);
            break;
        case SHELL_INT32:
            my_printf("SHELL_DATA_INT32_T\t");
            my_printf("%d\r\n", *(int32_t *)p_shell->p_var);
            break;
        case SHELL_FP32:
            my_printf("SHELL_DATA_FLOAT\t");
            my_printf("%f\r\n", *(float *)p_shell->p_var);
            break;
        }
        p_shell = p_shell->p_next; // 移动到下一个命令
    }
}

REG_SHELL_CMD(list, list);
