// SPDX-License-Identifier: MIT
/**
 * @file    platform_probe.c
 * @brief   Zynq-7020 section platform runtime probe.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Confirm section-based initialization through a UART banner
 *          - Confirm periodic task scheduling with a software counter
 *          - Verify the PL 3P3Z IIR peripheral and expose Shell diagnostics
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The periodic task does not access hardware or block
 *          - Output is routed through the shared UART communication link
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_iir.h"
#include "bsp_oled.h"
#include "bsp_timer.h"
#include "bsp_usart.h"
#include "comm.h"
#include "comm_addr.h"
#include "comm_link.h"
#include "demo.h"
#include "perf.h"
#include "scope.h"
#include "section.h"
#include "sfra_service.h"
#include "shell.h"
#include "trace.h"
#include "xstatus.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint32_t s_probe_task_count = 0U; /* 100 ms 平台探针任务累计执行次数。 */
static uint8_t s_self_test_reported = 0U; /* 平台自主测试报告已输出标志。 */
static uint32_t s_probe_start_tick = 0U;  /* section 初始化完成时的 100us 启动基准。 */

#define PLATFORM_SELF_TEST_FRAME_SIZE 64U  /* 通信回环测试帧缓冲区容量。 */
#define PLATFORM_SELF_TEST_TEXT_SIZE 128U  /* Shell 回环测试文本缓冲区容量。 */
#define PLATFORM_SELF_TEST_COMM_SOP 0xE8U  /* Comm 协议帧起始字节。 */
#define PLATFORM_PL_DMA_TEST_BYTES 4096U    /* Board internal-loopback test size. */
#define PLATFORM_PL_DMA_TIMEOUT_TICKS 30000U /* Three-second test timeout. */
#define PLATFORM_OLED_TEST_FRAMES 1000U /* Manual OLED DMA frames per board test. */

DECLARE_COMM_CTX(s_self_test_comm_ctx, sizeof(demo_comm_frame_t), HOST_ADDR, USART0_LINK);
DECLARE_SHELL_CTX(s_self_test_shell_ctx);

static uint8_t s_self_test_request[PLATFORM_SELF_TEST_FRAME_SIZE]; /* 编码后的通信请求帧。 */
static uint8_t s_self_test_ack[PLATFORM_SELF_TEST_FRAME_SIZE];     /* Demo handler 返回的 ACK 帧。 */
static uint16_t s_self_test_request_len = 0U;                      /* 通信请求帧实际长度。 */
static uint16_t s_self_test_ack_len = 0U;                          /* ACK 帧实际长度。 */
static uint8_t s_self_test_capture_ack = 0U;                       /* 当前发送捕获目标选择标志。 */
static char s_self_test_text[PLATFORM_SELF_TEST_TEXT_SIZE];        /* Shell 命令格式化输出。 */

