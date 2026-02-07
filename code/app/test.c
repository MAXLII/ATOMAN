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
#include "comm.h"
#include "shell.h"

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
        my_printf->my_printf("Trigger set\r\n");
    }
    else
    {
        my_printf->my_printf("Trigger not set\r\n");
    }
}
// 注册trig变量到Shell，类型为SHELL_UINT8
REG_SHELL_VAR(trig, trig, SHELL_UINT8, 0xFF, 0, NULL, SHELL_STA_NULL)

REG_PERF_RECORD(tsmtime)
REG_PERF_RECORD(scope)

/**
 * @brief 1ms周期任务，产生50Hz正弦和余弦信号并采集
 * @details
 * - 生成sin_data/cos_data信号
 * - 调用SCOPE(tsm)进行采集
 * - 分时打印Scope数据（如果激活）
 */
void test_task(void)
{
    PERF_START(tsmtime);
    static uint32_t tick = 0;
    // 1ms周期，50Hz信号，周期T=20ms，步进2*PI/20
    float theta = 2.0f * 3.1415926f * (tick % 13) / 13.0f;
    sin_data = sinf(theta);
    cos_data = cosf(theta);
    sin_cos_data = sin_data * cos_data;
    tick++;

    PERF_START(scope);
    static float sin_data_last = 0.0f;
    if ((trig == 1) &&
        (sin_data_last * sin_data < 0.0f))
    {
        trig = 0;
        SCOPE_TRIGGER(tsm);
    }
    SCOPE_RUN(tsm);
    PERF_END(scope);
    sin_data_last = sin_data;
    PERF_END(tsmtime);
}
// 注册test_task为1ms周期定时任务
REG_TASK_MS(1, test_task)

test_value_t test_value = {
    .fp32 = 3.1415926f,
    .u8 = 0xAB,
    .u16 = 0x1234,
    .i16 = -1234,
    .i32 = -12345678,
    .i8 = -12,
    .u32 = 0xFFFFFFFF};

/**
 * @brief Shell回调：打印测试值
 * @param my_printf 打印函数
 * @details
 * 通过Shell命令打印test_value的所有字段
 */
REG_SHELL_VAR(test_value_fp32, test_value.fp32, SHELL_FP32, 1000.0f, -1000.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(test_value_u8, test_value.u8, SHELL_UINT8, 0xFF, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(test_value_u16, test_value.u16, SHELL_UINT16, 0xFFFF, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(test_value_i16, test_value.i16, SHELL_INT16, 32767, -32768, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(test_value_i32, test_value.i32, SHELL_INT32, 2147483647, -2147483648, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(test_value_i8, test_value.i8, SHELL_INT8, 127, -128, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(test_value_u32, test_value.u32, SHELL_UINT32, 0xFFFFFFFFU, 0U, NULL, SHELL_STA_NULL)

void test_comm(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    if (p_pack->len != sizeof(test_value_t))
    {
        my_printf->my_printf("Invalid data length: %d\r\n", p_pack->len);
        return;
    }
    my_printf->my_printf("oo\r\n");
    test_value_t *p_value = (test_value_t *)p_pack->p_data;
    test_value.fp32 = p_value->fp32;
    test_value.u8 = p_value->u8;
    test_value.u16 = p_value->u16;
    test_value.i16 = p_value->i16;
    test_value.i32 = p_value->i32;
    test_value.i8 = p_value->i8;
    test_value.u32 = p_value->u32;
    comm_send_data(p_pack, my_printf);
}

REG_COMM(0x22, 0x01, test_comm)
