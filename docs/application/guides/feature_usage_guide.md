# 功能使用接入指南

## 1. 适用范围

本文档说明当前工程常用框架功能的代码接入方式。重点不是解释内部设计，而是说明使用某个功能时需要改哪些文件、加哪些宏、调用哪些接口。

相关设计文档：

- Section 自动注册：[section_design.md](../../design/framework/section/section_design.md)
- Shell 调试变量：[shell_design.md](../../design/debug/shell/shell_design.md)
- Perf 性能测量：[perf_design.md](../../design/debug/perf/perf_design.md)
- Scope 波形捕获：[scope_design.md](../../design/debug/scope/scope_design.md)
- Trace 执行路径记录：[trace_design.md](../../design/debug/trace/trace_design.md)
- SFRA 扫频分析：[sfra_design.md](../../design/debug/sfra/sfra_design.md)

## 2. 平台工程必须接入的主循环

### 2.1 启动初始化

平台 `main()` 中必须调用：

```c
#include "section.h"

int main(void)
{
    bsp_init();
    section_init();

    while (1)
    {
        run_task();
    }
}
```

`section_init()` 会扫描所有 `REG_INIT()`、`REG_TASK()`、`REG_INTERRUPT()`、`REG_LINK()`、`REG_COMM()` 等注册对象。

`run_task()` 会驱动周期任务、通信链路轮询、调试服务和上层业务任务。

### 2.2 硬件中断入口

如果要使用 `REG_INTERRUPT()` 注册控制环或高速采样回调，硬件 ISR 中调用：

```c
#include "section.h"

void TIMER_IRQHandler(void)
{
    timer_interrupt_flag_clear();
    section_interrupt();
}
```

`section_interrupt()` 会按 `REG_INTERRUPT(priority, func)` 中的 `priority` 从小到大执行。

## 3. 注册初始化函数

### 3.1 修改位置

在业务模块 `.c` 文件中添加初始化函数和注册宏。

### 3.2 示例

```c
#include "section.h"

static void demo_module_init(void)
{
    demo_state_reset();
}

REG_INIT(0, demo_module_init)
```

`priority` 越小越早执行。平台外设初始化通常在 `section_init()` 之前由 `bsp_init()` 完成，业务对象初始化放在 `REG_INIT()` 中。

## 4. 注册周期任务

### 4.1 修改位置

在需要后台周期执行的模块 `.c` 文件中添加任务函数和 `REG_TASK()` 或 `REG_TASK_MS()`。

### 4.2 示例：毫秒周期

```c
#include "section.h"

static void demo_10ms_task(void)
{
    demo_update_state();
}

REG_TASK_MS(10, demo_10ms_task)
```

### 4.3 示例：系统 tick 周期

当前 `SECTION_SYS_TICK` 的单位由平台配置决定。`REG_TASK(1, func)` 表示每 1 个系统 tick 执行一次。

```c
static void demo_fast_task(void)
{
    demo_fast_update();
}

REG_TASK(1, demo_fast_task)
```

### 4.4 检查点

- 主循环必须持续调用 `run_task()`。
- 任务函数不能阻塞等待串口、Flash 或外设完成。
- 需要分步执行的长任务优先用状态机或 `REG_TASK_STEP()`。

## 5. 注册中断回调

### 5.1 修改位置

在控制模块或采样模块 `.c` 文件中添加中断回调和 `REG_INTERRUPT()`。

### 5.2 示例

```c
#include "section.h"

static void demo_control_isr(void)
{
    demo_adc_sample();
    demo_control_run();
    demo_pwm_update();
}

REG_INTERRUPT(3, demo_control_isr)
```

### 5.3 平台 ISR 入口

```c
void ADC_IRQHandler(void)
{
    adc_interrupt_flag_clear();
    section_interrupt();
}
```

### 5.4 检查点

- 中断回调不能调用阻塞式打印、Flash 擦写或动态分配。
- 多个中断回调用 `priority` 控制顺序，数值越小越早执行。
- 打开 Perf 后，中断回调会自动进入中断耗时统计。

## 6. 接入串口 Shell 和二进制通信

### 6.1 修改位置

平台链路适配通常放在 `code/interface/<platform>/comm_link.c` 或平台专用 `interface` 文件中。

GD32 AC 当前示例在：

```text
code/interface/ac/comm_link.c
```

### 6.2 准备发送函数

