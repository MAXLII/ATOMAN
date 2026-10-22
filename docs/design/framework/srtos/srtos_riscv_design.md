# RISC-V SRTOS 设计

## 1. 定位与文件边界

`code/section/srtos_riscv/` 是与 `srtos_m/`、`srtos_a9/` 并列的 RV32 整数现场实现。它沿用 Section 的静态注册及 SRTOS 的统一 FIFO、固定全局时间片和共享栈模型，调度原则见 [SRTOS 设计思想](srtos.md)。本目录不包含 ESP-IDF、Wi-Fi 或 FreeRTOS API。

| 文件 | 职责 |
|---|---|
| `section.c`、`section.h` | 注册发现、周期资格、统一 FIFO、共享运行栈、挂起现场池、临界区及故障记录 |
| `section_riscv_port.S` | RV32 整数寄存器恢复、主动 yield 保存，以及返回宿主 dispatcher |
| `section_riscv_port.h` | 架构现场接口及由平台提供的 idle、housekeeping、fault 边界 |
| `code/section/platform.h` | 平台身份和能力映射，包括 CLIC 初始返回值；架构代码只使用映射后的契约 |
| `platform/esp32c5/bsp/bsp_srtos.c/.h`、`bsp_srtos_hooks.S` | ESP-IDF 外层中断边界、回调绑定、SDK 让步及外部调用 guard |
| `platform/esp32c5/src/app_main.c` | 提供宿主栈内运行区，初始化后进入 `section_riscv_run()` |

当前适配对象是单核 ESP32-C5 的整数执行环境及 ESP-IDF 6.1 中断帧 ABI。其他 RV32 平台复用调度模型时，必须实现自己的异常返回、时钟和平台回调；有 FPU 或其他协处理器时应先扩展现场，不得直接复用整数上下文宣称支持。

## 2. 宿主与内存模型

应用的 `REG_TASK` 全部由 SRTOS 调度。ESP-IDF 的 Wi-Fi、lwIP、事件循环及系统服务仍由 SDK 自身的 FreeRTOS 运行时调度；SRTOS 在 SDK main 任务中运行，保留 SDK 对其 TCB、栈界限和中断退出的所有权。

| 资源 | 当前默认值 | 计入方式 |
|---|---|---|
| SDK main 栈 | 8192 字节 | SDK 宿主的整体栈预算 |
| SRTOS 公共运行区 | 1024 word，即 4096 字节 | `app_main()` 的 16 字节对齐局部数组，包含在 main 栈内，不额外重复计入 |
| 挂起现场池 | 512 word，即 2048 字节 | 独立静态数组，保存同时挂起任务实际用过的栈片段 |
| IRQ shadow | 36 word，即 144 字节 | 暂存中断现场，保护 SDK 返回过程中的寄存器原值 |
| dispatcher 保存区 | 64 字节 | 汇编保存宿主返回地址、SP、callee-saved 寄存器及状态 |

另有任务描述符、就绪链表、池索引和调试计数；表中不是全固件 RAM 总量。运行区低端预留 4 word canary，故 4096 字节并非全部可用任务栈。SDK main 栈剩余部分还要承载启动、dispatcher、SDK 调用及中断进入前的短序言，需要结合真实调用深度检查。

每个任务不分配独立栈。抢占时只复制从保存现场 SP 到运行区顶端的有效片段；恢复前将快照复制回同一运行地址，因此原有栈内指针仍指向原地址。现场池按 FIFO 分配/释放并处理尾端回绕空隙，不能绕过队首恢复较新的挂起任务。

运行区越界、现场池不足、释放顺序错误或池计数损坏都必须留下调试原因；默认现场池不足策略为 fault，不丢弃现场继续运行。最终栈和池容量需要用长任务、嵌套调用及真实 SDK 操作的峰值验证。

## 3. 周期与公平调度

SECTION 时间单位为 100 us，`SECTION_TASK_SLICE_TICKS=10`，即全局固定 1 ms 时间片。任务完成、启动下一个任务或恢复旧任务均不重置时间片边界。SDK guard 期间只能推迟实际切换，不能把边界改成“每个任务重新得到 1 ms”。

新到期任务、时间片中断挂起的任务和主动分步任务共用一个 FIFO。普通任务在时间片到期后保存现场并入队尾，从队首选择下一项；`READY_NEW` 从入口启动，`READY_OLD` 从旧现场继续。任务本次未结束时不创建重叠实例。

周期时间戳沿用裸机语义：间隔在一个周期到两个周期之间时加一个周期，间隔达到两个周期或更长时更新为当前 tick，避免追赶过时的重复实例。主动 `section_task_yield()` 保存完整现场后进入同一恢复路径，不等同于重新执行任务入口。