static uint8_t pl_dma_loopback_test(bsp_usart_pl_status_t *final_status)
{
    bsp_usart_pl_config_t loopback_config = {
        .baud_rate = 921600U,
        .data_bits = 8U,
        .parity = BSP_USART_PL_PARITY_NONE,
        .stop_bits = 1U,
        .internal_loopback = 1U,
    }; /* Internal-loopback test configuration. */
    bsp_usart_pl_config_t physical_config = {
        .baud_rate = 921600U,
        .data_bits = 8U,
        .parity = BSP_USART_PL_PARITY_NONE,
        .stop_bits = 1U,
        .internal_loopback = 0U,
    }; /* Normal COM7 configuration restored after the test. */
    uint8_t tx_buffer[256]; /* One deterministic PRBS transmit block. */
    uint32_t tx_lfsr = 0x13579BDFU; /* Transmit PRBS state. */
    uint32_t rx_lfsr = 0x13579BDFU; /* Expected receive PRBS state. */
    uint32_t sent = 0U;             /* Bytes published to the TX ring. */
    uint32_t received = 0U;         /* Bytes consumed from the RX ring. */
    uint32_t start_tick = 0U;       /* Test timeout origin. */
    uint8_t passed = 1U;            /* Aggregate loopback result. */

    comm_link_usart1_suspend(1U); /* Give the diagnostic exclusive ownership of the PL UART. */
    if ((bsp_usart_pl_reset() != 0) ||
        (bsp_usart_pl_configure(&loopback_config) != 0))
    {
        passed = 0U;
    }

    while ((passed != 0U) && (sent < PLATFORM_PL_DMA_TEST_BYTES))
    {
        uint32_t block_length = PLATFORM_PL_DMA_TEST_BYTES - sent;
        uint32_t index = 0U;

        if (block_length > (uint32_t)sizeof(tx_buffer))
        {
            block_length = (uint32_t)sizeof(tx_buffer);
        }
        for (index = 0U; index < block_length; ++index)
        {
            tx_lfsr = (tx_lfsr >> 1U) ^
                      ((0U - (tx_lfsr & 1U)) & 0xD0000001U);
            tx_buffer[index] = (uint8_t)tx_lfsr;
        }
        if (bsp_usart_pl_tx_dma(tx_buffer, block_length) != 0)
        {
            passed = 0U;
        }
        sent += block_length;
    }

    start_tick = bsp_timer_gettime_100us();
    while ((passed != 0U) && (received < PLATFORM_PL_DMA_TEST_BYTES))
    {
        uint8_t actual = 0U;

        if (bsp_usart_pl_rx_get_byte(&actual) != 0U)
        {
            rx_lfsr = (rx_lfsr >> 1U) ^
                      ((0U - (rx_lfsr & 1U)) & 0xD0000001U);
            if (actual != (uint8_t)rx_lfsr)
            {
                passed = 0U;
            }
            received++;
        }
        else if ((bsp_timer_gettime_100us() - start_tick) >=
                 PLATFORM_PL_DMA_TIMEOUT_TICKS)
        {
            passed = 0U;
        }
    }

    bsp_usart_pl_status_get(final_status);
    if ((final_status->error_irq_count != 0U) ||
        (final_status->irq_status != 0U) ||
        (final_status->uart_errors != 0U) ||
        (final_status->dma_errors != 0U) ||
        (final_status->dma_stop_reason != 0U) ||
        (final_status->rx_produced != PLATFORM_PL_DMA_TEST_BYTES) ||
        (final_status->rx_consumed != PLATFORM_PL_DMA_TEST_BYTES) ||
        (final_status->tx_produced != PLATFORM_PL_DMA_TEST_BYTES) ||
        (final_status->tx_consumed != PLATFORM_PL_DMA_TEST_BYTES))
    {
        passed = 0U;
    }

    if ((bsp_usart_pl_configure(&physical_config) != 0) ||
        (bsp_usart_pl_reset() != 0))
    {
        passed = 0U;
    }
    comm_link_usart1_suspend(0U); /* Restore normal COM7 Shell and protocol processing. */

    return passed;
}

static void self_test_tx_capture(char *data, int length)
{
    uint8_t *destination = s_self_test_request; /* 当前帧捕获目标缓冲区。 */
    uint16_t *captured_length = &s_self_test_request_len; /* 当前帧捕获长度地址。 */
    uint16_t copy_length = 0U; /* 经容量约束后的帧复制长度。 */

    if ((data == NULL) || /* 发送回调未提供有效帧地址。 */
        (length <= 0))    /* 发送回调未提供有效帧长度。 */
    {
        return;
    }

    if (s_self_test_capture_ack == 1U)
    {
        destination = s_self_test_ack;
        captured_length = &s_self_test_ack_len;
    }

    copy_length = ((uint32_t)length > PLATFORM_SELF_TEST_FRAME_SIZE) ?
                      (uint16_t)PLATFORM_SELF_TEST_FRAME_SIZE :
                      (uint16_t)length;
    (void)memcpy(destination, data, copy_length);
    *captured_length = copy_length;
}

static void self_test_printf(const char *format, ...)
{
    va_list arguments; /* Shell 命令格式化参数。 */
    int length = 0;    /* Shell 命令生成的文本长度。 */

    if (format == NULL)
    {
        return;
    }

    va_start(arguments, format);
    length = vsnprintf(s_self_test_text, sizeof(s_self_test_text), format, arguments);
    va_end(arguments);

    if (length < 0)
    {
        s_self_test_text[0] = '\0';
    }
}