```c
static void usart_tx_by_dma_cb(char *ptr, int len)
{
    if ((ptr == NULL) || (len <= 0))
    {
        return;
    }

    bsp_usart_tx(ptr, len);
}

static section_link_tx_func_t s_usart_tx_func = {
    .my_printf = bsp_usart_printf,
    .tx_by_dma = usart_tx_by_dma_cb,
};
```

### 6.3 准备接收函数

```c
static uint8_t usart_rx_get_byte(uint8_t *p_data)
{
    return bsp_usart_rx_get_byte(p_data);
}
```

有数据时返回 `1u`，无数据时返回 `0u`。

### 6.4 同一串口同时接 Shell 和 comm

```c
#include "comm.h"
#include "shell.h"

DECLARE_SHELL_CTX(s_usart_shell_ctx);
DECLARE_COMM_CTX(s_usart_comm_ctx, 1100u, HOST_ADDR, USART0_LINK);

static const section_link_handler_item_t s_usart_handler_arr[] = {
    {.func = shell_run, .ctx = (void *)&s_usart_shell_ctx},
    {.func = comm_run, .ctx = (void *)&s_usart_comm_ctx},
};

REG_LINK(USART0_LINK,
         s_usart_tx_func,
         usart_rx_get_byte,
         s_usart_handler_arr,
         sizeof(s_usart_handler_arr) / sizeof(s_usart_handler_arr[0]));
```

每个接收字节会依次送入 `shell_run()` 和 `comm_run()`。Shell 负责解析 `\n` 结束的字符串命令，comm 负责解析 `0xE8` 二进制帧。

### 6.5 打开字符串 Shell

在 `code/dbg/shell.h` 中配置：

```c
#define SHELL_STRING_ENABLE 1u
```

Makefile 和 Keil 工程不定义字符串 Shell 功能宏。

常用串口输入：

```text
help
time
reset
DEMO_COUNTER
DEMO_COUNTER:100
DEMO_GAIN:1.5
```

## 7. 注册 Shell 变量和命令

### 7.1 修改位置

在业务模块 `.c` 文件中添加 `#include "shell.h"`，然后注册变量或命令。

### 7.2 变量示例

```c
#include "shell.h"

static uint32_t s_demo_counter = 0u;
static float s_demo_gain = 1.0f;

REG_SHELL_VAR(DEMO_COUNTER, s_demo_counter, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_GAIN, s_demo_gain, SHELL_FP32, 10.0f, 0.0f, NULL, SHELL_STA_NULL)
```

Shell 字符串访问：

```text
DEMO_COUNTER
DEMO_COUNTER:123
DEMO_GAIN:2.5
```

### 7.3 写入回调示例

```c
static uint8_t s_demo_enable = 0u;

static void demo_enable_changed(DEC_MY_PRINTF)
{
    (void)my_printf;
    demo_set_enable(s_demo_enable);
}

REG_SHELL_VAR(DEMO_ENABLE, s_demo_enable, SHELL_UINT8, 1u, 0u, demo_enable_changed, SHELL_STA_NULL)
```

### 7.4 命令示例

```c
static void demo_ping_cmd(DEC_MY_PRINTF)
{
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pong\r\n");
    }
}

REG_SHELL_CMD(DEMO_PING, demo_ping_cmd)
```

Shell 输入：

```text
DEMO_PING
```

## 8. 注册 comm 二进制命令

### 8.1 修改位置

在业务模块 `.c` 文件中添加 `#include "comm.h"`，定义命令处理函数并注册。

### 8.2 示例

```c
#include "comm.h"

#define CMD_SET_DEMO  0x01u
#define CMD_WORD_PING 0x30u

typedef struct
{
    uint32_t value;
} demo_ping_ack_t;

static void demo_ping_act(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    demo_ping_ack_t ack = {0};
    section_packform_t pack = {0};

    if ((p_pack == NULL) || (p_pack->is_ack != 0u))
    {
        return;
    }

    ack.value = 0x12345678u;

    pack.src = p_pack->dst;
    pack.d_src = p_pack->d_dst;
    pack.dst = p_pack->src;
    pack.d_dst = p_pack->d_src;
    pack.cmd_set = CMD_SET_DEMO;
    pack.cmd_word = CMD_WORD_PING;
    pack.is_ack = 1u;
    pack.len = (uint16_t)sizeof(ack);
    pack.p_data = (uint8_t *)&ack;

    comm_send_data(&pack, my_printf);
}

REG_COMM(CMD_SET_DEMO, CMD_WORD_PING, demo_ping_act)
```

