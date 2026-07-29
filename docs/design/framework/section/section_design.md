# Section 设计文档

## 1. 模块定位

Section 模块是工程的自动注册和运行时调度框架。它通过链接器 section 收集各模块声明的服务对象，并在运行时构建初始化、任务、中断和通信链路等链表。

Section 提供裸机、Cortex-M SRTOS 和 Cortex-A9 SRTOS 三套完全独立的运行时实现。业务模块通过统一注册宏把自己的函数或对象放入注册表，构建工程只编译其中一套 `section.c/.h`。

## 2. 目录与构建选择

`code/section/` 的运行时目录如下：

| 目录 | 运行时 | 调度方式 |
| --- | --- | --- |
| `baremetal/` | 裸机 | 主循环协作式调度 |
| `srtos_m/` | Cortex-M SRTOS | PendSV 抢占、公共运行栈和公共现场池 |
| `srtos_a9/` | Cortex-A9 SRTOS | SVC/IRQ 抢占、公共运行栈和公共现场池 |

三个目录都提供同名的 `section.c` 和 `section.h`。工程文件必须：

1. 只编译所选目录中的 `section.c`。
2. 把所选目录放在头文件搜索路径首位，使现有 `#include "section.h"` 解析到对应头文件。
3. 同时加入 `code/section/`，用于访问公共的 `platform.h`、`my_math.h` 和 `timing.h`。

运行时不再通过 `SRTOS` 预处理宏切换。三套实现的 `REG_INIT`、`REG_TASK`、`REG_INTERRUPT`、`REG_FSM`、`REG_LINK` 等注册接口保持一致。

## 3. 统一注册描述符

所有自动注册对象都先包装成 `reg_section_t`：

```c
typedef struct
{
    uint32_t section_type;
    void *p_str;
} reg_section_t;
```

`section_type` 表示注册对象类型，`p_str` 指向真实对象。

基础注册宏：

```c
REG_SECTION_FUNC(section_type, object)
```

该宏把 `reg_section_t` 放入 `AUTO_REG_SECTION` 指定的链接段。链接脚本或平台工程需要提供：

```text
SECTION_START
SECTION_STOP
```

所选实现的 `section_init()` 会遍历这两个符号之间的所有 `reg_section_t`。

## 4. 注册类型

当前支持的 `SECTION_E`：

| 类型 | 处理模块 | 说明 |
| --- | --- | --- |
| `SECTION_INIT` | `section.c` | 初始化函数 |
| `SECTION_TASK` | `section.c` | 周期任务 |
| `SECTION_INTERRUPT` | `section.c` | 中断阶段回调 |
| `SECTION_SHELL` | `shell.c` | Shell 变量和命令 |
| `SECTION_LINK` | `section.c` | 通信链路 |
| `SECTION_PERF` | `perf.c` | Perf 时间基准和 record |
| `SECTION_COMM` | `comm.c` | 通信命令 |
| `SECTION_COMM_ROUTE` | `comm.c` | 通信路由 |
| `SECTION_SCOPE` | `scope_service.c` | Scope 实例 |
| `SECTION_SFRA` | `sfra_service.c` | SFRA 实例 |

`section_init()` 只直接处理 `INIT`、`TASK`、`INTERRUPT` 和 `LINK`。其他类型由对应模块在自己的初始化函数中再次扫描。

## 5. 初始化链表

初始化对象使用 `reg_init_t`：

```c
typedef struct reg_init
{
    int8_t priority;
    void (*p_func)(void);
    struct reg_init *p_next;
} reg_init_t;
```

注册宏：

```c
REG_INIT(priority, func)
```

`section_init()` 会按 `priority` 升序插入初始化链表。同优先级的对象保持扫描时的相对顺序。构建链表后，`section_init()` 依次执行所有初始化函数。

## 6. 任务调度

任务对象使用 `reg_task_t`。三套头文件保持任务注册所需的公共字段一致；SRTOS 实现在结构体末尾增加运行状态和现场元数据。

```c
typedef struct reg_task_t
{
    uint32_t t_period;
    uint32_t time_last;
    void (*p_func)(void);
    section_perf_record_t *p_perf_record;
    struct reg_task_t *p_next;
} reg_task_t;
```