统一 FIFO 约束的是入队和恢复顺序。存在 SDK 抢占、housekeeping 和其他周期任务时，不据此承诺两个相同工作量任务的入口计数严格相等；若验收严格公平性，应单独运行双任务测例，不能直接以混合负载下的计数代替判断。

## 4. 真实抢占与 SDK 返回边界

ESP32 BSP 通过链接包装 `rtos_int_enter` 和 `rtos_int_exit` 接入 SDK 边界，不替换 SDK 向量表、TCB 或完整异常处理器。

1. SDK 保存原任务的 128 字节整数 IRQ 帧后，进入包装入口。BSP 记录外层帧地址、`mstatus` 和 `mcause`，随后执行原 SDK 入口；嵌套中断只更新深度。
2. SDK 执行原中断服务，包括板级采样；在外层 `rtos_int_exit` 前、SDK 已禁止嵌套切换时，BSP 调用调度器检查时间片。
3. 只在当前虚拟任务运行、现场属于公共运行区、guard 深度为 0 且没有待处理跳转时，复制寄存器到 shadow，并把 SDK 帧中的返回 PC 改为 `section_riscv_dispatch_return`。
4. 原 SDK 退出逻辑仍负责自己的栈恢复和系统任务调度。宿主真正恢复时才进入 trampoline，回到原 dispatcher 栈；在该栈上完成公共运行区快照与 FIFO 选择。
5. 汇编恢复下一任务的整数寄存器、TLS、PC 及中断返回状态，经 `mret` 继续原指令位置。主动 yield 通过独立保存入口产生等价的 36-word 现场。

36-word 现场包含 4 word 状态前缀和 32 word SDK 整数帧。`gp` 在同一固件中保持不变，`tp` 保留宿主 TLS。ESP32 的 CLIC 先前中断级别位通过平台提供的 `mcause` 初始值及实际保存值恢复；不能套用 A9 的 CPSR 判断。BSP 使用 SDK 帧字段静态断言检查关键偏移，并限制所支持 SDK ABI。

## 5. SDK 调用保护和采样时序

所有虚拟任务共用同一个 SDK 宿主身份，因此 SDK/libc 进入过程、公共发送队列事务以及连接的读改写不能任意交错。`bsp_system_runtime_enter()/exit()` 通过绑定的 SRTOS guard 增减深度，暂缓应用任务切换；它不在整个 socket 调用期间关闭采样 IRQ。

当前保护覆盖 Interface 的 transport/radio 任务、框架 `REG_LINK` 接收与回调、公共响应入队及格式化、上传端断开等路径。临界事务结束后，从后续可抢占中断退出恢复正常调度。guard 不会让 SDK 的锁等待、网络背压或同步校准变为无耗时操作，这些耗时仍需诊断记录。

任务全部空闲时，BSP 的 `section_riscv_idle()` 通过 `esp_cpu_wait_for_intr()` 等待中断，返回后重新检查到期任务，避免每次空闲都休眠一个 SDK tick，吞掉 100 us 周期任务的运行机会。仅 housekeeping 在距上次让步至少 10 ms 时调用 `vTaskDelay(1)`，返回后更新时间戳，为 SDK idle 提供机会并避免 WDT 告警。housekeeping 在选择并恢复下一任务之前执行，避免已经选出的任务尚未执行便被 SDK 等待消耗时间片。全局时间片仍按实际 tick 推进。当前 SDK tick 为 1 kHz。该依赖集中在 BSP，应用不额外创建 FreeRTOS 任务、队列或信号量；不能据此称固件“零 FreeRTOS”。

板级控制中断与任务时间片独立。采样函数仍通过 demo 的 `REG_INTERRUPT(255, sample_interrupt)` 注册，通用 scope 不创建定时器。SDK guard 保持硬件采样开放；调度器仅对就绪队列指针和任务状态的短事务关闭 IRQ，不在完整切换过程外层加锁。现场池分配、快照复制和恢复在 dispatcher 栈上进行，期间允许采样 IRQ。IRQ hook 只接受公共运行区内的运行任务帧，因此不会把 dispatcher 的中断误识别为待抢占任务；shadow 回填完成前保持 `irq_armed`，防止嵌套覆盖。

SDK 中断退出 hook、调度器正常 IRQ 检查路径、汇编恢复边界以及主动 yield 的 `section_riscv_yield_commit()` 均放入 IRAM，避免关 IRQ 路径因指令缓存未命中而延长。异常故障处理不属于正常采样时延保证。

`SRTOS_CRITICAL_MAX` 记录调度核心 `section_critical_enter()/exit()` 的最大临界周期数，`SRTOS_CRITICAL_COUNT` 记录次数；统计不涵盖汇编恢复、外部 guard 深度修改、BSP 或 SDK 自身的关 IRQ 区域，不能代表全系统最大中断屏蔽时间。必须结合实际 ISR 时间戳评价 32 us 采样间隔。EA 帧无缺号只能说明已经采到的数据连续，不能单独证明硬件周期未延迟或漏触发。

