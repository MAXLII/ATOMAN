# Section Perf 协作设计文档

## 1. 设计定位

Section Perf 协作层把性能测量嵌入任务和中断调度边界，同时保持调度器在没有 Perf 后端时仍可独立链接。

它关注的是“在什么时刻测量”和“调度对象如何关联 record”。计数器注册、record 统计、负载计算和上位机服务属于 Perf 模块，完整设计见 [Perf 设计文档](../../debug/perf/perf_design.md)。

## 2. 三阶段绑定

Section 与 Perf 的连接分成三个时刻：

```mermaid
flowchart LR
    A["编译期：注册宏生成 record 指针"] --> B["初始化期：Perf 扫描 SECTION_PERF"]
    B --> C["运行期：Section 在调度边界调用 hook"]
    C --> D["Perf 累计耗时与负载"]
```

### 2.1 编译期注入

`REG_TASK()` 和 `REG_INTERRUPT()` 在对应测量开关打开时，自动生成 Perf record，并把 record 指针写入任务或中断描述符。业务任务不需要手工包裹测量代码。

关闭开关时，record 字段、注册对象和运行路径通过宏消失，而不是在运行期判断是否启用。这样关闭测量后不会保留无意义的 record 遍历和热路径分支。

### 2.2 初始化期关联

Perf 通过 `REG_INIT(0, perf_init)` 接入初始化链，并独立扫描 `SECTION_PERF`：

- `SECTION_PERF_BASE` 提供自由运行硬件计数器和单 tick 周期；
- `SECTION_PERF_RECORD` 提供代码段、任务和中断 record；
- record 被串成链表、分配 ID，并统一指向当前时间基准。

Section 主初始化只识别任务和中断描述符，不负责建立 Perf 链表。这避免调度器承担统计模块的对象管理。

### 2.3 运行期 hook

Section 在用户函数的边界调用：

```text
任务：begin → 任务函数或 step 函数 → end
中断：begin → 注册的中断回调 → end
```

hook 测量的是注册回调本身，不包含任务查找、队列操作和中断链表遍历。测量边界因此稳定地对应“用户工作量”，而不是整个调度器开销。

## 3. 弱符号解耦

三套 `section.c` 都提供 Perf hook 的弱实现。Perf 后端编入工程时，`perf.c` 中的同名强符号覆盖弱实现；没有后端时，弱实现保持空行为。

这种设计把依赖分成两层：

- 结构层：Section 知道 record 指针和 hook 签名；
- 实现层：Section 不要求最终镜像必须包含 Perf 统计器。

结果是调度逻辑不需要持有 Perf 全局状态，也不需要通过函数表初始化 Perf。

## 4. 任务测量脉络

任务插入运行时链表时，Section 根据 `t_period` 和 `SECTION_SYS_TICK_UNIT_US` 写入 record 的配置周期。每次实际执行任务时：

1. `section_perf_task_begin()` 读取硬件计数器；
2. 更新相邻两次开始间隔和上次结束到本次开始间隔；
3. 标记当前正在测量的任务；
4. Section 执行任务函数或 step 函数；
5. `section_perf_task_end()` 再次读取计数器；
6. 扣除任务执行期间累计的中断时间；
7. 更新最近耗时、峰值和统计窗口累计值。

配置周期与实测周期被有意分开：`period_us` 表示调度目标，`start_to_start_time` 表示实际启动间隔。两者的差异可以反映调度延迟和抖动。

## 5. 中断测量脉络

`section_interrupt()` 按优先级遍历注册回调。启用中断测量时，每个回调独立执行 begin/end：

```text
读取开始计数
  → 执行一个注册中断回调
  → 读取结束计数
  → 更新该回调 record
  → 若任务正在运行，将本次中断耗时累计到任务中断时间
```

任务结束时减去这部分时间，从而避免把 Section 管理的中断回调执行时间同时计入任务负载和中断负载。

中断 hook 和 `section_interrupt()` 热路径使用 `FUNC_RAM`，平台可以将其放入 RAM，减少 Flash wait-state 对测量值和中断延迟的影响。

## 6. 时间基准原则

Section 不读取某一种固定定时器。平台通过 `REG_PERF_BASE_CNT()` 注册一个单调递增、自然回卷的 32 位计数器及其真实 tick 周期。

begin/end 使用无符号减法计算差值，因此只要一次被测区间不跨越超过一个完整计数周期，单次回卷仍能正确计算。热路径保留原始计数，单位换算放在统计或输出阶段，避免浮点和除法进入任务、中断边界。

## 7. 编译期开关

| 开关 | 影响范围 |
| --- | --- |
| `PERF_TASK_ENABLE` | 任务描述符 record 字段、任务 record 注册和任务 hook |
| `PERF_INTERRUPT_ENABLE` | 中断描述符 record 字段、中断 record 注册和中断 hook |
| `PERF_CODE_ENABLE` | 手工 `PERF_START/PERF_END` 代码段测量 |
| `PERF_ENABLE` | Perf 公共结构和 Section 总体 hook 路径 |

中断开关在预处理阶段选择“带 hook”或“直接调用”路径，运行时没有额外的 enable 判断。

## 8. 统计边界

Perf 后端按固定窗口汇总 record 的 `run_time`，分别得到任务总负载和中断总负载。Section 只产生可靠的边界时间戳，不决定统计窗口、输出格式或 record 字典协议。

当前协作模型具有以下边界：

- 只自动测量通过 Section 调度的任务和注册中断回调；
- 硬件 ISR 在调用 `section_interrupt()` 前后的代码不属于中断 record；
- 调度器自身的队列、查找和切换开销不属于任务 record；
- 单一“当前任务”指针用于扣除中断时间，依赖任务执行边界不重叠；
- 中断嵌套若造成测量区间重叠，需要平台避免重复累计；
- 未注册有效时间基准时，record 可以存在，但测量值没有有效硬件时间含义。

这些边界让测量结果更接近业务执行成本，同时避免 Perf 深度侵入调度器实现。

## 9. 关联导航

### 应用文档

- [Section 使用文档](../../../application/framework/section/section_usage.md)
- [Perf 使用文档](../../../application/debug/perf/perf_usage.md)

### 基础教材

- [嵌入式通信分发、性能与可靠性基础](../../../tutorial/communication_performance_reliability.md)
- [实时任务与调度基础](../../../tutorial/realtime_task_scheduling.md)