static section_link_tx_func_t s_self_test_link = {
    .my_printf = self_test_printf,
    .tx_by_dma = self_test_tx_capture,
}; /* 软件回环测试使用的虚拟通信链路。 */

static uint8_t communication_loopback_test(void)
{
    demo_comm_frame_t payload = {
        .counter = 0x12345678UL,
        .led_mask = 0x5AU,
        .temperature_x10 = -125,
    }; /* Demo 通信命令的已知测试 payload。 */
    section_packform_t request = {
        .src = PC_ADDR,
        .d_src = 0U,
        .dst = HOST_ADDR,
        .d_dst = 0U,
        .cmd_set = DEMO_CMD_SET_LOOPBACK,
        .cmd_word = DEMO_CMD_WORD_LOOPBACK,
        .is_ack = 0U,
        .len = (uint16_t)sizeof(payload),
        .p_data = (uint8_t *)&payload,
    }; /* 交给 comm_send_data 编码的 loopback 请求。 */
    uint16_t expected_length = (uint16_t)(15U + (uint16_t)sizeof(payload)); /* 请求与 ACK 的协议帧长度。 */
    uint16_t index = 0U; /* 当前注入 comm 解析器的字节索引。 */

    s_self_test_request_len = 0U;
    s_self_test_ack_len = 0U;
    s_self_test_capture_ack = 0U;
    (void)memset(s_self_test_request, 0, sizeof(s_self_test_request));
    (void)memset(s_self_test_ack, 0, sizeof(s_self_test_ack));

    comm_send_data(&request, &s_self_test_link); /* 生成带 CRC 和 EOP 的真实协议请求帧。 */
    if (s_self_test_request_len != expected_length)
    {
        return 0U;
    }

    s_self_test_capture_ack = 1U;
    for (index = 0U; index < s_self_test_request_len; ++index)
    {
        comm_run(s_self_test_request[index], &s_self_test_link, &s_self_test_comm_ctx); /* 逐字节走真实协议解析器。 */
    }

    if (s_self_test_ack_len != expected_length)
    {
        return 0U;
    }
    if (s_self_test_ack[0] != PLATFORM_SELF_TEST_COMM_SOP)
    {
        return 0U;
    }
    if (s_self_test_ack[6] != DEMO_CMD_SET_LOOPBACK)
    {
        return 0U;
    }
    if (s_self_test_ack[7] != DEMO_CMD_WORD_LOOPBACK)
    {
        return 0U;
    }
    if (s_self_test_ack[8] != 1U)
    {
        return 0U;
    }
    if (memcmp(&s_self_test_ack[11], &payload, sizeof(payload)) != 0)
    {
        return 0U;
    }

    return 1U;
}

static uint8_t shell_loopback_test(void)
{
    static const uint8_t command[] = "DEMO_SHELL_PING\r\n"; /* 注入 Shell 解析器的测试命令。 */
    uint32_t index = 0U; /* 当前注入 Shell 解析器的字符索引。 */

    (void)memset(s_self_test_text, 0, sizeof(s_self_test_text));
    for (index = 0U; index < (uint32_t)(sizeof(command) - 1U); ++index)
    {
        shell_run(command[index], &s_self_test_link, &s_self_test_shell_ctx); /* 逐字节走真实 Shell 命令路径。 */
    }

    return (strstr(s_self_test_text, "shell ping:") != NULL) ? 1U : 0U;
}

typedef struct
{
    uint32_t init_count;      /* SECTION_INIT 注册项数量。 */
    uint32_t task_count;      /* SECTION_TASK 注册项数量。 */
    uint32_t interrupt_count; /* SECTION_INTERRUPT 注册项数量。 */
    uint32_t shell_count;     /* SECTION_SHELL 注册项数量。 */
    uint32_t link_count;      /* SECTION_LINK 注册项数量。 */
    uint32_t perf_count;      /* SECTION_PERF 注册项数量。 */
    uint32_t comm_count;      /* SECTION_COMM 注册项数量。 */
    uint32_t scope_count;     /* SECTION_SCOPE 注册项数量。 */
    uint32_t sfra_count;      /* SECTION_SFRA 注册项数量。 */
} platform_registry_counts_t;

