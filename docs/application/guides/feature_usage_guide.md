# 公共功能接入总览

## 1. 文档定位

本文是公共功能的接入入口，用于确定一个平台工程需要选择哪些软件能力以及按什么顺序接入。具体接口、示例和验收要求由对应应用文档维护，本文件不重复展开各模块操作。

## 2. 最小运行骨架

使用 Section 公共能力的平台需要完成三个入口：

```c
int main(void)
{
    bsp_init();
    section_port_init();
    section_init();

    while (1)
    {
        run_task();
    }
}
```

需要统一高速回调时，在对应硬件中断中调用：

```c
void control_irq_handler(void)
{
    bsp_control_irq_clear();
    section_interrupt();
}
```

平台首先保证时钟、中断、内存和外设处于安全状态，再把业务能力注册到 Section。

## 3. 能力入口

| 目标 | 应用文档 | 主要平台准备 |
|---|---|---|
| 初始化、任务、中断和 FSM | [Section 使用](../framework/section/section_usage.md) | tick、ISR 入口和链接段 |
| 串口或其他物理链路 | [FRAME 通信接入](../communication/frame_usage.md) | 收发驱动、Link 和解析上下文 |
| 新增二进制命令 | [通信命令开发](../communication/command_development_usage.md) | 命令号和业务处理入口 |
| 在线变量与命令 | [Shell 使用](../debug/shell/shell_usage.md) | Link 和可选文本输出 |
| 执行时间与负载 | [Perf 使用](../debug/perf/perf_usage.md) | 自由运行计数器 |
| 波形捕获 | [Scope 使用](../debug/scope/scope_usage.md) | 采样调用点和静态缓冲区 |
| 事件轨迹 | [Trace 使用](../debug/trace/trace_usage.md) | 时间源和记录点 |
| 闭环扫频 | [SFRA 使用](../debug/sfra/sfra_usage.md) | 控制环注入与采样点 |
| 控制模块 | [控制模块通用接入](../control/ctrl_usage.md) | timing、HAL、setpoint 和 FSM |
| Flash 管理 | [FAL 接入](../storage/fal_usage.md) | Flash 驱动、geometry 和分区表 |
| 固件升级 | [Bootloader 升级运行](../bootloader/bootloader_upgrade_usage.md) | FAL、平台启动接口和通信链路 |

## 4. 推荐接入顺序

1. 建立平台构建、链接、启动和最小主循环。
2. 接入 Section 初始化、tick 和中断入口。
3. 建立一条可验证的 Link，再接入 FRAME 或 Shell。
4. 接入 Perf，确认任务和中断周期符合设计。
5. 按需加入 Scope、Trace 和 SFRA，不默认同时开启全部调试能力。
6. 接入控制模块时，先完成采样与 PWM 安全验证，再允许 FSM 启动。
7. 接入 FAL 和 Bootloader 时，最后核对链接布局和掉电恢复路径。

每一步都应独立可验证。不要在通信、控制、存储和升级尚未分别跑通前一次性组合全部功能。

## 5. 示例入口

`code/app/demo/` 提供静态注册、任务、通信、Shell、Perf、Scope、Trace 和 SFRA 的最小代码样例。样例清单与构建入口见[公共功能演示](../examples/demo.md)。

示例用于查看调用形态，不替代模块应用文档中的容量、并发、安全和生命周期要求。

## 6. 综合验收

- `section_init()` 后所有必需注册对象可被发现。
- `run_task()` 持续运行，周期任务没有异常饥饿。
- 硬件 ISR 清除中断源后再进入 Section 分发。
- 每条物理链路拥有独立接收缓冲区和协议上下文。
- 调试功能关闭后业务行为保持不变。
- 控制输出在未 ready、stop 和保护状态下保持安全。
- 存储和升级操作不阻塞通信回调或实时控制路径。

## 7. 关联导航

- 源码：[Section接口](../../../code/section/baremetal/section.h) · [通信接口](../../../code/comm/comm.h) · [Perf接口](../../../code/dbg/perf.h) · [示例入口](../../../code/app/demo/demo.c)
- 设计：[工程设计](../../engineering_design.md) · [公共软件组件模型](../../design/framework/component_model.md) · [Section 设计](../../design/framework/section/section_design.md)
