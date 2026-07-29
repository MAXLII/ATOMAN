# 故障现场诊断使用方法

## 1. 诊断对象

Cortex-M SRTOS 工程通过 `g_section_fault_debug` 保存处理器异常现场、当前任务信息和上下文池状态。该对象用于调试器停机查看；当前数据位于普通 RAM，复位后不保证保留。

## 2. 推荐诊断顺序

1. 保留发生故障时对应版本的 ELF 和链接映射文件。
2. 调试器连接后读取 `g_section_fault_debug`。
3. 先看 `task_fault_reason` 和 `task_fault_policy`，区分任务主动报告与 CPU 异常。
4. 查看 `hfsr`、`cfsr`，再根据有效位判断 `bfar`、`mmfar` 是否可信。
5. 根据 `exc_return` 确认异常来自 MSP 还是 PSP，以及是否存在扩展浮点现场。
6. 对 `stacked_pc`、`stacked_lr` 和 `task_pc` 做符号化。
7. 检查任务栈边界、已用空间、上下文池和保存/释放失败计数。

## 3. 地址符号化

GNU 工具链可使用目标工程对应的 `addr2line`：

```text
arm-none-eabi-addr2line -e firmware.elf -f -C 0x08001234
```

地址必须来自同一份固件。若启用了优化或链接时优化，源码行可能折叠，应结合反汇编和调用上下文判断。

## 4. Cortex-M 状态寄存器

- `HFSR` 指示硬故障来源及是否由可配置异常升级而来。
- `CFSR` 汇总存储管理、总线和用法异常。
- `BFAR` 仅在总线故障地址有效位置位时解释。
- `MMFAR` 仅在存储管理地址有效位置位时解释。
- `stacked_xpsr` 可用于检查异常前执行状态和 Thumb 状态。

不要只看出错地址。无效指针、栈破坏、错误返回地址和浮点现场解释错误都可能在相邻指令才触发异常。

## 5. 栈与调度器信息

若 CPU 寄存器没有直接说明原因，应继续检查：

- `task_stack_start`、`task_stack_size` 与 `task_stack_used`；
- `task_sp` 是否位于任务栈合法范围；
- `context_used` 与 `context_capacity`；
- `save_fail_count` 与 `release_fail_count`；
- 当前任务的 `task_pc` 与保存的 `task_xpsr`。

处理器异常和调度器资源耗尽属于不同故障域，必须分别解释，不能把所有停机都归因于 HardFault。

## 6. 发布构建要求

用于现场问题复盘的发布物应同时归档：固件二进制、ELF、map、版本号和构建选项。只有二进制文件不足以把地址稳定映射回源代码。

## 7. 关联导航

- 源码：[SRTOS-M Section 接口](../../../code/section/srtos_m/section.h) · [HC32F334 异常入口](../../../platform/hc32f334/src/hc32f334_it.c)
- 设计：[故障现场诊断设计](../../design/debug/fault_diagnosis_design.md) · [调试与观测系统总设计](../../design/debug/debug_system_design.md)
