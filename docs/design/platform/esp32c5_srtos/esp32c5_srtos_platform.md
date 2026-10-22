# ESP32-C5 SRTOS 平台

本工程位于 `platform/esp32c5_srtos`，使用 ESP32-C5、ESP-IDF 6.1 和独立的 RV32 SRTOS 核心。`platform/esp32c5` 保留裸机 SECTION；两个平台分别维护自己的 BSP、启动入口、配置、构建脚本、链接脚本、测试和 Interface 绑定副本，不相互包含、编译或链接。

## 目录与依赖

| 位置 | 职责 |
|---|---|
| `platform/esp32c5_srtos/bsp/` | 本平台 USB、Wi-Fi、TCP、时钟、中断、内存校验及 SDK 中断返回适配 |
| `platform/esp32c5_srtos/bsp/comm_discovery.c/.h` | CommLink 使用的设备发现应答及 UDP 回环服务 |
| `platform/esp32c5_srtos/inc/` | 启动入口 `app_main.h` 和默认联网配置 |
| `platform/esp32c5_srtos/src/app_main.c` | 初始化本平台 BSP，在 SDK main 栈内启动 SRTOS |
| `platform/esp32c5_srtos/main/` | 本平台组件源清单、包含目录、链接参数与注册段保留 |
| `platform/esp32c5_srtos/gcc/compile.ps1` | 本平台独立构建和烧录脚本 |
| `platform/esp32c5_srtos/tests/` | 本平台 BSP 主机测试、抢占探针、真机采集和编译隔离检查 |
| `code/interface/demo/esp32c5_srtos/` | 字节流和会话事件转接、Wi-Fi 参数转换、示波器与 Perf 回调绑定 |
| `code/business/demo/esp32c5_srtos/` | 协议状态机、命令和诊断、采样缓存及绑定重试、SRTOS 观察参数 |
| `code/section/srtos_riscv/` | 与 SDK 解耦的 RV32 共享栈、挂起现场池及 FIFO 调度核心 |

平台标识为 `PLATFORM_ESP32C5_SRTOS`；SDK 芯片目标仍为 `esp32c5`。公共 `platform.h` 通过本平台独立的包含目录选择 Interface 的 `platform_bindings.h`，与同目录 `platform_bindings.c` 配套；启动入口单独使用 `inc/app_main.h` 与 `src/app_main.c`。TCP 通道声明与实现配套放在 BSP 的 `bsp_wifi_channels.c/.h`，不混入无线电接口 `bsp_wifi.h`。除仓库公共框架、demo 和外部 SDK 外，不引用另一个 ESP32 平台的文件；同名 BSP 代码分别存放，后续修改也分别维护。

`bsp_system_runtime_enter()/exit()` 是本平台 SDK 事务边界。SRTOS BSP 用强符号接入抢占保护；裸机工程自己的同名函数为空操作，两边不通过链接另一平台实现该接口。

## Interface 边界

Interface 是 BSP 与上层之间的桥梁，只做参数/类型转换、回调绑定、字节及会话事件转接和必要的注册接线。协议状态机、Shell 命令及文本展示、采样缓存和绑定重试生命周期归上层 demo；设备发现、网络队列、SDK 状态和堆查询归 BSP。上层 demo 不直接包含 BSP/SDK 头文件，Interface 不包含 SDK 头文件、不注册业务命令、不持有采样缓存。

`tests/test_platform_isolation.py` 同时检查这些分层依赖和两份平台之间的隔离；只共享通用框架，不通过另一平台的 BSP、Interface 或上层 demo 编译。

## 构建与隔离检查

```powershell
./platform/esp32c5_srtos/gcc/compile.ps1
./platform/esp32c5_srtos/gcc/compile.ps1 -Port COM7 -Flash
python platform/esp32c5_srtos/tests/test_drivers.py
python platform/esp32c5_srtos/tests/test_platform_isolation.py --nm <riscv32-esp-elf-nm.exe>
```

产物为本目录下 `build/esp32c5_srtos_demo.elf/.bin/.map`。`sdkconfig`、构建缓存和 `build/private/wifi_config.h` 均属于本平台自己的忽略目录；Wi-Fi 配置使用本平台脚本的 `-WifiConfigPath` 导入，不能提交到 Git。BIN/ELF 也包含联网配置，按私有产物管理。

隔离检查读取实际 `compile_commands.json`，拒绝另一平台及其 Interface、上层 demo 的源文件、包含目录和编译参数；随后检查 ELF，确认链接 RV32 SRTOS、中断入口包装和强事务保护符号。裸机平台有相同检查脚本，要求只链接裸机核心且不出现 RV32 调度器及 IRQ 包装符号。

公共纯软件、上层 demo 及 Interface 源使用 C11；包含 IDF 内联 `asm` 的适配源使用 GNU C11。严格警告保留，未修改 SDK 头文件。

## 调度与通信

