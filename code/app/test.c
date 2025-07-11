/**
 * @file test.c
 * @brief 测试与演示任务及Scope功能
 * @author Max.Li
 * @version 1.0
 * @date 2025-07-10
 *
 * @details
 * 本文件演示了如何使用Scope模块进行信号采集、Shell命令交互等功能。
 */

#include "test.h"
#include "scope.h"
#include "math.h"
#include "section.h"

// 注册Scope采集对象：采集sin_data和cos_data两个变量，缓冲区256，触发点100
REG_SCOPE(tsm, 256, 128, sin_data, cos_data, sin_cos_data)

// 触发变量，通过Shell写1触发Scope
uint8_t trig = 0;

/**
 * @brief Shell回调：触发Scope采集
 * @param my_printf 打印函数
 * @details
 * 通过Shell设置trig=1后自动触发Scope采集
 */
void test_scope_trig(DEC_MY_PRINTF)
{
    if (trig)
    {
        trig = 0;
        scope_trigger(&scope_tsm);
        my_printf("Trigger set\r\n");
    }
    else
    {
        my_printf("Trigger not set\r\n");
    }
}
// 注册trig变量到Shell，类型为SHELL_UINT8
REG_SHELL_VAR(trig, trig, SHELL_UINT8, NULL)

/**
 * @brief 1ms周期任务，产生50Hz正弦和余弦信号并采集
 * @details
 * - 生成sin_data/cos_data信号
 * - 调用SCOPE(tsm)进行采集
 * - 分时打印Scope数据（如果激活）
 */
void test_task(void)
{
    static uint32_t tick = 0;
    // 1ms周期，50Hz信号，周期T=20ms，步进2*PI/20
    float theta = 2.0f * 3.1415926f * (tick % 13) / 13.0f;
    sin_data = sinf(theta);
    cos_data = cosf(theta);
    sin_cos_data = sin_data * cos_data;
    tick++;

    static float sin_data_last = 0.0f;
    if ((trig == 1) &&
        (sin_data_last * sin_data < 0.0f))
    {
        trig = 0;
        SCOPE_TRIGGER(tsm);
    }
    SCOPE(tsm);
    sin_data_last = sin_data;
}
// 注册test_task为1ms周期定时任务
REG_TASK_MS(1, test_task)

