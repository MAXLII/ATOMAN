# SRTOS 测试用例

## 1. 测试目标

本文档定义 SRTOS 的基础功能、时间片调度、上下文保存恢复、公共现场池、PSP 运行栈、串口压力和异常保护测试用例。

测试关注点：

- 新 Ready 任务优先运行。
- 无新 Ready 任务时恢复最早未完成任务。
- 长任务被切出后，局部变量、调用链和返回位置能够恢复。
- 公共现场池满时，两种策略行为正确。
- PSP 运行栈越界或配置过小时能够进入保护。
- 低速串口输出压力下，RTOS 不出现 fault、保存失败或释放失败。

## 2. 观测变量

建议每个用例至少观测以下变量：

| 变量 | 说明 | 期望 |
| --- | --- | --- |
| `g_section_fault_debug.task_fault_reason` | RTOS fault 原因 | 正常用例为 `0` |
| `g_section_fault_debug.task_fault_policy` | 公共现场池满策略 | 与配置一致 |
| `g_section_fault_debug.task_context_required_words` | 本次需要保存的现场大小 | 故障注入时用于确认原因 |
| `g_section_fault_debug.task_context_pool_words` | 公共现场池容量 | 与配置一致 |
| `g_section_fault_debug.task_context_pool_used` | 公共现场池当前占用 | 不超过容量 |
| `g_section_fault_debug.task_context_save_fail_count` | 现场保存失败次数 | 正常用例为 `0` |
| `g_section_fault_debug.task_context_release_fail_count` | 现场释放失败次数 | 正常用例为 `0` |
| `g_section_fault_debug.task_stack_free_words` | PSP 运行栈剩余水位 | 大于安全余量 |

## 3. 基础运行测试

| 编号 | 用例 | 配置 | 操作 | 期望 |
| --- | --- | --- | --- | --- |
| SRTOS-BASIC-001 | 默认配置长跑 | 默认配置 | 运行 20s、1min、10min | `fault_reason=0`，`save_fail=0`，`release_fail=0` |
| SRTOS-BASIC-002 | 两个长任务并行 | 100ms/123ms 打印任务 | 观察串口周期输出 | 两个任务持续输出，互不阻塞 |
| SRTOS-BASIC-003 | 多个轻任务加一个长任务 | 3 个 100us 任务 + 1 个长任务 | 观察轻任务执行周期 | 轻任务不被长任务长期饿死 |
| SRTOS-BASIC-004 | 无新任务恢复未完成任务 | 默认配置 | 长任务被切出后等待 Ready 队列为空 | 最早未完成任务被恢复运行 |

## 4. 时间片调度测试

| 编号 | 用例 | 配置 | 操作 | 期望 |
| --- | --- | --- | --- | --- |
| SRTOS-SLICE-001 | 高频切换压力 | `SECTION_TASK_SLICE_TICKS=1` | 运行 10s | 无 fault，无保存释放失败 |
| SRTOS-SLICE-002 | 默认时间片 | `SECTION_TASK_SLICE_TICKS=10` | 运行 20s | 调度正常 |
| SRTOS-SLICE-003 | 低频切换 | `SECTION_TASK_SLICE_TICKS=50` | 运行 10s | 调度正常，现场池占用相对降低 |
| SRTOS-SLICE-004 | 多任务同一 tick 到期 | 多个相同周期任务 | 观察执行顺序 | 按 `REG_TASK` 扫描顺序进入 Ready 队列 |
| SRTOS-SLICE-005 | Ready 连续很多任务 | 多个短周期任务 | 观察未完成任务是否恢复 | `READY_BURST_MAX` 后未完成任务能得到执行机会 |

## 5. 上下文保存恢复测试

| 编号 | 用例 | 配置 | 操作 | 期望 |
| --- | --- | --- | --- | --- |
| SRTOS-CTX-001 | 长任务局部变量保持 | 长任务内使用局部计数变量 | 多次切出/恢复 | 局部变量连续，不跳变 |
| SRTOS-CTX-002 | 深层函数调用 | 长任务内多层函数调用 | 在调用链深处被切出 | 返回地址和调用链恢复正确 |
| SRTOS-CTX-003 | 浮点任务 | 任务内执行浮点计算 | 多次切出/恢复 | 计算结果连续，无异常 |
| SRTOS-CTX-004 | 恢复后完成 | 未完成任务恢复后自然结束 | 观察现场池 | 任务完成后不再进入未完成队列 |
| SRTOS-CTX-005 | 恢复后再次被切出 | 恢复任务继续长时间运行 | 再次触发时间片 | 新现场进入未完成队列尾部 |
| SRTOS-CTX-006 | 任务时长时短 | 同一任务随机或按计数交替执行短路径和长路径 | 短路径一次完成，长路径跨多个时间片 | 短路径不占现场池；长路径被保存恢复；任务状态不残留、不泄漏 |

## 6. 任务时长时短专项测试

该用例用于验证同一个任务在不同周期内，有时一次性完成，有时成为未完成任务时，调度状态和公共现场池都能正确处理。

建议任务行为：

```c
static void task_variable_length(void)
{
    static uint32_t count;
    uint32_t local_guard = 0x12345678u;

    count++;
    if ((count % 3u) == 0u)
    {
        /* 长路径：运行时间超过一个时间片，触发切出和恢复。 */
        variable_length_busy_work();
    }
    else
    {
        /* 短路径：快速完成，不应进入未完成队列。 */
        local_guard ^= count;
    }

    variable_length_check(local_guard, count);
}
```