static platform_registry_counts_t registry_counts_get(void)
{
    platform_registry_counts_t counts = {0}; /* 当前固件链接段内各类注册项计数。 */
    const reg_section_t *entry = (const reg_section_t *)&SECTION_START; /* 当前扫描的注册描述符。 */
    const reg_section_t *end = (const reg_section_t *)&SECTION_STOP;    /* 注册描述符扫描结束地址。 */

    for (; entry < end; ++entry)
    {
        switch ((SECTION_E)entry->section_type)
        {
        case SECTION_INIT:
            counts.init_count++;
            break;
        case SECTION_TASK:
            counts.task_count++;
            break;
        case SECTION_INTERRUPT:
            counts.interrupt_count++;
            break;
        case SECTION_SHELL:
            counts.shell_count++;
            break;
        case SECTION_LINK:
            counts.link_count++;
            break;
        case SECTION_PERF:
            counts.perf_count++;
            break;
        case SECTION_COMM:
        case SECTION_COMM_ROUTE:
            counts.comm_count++;
            break;
        case SECTION_SCOPE:
            counts.scope_count++;
            break;
        case SECTION_SFRA:
            counts.sfra_count++;
            break;
        default:
            break;
        }
    }

    return counts;
}

static uint8_t interrupt_perf_has_run(void)
{
    section_item_t *p_item = p_perf_first; /* 当前扫描的 Perf 链表节点。 */

    while (p_item != NULL)
    {
        section_perf_record_t *record = perf_record_from_item(p_item);

        if ((record != NULL) &&
            (record->record_type == SECTION_PERF_RECORD_INTERRUPT) && /* 当前记录属于中断回调。 */
            (record->run_time > 0U))                                  /* 当前中断回调已经累计运行时间。 */
        {
            return 1U;
        }
        p_item = p_item->p_next;
    }

    return 0U;
}

static void platform_self_test_report(void)
{
    platform_registry_counts_t counts = registry_counts_get(); /* 固件链接段注册项统计。 */
    uint8_t passed = 1U;                                        /* 平台组件自主测试总结果。 */
    uint8_t perf_ready = perf_base_is_ready();                  /* 全局计时器 Perf 基准可用标志。 */
    uint8_t interrupt_ran = interrupt_perf_has_run();            /* SECTION_INTERRUPT 已运行标志。 */
    uint8_t comm_loop_passed = communication_loopback_test();    /* 通信编码、CRC、解析、handler 与 ACK 回环结果。 */
    uint8_t shell_loop_passed = shell_loopback_test();           /* Shell 字节解析与命令执行回环结果。 */
    uint8_t iir_passed = bsp_iir_self_test(NULL);                /* 3P3Z 运算、历史和饱和自测结果。 */
    bsp_usart_pl_status_t pl_dma_status = {0};                    /* PL UART loopback DMA evidence. */
    uint8_t pl_dma_passed = pl_dma_loopback_test(&pl_dma_status); /* PL UART autonomous ring test. */
    uint32_t scheduler_started = section_task_scheduler_started(); /* 当前 section 调度器运行状态。 */
    uint32_t shell_items = shell_count_get();                    /* Shell 命令与变量数量。 */
    uint32_t trace_records = dbg_trace_record_count_get();       /* Demo 已产生的 Trace 记录数量。 */

    if (s_probe_task_count == 0U)
    {
        passed = 0U;
    }
    if (counts.task_count == 0U)
    {
        passed = 0U;
    }
    if (counts.interrupt_count == 0U)
    {
        passed = 0U;
    }
    if (interrupt_ran == 0U)
    {
        passed = 0U;
    }
    if (shell_items == 0U)
    {
        passed = 0U;
    }
    if (counts.comm_count == 0U)
    {
        passed = 0U;
    }
    if (comm_loop_passed == 0U)
    {
        passed = 0U;
    }
    if (shell_loop_passed == 0U)
    {
        passed = 0U;
    }
    if (iir_passed == 0U)
    {
        passed = 0U;
    }
    if (pl_dma_passed == 0U)
    {
        passed = 0U;
    }
    if ((section_runtime_preemptive_get() == 1U) && /* 当前选择的是抢占式 section 实现。 */
        (scheduler_started == 0U))                  /* 抢占调度器尚未成功接管任务现场。 */
    {
        passed = 0U;
    }
    if (perf_ready == 0U)
    {
        passed = 0U;
    }
    if (p_scope_first == NULL)
    {
        passed = 0U;
    }
    if (p_sfra_first == NULL)
    {
        passed = 0U;
    }
    if (trace_records == 0U)
    {
        passed = 0U;
    }

    bsp_usart_dbg_printf("[SELFTEST] mode=%s result=%s reg(init=%lu task=%lu irq=%lu shell=%lu link=%lu perf=%lu comm=%lu scope=%lu sfra=%lu) runtime(task=%lu irq=%u shell=%lu perf=%u scope=%u sfra=%u trace=%lu comm_loop=%u shell_loop=%u srtos=%lu iir=%u pl_dma=%u/%lu)\r\n",
                         section_runtime_name_get(),
                         (passed == 1U) ? "PASS" : "FAIL",
                         (unsigned long)counts.init_count,
                         (unsigned long)counts.task_count,
                         (unsigned long)counts.interrupt_count,
                         (unsigned long)counts.shell_count,
                         (unsigned long)counts.link_count,
                         (unsigned long)counts.perf_count,
                         (unsigned long)counts.comm_count,
                         (unsigned long)counts.scope_count,
                         (unsigned long)counts.sfra_count,
                         (unsigned long)s_probe_task_count,
                         (unsigned)interrupt_ran,
                         (unsigned long)shell_items,
                         (unsigned)perf_ready,
                         (p_scope_first != NULL) ? 1U : 0U,
                         (p_sfra_first != NULL) ? 1U : 0U,
                         (unsigned long)trace_records,
                         (unsigned)comm_loop_passed,
                         (unsigned)shell_loop_passed,
                         (unsigned long)scheduler_started,
                         (unsigned)iir_passed,
                         (unsigned)pl_dma_passed,
                         (unsigned long)pl_dma_status.rx_produced);
}

