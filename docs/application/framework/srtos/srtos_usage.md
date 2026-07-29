# SRTOS 使用方法

## 1. 文档目标

本文说明业务模块如何使用 SRTOS 注册任务、组织长流程、调整调度参数并验证运行状态。

平台首次接入见：

- [M 系列 SRTOS 接入](srtos_m_porting.md)
- [A 系列 SRTOS 接入](srtos_a_porting.md)

调度思想、共享栈模型和优缺点见 [SRTOS 设计思想](../../../design/framework/srtos/srtos.md)。Section 的其他注册能力见 [Section 使用文档](../section/section_usage.md)。

## 2. 用户代码与内部代码边界

使用 SRTOS 时，代码分为三类：

| 类型 | 用户操作 | 内容 |
| --- | --- | --- |
| 业务任务 | 编写和修改 | 任务函数、任务上下文、周期和完成条件 |
| 工程配置 | 按资源修改 | 运行栈、现场池、时间片和 Ready 连选上限 |
| SRTOS 内部 | 只调用公共接口 | 队列、现场保存、栈复制和任务状态转换 |

业务代码统一包含：

```c
#include "section.h"
```

业务代码不直接修改 `reg_task_t`，不访问 Ready 队列，不保存任务 SP，也不调用 `section_task_switch_sp()`。这些内容由 SRTOS 与平台切换入口共同管理。

## 3. 注册普通周期任务

使用毫秒周期：

```c
#include "section.h"

static void status_sample_task(void)
{
    /* 用户修改：读取已经准备好的数据并更新业务状态。 */
}

REG_TASK_MS(10u, status_sample_task)
```

使用系统 tick 周期：

```c
static void fast_service_task(void)
{
    /* 用户修改：执行一个短小、可直接返回的服务步骤。 */
}

REG_TASK(1u, fast_service_task)
```

用户可以修改：

- 任务函数名。
- 函数中的业务逻辑。
- `REG_TASK_MS()` 的毫秒周期。
- `REG_TASK()` 的 tick 数。

注册宏应放在模块 `.c` 文件中。任务函数和注册对象都具有静态生命周期，不需要运行期创建或释放。

`REG_TASK_MS(10u, ...)` 表示任务每 10 ms 获得一次新的执行资格。任务上一次执行尚未完成时，不会为同一任务叠加第二个实例。

## 4. 普通任务的使用原则

普通任务函数允许被 SRTOS 在时间片边界切出，并在之后从原调用位置继续。但是任务仍应遵守以下规则：

- 不无限等待一个可能永远不到来的硬件状态。
- 不跨时间片长期关闭中断。
- 不跨时间片持有要求立即释放的自旋锁或外设独占锁。
- 不把任务周期理解为硬实时执行时刻。
- 访问被中断同时修改的数据时，仍按平台并发规则保护。

适合普通任务的代码：

```c
static void calculate_task(void)
{
    uint32_t sum = 0u;

    for (uint32_t index = 0u; index < 1024u; ++index)
    {
        sum += calculate_one_item(index);
    }

    result_publish(sum);
}

REG_TASK_MS(20u, calculate_task)
```

局部变量 `sum`、`index` 和函数调用关系会随任务现场保存。用户不需要为了被切出而把所有局部变量改成全局变量。

## 5. 使用分步任务

等待 Flash、通信、DMA 或外部状态时，优先把流程写成分步任务。分步任务主动描述“本次是否完成”，比在普通任务中忙等更节省 CPU。

### 5.1 带上下文的分步任务