| 检查项 | 期望 |
| --- | --- |
| 短路径周期 | 任务完成后进入 sleeping 状态，不保存现场 |
| 长路径周期 | 任务被切出后进入未完成队列，恢复后继续执行 |
| 长短交替 | 不出现重复入队、状态残留或现场池泄漏 |
| 局部变量 | 长路径被切出后恢复仍然正确 |
| 现场池 | `task_context_pool_used` 随保存/恢复变化，不单调增长 |
| 错误计数 | `save_fail=0`，`release_fail=0` |

## 7. 公共现场池测试

| 编号 | 用例 | 配置 | 操作 | 期望 |
| --- | --- | --- | --- | --- |
| SRTOS-POOL-001 | 默认 4KB 现场池 | `SECTION_TASK_CONTEXT_POOL_WORDS=1024` | 长跑并采样 | 占用低于容量 |
| SRTOS-POOL-002 | 小现场池压力 | `SECTION_TASK_CONTEXT_POOL_WORDS=64` | 运行长任务压力 | 不足时触发配置策略 |
| SRTOS-POOL-003 | 现场池满 fault 策略 | `SECTION_TASK_CONTEXT_POOL_WORDS=16`，默认策略 | 运行到保存失败 | `fault_reason=1` |
| SRTOS-POOL-004 | 现场池满继续当前任务策略 | `SECTION_TASK_CONTEXT_POOL_WORDS=16`，`SECTION_TASK_CONTEXT_POOL_KEEP_RUNNING` | 运行 5s | `fault_reason=0`，`save_fail` 累加 |
| SRTOS-POOL-005 | 环形 head/tail 回绕 | 默认或小现场池 | 长时间保存/恢复 | head/tail 正常回绕，`release_fail=0` |

## 8. PSP 运行栈测试

| 编号 | 用例 | 配置 | 操作 | 期望 |
| --- | --- | --- | --- | --- |
| SRTOS-STACK-001 | 默认 512 words | `SECTION_TASK_RUNTIME_STACK_WORDS=512` | 长跑采样水位 | 不越界 |
| SRTOS-STACK-002 | 64 words 压力 | `SECTION_TASK_RUNTIME_STACK_WORDS=64` | 运行长任务 | 可运行，水位接近但不越界 |
| SRTOS-STACK-003 | 32 words 过小保护 | `SECTION_TASK_RUNTIME_STACK_WORDS=32` | 启动任务 | `fault_reason=5` |
| SRTOS-STACK-004 | 大局部数组 | 任务内定义较大局部数组 | 触发切换 | 越界时进入 PSP 保护 |
| SRTOS-STACK-005 | 恢复异常大现场 | 故障注入异常快照大小 | 恢复任务 | `fault_reason=3` |

## 9. 串口低速压力测试

| 编号 | 用例 | 配置 | 操作 | 期望 |
| --- | --- | --- | --- | --- |
| SRTOS-UART-001 | 921600 波特率 | 默认 | 采集 6s 输出 | RTOS 无 fault |
| SRTOS-UART-002 | 115200 波特率 | 串口配置 115200 | 采集 6s 输出 | RTOS 无 fault |
| SRTOS-UART-003 | 57600 波特率 | 串口配置 57600 | 采集 6s 输出 | RTOS 无 fault |
| SRTOS-UART-004 | 38400 波特率 | 串口配置 38400 | 采集 6s 输出 | RTOS 无 fault |
| SRTOS-UART-005 | 19200 波特率 | 串口配置 19200 | 采集 6s 输出 | RTOS 无 fault，允许少量输出截断 |
| SRTOS-UART-006 | 9600 波特率 | 串口配置 9600 | 采集 6s 输出 | RTOS 无 fault，允许少量输出截断或坏行 |

## 10. 异常保护测试

| 编号 | 用例 | 配置 | 操作 | 期望 |
| --- | --- | --- | --- | --- |
| SRTOS-FAULT-001 | 现场池满 | 小现场池 + 默认策略 | 触发保存失败 | `fault_reason=1` |
| SRTOS-FAULT-002 | PSP 越界 | 大局部变量或异常 SP 注入 | 触发切换 | `fault_reason=2` |
| SRTOS-FAULT-003 | 恢复现场过大 | 异常快照大小注入 | 恢复任务 | `fault_reason=3` |
| SRTOS-FAULT-004 | 现场释放顺序错误 | release 顺序注入 | 恢复释放 | `fault_reason=4` |
| SRTOS-FAULT-005 | 运行栈配置过小 | `SECTION_TASK_RUNTIME_STACK_WORDS=32` | 启动任务 | `fault_reason=5` |

## 11. 当前已实测结果

| 用例 | 结果 |
| --- | --- |
| 默认 20s | 通过 |
| `SECTION_TASK_SLICE_TICKS=1` | 通过 |
| `SECTION_TASK_SLICE_TICKS=50` | 通过 |
| `SECTION_TASK_CONTEXT_POOL_WORDS=64` | 正确触发现场池保护 |
| `SECTION_TASK_CONTEXT_POOL_WORDS=16` 默认策略 | 正确触发 `fault_reason=1` |
| `SECTION_TASK_CONTEXT_POOL_WORDS=16` keep running 策略 | 不进入 fault，`save_fail` 累加 |
| `SECTION_TASK_RUNTIME_STACK_WORDS=64` | 通过 |
| `SECTION_TASK_RUNTIME_STACK_WORDS=32` | 正确触发 `fault_reason=5` |
| 串口 `921600` 到 `9600` | RTOS 无 fault，无保存释放失败 |