static void platform_probe_init(void)
{
    s_probe_start_tick = bsp_timer_gettime_100us(); /* 以本次固件启动时刻建立自主测试延时基准。 */
    bsp_usart_dbg_printf("section init ready; mode=%s; COM6=PS COM7=PL 921600 8N1\r\n",
                         section_runtime_name_get()); /* 确认链接段初始化回调已经执行。 */
    bsp_usart_dbg_printf("commands: help, ZYNQ_STATUS, IIR_TEST, PL_UART_STATUS, PL_UART_RESET, PL_UART_DMA_TEST, PL_UART_ERROR_CLEAR, PL_OLED_STATUS, PL_OLED_CLEAR, PL_OLED_TEST, PL_OLED_RESET, PL_OLED_INVERT, PL_OLED_CONTRAST\r\n");
}

REG_INIT(1, platform_probe_init)

static void platform_probe_task(void)
{
    uint32_t elapsed_tick = 0U; /* section 初始化后已经运行的 100us tick 数。 */

    s_probe_task_count++; /* 累计周期任务运行次数，供串口状态命令检查。 */
    elapsed_tick = bsp_timer_gettime_100us() - s_probe_start_tick;

    if ((s_self_test_reported == 0U) &&                 /* 当前启动周期尚未输出自主测试结果。 */
        (elapsed_tick >= 25000U))                       /* 平台已运行至少 2.5 秒，避开启动阶段输出。 */
    {
        s_self_test_reported = 1U;
        platform_self_test_report(); /* 汇总 section、dbg 与 app demo 的运行证据。 */
    }
}

REG_TASK_MS(100, platform_probe_task)

static void platform_status_command(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) ||             /* 当前 Shell 链路未提供发送上下文。 */
        (my_printf->my_printf == NULL))    /* 当前 Shell 链路未绑定格式化输出函数。 */
    {
        return;
    }

    my_printf->my_printf("zynq mode=%s tick_100us=%lu task_count=%lu section=%08lX-%08lX srtos=%lu fault=%lu save_fail=%lu release_fail=%lu pool=%lu/%lu stack_free=%lu\r\n",
                         section_runtime_name_get(),
                         (unsigned long)bsp_timer_gettime_100us(),
                         (unsigned long)s_probe_task_count,
                         (unsigned long)(uintptr_t)&SECTION_START,
                         (unsigned long)(uintptr_t)&SECTION_STOP,
                         (unsigned long)section_task_scheduler_started(),
                         (unsigned long)g_section_fault_debug.task_fault_reason,
                         (unsigned long)g_section_fault_debug.task_context_save_fail_count,
                         (unsigned long)g_section_fault_debug.task_context_release_fail_count,
                         (unsigned long)g_section_fault_debug.task_context_pool_used,
                         (unsigned long)g_section_fault_debug.task_context_pool_words,
                         (unsigned long)g_section_fault_debug.task_stack_free_words);
}

