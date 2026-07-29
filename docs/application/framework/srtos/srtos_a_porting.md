# A 系列 SRTOS 接入方法

## 1. 接入范围

本文说明具有 ARM A-profile 异常模式的平台如何接入 `code/section/srtos_a9/`。当前仓库的可执行参考端口位于 `platform/zynq7020/ps/srtos/`。

业务任务使用方法见 [SRTOS 使用方法](srtos_usage.md)。

A 系列接入包含两部分：

- 公共 SRTOS：任务到期、队列选择、公共运行栈和现场池。
- 平台端口：异常向量、SVC/IRQ 栈、寄存器现场、切换请求和故障入口。

公共 SRTOS 直接复用；平台端口必须与目标 CPU、启动代码、中断控制器和 ABI 匹配。

## 2. 加入构建目标

公共 C 源码：

```make
C_SOURCES += $(PROJECT_ROOT)/code/section/srtos_a9/section.c
```

平台端口源码：

```make
C_SOURCES += ./srtos/a9_section_control.c
ASM_SOURCES += ./srtos/a9_section_port.S
```

头文件顺序：

```make
C_INCLUDES += -I./srtos
C_INCLUDES += -I$(PROJECT_ROOT)/code/section/srtos_a9
C_INCLUDES += -I$(PROJECT_ROOT)/code/section
```

用户修改路径以匹配平台工程。一个构建目标只能选择一套 Section 运行时。

当前 Cortex-A9 hard-float 参考参数为：

```make
CPU_FLAGS := -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
```

用户根据目标 A 系列内核和浮点 ABI 修改参数，并同步修改平台汇编端口保存的浮点现场。C 文件、汇编文件和链接库必须使用一致 ABI。

## 3. 平台端口文件职责

参考文件：

- [A9 异常与现场端口](../../../../platform/zynq7020/ps/srtos/a9_section_port.S)
- [A9 切换控制](../../../../platform/zynq7020/ps/srtos/a9_section_control.c)
- [A9 端口接口](../../../../platform/zynq7020/ps/srtos/a9_section_port.h)

### 3.1 汇编端口

汇编端口负责：

- 提供异常向量表。
- 安装向量基地址。
- 通过 SVC 启动或主动让出任务。
- 在 IRQ 返回前处理挂起的任务切换。
- 保存和恢复通用寄存器、浮点状态、返回位置与处理器状态。
- 在独立异常栈上调用公共调度函数。

用户必须按目标 CPU 修改：

- `.cpu`、`.fpu` 和指令模式。
- 任务模式、SVC 模式与 IRQ 模式约定。
- 寄存器和浮点现场布局。
- 原有启动向量及 IRQ/FIQ 分发函数名称。
- 异常返回指令序列。

以下公共函数名和语义保持不变：

```c
uint32_t *section_task_start_sp_get(void);
uint32_t *section_task_switch_sp(uint32_t *p_sp);
```

汇编端口向它们传递完整任务现场的栈指针，不解析 Ready 队列和现场池。

### 3.2 C 控制端口

C 控制端口向公共 SRTOS 提供：

```c
void a9_section_port_install_vector_table(void);
void a9_section_port_yield(void);
void a9_section_port_switch_request(void);
uint32_t a9_section_port_irq_save(void);
void a9_section_port_irq_restore(uint32_t saved_status);
void a9_section_port_wait_for_interrupt(void);
void a9_section_port_fault(uint32_t reason);
```

新 A 系列平台可以修改函数前缀，但需要同步修改 `srtos_a9/section.c` 的端口绑定。更稳妥的方式是保持接口名称，只替换函数内部的处理器指令和中断屏蔽实现。

用户可修改：

- 向量表安装方式。
- 切换请求的发布与内存屏障。
- IRQ 屏蔽状态保存和恢复。
- 空闲等待指令。
- 故障记录和复位策略。

用户不在控制端口中重新实现任务选择。

## 4. 提供平台时间与 Section 边界

平台定义示例：

```c
#include "platform_timer.h"

#define SECTION_SYS_TICK platform_timer_get_100us()
#define SECTION_SYS_TICK_UNIT_US 100u

extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end

#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))
```

用户替换时间函数，并保证：

- 返回值单调递增。
- 读取可在任务和 IRQ 上下文安全执行。
- tick 单位与 `SECTION_SYS_TICK_UNIT_US` 一致。
- 32 位回绕保持自然的无符号差值语义。

## 5. 链接段与异常栈

### 5.1 Section 注册段

GCC 链接脚本加入：

```ld
.section_registry ALIGN(4) :
{
    __section_start = .;
    KEEP(*(section))
    KEEP(*(section.*))
    __section_end = .;
} > APP_MEMORY
```

用户把 `APP_MEMORY` 替换为当前应用可访问存储区，并同步平台头文件中的起止符号。

### 5.2 异常栈隔离

任务在公共运行栈上执行，SVC 和 IRQ 调度路径必须使用独立异常栈。平台启动代码或链接脚本需要为各异常模式建立栈空间。

参考工程通过链接参数配置 Supervisor 栈：

```make
LDFLAGS += -Wl,--defsym,_SUPERVISOR_STACK_SIZE=0x2000
```

用户根据以下消耗确定栈容量：

- IRQ 分发调用深度。
- SRTOS 公共调度函数调用深度。
- 平台驱动在异常上下文中的局部变量。
- 浮点现场和 ABI 对齐要求。

异常栈不能与任务公共运行栈、现场池、DMA 区域或其他保留内存重叠。

## 6. 中断入口接入

参考汇编向量把 IRQ 交给平台已有的 `IRQInterrupt` 分发函数。注册中断回调完成后，汇编入口检查切换请求标志；只有存在请求时才保存任务现场并调用调度器。