- 应用 `REG_TASK` 由 SRTOS 调度；全局时间片固定 1 ms。4 KiB 公共运行栈包含在 SDK main 的 8 KiB 栈中，另有 2 KiB 挂起现场池，不能重复计算运行栈。
- 硬件中断固定为 32 us、31250 Hz。采样通过 demo 的 `REG_INTERRUPT(255, sample_interrupt)` 接入，通用示波器不建立定时器。
- SDK/libc 事务内暂缓应用任务切换，保持采样 IRQ 开放。空闲等待中断，每 10 ms 保留 SDK housekeeping 机会。
- SDK Wi-Fi、事件循环和底层运行时仍依赖 ESP-IDF 自带 FreeRTOS；应用没有另建 FreeRTOS 任务，不能称整个固件已移除 FreeRTOS。
- USB 或 TCP 5000 承担 E8 参数及示波器控制；TCP 9001 只上传数据。数据端点不接受上位机控制包。
- 周期 Wi-Fi 状态通过事件缓存更新，不调用同步 AP 查询；`wifi` 只读缓存，`wifi_rssi` 是可能阻塞的显式诊断。
- 发送按 1440 B MSS 聚合，3 KiB 暂存可一次提交 2880 B，短尾最多主动聚合 1 ms；部分发送及 EAGAIN 保留原字节顺序。

## 低内存与发送预算

| 项目 | 当前默认值 |
|---|---:|
| 内部采样队列 `BASE_SCOPE_QUEUE_BYTES` | 16384 B |
| 上传暂存 `BASE_WIFI_UPLOAD_BYTES` | 3072 B |
| 启动检查预算 `BASE_SCOPE_BYTE_BUDGET` | 1700000 B/s |
| TCP 发送窗口动态上限 | 41760 B |
| TCP 接收窗口 / 接收 mailbox | 8760 B / 8 |
| PSRAM | 关闭 |

8 路 32-bit 变量每帧 37 B，31250 Hz 对应 1156250 B/s；16 KiB 采样队列容纳 442 帧、14.144 ms。12 路时每帧 53 B、1656250 B/s，容纳 309 帧、9.888 ms。缓存按完整帧使用，尾部不足一帧的字节不能计入容量。预算限制的是应用帧字节率，不是 Wi-Fi PHY 速率；通道槽位仍为 32 个。

该配置用 16 KiB 内部队列替代早期 4 MiB PSRAM 队列。TCP 窗口是动态分配上限，Wi-Fi/SDK 堆、协议注册和代码 IRAM 另行统计；不能将 19 KiB 两个数组视为全部固件内存。

## 验证记录

拆分前的同源 SRTOS 原型固件（ELF SHA-256 `2314b5d6fa1bfa90d9ada901fb314e7b0e483c239613475dd20961f2a955c83b`）已使用真实开发板验证：

| 接收路径及负载 | 结果 |
|---|---|
| FRAME 原生后端，TCP 控制及上传，8 路、60 s | 1874812 帧，31249.999245 Hz，约 1.155826 MB/s；序号缺口、校验拒绝、重复、样点计数缺口和采样溢出均为 0；队列峰值 3367 B |
| 独立原始 TCP 接收，TCP 控制，12 路、60 s | 1874799 帧，31249.997513 Hz，约 1.655659 MB/s；连续性检查通过，队列峰值 5724 B |
| FRAME 原生后端，TCP 控制及上传，12 路 | 曾发生发送背压并填满队列，未通过持续验收；不能把原始 TCP 通过结果等同于 FRAME 全链路稳定 |

8 路验收最大相邻 ISR 间隔为 45.475 us，超过 48 us 的事件为 0。EA 序号和样点计数连续不单独证明定时器绝无抖动，故同时记录硬件时间戳。证据位于本地旧实验目录 `platform/esp32c5/build/srtos-final-q16k-tcp8/`、`srtos-final-q16k-raw12/`；仅作为历史证据，不参与本平台构建。

2026-10-23，两个独立工程均构建通过；两份 BSP 主机测试通过。实际编译及链接输入检查分别覆盖裸机 785 个、SRTOS 789 个编译单元，跨平台引用均为 0。裸机 ELF 不含 RV32 调度和 IRQ 包装，SRTOS ELF 确认存在，事务边界分别为弱空操作和强调度保护符号。

独立 SRTOS ELF SHA-256 为 `3a7751a42f6de43fa238609a7b0efbde41d8e4007ec3363adc690071f97b05c0`，静态 RAM 为 97124 B（data+bss+noinit，包含 SDK），IRAM 代码及尾部对齐为 116352 B。从 SRAM 起点到堆起点共保留 213488 B，包含上述静态量、IRAM 及段对齐，不能再重复相加；SDK 动态堆另计。构建与隔离结果位于本平台 `build/header-pair-build.log`、`platform-isolation-result.json`、`independent-memory.json`。

本次检查时 ESP32 USB 串口未枚举，只有其他开发板串口；因此拆分后的新 ELF 已编译但尚未烧录，也没有将原型验收冒充为新 ELF 验收。拆分前双 649 us 任务的抢占及主动 yield 现场验证见 [RISC-V SRTOS 设计](../../framework/srtos/srtos_riscv_design.md)。