ACK 原则：只有对同一个 `cmd_set/cmd_word` 的直接响应才设置 `is_ack = 1u`。

### 8.3 payload 兼容解析

协议扩展时只在结构体尾部追加字段。接收时按实际长度取小：

```c
demo_req_t req = {0};
uint16_t copy_len = p_pack->len;

if (copy_len > sizeof(req))
{
    copy_len = (uint16_t)sizeof(req);
}

(void)memcpy(&req, p_pack->p_data, copy_len);
```

## 9. 打开任务和中断 Perf

### 9.1 修改位置

在 `code/dbg/perf.h` 中分别配置任务、中断和手工代码插桩：

```c
#define PERF_TASK_ENABLE 1u
#define PERF_INTERRUPT_ENABLE 1u
#define PERF_CODE_ENABLE 1u
```

Perf 功能开启后，Shell 输出和二进制 service 同步启用，不再设置独立的 service 输出开关。`section.h` 直接读取 `perf.h` 中的开关，不需要强制预包含，也不需要在编译工程中传入功能宏。

### 9.2 平台计数器

平台 BSP 需要注册自由运行计数器：

```c
#include "perf.h"

REG_PERF_BASE_CNT((uint32_t *)(uintptr_t)(TIMER1 + 0x00000024u), BSP_TIMER_CNT_PERIOD_S)
```

`period_s` 必须等于计数器实际 tick 秒数。

### 9.3 自动测量

打开 `PERF_TASK_ENABLE` 和 `PERF_INTERRUPT_ENABLE` 后，所有：

```c
REG_TASK(...)
REG_TASK_MS(...)
REG_INTERRUPT(...)
```

会自动生成 task/interrupt perf record，不需要在每个任务手动加 `PERF_START()`。

### 9.4 手动代码段测量

```c
#include "perf.h"

REG_PERF_RECORD(demo_work)

static void demo_run(void)
{
    PERF_START(demo_work);
    demo_workload();
    PERF_END(demo_work);
}
```

### 9.5 Shell 查看

Perf 功能开启后可用：

```text
perf_info
perf_summary
perf_print_task
perf_print_interrupt
perf_print_code
perf_reset_peak
```

## 10. 使用 Scope 捕获波形

### 10.1 修改位置

在需要观测波形的模块 `.c` 中添加 `#include "scope.h"`，注册 Scope，并在采样点调用 `SCOPE_RUN()`。

### 10.2 示例

```c
#include "scope.h"

REG_SCOPE_EX(ctrl_scope, 512, 128, 50u, scope_vbus, scope_iout, scope_duty)

static void control_isr(void)
{
    scope_vbus = adc_vbus_get();
    scope_iout = adc_iout_get();
    scope_duty = ctrl_duty_get();

    SCOPE_RUN(ctrl_scope);
}
```

`50u` 表示采样周期 50 us，必须和 `SCOPE_RUN()` 的实际调用周期一致。

### 10.3 触发示例

```c
if (fault_edge_detected != 0u)
{
    SCOPE_TRIGGER(ctrl_scope);
}
```

上位机可以通过 Scope 二进制协议查询列表、启动、触发、读取样本。

## 11. 使用 Trace 记录执行路径

### 11.1 修改位置

在模块 `.c` 中添加 `#include "trace.h"`，先绑定时间源，再添加 trace 点。

### 11.2 示例

```c
#include "trace.h"

extern volatile uint32_t g_system_tick;

static void demo_trace_init(void)
{
    DBG_TRACE_BIND_TIME(&g_system_tick);
}

REG_INIT(0, demo_trace_init)

static void demo_state_run(void)
{
    DBG_TRACE_MARK();

    if (demo_fault_active() != 0u)
    {
        DBG_TRACE_MARK();
        return;
    }

    DBG_TRACE_MARK();
}
```

Shell 命令：

```text
dbg_trace_print
dbg_trace_clear
```

Trace 缓冲有限，定位完成后应删除无用 trace 点。

## 12. 使用 SFRA

### 12.1 修改位置

在控制模块 `.c` 中注册 SFRA 实例，在控制 ISR 中接入注入和采样，在后台任务中调用 `sfra_task()`。