REG_SHELL_CMD(ZYNQ_STATUS, platform_status_command)

static void platform_iir_test_command(DEC_MY_PRINTF)
{
    bsp_iir_self_test_result_t result = {0}; /* 当前 Shell 命令生成的 IIR 自测数据。 */
    uint8_t passed = 0U;                     /* 当前 IIR 外设自测结果。 */

    if ((my_printf == NULL) ||             /* 当前 Shell 链路未提供发送上下文。 */
        (my_printf->my_printf == NULL))    /* 当前 Shell 链路未绑定格式化输出函数。 */
    {
        return;
    }

    passed = bsp_iir_self_test(&result); /* 执行单周期 3P3Z、限幅历史和双向饱和闭环。 */
    my_printf->my_printf("iir result=%s impulse=%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld sat=%ld/%ld limits=%ld/%ld feedback=%ld count=%lu status=%08lX version=%08lX format=%08lX\r\n",
                         (passed == 1U) ? "PASS" : "FAIL",
                         (long)result.impulse_output[0],
                         (long)result.impulse_output[1],
                         (long)result.impulse_output[2],
                         (long)result.impulse_output[3],
                         (long)result.impulse_output[4],
                         (long)result.impulse_output[5],
                         (long)result.impulse_output[6],
                         (long)result.impulse_output[7],
                         (long)result.positive_saturation_output,
                         (long)result.negative_saturation_output,
                         (long)result.upper_limited_output,
                         (long)result.lower_limited_output,
                         (long)result.limited_feedback_output,
                         (unsigned long)result.sample_count,
                         (unsigned long)result.final_status,
                         (unsigned long)result.version,
                         (unsigned long)result.format);
}

REG_SHELL_CMD(IIR_TEST, platform_iir_test_command)

static void platform_pl_uart_status_command(DEC_MY_PRINTF)
{
    bsp_usart_pl_status_t status = {0}; /* Current PL UART and ring snapshot. */

    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    bsp_usart_pl_status_get(&status);
    my_printf->my_printf("pl_uart version=%08lX status=%08lX irq=%08lX/%lu/%08lX rx=%lu/%lu tx=%lu/%lu uart_err=%08lX dma_err=%08lX stop=%08lX\r\n",
                         (unsigned long)status.version,
                         (unsigned long)status.status,
                         (unsigned long)status.irq_status,
                         (unsigned long)status.error_irq_count,
                         (unsigned long)status.error_irq_latched,
                         (unsigned long)status.rx_produced,
                         (unsigned long)status.rx_consumed,
                         (unsigned long)status.tx_produced,
                         (unsigned long)status.tx_consumed,
                         (unsigned long)status.uart_errors,
                         (unsigned long)status.dma_errors,
                         (unsigned long)status.dma_stop_reason);
}

REG_SHELL_CMD(PL_UART_STATUS, platform_pl_uart_status_command)

static void platform_pl_uart_reset_command(DEC_MY_PRINTF)
{
    int32_t status = bsp_usart_pl_reset(); /* PL UART reset and recovery result. */

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pl_uart_reset result=%s\r\n",
                             (status == 0) ? "PASS" : "FAIL");
    }
}

REG_SHELL_CMD(PL_UART_RESET, platform_pl_uart_reset_command)