注册宏：

```c
REG_TASK(period, func)
REG_TASK_MS(period_ms, func)
```

`REG_TASK()` 的 `period` 单位是 `SECTION_SYS_TICK`。`REG_TASK_MS()` 把毫秒转换为系统 tick，当前转换关系为 `period_ms * 10u`。

任务链表按扫描顺序追加。`run_task()` 在主循环中持续调用。

裸机实现的调度规则：

1. 读取当前 `SECTION_SYS_TICK`。
2. 计算 `elapsed = now - time_last`。
3. 如果 `elapsed >= t_period`，执行任务。
4. 执行完成后，`time_last += (elapsed / t_period) * t_period`。

这种更新方式不会因为主循环偶发延迟而持续积累相位漂移。每次到期只执行一次，不循环追赶历史遗漏周期。

`srtos_m` 和 `srtos_a9` 会把到期任务加入 Ready 队列。长任务时间片到期后，运行现场保存到公共现场池；调度器优先运行新到期任务，再按 FIFO 恢复未完成任务。两套 SRTOS 使用不同的处理器端口，但保持相同的任务选择和公共栈设计。

## 7. 中断调度

中断对象使用 `reg_interrupt_t`：

```c
typedef struct reg_interrupt
{
    uint8_t priority;
    void (*p_func)(void);
    section_perf_record_t *p_perf_record;
    struct reg_interrupt *p_next;
} reg_interrupt_t;
```

注册宏：

```c
REG_INTERRUPT(priority, func)
```

`section_interrupt()` 按 `priority` 升序执行所有中断回调。具体硬件 ISR 只需要调用 `section_interrupt()`，各模块的中断阶段逻辑通过注册宏接入。

`section_interrupt()` 使用 `FUNC_RAM` 修饰，平台可把该函数放到 RAM 中运行，降低中断热路径的 Flash wait-state 影响。

## 8. 扩展执行机制

Section 的通用注册与调度能力承载三种扩展执行机制：

| 机制 | 核心思想 | 详细设计 |
| --- | --- | --- |
| Link | 将平台字节流同步扇出给多个独立解析上下文 | [Section Link 设计](link_design.md) |
| FSM | 将进入、执行、事件判定和退出组织成表驱动状态执行器 | [Section FSM 设计](fsm_design.md) |
| Perf | 在任务和中断边界注入可裁剪测量 hook，并由独立后端统计 | [Section Perf 协作设计](perf_design.md) |

Link 作为普通周期任务运行，FSM 由注册宏生成 1 ms 任务，Perf 则嵌入任务和中断的执行边界。三者共享 Section 的静态注册和调度基础，但各自维护数据模型与语义。

## 9. 当前约束

- 不使用动态内存。
- 注册对象依赖链接段收集。
- 裸机任务调度是协作式调度，任务到期后每轮最多执行一次。
- Cortex-M 与 Cortex-A9 SRTOS 均使用公共运行栈和公共现场池，不为每个任务静态分配独立栈。
- 一个构建目标只允许编译一套 `section.c`，且所选 `section.h` 必须位于公共目录之前。
- 中断注册项由实际 ISR 调用 `section_interrupt()` 后执行。
- `REG_FSM()` 固定生成 1 ms 周期任务。
- 链路处理按字节分发，handler 需要维护自己的解析上下文。

## 10. 关联导航

### 应用文档

- [Section 使用文档](../../../application/framework/section/section_usage.md)

### 基础教材

- [C语言静态注册与链接基础](../../../tutorial/static_registration_and_linking.md)
- [裸机调度、中断与状态机基础](../../../tutorial/baremetal_scheduling_interrupt_fsm.md)
- [嵌入式通信分发、性能与可靠性基础](../../../tutorial/communication_performance_reliability.md)
- [实时任务与调度基础](../../../tutorial/realtime_task_scheduling.md)
- [处理器现场、栈与上下文切换基础](../../../tutorial/processor_context_and_stack.md)
- [RTOS并发、内存与可靠性基础](../../../tutorial/rtos_concurrency_and_reliability.md)