平台定时器处理函数按以下顺序组织：

```c
static void scheduler_timer_handler(void *p_context)
{
    platform_timer_interrupt_clear(p_context);
    section_interrupt();
    section_task_irq_exit_request();
}
```

用户替换清中断函数和回调参数。`section_task_irq_exit_request()` 不直接在 C ISR 中交换栈，只发布一次在 IRQ 返回路径消费的切换请求。

以下顺序必须保持：

1. 进入平台 IRQ 总入口并保存 IRQ 自身需要的现场。
2. 分发并完成具体设备 ISR。
3. 检查 SRTOS 切换请求。
4. 必要时把被中断任务保存到公共现场池。
5. 恢复目标任务并从 IRQ 返回。

如果平台已有自己的向量表和 IRQ 包装器，需要把上述切换检查合并到唯一的 IRQ 返回路径，不能同时保留两个相互独立的向量入口。

## 7. 初始化顺序

```c
int main(void)
{
    int32_t status = 0;

    platform_timebase_init();
    section_port_init();
    status = platform_interrupt_controller_init();
    if (status != 0)
    {
        platform_fatal_stop();
    }

    section_init();
    status = platform_schedule_tick_start();
    if (status != 0)
    {
        platform_fatal_stop();
    }

    for (;;)
    {
        run_task();
    }
}
```

用户实现所有 `platform_*` 函数。

关键顺序：

- 先建立单调时间基准。
- `section_port_init()` 安装 SRTOS 异常向量。
- 初始化中断控制器并连接定时器。
- `section_init()` 完成注册项发现。
- 最后启动周期 IRQ 并进入 `run_task()`。

如果平台中断控制器初始化会重设向量基址，应在其后再次确认 SRTOS 向量表仍然生效。

## 8. 用户可配置资源

A 系列默认配置可以通过平台配置或编译参数覆盖：

```c
#define SECTION_TASK_RUNTIME_STACK_WORDS 512u
#define SECTION_TASK_CONTEXT_POOL_WORDS 2048u
#define SECTION_TASK_SLICE_TICKS 10u
#define SECTION_TASK_READY_BURST_MAX 4u
```

A 系列完整现场通常比 M 系列更大，尤其启用 VFP/NEON 状态保存时。现场池容量应按同时悬挂任务的实测快照总量配置，不能直接照搬 M 系列数值。

## 9. 构建验证

参考检查命令：

```make
verify: firmware.elf
	arm-none-eabi-readelf -S $< | findstr /C:"section_registry"
	arm-none-eabi-nm -n $< | findstr /C:"__section_start" /C:"__section_end"
	arm-none-eabi-nm -n $< | findstr /C:"_vector_table" /C:"section_task_switch_sp"
```

map 文件还需确认：

- 向量表地址满足处理器对齐要求。
- `section_port_init()` 安装的地址就是最终向量表地址。
- System、SVC、IRQ 和 FIQ 栈区域合法且互不重叠。
- 公共运行栈和现场池位于 CPU 可读写内存。
- 浮点 ABI 与汇编保存范围一致。
- 原有 IRQ 分发函数只从 SRTOS IRQ 包装入口进入一次。

## 10. 接入验收

1. 启动后读取向量基址，确认指向 SRTOS 向量表。
2. 定时器 IRQ 能持续进入，普通设备 IRQ 仍可正常分发。
3. 首个任务通过 SVC 从任务入口开始执行。
4. 长任务被 IRQ 切出后能从原执行位置恢复。
5. 整数任务、浮点任务交替运行时结果保持独立。
6. SVC、IRQ 与任务公共栈的高水位都在安全范围内。
7. 连续 Ready 任务存在时，未完成任务仍能按有界公平规则推进。
8. 制造非法现场或现场池不足，平台故障入口能够保留原因并停止错误恢复。

## 11. 常见接入错误

| 错误 | 表现 | 检查位置 |
| --- | --- | --- |
| 浮点 ABI 与汇编端口不一致 | 切换后浮点值或返回地址异常 | CPU flags 与保存/恢复宏 |
| SVC/IRQ 没有独立栈 | 调度器调用覆盖任务公共栈 | 启动代码、链接脚本和模式 SP |
| 向量基址被后续初始化覆盖 | IRQ 或 SVC 进入旧向量 | `section_port_init()` 与中断控制器初始化顺序 |
| IRQ 返回前未检查切换请求 | tick 正常但长任务无法被切出 | 唯一 IRQ 包装入口 |
| 两套 IRQ 入口同时存在 | 中断重复分发或现场层级错误 | BSP 向量和 SRTOS 向量整合 |
| 注册段被回收 | `section_init()` 找不到任务 | 链接脚本 `KEEP()` 与边界符号 |
| A 系列沿用 M 系列现场池大小 | 多个悬挂任务时池快速耗尽 | 实测快照峰值与浮点现场大小 |
| 在具体 ISR 中直接交换任务 SP | 模式栈和返回状态损坏 | ISR 只发布请求，由统一 IRQ 返回路径切换 |

## 12. 关联导航

### 源代码

- [A 系列 SRTOS 头文件](../../../../code/section/srtos_a9/section.h)
- [A 系列 SRTOS 实现](../../../../code/section/srtos_a9/section.c)
- [Section 平台定义](../../../../code/section/platform.h)
- [A9 异常与现场端口](../../../../platform/zynq7020/ps/srtos/a9_section_port.S)
- [A9 切换控制](../../../../platform/zynq7020/ps/srtos/a9_section_control.c)
- [A9 端口接口](../../../../platform/zynq7020/ps/srtos/a9_section_port.h)

### 设计文档

- [SRTOS 设计思想](../../../design/framework/srtos/srtos.md)
- [Section 设计文档](../../../design/framework/section/section_design.md)