```c
#include "section.h"

typedef struct
{
    uint32_t state;
    uint32_t offset;
    uint32_t retry_count;
} update_task_context_t;

static update_task_context_t s_update_context = {
    .state = 0u,
    .offset = 0u,
    .retry_count = 0u,
};

static section_task_status_t update_task_step(void *p_context)
{
    update_task_context_t *p_update = (update_task_context_t *)p_context;

    switch (p_update->state)
    {
        case 0u:
            update_start();
            p_update->state = 1u;
            return SECTION_TASK_RUNNING;

        case 1u:
            if (update_is_busy() != 0u)
            {
                return SECTION_TASK_RUNNING;
            }
            p_update->state = 2u;
            return SECTION_TASK_RUNNING;

        case 2u:
            update_finish();
            p_update->state = 0u;
            return SECTION_TASK_DONE;

        default:
            p_update->state = 0u;
            return SECTION_TASK_DONE;
    }
}

REG_TASK_STEP_CTX_MS(1u, update_task_step, &s_update_context)
```

用户可以修改：

- 上下文结构体中的业务字段。
- 状态编号及各状态操作。
- `SECTION_TASK_RUNNING` 和 `SECTION_TASK_DONE` 的返回条件。
- 注册周期和上下文对象。

返回值含义：

| 返回值 | 行为 |
| --- | --- |
| `SECTION_TASK_RUNNING` | 当前工作尚未完成，进入未完成任务序列，等待后续继续推进 |
| `SECTION_TASK_DONE` | 本轮工作完成，任务回到 Sleeping，等待下一个周期 |

上下文对象必须在任务整个生命周期内有效，通常使用文件内 `static` 对象。不能把自动局部变量地址作为注册上下文传入。

### 5.2 不带显式上下文的分步任务

```c
static section_task_status_t monitor_task_step(void)
{
    if (monitor_one_step() == 0u)
    {
        return SECTION_TASK_RUNNING;
    }

    return SECTION_TASK_DONE;
}

REG_TASK_STEP_MS(5u, monitor_task_step)
```

这种形式适合状态已经由模块内部静态对象持有的流程。

## 6. 主动让出执行权

普通任务可以调用：

```c
section_task_yield();
```

它只请求一次调度机会。是否切换以及选择哪个任务仍由 SRTOS 决定。

主动让出适合以下位置：

- 已完成一段较大的纯计算。
- 已发布一批数据，希望先让其他 Ready 任务运行。
- 一个循环可以安全地从循环边界继续。

示例：

```c
static void batch_task(void)
{
    for (uint32_t batch = 0u; batch < 8u; ++batch)
    {
        process_one_batch(batch);
        section_task_yield();
    }
}

REG_TASK_MS(100u, batch_task)
```

不要在关闭中断、持有不可重入外设或数据处于半提交状态时主动让出。

## 7. 用户可调整的调度参数

以下宏可以由平台配置或构建参数覆盖：

```c
#define SECTION_TASK_RUNTIME_STACK_WORDS 768u
#define SECTION_TASK_CONTEXT_POOL_WORDS 2048u
#define SECTION_TASK_SLICE_TICKS 10u
#define SECTION_TASK_READY_BURST_MAX 4u
```

| 参数 | 用户调整依据 | 调大后的主要影响 |
| --- | --- | --- |
| `SECTION_TASK_RUNTIME_STACK_WORDS` | 单个任务最大调用深度 | 增加静态 RAM，占用不随任务数增长 |
| `SECTION_TASK_CONTEXT_POOL_WORDS` | 同时悬挂任务的实际现场总量 | 增加静态 RAM，可容纳更多悬挂现场 |
| `SECTION_TASK_SLICE_TICKS` | 允许的连续运行时间 | 减少切换开销，但增加其他任务等待时间 |
| `SECTION_TASK_READY_BURST_MAX` | 新任务响应与旧任务推进的平衡 | 新任务偏好增强，未完成任务等待可能增加 |

构建参数覆盖示例：

```make
C_DEFS += -DSECTION_TASK_RUNTIME_STACK_WORDS=768u
C_DEFS += -DSECTION_TASK_CONTEXT_POOL_WORDS=2048u
C_DEFS += -DSECTION_TASK_SLICE_TICKS=10u
C_DEFS += -DSECTION_TASK_READY_BURST_MAX=4u
```

用户应根据测得的栈高水位、现场池峰值和任务响应时间调整，而不是只按任务数量估算。

