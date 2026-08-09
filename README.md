# 数字电源基础工程

base 是面向数字电源开发、仿真和验证的公共工程。仓库统一维护平台无关的嵌入式软件、多芯片平台工程、MATLAB 与 PLECS 模型、Zynq PS/PL 集成、可复用 FPGA IP，以及直接编译生产代码的主机测试。

工程的核心目标是让控制算法、通信、调试、升级和调度能力在仿真与不同硬件平台之间复用。公共代码通过接口和函数表访问硬件资源，平台目录负责外设驱动、链接布局、工具链和运行入口。

## 与 FRAME 配合使用

本仓库在 GitHub 上的名称为 [ATOMAN](https://github.com/MAXLII/ATOMAN)，负责下位机固件、公共协议服务、硬件平台与 PLECS 仿真；[FRAME](https://github.com/MAXLII/FRAME) 是配套的 Windows 上位机，负责连接、控制、数据显示和调试操作。

- ATOMAN（base）：<https://github.com/MAXLII/ATOMAN>
- FRAME：<https://github.com/MAXLII/FRAME>
- 在线配合使用文档：<https://github.com/MAXLII/ATOMAN/blob/master/docs/application/communication/frame_atoman_integration.md>

从获取两个仓库、接入下位机通信服务，到 MCU 串口联调和 PLECS TCP 联调的完整步骤，见 [ATOMAN 与 FRAME 配合使用](docs/application/communication/frame_atoman_integration.md)。

## 工程能力

### 数字电源控制

- Buck、Boost、Buck-Boost、PFC、LLC 和逆变器等拓扑控制模块。
- PI/PR、SOGI、DFT、RMS、滤波、查表与插值等通用算法组件。
- 浮点与整数控制实现，覆盖采样代码域、PWM 比较域、限幅、移位和定点系数设计。
- MATLAB 参数分析、时域模型与 PLECS 开关级仿真。

### 公共嵌入式框架

- 应用流程、保护、故障与告警、上电管理和业务状态机。
- FRAME 字节流协议、CRC、命令分发和多链路路由。
- Section 注册、初始化、任务与中断调度、FSM 和链路分发。
- ADC、PWM、GPIO、USART、Flash 等平台接口抽象。
- FAL 异步 Flash 管理、Bootloader、IAP、镜像校验和升级恢复。

### 调试与观测

- Shell 参数读写与命令执行。
- Scope 软件录波。
- SFRA 在线扫频分析。
- Perf 运行时间统计。
- Trace 代码执行跟踪。
- Black Box、Section 链表查询和调试协议服务。

这些能力可以通过配套的 [FRAME Windows 上位机](https://github.com/MAXLII/FRAME) 进行串口、CAN 或 Ethernet 联调。

### 平台与可编程逻辑

- APM32、GD32E507、GD32G553C、HC32F334、HC32F558 MCU 平台。
- Zynq-7020 PS 裸机/SRTOS 工程、Bootloader 与 PL 集成。
- 3P3Z IIR、UART DMA 和 OLED DMA 等 Verilog IP。
- SystemVerilog 自检环境和 Vivado 集成脚本。

## 仓库结构

```text
base/
├─ code/
│  ├─ app/                    应用流程、保护和业务状态机
│  ├─ comm/                   FRAME 协议、CRC、命令与路由
│  ├─ ctrl/                   数字电源拓扑控制模块
│  ├─ dbg/                    Scope、SFRA、Perf、Trace 与 Shell
│  ├─ fal/                    Flash 抽象相关公共内容
│  ├─ interface/              ADC、PWM、通信和存储接口
│  ├─ lib/                    控制算法与通用基础组件
│  ├─ section/                注册、初始化、调度、FSM 与链路框架
│  └─ legacy/                 历史兼容与参考实现
├─ platform/
│  ├─ apm32/                  APM32 MCU 平台
│  ├─ gd32e507/               GD32E507 MCU 平台
│  ├─ gd32g553c/              GD32G553C MCU 平台
│  ├─ hc32f334/               HC32F334 MCU 平台
│  ├─ hc32f558/               HC32F558 MCU 平台
│  ├─ zynq7020/               Zynq-7020 PS/PL 平台
│  ├─ matlab/                 参数分析与时域仿真
│  └─ plecs/                  开关级仿真与 FRAME 桥接
├─ verilog/                   可复用 RTL、仿真和 IP 文档
├─ tests/
│  ├─ host/                   MinGW 主机测试
│  ├─ golden/                 黄金参考数据
│  └─ hardware/               硬件验证资源
├─ docs/                      设计、应用、测试、教材与其他文档
├─ references/                工程参考资料
├─ check.bat                  仓库差异与空白检查
└─ clean.bat                  生成物清理入口
```

详细的当前模块职责见 [工程设计](docs/engineering_design.md)。

## 软件分层

```text
应用与业务流程          code/app
控制与调试服务          code/ctrl + code/dbg
通信、调度与基础算法    code/comm + code/section + code/lib
统一硬件接口            code/interface
平台驱动与工程入口      platform/<target>
```

公共控制和算法逻辑保持硬件无关。采样、PWM、通信、Flash 和中断等差异在 `code/interface/` 与平台 BSP 中完成适配；平台工程只选择自身需要的公共模块参与构建。

## 开始使用

base 不是单一目标的开箱即用固件。使用时先确定目标，再进入对应平台或仿真目录：

1. 在 `code/` 中选择需要的控制、通信、调试和基础模块。
2. 在 `platform/<target>/` 中完成 BSP、接口函数表、链接脚本和构建目标绑定。
3. 先运行主机测试或仿真验证公共逻辑，再构建目标平台。
4. 使用 FRAME 上位机进行参数、Scope、SFRA、Perf、Trace 和升级联调。

新平台的接入方法、模块用法和构建流程从 [应用文档总纲](docs/application/APPLICATION_INDEX.md) 进入。

## 构建示例

各目标独立维护构建入口。以下命令均在仓库根目录执行。

### MCU

```bat
platform\gd32g553c\compile.bat
platform\gd32e507\compile.bat
platform\hc32f334\gcc\compile.bat
platform\apm32\compile.bat
```

HC32F334 同时维护 Keil MDK 构建入口：

```bat
platform\hc32f334\keil_mdk\compile.bat
```

### PLECS

PLECS 工程按拓扑独立编译公共 C 代码，例如：

```bat
platform\plecs\buck\compile.bat
platform\plecs\pfc\compile.bat
platform\plecs\inv\compile.bat
platform\plecs\llc\compile.bat
```

### Zynq-7020

```powershell
.\platform\zynq7020\ps\compile.ps1
.\platform\zynq7020\pl\build_pl.ps1
```

Zynq 的具体 Vivado/Vitis 环境、PL 构建、PS 构建和下载步骤以平台应用文档为准。

## 测试与检查

主机测试位于 `tests/host/`，使用 MinGW C11 与严格警告编译，并直接链接生产代码。当前测试覆盖 FAL、Bootloader、通信、Section、控制环路、调试核心、参数服务、安全镜像和故障注入等模块。

典型测试目录：

```text
tests/host/fal_core/
tests/host/bootloader_core/
tests/host/section_core/
tests/host/comm/
tests/host/control_blocks/
tests/host/grid_ctrl/
```

进入具体测试目录后按该目录的构建文件或说明执行。提交前在仓库根目录运行：

```bat
check.bat
```

该脚本检查未暂存和已暂存差异中的空白错误。硬件验证资源保存在 `tests/hardware/`，RTL 自检保存在各 `verilog/<ip>/sim/` 目录。

## 文档导航

[文档总览](docs/DOCUMENT_INDEX.md) 是仓库文档统一入口：

- [ATOMAN 与 FRAME 配合使用](docs/application/communication/frame_atoman_integration.md)：两个工程的获取、配置、连接和功能联调流程。
- [设计总纲](docs/design/DESIGN_INDEX.md)：工程架构、模块设计、接口关系和平台结构。
- [应用总纲](docs/application/APPLICATION_INDEX.md)：模块接入、构建下载、平台移植和操作方法。
- [教材总纲](docs/tutorial/TUTORIAL_INDEX.md)：控制原理、参数整定、MATLAB 脚本和学习资料。
- [其他总纲](docs/other/OTHER_INDEX.md)：厂商资料与辅助说明。

FPGA IP 的设计、验证和应用文档位于各 `verilog/<ip>/doc/` 目录，并由文档总纲统一导航。

## 工程约定

- 公共模块使用 C11，不依赖动态内存完成核心实时路径。
- 平台差异通过 BSP 与接口层适配，不进入共享控制算法。
- 协议扩展只在结构体尾部追加字段，并按收到长度与本地结构体大小取小解析。
- Section 用于静态注册和统一调度，目标工程通过链接配置保留对应段。
- 生成物、工具缓存和本地测试资料不作为工程设计内容维护。

## 许可证

本仓库自有代码使用 MIT License，详见 [LICENSE](LICENSE)。

芯片厂商 SDK、CMSIS、外设库、Keil/J-Link 相关文件和其他第三方材料继续受各自原始许可证约束。
