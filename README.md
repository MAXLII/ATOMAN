# ATOMAN — 数字电源公共工程

ATOMAN 是面向数字电源的嵌入式公共工程，本地通常称为 `base`。仓库集中维护控制算法、应用流程、通信与调试框架，通过平台适配用于 MCU、Zynq、PLECS 和主机测试。

## 仓库组织架构

仓库按公共软件、运行平台、可编程逻辑和工程资料组织。公共软件由目标工程选择和接入，硬件差异通过接口与 BSP 适配。

| 一级目录 | 有些什么 |
|---|---|
| `code/` | 应用与业务、功率控制、公共算法、通信数据源、数据池、硬件接口、注册调度和调试服务；另保留少量历史参考实现 |
| `platform/` | APM32、GD32、HC32、TI C2000、Zynq 工程，以及 MATLAB 分析、PLECS 仿真和 GCC Testbench |
| `verilog/` | IIR、UART DMA、OLED DMA 等 FPGA IP 及配套仿真 |
| `tests/` | 独立主机测试、黄金数据和硬件验证资源，部分内容仅在本地维护 |
| `docs/` | 设计、接入、构建、测试、教材和辅助文档 |
| `references/` | 工程参考资料 |

各模块的内部文件和调用流程在独立文档中说明，README 只列内容与入口。

## 主要内容与文档入口

| 内容 | 简介 | 文档 |
|---|---|---|
| 应用与业务 | 产品流程、保护策略和功能演示 | [工程设计](docs/engineering_design.md) · [功能接入总览](docs/application/guides/feature_usage_guide.md) |
| 功率控制 | Buck、Boost、BB、LLC、CLLC、PFC、逆变器和 NPC，包含浮点与整数实现 | [CTRL 组织架构](docs/design/control/ctrl_design.md) |
| 公共算法 | PI/PR、SOGI/DSOGI、SVPWM、滤波、RMS、查表、检测和时序组件 | [控制算法](docs/application/library/control_blocks_usage.md) · [信号处理](docs/application/library/signal_processing_usage.md) · [检测与时序](docs/application/library/detection_sequence_usage.md) |
| 通信与数据 | 多链路、多协议分发，FRAME 通信，以及项目数据交换 | [Link / link_process](docs/design/framework/section/link_design.md) · [FRAME](docs/design/communication/frame_design.md) · [数据池与模块边界](docs/engineering_design.md) |
| 仿真 TCP 通信 | MATLAB / PLECS 仿真模块间的 TCP 通信与路由，以及连接 FRAME 上位机进行参数读写和在线调试 | [TCP 组件与接入](docs/SIM_COMM_USAGE.md) · [MATLAB 双节点 S-Function](docs/MATLAB_FRAME_ROUTE_BRIDGE_USAGE.md) · [PLECS 与 FRAME 联调](docs/application/communication/plecs_frame_bridge.md) |
| 注册与调度 | 初始化、周期任务、中断、状态机和静态模块注册 | [Section 组织架构](docs/design/framework/section/section_design.md) |
| 调试与观测 | 参数、波形、频响、性能、运行轨迹和故障现场 | [调试系统](docs/design/debug/debug_system_design.md) |
| 存储与升级 | Flash 分区与访问、Bootloader、IAP、镜像校验和恢复 | [FAL](docs/design/storage/fal_design.md) · [Bootloader](docs/design/bootloader/bootloader_design.md) |
| 接口与平台 | 项目接口、BSP、平台入口及构建适配 | [工程设计](docs/engineering_design.md) · [MCU 移植](docs/application/porting/mcu_platform_porting.md) · [Zynq](docs/design/platform/zynq7020/zynq7020_platform.md) |
| 分析与验证 | MATLAB、PLECS、主机测试和 FPGA 仿真 | [应用文档](docs/application/APPLICATION_INDEX.md) |
| FPGA IP | 可复用逻辑模块、接口和验证资源 | [FPGA 设计入口](docs/design/DESIGN_INDEX.md#fpga-ip) |

不同目标选择所需模块，不表示每个平台均已接入全部能力。

## 开始使用

| 目标 | 入口 |
|---|---|
| 查找功能及接入方式 | [公共功能接入总览](docs/application/guides/feature_usage_guide.md) |
| 构建 PLECS 工程 | [PLECS 构建指南](docs/application/build/plecs_build_guide.md) |
| 新增或适配 MCU 平台 | [MCU 平台移植](docs/application/porting/mcu_platform_porting.md) |
| 建立算法主机测试 | [GCC Host Testbench 指南](docs/application/framework/testbench/testbench_usage_zh.md) |
| 连接上位机调试 | [ATOMAN 与 FRAME 配合使用](docs/application/communication/frame_atoman_integration.md) |

各平台独立维护构建入口。下面是从仓库根目录运行的示例，其余目标按对应平台文档操作：

```bat
platform\plecs\npc\compile.bat
platform\gd32e507\compile.bat
```

`check.bat` 提供仓库与文档检查入口，`clean.bat` 用于清理生成物。主机构建、仿真与真实硬件验证分别记录，不能相互替代。

## 与 FRAME 配合

[FRAME](https://github.com/MAXLII/FRAME) 是配套 Windows 上位机，负责设备连接、参数读写、波形显示与在线调试。ATOMAN 提供设备侧协议、调试服务和平台接口；同一套协议可用于 MCU 与 PLECS 联调。

## 文档导航

从 [文档总览](docs/DOCUMENT_INDEX.md) 进入，或按需要选择：

- [设计文档](docs/design/DESIGN_INDEX.md)：组织结构、模块职责和接口关系。
- [应用文档](docs/application/APPLICATION_INDEX.md)：接入、构建、移植与操作方法。
- [教材](docs/tutorial/TUTORIAL_INDEX.md)：控制原理、参数分析与学习资料。
- [其他资料](docs/other/OTHER_INDEX.md)：厂商资料及辅助说明。

FPGA IP 的文档位于对应 `verilog/模块/doc/`。新增控制拓扑也可使用仓库内的 [控制拓扑技能](.agents/skills/base-ctrl-topology/SKILL.md)，统一代码组织格式。

## 技术合作

如果你有数字电源产品研发、控制算法设计、嵌入式软件开发、MCU/Zynq 平台适配、MATLAB/PLECS 仿真、通信调试或测试验证等需求，欢迎联系交流与合作。

微信：`Max_0661`

添加时请备注“数字电源”并简要说明需求，方便快速了解项目方向。

## License

仓库自有代码使用 [MIT License](LICENSE)。芯片厂商 SDK、CMSIS、外设库、Keil/J-Link 文件及其他第三方内容继续遵循各自许可证。