### 12.2 示例

```c
#include "sfra.h"

REG_SFRA(pfc_sfra,
         1u,
         50.0e-6f,
         0.01f,
         10.0f,
         1000.0f,
         NULL,
         NULL)

static void pfc_ctrl_isr(void)
{
    sfra_isr_pre_sample(&pfc_sfra);

    pfc_current_ref += pfc_sfra_inject;
    pfc_control_run();

    pfc_sfra_collect = pfc_current_feedback;
    sfra_isr_post_sample(&pfc_sfra);
}

static void pfc_sfra_task(void)
{
    sfra_task(&pfc_sfra);
}

REG_TASK_MS(1, pfc_sfra_task)
```

`ts` 必须等于 `sfra_isr_pre_sample/post_sample` 的实际调用周期。

## 13. 固件升级功能接入

当前升级软件按职责分为四个目录：

```text
code/app/bootloader/
├── core/      启动判断、下载、安装、恢复和元数据状态机
├── protocol/  FRAME 0x08、0x09、0x0A、0x0B 协议
├── iap/       IAP 内的升级触发、准备回调和复位服务
└── common/    IAP 与 Bootloader 共享的保留 SRAM 请求格式
```

Bootloader Core 通过 `bootloader_flash_ops_t` 使用逻辑 Flash，不包含 FAL 或平台头文件。平台的 `bootloader_fal_adapter.c` 将逻辑区映射到平台 FAL 分区，并挂载查询、读、写、擦、忙状态和结果查询函数。FAL 的实例、配置、初始化和周期调度由平台持有。

IAP 固件只编译 `iap/iap_update_service.c`。该服务接收命令 `0x08` 后直接 ACK，在 1 ms 任务中轮询弱定义的 `iap_update_prepare()`；准备完成后写入共享请求并通过 `SYSTEM_RESET` 进入 Bootloader。ISP 和 Bootloader 固件不编译该服务。

平台移植所需文件、FAL 分区、链接区域和验证顺序见 [Bootloader 平台移植](../porting/bootloader_platform_porting.md)。产品专用的旧升级实现集中保存在 `code/legacy/update/`，用于已有协议与业务行为对照。

## 14. 当前默认调试开关

调试功能开关统一放在对应模块头文件中：

```c
/* code/dbg/shell.h */
#define SHELL_STRING_ENABLE 1u

/* code/dbg/perf.h */
#define PERF_TASK_ENABLE 1u
#define PERF_INTERRUPT_ENABLE 1u
#define PERF_CODE_ENABLE 1u

/* code/dbg/scope.h */
#define SCOPE_ENABLE 1u

/* code/dbg/trace.h */
#define TRACE_ENABLE 1u
```

平台 Makefile 和 `.uvprojx` 只保留固件类型、MCU/芯片族、工具链、链接脚本选择等工程身份与构建配置，不定义调试功能开关。

MCU 工程的实际编译、产物和下载命令见 [mcu_build_download_guide.md](../build/mcu_build_download_guide.md)。

当前 GD32 AC 串口链路：

- USART0：Shell 字符串 + comm 二进制帧
- USART2：Shell 字符串 + comm 二进制帧
- CAN：comm 二进制帧

## 15. 新功能接入检查清单

新增一个可被上位机调试的业务模块时，按以下顺序检查：

1. 模块 `.c` 是否被平台 makefile 编译。
2. 初始化是否用 `REG_INIT()` 注册。
3. 周期逻辑是否用 `REG_TASK()` 或 `REG_TASK_MS()` 注册。
4. 高速逻辑是否用 `REG_INTERRUPT()` 注册，并确认硬件 ISR 调用了 `section_interrupt()`。
5. 调试变量是否用 `REG_SHELL_VAR()` 注册。
6. 调试命令是否用 `REG_SHELL_CMD()` 注册。
7. 二进制命令是否用 `REG_COMM()` 注册。
8. 需要波形时是否注册 `REG_SCOPE()` 并周期调用 `SCOPE_RUN()`。
9. 需要耗时统计时是否在 `perf.h` 打开对应 Perf 开关并注册平台计数器。
10. 需要执行路径定位时是否绑定 Trace 时间源。
11. 平台构建文件中是否只保留工程身份和构建必需宏。
12. 编译后是否能在 map 文件中看到对应注册符号。
