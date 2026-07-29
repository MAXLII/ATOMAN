# M 系列 SRTOS 接入方法

## 1. 接入范围

本文说明具有 Cortex-M 异常模型的平台如何接入 `code/section/srtos_m/`。业务任务使用方法见 [SRTOS 使用方法](srtos_usage.md)。

接入工作分为：

```text
构建选择
  → Section 注册链接段
  → 平台时间基准
  → SVC/PendSV 切换入口
  → tick 请求切换
  → 启动与运行验证
```

用户需要修改平台工程、链接脚本和异常入口。`code/section/srtos_m/section.c` 是公共调度实现，移植时直接复用。

## 2. 加入公共源码

构建目标加入：

```make
PROJECT_SOURCES += ../../../code/section/srtos_m/section.c
```

头文件搜索顺序必须让 SRTOS M 目录先于通用 Section 目录：

```make
C_INCLUDES += -I../../../code/section/srtos_m
C_INCLUDES += -I../../../code/section
```

用户修改相对路径以匹配平台目录。一个目标不能再编译 `code/section/baremetal/section.c` 或 `code/section/srtos_a9/section.c`。

编译器至少启用当前 M 内核对应的 CPU、Thumb 和浮点 ABI 参数。所有包含 `section.h` 的文件必须使用一致的 CPU/FPU 选项。

## 3. 提供平台定义

当前工程在 `code/section/platform.h` 的平台分支中提供 SRTOS 所需定义。新平台需要给出以下内容：

```c
#include "platform_systick.h"
#include "device_cmsis.h"

#define SECTION_SYS_TICK platform_tick_get()
#define SECTION_SYS_TICK_UNIT_US 100u

extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end

#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))

#define SECTION_PORT_CONTEXT_SWITCH_REQUEST() \
    do                                       \
    {                                        \
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;  \
        __DSB();                             \
        __ISB();                             \
    } while (0)
```

用户必须替换：

- `platform_systick.h` 和 `device_cmsis.h`。
- `platform_tick_get()`。
- `SECTION_SYS_TICK_UNIT_US`，使它与时间函数真实单位一致。
- 注册段起止符号，使它们与链接脚本一致。

`SECTION_PORT_CONTEXT_SWITCH_REQUEST()` 的语义是挂起一次最低优先级任务切换异常。使用 CMSIS 的平台可以保留上述写法。

### 3.1 FPU lazy stacking

带 FPU 的 M 系列平台配置：

```c
#if defined(FPU) && (__FPU_PRESENT == 1U)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE()   \
    do                                             \
    {                                              \
        FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;        \
    } while (0)
#else
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                            \
    {                                             \
    } while (0)
#endif
```

用户只修改 FPU 可用条件以匹配设备 CMSIS 定义。关闭 lazy stacking 后，切换入口能够根据异常返回信息确定浮点现场是否存在。

### 3.2 故障入口

平台提供无法恢复时的处理方式：

```c
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                                  \
    {                                   \
        platform_fault_record(reason);  \
        __disable_irq();                \
        for (;;)                        \
        {                               \
        }                               \
    } while (0)
```

用户实现 `platform_fault_record()`，可以记录故障原因、触发看门狗复位或保留现场等待调试。故障钩子不能返回后继续运行损坏的任务现场。

## 4. 配置 Section 注册链接段

GCC 链接脚本加入：

```ld
.section_registry :
{
    . = ALIGN(4);
    __section_start = .;
    KEEP(*(section))
    KEEP(*(section.*))
    __section_end = .;
} > FLASH
```

用户修改：

- `FLASH` 为注册描述符实际存放的只读存储区。
- 起止符号名称；修改后同步更新 `SECTION_START` 和 `SECTION_STOP`。

`KEEP()` 必须保留，否则 `--gc-sections` 可能删除没有显式引用的注册对象。

如果工程使用 `FUNC_RAM`，链接脚本还需定义 `.func_ram` 的运行地址和加载地址，并由启动代码在进入 `main()` 前完成复制。该段属于 Section 中断分发需求，不由 SRTOS 自动搬运。

## 5. 接入异常入口

M 系列使用 SVC 启动首个任务，使用 PendSV 保存和恢复任务现场。异常向量表中的 `SVC_Handler` 和 `PendSV_Handler` 必须指向平台适配实现。

仓库中可直接作为移植模板的文件：

- [HC32F334 异常入口](../../../../platform/hc32f334/src/hc32f334_it.c)
- [GD32G553 异常入口](../../../../platform/gd32g553c/src/gd32g5x3_it.c)

### 5.1 SVC 入口

```c
#if defined(__GNUC__) || defined(__ARMCC_VERSION)
#define PLATFORM_EXCEPTION_NAKED __attribute__((naked))
#else
#define PLATFORM_EXCEPTION_NAKED
#endif

void PLATFORM_EXCEPTION_NAKED SVC_Handler(void)
{
    __ASM volatile(
        "push {r0, lr}                     \n"
        "bl section_task_start_request     \n"
        "pop {r0, r1}                      \n"
        "bx r1                             \n");
}
```

用户可以修改异常函数名，使它与启动文件向量表一致。汇编调用的 `section_task_start_request` 是 SRTOS 公共接口，不修改名称和调用约定。

### 5.2 PendSV 入口

PendSV 入口必须完成：

1. 判断调度器是否已经接管任务现场。
2. 从 PSP 保存软件寄存器和必要的浮点寄存器。
3. 调用 `section_task_switch_sp()`。
4. 把返回的任务现场恢复到 PSP。
5. 通过异常返回继续目标任务。

这段代码与编译器 ABI、FPU 配置和异常帧格式直接相关。用户应复制上述现有 M 系列模板，再修改设备头文件、异常函数名和 FPU 条件，不应把它改写为普通 C 函数。