## 8. 故障策略配置

默认现场池不足时进入故障处理：

```c
#define SECTION_TASK_CONTEXT_POOL_FULL_POLICY SECTION_TASK_CONTEXT_POOL_FAULT
```

也可以配置为保存失败时让当前任务继续：

```c
#define SECTION_TASK_CONTEXT_POOL_FULL_POLICY SECTION_TASK_CONTEXT_POOL_KEEP_RUNNING
```

用户修改的是策略宏，不修改现场池分配代码。

`FAULT` 适合要求执行现场必须可信的系统。`KEEP_RUNNING` 能避免立即停止当前任务，但其他任务可能长期得不到执行，因此必须结合看门狗和运行监控评估。

## 9. 初始化与主循环

完成平台接入后，应用入口保持统一：

```c
int main(void)
{
    platform_clock_init();
    platform_timebase_init();
    section_port_init();
    section_init();
    platform_schedule_tick_start();

    for (;;)
    {
        run_task();
    }
}
```

用户替换 `platform_*` 函数为当前工程的真实初始化函数。以下调用名称和顺序属于 Section/SRTOS 接口：

1. 时间基准必须在 `section_init()` 前可读取。
2. `section_port_init()` 建立平台切换能力。
3. `section_init()` 扫描注册表并执行初始化项。
4. 周期中断开始后持续调用 `run_task()`。

## 10. 运行观察

通用故障信息通过：

```c
g_section_fault_debug
```

重点观察：

- 当前任务名称。
- 运行栈剩余空间。
- 现场池容量与使用量。
- 保存现场所需空间。
- 保存、恢复或释放失败原因。

支持更完整调度统计的平台还可以观察任务切换次数、Ready/未完成选择次数、现场池高水位和运行栈峰值。

## 11. 使用验收

接入业务任务后至少验证：

1. 短周期任务能够持续运行。
2. 长任务运行期间，其他任务仍能获得执行权。
3. 被切出的普通任务能够保持局部变量和调用关系。
4. 分步任务返回 RUNNING 后可以继续，返回 DONE 后按周期重新激活。
5. 同时运行最大业务组合时，公共运行栈和现场池仍有余量。
6. tick 回绕时，周期判断仍保持无符号时间差语义。
7. 故意缩小现场池时，系统按配置的故障策略执行。
8. 业务代码没有直接修改 SRTOS 队列、任务状态或保存指针。

## 12. 常见误用

| 误用 | 结果 | 正确方式 |
| --- | --- | --- |
| 把任务周期当作硬实时执行时刻 | 忽略时间片和中断占用造成的延迟 | 对硬实时逻辑使用硬件中断，任务处理后台工作 |
| 在任务中无限等待硬件 | 虽可被抢占，但任务永远无法完成 | 使用 `REG_TASK_STEP_CTX_MS()` 轮询状态 |
| 用自动局部对象作为分步上下文 | 函数返回后指针失效 | 使用静态或模块生命周期对象 |
| 只按任务数配置现场池 | 深调用任务同时悬挂时容量不足 | 根据现场池峰值和最坏业务组合配置 |
| 直接调用上下文切换内部接口 | 破坏任务状态与平台入口配合 | 只使用注册宏、`run_task()` 和 `section_task_yield()` |
| 同时编译多套 Section 运行时 | 重复符号或接口不一致 | 一个构建目标只选择 baremetal、srtos_m、srtos_a9 中一套 |

## 13. 关联导航

### 源代码

- [M 系列 SRTOS 头文件](../../../../code/section/srtos_m/section.h)
- [M 系列 SRTOS 实现](../../../../code/section/srtos_m/section.c)
- [A 系列 SRTOS 头文件](../../../../code/section/srtos_a9/section.h)
- [A 系列 SRTOS 实现](../../../../code/section/srtos_a9/section.c)

### 设计文档

- [SRTOS 设计思想](../../../design/framework/srtos/srtos.md)
- [Section 设计文档](../../../design/framework/section/section_design.md)