## 6. 验证状态与实板验收

2026-10-22，ESP32-C5 实板已完成以下混合负载测试：8 路 32-bit 通道、查询采样率 31250 Hz、两个约 650 us 固定工作量的 1 ms 周期任务，同时保留 Wi-Fi 和其他注册任务。前两轮为旧空闲等待方式的历史对照，对应 ELF SHA-256 为 `674a456ff9602e3af1b18e4cd28c5b504a82a68585c7b85f7c9e93e9afff8501`；最终轮使用第 5 节的中断等待方式，双任务校准工作量均为 649 us，并开启任务中途主动 yield。

| 测例 | 采集时长 | 接收帧数 | 硬件时间戳测得频率 | 最大相邻采样间隔 | 现场检查 |
|---|---|---|---|---|---|
| 时间片抢占 | 10 s | 312784 | 31250.00695 Hz | 56 us | fault、invariant、寄存器/栈错误均为 0 |
| 加入任务中途主动 yield | 8 s | 250011 | 31250.01035 Hz | 57.279 us | fault、invariant、寄存器/栈错误均为 0 |
| 新 idle：双 649 us 任务及主动 yield | 8 s | 250070 | 31250.018647 Hz | 56.75 us | SRTOS_ERRORS、FAULT、INVARIANTS 均为 0 |

本地证据分别保存在 `platform/esp32c5/build/srtos-second-8ch/result.json` 和 `platform/esp32c5/build/srtos-yield-8ch/result.json`。时间片抢占轮的调度临界最大值为 1041 CPU cycles，按 240 MHz 换算约 4.34 us；主动 yield 轮记录到 5 次超过 48 us 的相邻采样间隔。该 overdue 计数表示采样抖动，不直接等价于漏采次数，应结合硬件累计频率、采样计数和上传连续性判断。

最终轮证据为 `platform/esp32c5/build/srtos-final-probe/result.json`，结果为通过；对应 ELF SHA-256 为 `3142e0d1d919437efc46f7cadb9baa08c370296a8f59f3090673945dbd52dae1`。EA 序号缺口、拒收帧和采样计数缺口均为 0，采样队列峰值 4292 字节。记录到 11 次超过 48 us 的相邻采样间隔，仍按抖动指标解释。现场保存增加 5647 次、恢复增加 5648 次；前后逐项查询的观测窗口不同，允许相差 1，不能将两者视为同一瞬间的精确配对计数。

上述证据确认各自固件在测试时长内真实抢占及主动 yield 的现场完整性，不宣称严格 A/B 计数公平，也不替代关闭压力探针后的生产低内存、持续吞吐和 Wi-Fi 抖动验收。生产预算与最终验收由对应实测补充。

独立平台 `platform/esp32c5_srtos` 的当前配置采用 16 KiB 内部采样队列和 3072 字节上传暂存；挂起现场池仍为 2048 字节，SDK main 栈仍为 8192 字节且包含 4096 字节公共运行区。通用 scope 源码保持不变，Interface 只做上下层绑定；采样缓存与绑定重试归上层 demo，网络缓存和 SDK 查询归 BSP，容量预算由工程配置。新空闲等待方式已通过上表最终压力测试，关闭探针后的实板记录和独立工程验收见平台文档。

必要验收包括：

- 两个长任务经过多个时间片后都继续推进，寄存器及栈局部值保持；主动 yield 后继续执行原位置。严格公平性另用双任务专用测例评估。
- 固定 1 ms 边界不因任务完成重置，现场池回绕及 FIFO 释放顺序正确，canary、故障和 invariant 计数正常。
- 持续任务负载下 SDK idle 得到运行机会，没有 WDT 报警；公共控制、参数同步和断线重连继续工作。
- 示波器按设备查询频率采集；同时核对实际 ISR 间隔、队列高水位、overflow、EA 序号、上传字节量和待发时间。
- 比较内部采样队列容量、上传暂存及 lwIP 配置下的持续吞吐和内部堆峰值，当前候选为 16 KiB 采样队列和 3072 字节上传暂存。先确定稳定条件，再下调预算；Wi-Fi 重传抖动不由 SRTOS 或有限缓存消除。

ESP32 的构建、通信分工及当前低内存参数见 [ESP32-C5 SRTOS 平台](../../platform/esp32c5_srtos/esp32c5_srtos_platform.md)。

`platform/esp32c5` 保留裸机，`platform/esp32c5_srtos` 链接本核心。两个平台的 BSP、Interface、配置及脚本独立，不通过源文件或包含路径互相依赖；公共 `platform.h` 分别选择各自的平台标识和头文件。