需要保持的公共调用为：

```c
uint32_t section_task_scheduler_started(void);
uint32_t *section_task_switch_sp(uint32_t *p_sp);
```

`section_task_switch_sp()` 的参数和返回值都是任务现场栈指针。平台入口只负责传递，不解析 SRTOS 的任务队列。

### 5.3 异常优先级

PendSV 必须配置为系统可用的最低优先级，使控制中断和普通外设中断先完成，再进行后台任务切换。

```c
static void platform_srtos_priority_init(void)
{
    NVIC_SetPriority(PendSV_IRQn, (1u << __NVIC_PRIO_BITS) - 1u);
}
```

用户可根据芯片优先级分组调整写法，但不能让 PendSV 高于实时控制中断。

## 6. 在 tick 中断请求切换

时间基准更新后调用：

```c
void SysTick_Handler(void)
{
    platform_tick_increment();
    section_task_irq_exit_request();
}
```

用户替换 `platform_tick_increment()`。如果 `SECTION_SYS_TICK` 来自自由运行硬件计数器，则不需要软件累加，只保留调度请求。

当同一硬件中断还承担 Section 中断回调时：

```c
void SchedulerTimer_IRQHandler(void)
{
    platform_timer_flag_clear();
    section_interrupt();
    section_task_irq_exit_request();
}
```

顺序要求是先清中断标志并完成注册中断回调，最后请求任务切换。`section_task_irq_exit_request()` 自己判断调度器是否启动以及时间片是否到期。

## 7. 启动顺序

```c
int main(void)
{
    platform_clock_init();
    platform_tick_init();
    platform_srtos_priority_init();
    section_port_init();
    section_init();
    platform_tick_interrupt_enable();

    for (;;)
    {
        run_task();
    }
}
```

用户实现所有 `platform_*` 函数。SRTOS M 当前的 `section_port_init()` 为空操作，但应用仍可统一调用，便于不同运行时保持一致入口。

时间函数必须在 `section_init()` 前可读。`run_task()` 发现首个 Ready 任务后会通过 SVC/PendSV 进入公共运行栈，不需要用户直接调用启动和切换内部接口。

## 8. 用户可配置资源

可通过平台配置或编译参数覆盖：

```c
#define SECTION_TASK_RUNTIME_STACK_WORDS 512u
#define SECTION_TASK_CONTEXT_POOL_WORDS 1024u
#define SECTION_TASK_SLICE_TICKS 10u
#define SECTION_TASK_READY_BURST_MAX 4u
```

这些值分别控制单个运行任务可用栈、所有悬挂现场共享容量、时间片长度和 Ready 新任务连续选择上限。调整方法见 [SRTOS 使用方法](srtos_usage.md)。

## 9. 构建检查

GCC 工程可检查：

```make
verify: firmware.elf
	arm-none-eabi-nm -n $< | findstr /C:"__section_start" /C:"__section_end"
	arm-none-eabi-objdump -t $< | findstr /C:"SVC_Handler" /C:"PendSV_Handler"
```

同时确认 map 文件中：

- 只存在一份 `section.c` 对应符号。
- 注册段没有被垃圾回收。
- SVC 和 PendSV 来自平台适配文件，不是启动文件中的默认死循环。
- 公共运行栈与现场池位于有效 RAM。
- FPU 编译参数与 PendSV 保存路径一致。

## 10. 接入验收

1. 注册一个短周期计数任务，确认周期持续增长。
2. 注册一个执行时间超过时间片的任务，确认短任务仍能运行。
3. 在长任务中保留局部校验变量，确认多次切出恢复后数值不变。
4. 同时运行浮点任务和整数任务，确认 FPU 现场互不污染。
5. 检查 PendSV 始终低于控制中断优先级。
6. 观察 `g_section_fault_debug`，确认没有栈越界和现场池错误。
7. 关闭所有其他任务后确认时间片到期不会产生无意义的任务轮换。

## 11. 常见接入错误

| 错误 | 表现 | 检查位置 |
| --- | --- | --- |
| 头文件顺序错误 | 编译到 baremetal 接口或结构不一致 | include path 中 `srtos_m` 必须在 `code/section` 前 |
| 默认异常处理未替换 | 首次运行任务即停在弱定义死循环 | 启动文件向量与 `SVC_Handler`、`PendSV_Handler` |
| PendSV 优先级过高 | 后台切换打断控制中断 | NVIC 系统异常优先级 |
| FPU 选项不一致 | 浮点任务恢复后数据异常 | CPU/FPU 编译参数和 PendSV 模板 |
| 注册段缺少 KEEP | 任务和初始化项无法发现 | 链接脚本与 map 文件 |
| tick 单位定义错误 | 所有任务周期和时间片比例错误 | `SECTION_SYS_TICK_UNIT_US` 与时间函数 |
| 在 ISR 中直接切换 | 异常嵌套和现场损坏 | ISR 只调用 `section_task_irq_exit_request()` |

## 12. 关联导航

### 源代码

- [M 系列 SRTOS 头文件](../../../../code/section/srtos_m/section.h)
- [M 系列 SRTOS 实现](../../../../code/section/srtos_m/section.c)
- [Section 平台定义](../../../../code/section/platform.h)
- [HC32F334 异常入口参考](../../../../platform/hc32f334/src/hc32f334_it.c)
- [GD32G553 异常入口参考](../../../../platform/gd32g553c/src/gd32g5x3_it.c)

### 设计文档

- [SRTOS 设计思想](../../../design/framework/srtos/srtos.md)
- [Section 设计文档](../../../design/framework/section/section_design.md)
