/* @file    apm32f402_403_int.c
 * @brief   PFC项目中断服务程序文件.
 * @details
 *          This file is part of the PFC project.
 *
 *          Module responsibilities:
 *          - 实现Cortex-M4异常处理函数
 *          - 定义SysTick中断处理
 *          - 提供系统时间计数功能
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - 异常处理进入死循环（系统故障）
 *          - systick_cnt用于系统时间计数
 *
 * @author  Max.Li
 * @date    2024-03-27
 * @version 1.0.0
 */

/* 包含头文件 */
#include "main.h"
#include "apm32f402_403_int.h"

/* 系统滴答计数器
 * 用于提供系统时间基准
 * 每10ms递增一次（由SysTick中断驱动） */
uint32_t systick_cnt = 0;
volatile uint32_t sys_tick_100us = 0;

/* 获取系统时间（100微秒为单位）
 * 返回：当前系统时间（100微秒单位）
 *
 * 功能说明：
 * - 读取systick_cnt的值
 * - 用于需要时间基准的模块（如trace） */
uint32_t systick_gettime_100us(void)
{
    return sys_tick_100us;
}

/* NMI异常处理函数
 * 功能说明：
 * - 不可屏蔽的中断
 * - 当前实现为空（死循环） */
void NMI_Handler(void)
{
}

/* 硬错误异常处理函数
 * 功能说明：
 * - 发生严重系统错误时触发
 * - 进入死循环，等待看门狗复位或调试 */
void HardFault_Handler(void)
{
    while (1)
    {
    }
}

/* 存储管理异常处理函数
 * 功能说明：
 * - 内存访问违规时触发
 * - 进入死循环 */
void MemManage_Handler(void)
{
    while (1)
    {
    }
}

/* 总线错误异常处理函数
 * 功能说明：
 * - 总线访问错误时触发
 * - 进入死循环 */
void BusFault_Handler(void)
{
    while (1)
    {
    }
}

/* 用法错误异常处理函数
 * 功能说明：
 * - 程序错误使用（如除零）时触发
 * - 进入死循环 */
void UsageFault_Handler(void)
{
    while (1)
    {
    }
}

/* SVCall异常处理函数
 * 功能说明：
 * - 系统服务调用
 * - 当前实现为空 */
void SVC_Handler(void)
{
}

/* 调试监控异常处理函数
 * 功能说明：
 * - 调试器使用
 * - 当前实现为空 */
void DebugMon_Handler(void)
{
}

/* PendSV异常处理函数
 * 功能说明：
 * - 可悬挂的系统调用
 * - 当前实现为空 */
void PendSV_Handler(void)
{
}

/* SysTick中断处理函数
 * 功能说明：
 * - 系统定时器中断
 * - 每0.1ms触发一次
 * - 递增systick_cnt提供时间基准 */
void SysTick_Handler(void)
{
    systick_cnt++;
    sys_tick_100us++;
}
