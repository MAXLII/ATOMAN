# Section 设计文档

## 1. 模块定位

Section 模块是工程的自动注册和运行时调度框架。它通过链接器 section 收集各模块声明的服务对象，并在运行时构建初始化、任务、中断和通信链路等链表。

Section 本身不实现具体业务逻辑。业务模块通过注册宏把自己的函数或对象放入统一注册表，主循环和中断入口再调用 Section 提供的调度函数。

## 2. 统一注册描述符

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

`section_init()` 会遍历这两个符号之间的所有 `reg_section_t`。

## 3. 注册类型

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

## 4. 初始化链表

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

## 5. 任务调度

任务对象使用 `reg_task_t`：

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

任务链表按扫描顺序追加。`run_task()` 在主循环中调用，遍历任务链表并执行到期任务。

调度规则：

1. 读取当前 `SECTION_SYS_TICK`。
2. 计算 `elapsed = now - time_last`。
3. 如果 `elapsed >= t_period`，执行任务。
4. 执行完成后，`time_last += (elapsed / t_period) * t_period`。

这种更新方式不会因为主循环偶发延迟而持续积累相位漂移。每次到期只执行一次，不循环追赶历史遗漏周期。

## 6. 中断调度

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

## 7. Perf Hook

Section 不直接依赖 Perf 模块。`section.c` 中提供弱符号：

```c
section_perf_task_begin()
section_perf_task_end()
section_perf_task_period_set()
section_perf_interrupt_begin()
section_perf_interrupt_end()
```

如果工程编译进 `perf.c`，Perf 模块会提供同名强符号覆盖这些空实现。

中断 Perf hook 使用 `FUNC_RAM` 修饰。`section_interrupt()` 内部按 `INTERRUPT_RECORD_PERF_ENABLE` 编译成两套路径：

- `INTERRUPT_RECORD_PERF_ENABLE == 1`：执行中断回调前后调用 `section_perf_interrupt_begin/end()`。
- `INTERRUPT_RECORD_PERF_ENABLE == 0`：直接遍历中断链表并执行回调。

这样中断热路径不需要在运行时判断 Perf 是否使能。

任务和中断注册宏会在对应开关打开时自动注册 Perf record：

| 开关 | 默认值 | 作用 |
| --- | --- | --- |
| `TASK_RECORD_PERF_ENABLE` | `1` | 任务自动测量 |
| `INTERRUPT_RECORD_PERF_ENABLE` | `1` | 中断自动测量 |

## 8. FSM

状态机由 `reg_fsm_t` 和 `reg_fsm_func_t` 描述。

状态表项使用：

```c
FSM_ENTRY(state, in, exe, chk, out)
```

状态机注册使用：

```c
REG_FSM(name, init_state, fsm_event, ...)
```

`REG_FSM()` 会生成一个 1 ms 周期任务，任务内部调用 `section_fsm_func()`。

状态机执行流程：

1. 当前状态第一次进入时调用 `func_in()`。
2. 每次调度调用 `func_exe()`。
3. 当事件变量非 0 时，调用 `func_chk(event)` 判断下一个状态。
4. 如果状态变化，先调用 `func_out()`，再切换状态。
5. 事件处理后清零事件变量。

## 9. 通信链路

链路对象使用 `section_link_t`：

```c
struct section_link_t
{
    uint8_t (*rx_get_byte)(uint8_t *p_data);
    DEC_MY_PRINTF;
    struct section_link_t *p_next;
    const section_link_handler_item_t *handler_arr;
    uint32_t handler_num;
    uint8_t link_id;
};
```

注册宏：

```c
REG_LINK(link, print, rx_get_byte, handler_arr, handler_num)
```

Section 内部注册了 `section_link_task`：

```c
REG_TASK(10, section_link_task)
```

该任务轮询所有注册 link，从 `rx_get_byte()` 取出字节，并把每个字节依次分发给 `handler_arr` 中的 handler。

## 10. 当前约束

- 不使用动态内存。
- 注册对象依赖链接段收集。
- 任务调度是协作式调度，不抢占。
- 任务到期后每轮最多执行一次，不做循环补偿。
- 中断注册项由实际 ISR 调用 `section_interrupt()` 后执行。
- `REG_FSM()` 固定生成 1 ms 周期任务。
- 链路处理按字节分发，handler 需要维护自己的解析上下文。