static void platform_pl_uart_dma_test_command(DEC_MY_PRINTF)
{
    bsp_usart_pl_status_t status = {0}; /* Internal-loopback DMA result data. */
    uint8_t passed = pl_dma_loopback_test(&status); /* Execute the board DMA test. */

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pl_uart_dma_test result=%s bytes=%lu rx=%lu/%lu tx=%lu/%lu irq=%lu errors=%08lX/%08lX stop=%08lX\r\n",
                             (passed != 0U) ? "PASS" : "FAIL",
                             (unsigned long)PLATFORM_PL_DMA_TEST_BYTES,
                             (unsigned long)status.rx_produced,
                             (unsigned long)status.rx_consumed,
                             (unsigned long)status.tx_produced,
                             (unsigned long)status.tx_consumed,
                             (unsigned long)status.error_irq_count,
                             (unsigned long)status.uart_errors,
                             (unsigned long)status.dma_errors,
                             (unsigned long)status.dma_stop_reason);
    }
}

REG_SHELL_CMD(PL_UART_DMA_TEST, platform_pl_uart_dma_test_command)

static void platform_pl_uart_error_clear_command(DEC_MY_PRINTF)
{
    bsp_usart_pl_error_clear();
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pl_uart_error_clear result=PASS\r\n");
    }
}

REG_SHELL_CMD(PL_UART_ERROR_CLEAR, platform_pl_uart_error_clear_command)

static void platform_pl_oled_status_command(DEC_MY_PRINTF)
{
    bsp_oled_status_t status = {0}; /* Current OLED DMA and display snapshot. */

    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    bsp_oled_status_get(&status);
    my_printf->my_printf("pl_oled version=%08lX status=%08lX fb=%08lX frame=%lu clear=%lu irq=%08lX/%lu/%08lX errors=%lu/%lu stop=%08lX\r\n",
                         (unsigned long)status.version,
                         (unsigned long)status.status,
                         (unsigned long)status.framebuffer_base,
                         (unsigned long)status.frame_count,
                         (unsigned long)status.clear_count,
                         (unsigned long)status.irq_status,
                         (unsigned long)status.error_irq_count,
                         (unsigned long)status.error_irq_latched,
                         (unsigned long)status.axi_error_count,
                         (unsigned long)status.command_error_count,
                         (unsigned long)status.dma_stop_reason);
}

REG_SHELL_CMD(PL_OLED_STATUS, platform_pl_oled_status_command)

static void platform_pl_oled_clear_command(DEC_MY_PRINTF)
{
    int32_t status = bsp_oled_display_clear(); /* Hardware clear-frame result. */

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pl_oled_clear result=%s\r\n",
                             (status == 0) ? "PASS" : "FAIL");
    }
}

REG_SHELL_CMD(PL_OLED_CLEAR, platform_pl_oled_clear_command)

