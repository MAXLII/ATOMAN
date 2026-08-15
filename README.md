# ATOMAN — 数字电源公共工程

ATOMAN 是一套面向数字电源控制、嵌入式平台适配、仿真和软件验证的公共工程。本地工程通常称为 `base`，GitHub 仓库名称为 [ATOMAN](https://github.com/MAXLII/ATOMAN)。

仓库不绑定单一产品或芯片。控制算法、通信、调试、调度和基础组件集中维护在 `code/`，MCU、Zynq、PLECS、MATLAB 与 GCC Host Testbench 在 `platform/` 中提供不同的运行环境。目标是让同一份核心软件可以先分析、再仿真、再测试，最后进入真实硬件。

## 工程思路

```text
                           ┌─ MATLAB：参数分析与离线计算
                           ├─ PLECS：控制与功率级联合仿真
公共软件 code/ ────────────┼─ Testbench：纯软件 DUT 主机验证
                           ├─ MCU：实时控制与产品工程
                           └─ Zynq：PS 软件与 PL 可编程逻辑

FRAME Windows 上位机 <──── 通信与调试协议 ────> MCU / PLECS / Zynq
```

公共代码通过 HAL、BSP 和接口函数访问外部资源。硬件寄存器、启动过程、链接布局和工具链留在具体平台中；控制和算法模块不携带芯片依赖。

## 主要能力

| 领域 | 当前内容 |
|---|---|
| 功率控制 | Buck、Boost、Buck-Boost、PFC、LLC、CLLC、逆变器以及浮点、整数控制实现 |
| 控制与信号算法 | PI、PR、SOGI、DFT、RMS、滤波、查表、插值、检测和时序组件 |
| 应用与框架 | 应用流程、保护、故障与告警、Section 静态注册、FSM、任务与中断调度 |
| 通信 | 以 `link_process()` 为核心的字节流分发、FRAME 协议、CRC、命令处理和跨链路路由 |
| 调试与观测 | Scope、SFRA、Perf、Trace、Shell、Black Box 和 Section 链表查询 |
| 存储与升级 | Flash 适配、Bootloader、IAP、镜像校验和升级恢复 |
| 仿真与验证 | MATLAB、PLECS、GCC Host Testbench、独立主机测试和 SystemVerilog 自检 |
| 可编程逻辑 | Zynq-7020 PS/PL 集成以及 IIR、UART DMA、OLED DMA 等 Verilog IP |

## 通信的核心：`link_process()`

`link_process()` 是 ATOMAN 通信体系中最小、也最关键的连接点。它不理解帧头、地址、CRC 或业务命令，只做两层循环：从当前物理链路不断取出字节，再把每个字节同步交给这条链路注册的全部 handler。

```text
UART / CAN / TCP / 仿真接口
            │
            ▼
      rx_get_byte()
            │
            ▼
      link_process()
            │ 每个字节都携带当前链路的发送接口
            ├─ Shell handler + 独立上下文
            ├─ FRAME handler + 独立上下文
            └─ 自定义协议 handler + 独立上下文
```

handler 接收到三个信息：当前字节、当前链路的发送能力以及自己的解析上下文。这三个参数形成了通信复用的基础：

- 一条物理链路可以同时承载 Shell、FRAME 和其他协议。
- 同一个协议函数可以挂到多条链路，只需为每个实例绑定独立上下文。
- handler 不需要知道底层是 UART、CAN、TCP 还是仿真接口。
- 请求从哪条链路进入，响应就可以通过随字节传入的发送接口返回原链路。
- 跨链路转发只依赖静态注册的 `link_id` 和目标链路发送接口，不侵入协议解析器。

`REG_LINK()` 在编译期声明物理链路、发送接口和 handler 组合，Section 初始化后形成链表；`section_link_task()` 遍历全部 Link，`link_process()` 每轮最多处理当前链路的固定字节预算，再让出执行权给下一条链路。新增硬件链路不需要修改协议，新增协议也不需要修改调度器，改变的只是静态绑定关系。

这套设计把“字节从哪里来”“字节代表什么”“解析状态保存在哪里”“响应从哪里发出”分成四个彼此独立的问题。完整设计见 [Section Link 设计文档](docs/design/framework/section/link_design.md)，接入方法见 [Section 使用文档](docs/application/framework/section/section_usage.md)。

## 软件分层

```text
应用与产品流程                code/app
功率拓扑控制                  code/ctrl
通信与调试服务                code/comm + code/dbg
调度、注册与通用算法          code/section + code/lib
统一硬件接口                  code/interface
平台驱动、链接与工程入口      platform/<target>
```

各层职责如下：

- `code/app/` 组织应用流程、保护策略、状态管理和升级业务。
- `code/ctrl/` 维护功率拓扑控制器、运行状态机、参数配置和 HAL 绑定。
- `code/comm/` 与 `code/dbg/` 提供通信协议、数据观测和调试服务。
- `code/lib/` 提供硬件无关的控制、信号处理和通用算法。
- `code/section/` 负责静态注册、初始化、调度、FSM，以及以 `link_process()` 为核心的链路分发。
- `code/interface/` 定义 ADC、PWM、GPIO、USART 和存储等统一接口。
- `platform/` 完成芯片驱动、接口挂载、构建系统和运行入口。

## 仓库结构

```text
ATOMAN/
├─ code/                       公共嵌入式软件与算法
│  ├─ app/                     应用流程与业务状态
│  ├─ comm/                    通信协议与路由
│  ├─ ctrl/                    数字电源控制模块
│  ├─ dbg/                     调试与观测服务
│  ├─ interface/               硬件接口抽象
│  ├─ lib/                     控制和信号算法
│  └─ section/                 注册、调度与 FSM 框架
├─ platform/
│  ├─ apm32/                   APM32 MCU 工程
│  ├─ gd32e507/                GD32E507 MCU 工程
│  ├─ gd32g553c/               GD32G553C MCU 工程
│  ├─ hc32f334/                HC32F334 MCU 工程
│  ├─ hc32f558/                HC32F558 MCU 工程
│  ├─ zynq7020/                Zynq-7020 PS/PL 工程
│  ├─ matlab/                  参数分析与离线模型
│  ├─ plecs/                   功率级与控制联合仿真
│  └─ testbench/               GCC 主机测试框架与 DUT 工程
├─ verilog/                    RTL、仿真与 FPGA IP
├─ tests/                      独立主机测试、黄金数据和硬件验证资源
├─ docs/                       设计、应用、测试和教材文档
├─ references/                 工程参考资料
├─ check.bat                   仓库与文档检查入口
└─ clean.bat                   生成物清理入口
```

完整目录职责见 [工程设计](docs/engineering_design.md)。

## 从哪里开始

根据当前目标选择入口：

| 目标 | 入口 |
|---|---|
| 了解仓库架构与模块边界 | [工程设计](docs/engineering_design.md) |
| 查找已有能力并接入工程 | [公共功能接入总览](docs/application/guides/feature_usage_guide.md) |
| 使用控制、调试、通信或算法模块 | [应用文档总纲](docs/application/APPLICATION_INDEX.md) |
| 理解模块内部设计 | [设计文档总纲](docs/design/DESIGN_INDEX.md) |
| 为纯软件模块建立 GCC 测试 | [GCC Host Testbench 中文指南](docs/application/framework/testbench/testbench_usage_zh.md) |
| 让 AI 从零落地 Testbench | [Testbench Implementation Prompt](docs/application/build/testbench_implementation_prompt.md) |
| 接入新的 MCU 平台 | [MCU 平台移植](docs/application/porting/mcu_platform_porting.md) |
| 联调 ATOMAN 与 FRAME | [ATOMAN 与 FRAME 配合使用](docs/application/communication/frame_atoman_integration.md) |

## 构建与验证

各目标独立维护构建入口。以下命令均从仓库根目录执行。

### GCC Host Testbench

`platform/testbench/common/` 提供公共 runner。DUT 和测试用例通过宏注册，每个测试工程生成一个可执行文件，并在一次运行中执行该工程注册的全部用例。测试环境可以记录 CSV 波形。

```powershell
mingw32-make -C platform/testbench/ac_loss_det test
mingw32-make -C platform/testbench/pi test
```

Testbench 支持直接编译仓库中的真实 C/C++ 软件模块。详细用法见 [中文指南](docs/application/framework/testbench/testbench_usage_zh.md) 或 [English Guide](docs/application/framework/testbench/testbench_usage.md)。

### MCU

```bat
platform\gd32g553c\compile.bat
platform\gd32e507\compile.bat
platform\hc32f334\gcc\compile.bat
platform\apm32\compile.bat
```

HC32F334 同时提供 Keil MDK 构建入口：

```bat
platform\hc32f334\keil_mdk\compile.bat
```

### PLECS

PLECS 工程直接编译所需的公共控制与算法代码：

```bat
platform\plecs\buck\compile.bat
platform\plecs\pfc\compile.bat
platform\plecs\inv\compile.bat
platform\plecs\llc\compile.bat
```

具体环境配置见 [PLECS 构建指南](docs/application/build/plecs_build_guide.md)。

### Zynq-7020

```powershell
.\platform\zynq7020\ps\compile.ps1
.\platform\zynq7020\pl\build_pl.ps1
```

### 独立主机测试

`tests/host/` 保存面向特定边界的独立测试工程，当前包括 FAL、Bootloader、Section 链表和整数逆变控制等测试。进入对应目录后使用该目录的 Makefile 构建和运行。

提交前可执行仓库检查：

```bat
check.bat
```

## 与 FRAME 配合

[FRAME](https://github.com/MAXLII/FRAME) 是与 ATOMAN 配套的 Windows 上位机，负责设备连接、命令交互、参数读写、数据观察和调试操作。FRAME 数据进入 ATOMAN 后，先由 `link_process()` 从具体物理链路分发给独立的 COMM 上下文，再完成帧校验、命令查找、业务调用和原链路响应。ATOMAN 维护下位机协议、调试服务、硬件平台和 PLECS 通信端。

- ATOMAN：<https://github.com/MAXLII/ATOMAN>
- FRAME：<https://github.com/MAXLII/FRAME>
- 联调说明：[ATOMAN 与 FRAME 配合使用](docs/application/communication/frame_atoman_integration.md)

## 文档体系

[文档总览](docs/DOCUMENT_INDEX.md) 将仓库资料分为四个主要入口：

- [设计文档](docs/design/DESIGN_INDEX.md)：工程架构、模块内部设计、接口关系与平台结构。
- [应用文档](docs/application/APPLICATION_INDEX.md)：模块接入、构建下载、平台移植与操作方法。
- [教材](docs/tutorial/TUTORIAL_INDEX.md)：控制原理、参数整定、MATLAB 脚本与学习资料。
- [其他资料](docs/other/OTHER_INDEX.md)：厂商资料与辅助说明。

FPGA IP 的设计与应用文档位于对应的 `verilog/<ip>/doc/` 目录。

## 工程约定

- 公共 C 模块以 C11 为基础，核心实时路径不依赖动态内存。
- 平台差异通过 BSP、HAL 和接口层适配，不进入共享控制算法。
- 生产代码、PLECS 与 Testbench 尽量复用同一份模块实现。
- Section 用于静态注册和统一调度，平台链接配置负责保留对应段。
- 通信协议扩展在已发布结构体尾部追加字段，并按实际数据长度兼容解析。
- 设计文档只描述仓库当前存在的目录、模块和行为。

## License

仓库自有代码使用 [MIT License](LICENSE)。芯片厂商 SDK、CMSIS、外设库、Keil/J-Link 文件及其他第三方内容继续遵循各自许可证。