static void platform_pl_oled_test_command(DEC_MY_PRINTF)
{
    uint32_t y = 0U; /* Test-pattern pixel row. */
    uint32_t x = 0U; /* Test-pattern pixel column. */
    uint32_t frame = 0U; /* Manual framebuffer transfer index. */
    uint32_t start_tick = 0U; /* Automatic-refresh observation origin. */
    uint32_t manual_start_count = 0U; /* Frame count before manual transfers. */
    uint32_t auto_100_start_count = 0U; /* Frame count before 100 ms refresh. */
    uint32_t auto_1000_start_count = 0U; /* Frame count before 1000 ms refresh. */
    uint32_t manual_frame_count = 0U; /* Frames completed during the manual loop. */
    bsp_oled_status_t status = {0}; /* Final OLED DMA test evidence. */
    uint8_t passed = 1U; /* Aggregate manual and automatic refresh result. */

    bsp_oled_frame_clear();
    for (y = 0U; y < BSP_OLED_HEIGHT; ++y)
    {
        for (x = 0U; x < BSP_OLED_WIDTH; ++x)
        {
            uint8_t enabled =
                (uint8_t)((((x >> 3U) + (y >> 3U)) & 1U) == 0U); /* 8x8 checkerboard pixel. */

            bsp_oled_pixel_set(x, y, enabled);
        }
    }
    bsp_oled_present(); /* Complete a synchronization frame before capturing the baseline. */
    bsp_oled_status_get(&status);
    manual_start_count = status.frame_count;
    for (frame = 0U; frame < PLATFORM_OLED_TEST_FRAMES; ++frame)
    {
        bsp_oled_present();
    }
    bsp_oled_status_get(&status);
    manual_frame_count = status.frame_count - manual_start_count;
    if ((manual_frame_count < PLATFORM_OLED_TEST_FRAMES) ||
        (manual_frame_count > (PLATFORM_OLED_TEST_FRAMES + 10U)))
    {
        passed = 0U;
    }

    auto_100_start_count = status.frame_count;
    if (bsp_oled_auto_refresh_set(1U, 100U) != 0)
    {
        passed = 0U;
    }
    start_tick = bsp_timer_gettime_100us();
    while ((bsp_timer_gettime_100us() - start_tick) < 5500U)
    {
        /* Hold the framebuffer stable while hardware performs 100 ms refreshes. */
    }
    (void)bsp_oled_auto_refresh_set(0U, 100U);
    bsp_oled_status_get(&status);
    if ((status.frame_count - auto_100_start_count) < 4U)
    {
        passed = 0U;
    }

    auto_1000_start_count = status.frame_count;
    if (bsp_oled_auto_refresh_set(1U, 1000U) != 0)
    {
        passed = 0U;
    }
    start_tick = bsp_timer_gettime_100us();
    while ((bsp_timer_gettime_100us() - start_tick) < 22000U)
    {
        /* Hold the framebuffer stable while hardware performs 1000 ms refreshes. */
    }
    (void)bsp_oled_auto_refresh_set(0U, 1000U);
    bsp_oled_status_get(&status);
    if ((status.frame_count - auto_1000_start_count) < 2U)
    {
        passed = 0U;
    }
    if ((status.irq_status != 0U) ||
        (status.error_irq_count != 0U) ||
        (status.axi_error_count != 0U) ||
        (status.command_error_count != 0U) ||
        (status.dma_stop_reason != 0U))
    {
        passed = 0U;
    }

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pl_oled_test result=%s pattern=checkerboard manual=%lu auto100=%lu auto1000=%lu irq=%lu errors=%lu/%lu stop=%08lX\r\n",
                             (passed == 1U) ? "PASS" : "FAIL",
                             (unsigned long)manual_frame_count,
                             (unsigned long)(auto_1000_start_count -
                                             auto_100_start_count),
                             (unsigned long)(status.frame_count -
                                             auto_1000_start_count),
                             (unsigned long)status.error_irq_count,
                             (unsigned long)status.axi_error_count,
                             (unsigned long)status.command_error_count,
                             (unsigned long)status.dma_stop_reason);
    }
}

REG_SHELL_CMD(PL_OLED_TEST, platform_pl_oled_test_command)

static void platform_pl_oled_reset_command(DEC_MY_PRINTF)
{
    int32_t status = bsp_oled_reset(); /* OLED DMA and controller recovery result. */

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pl_oled_reset result=%s\r\n",
                             (status == 0) ? "PASS" : "FAIL");
    }
}

REG_SHELL_CMD(PL_OLED_RESET, platform_pl_oled_reset_command)

static void platform_pl_oled_invert_command(DEC_MY_PRINTF)
{
    static uint8_t inverted = 0U; /* Shell-requested inversion state. */
    int32_t status = XST_FAILURE; /* Inversion command result. */

    inverted = (inverted == 0U) ? 1U : 0U;
    status = bsp_oled_invert_set(inverted);
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pl_oled_invert result=%s enabled=%u\r\n",
                             (status == 0) ? "PASS" : "FAIL",
                             (unsigned)inverted);
    }
}

REG_SHELL_CMD(PL_OLED_INVERT, platform_pl_oled_invert_command)

static void platform_pl_oled_contrast_command(DEC_MY_PRINTF)
{
    static uint8_t high_contrast = 1U; /* Shell-selected contrast preset. */
    uint8_t contrast = 0U; /* Contrast value sent to the PL protocol layer. */
    int32_t status = XST_FAILURE; /* Contrast command result. */

    high_contrast = (high_contrast == 0U) ? 1U : 0U;
    contrast = (high_contrast == 1U) ? 0xCFU : 0x40U;
    status = bsp_oled_contrast_set(contrast);
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pl_oled_contrast result=%s value=%u\r\n",
                             (status == 0) ? "PASS" : "FAIL",
                             (unsigned)contrast);
    }
}

REG_SHELL_CMD(PL_OLED_CONTRAST, platform_pl_oled_contrast_command)
