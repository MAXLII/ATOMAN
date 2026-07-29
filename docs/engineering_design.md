# 工程设计

## 1. 工程定位

本仓库是数字电源公共软件、硬件平台、控制仿真和 FPGA IP 的统一工程。代码按照公共能力与平台适配分层组织，使控制算法、通信、调试、升级和调度模块能够在不同目标工程中复用。

工程由五类内容组成：

- `code/`：平台无关的公共软件。
- `platform/`：MCU、Zynq、MATLAB 和 PLECS 平台工程。
- `verilog/`：可复用 FPGA IP、验证环境和设计资料。
- `tests/host/`：公共软件的主机测试。
- `docs/`：设计、应用、教材和其他工程文档。

公共软件通过接口和函数表使用平台能力，平台工程负责硬件初始化、驱动绑定、链接布局、编译目标和运行入口。各目标工程只选取自身需要的公共模块。

## 2. 仓库结构

```text
base/
├─ code/
│  ├─ app/                    应用流程和业务状态机
│  ├─ comm/                   FRAME 通信与命令分发
│  ├─ ctrl/                   电源拓扑控制模块
│  ├─ dbg/                    调试、观测和在线分析
│  ├─ interface/              公共硬件接口与 Flash 抽象层
│  ├─ legacy/                 历史兼容和参考实现
│  ├─ lib/                    通用算法与基础组件
│  └─ section/                注册、初始化和任务调度框架
├─ platform/
│  ├─ apm32/                  APM32 MCU 平台
│  ├─ gd32g553c/              GD32G553C MCU 平台
│  ├─ hc32f334/               HC32F334 MCU 平台
│  ├─ hc32f558/               HC32F558 MCU 平台
│  ├─ zynq7020/               Zynq-7020 PS/PL 平台
│  ├─ matlab/                 MATLAB 分析与时域仿真
│  └─ plecs/                  PLECS 开关级仿真
├─ verilog/
│  ├─ iir/                    3P3Z IIR IP
│  ├─ oled_dma/               OLED DMA IP
│  └─ uart_dma/               UART DMA IP
├─ tests/
│  └─ host/                   MinGW 主机测试
├─ docs/                      五类工程文档与统一索引
├─ .vscode/                   编辑器工程配置
├─ clean.bat                  生成物清理入口
└─ README.md                  仓库概览
```

## 3. 公共软件结构

### 3.1 应用层 `code/app/`

应用层组织面向完整功能流程的状态机和服务。

| 目录或模块 | 职责 |
|---|---|
| `ac/` | AC 应用流程 |
| `llc/`、`pfc/` | 对应拓扑的应用逻辑 |
| `demo/` | Section、通信和调试功能演示 |
| `bootloader/` | 平台无关升级核心、协议和 IAP 切换服务 |
| 根目录应用模块 | 故障、告警、上电、时间片、LED 和状态管理 |

Bootloader 目录进一步按职责划分：

```text
code/app/bootloader/
├─ core/                      升级状态机和冗余元数据
├─ protocol/                  FRAME 升级协议
├─ iap/                       IAP 升级触发与复位服务
└─ common/                    IAP 与 Bootloader 共享数据契约
```

### 3.2 通信层 `code/comm/`

通信层实现 FRAME 数据帧解析、CRC 校验、命令注册、ACK 发送和通信路由。业务模块通过 `REG_COMM` 注册命令处理函数，平台通过 Section link 提供字节收发能力。

### 3.3 控制层 `code/ctrl/`

控制层按电源拓扑组织闭环控制和运行状态：

```text
code/ctrl/
├─ bb/                        Buck-Boost
├─ boost/                     Boost
├─ buck/                      Buck
├─ cllc/                      CLLC
├─ inv/                       逆变器
├─ llc/                       LLC
├─ pfc/                       浮点 PFC
└─ pfc_i32/                   整数 PFC
```

控制模块使用公共接口访问采样值和 PWM 输出，并复用 `code/lib/` 中的控制算法。

### 3.4 算法库 `code/lib/`

算法库保存可独立复用的计算组件，包括 PI/PID、PR、SOGI、PLL/FLL、DFT、RMS、Notch、2P2Z、MPPT、线性插值、继电器时序和电网检测。模块使用调用方持有的状态对象和显式参数，不保存平台硬件配置。

### 3.5 调试层 `code/dbg/`

调试层由核心能力和通信服务组成：

| 模块 | 核心职责 | 服务职责 |
|---|---|---|
| Shell | 变量、命令和表达式管理 | 文本命令与二进制读写 |
| Scope | 多通道采样、触发和环形缓冲 | 配置、状态和采样数据传输 |
| Trace | 运行轨迹 FIFO | 轨迹控制与分批上报 |
| Perf | 任务、中断和代码段计时 | 负载与性能记录查询 |
| SFRA | 扫频状态机与响应计算 | 扫频配置、控制和结果查询 |

`*_service.c/.h` 负责 Section 注册和 FRAME 通信，核心文件负责数据结构、算法和实时路径。

### 3.6 接口层 `code/interface/`

接口层定义公共软件访问硬件和平台资源的统一边界：

| 目录 | 职责 |
|---|---|
| `common/` | 公共 GPIO、PWM 和基础接口契约 |
| `ac/` | AC 平台通信链路和接口绑定 |
| `cllc/` | CLLC 使用的接口定义 |
| `fal/` | Flash Abstraction Layer 核心 |

平台 BSP 实现接口要求，应用和控制模块通过接口读取采样、更新 PWM、发送通信数据或访问 Flash。

### 3.7 Section 框架 `code/section/`

Section 框架通过链接段收集模块注册项，统一完成初始化、周期任务、中断、有限状态机、通信链路和调试对象的调度。

```text
code/section/
├─ baremetal/                 裸机协作式调度
├─ srtos_m/                   Cortex-M SRTOS 运行时
├─ srtos_a9/                  Cortex-A9 SRTOS 运行时
├─ platform.h                平台能力入口
├─ timing.h                  时间换算定义
└─ my_math.h                 公共数学辅助定义
```

三个运行时提供一致的注册接口。平台构建目标选择其中一套 `section.c/.h`，业务模块使用 `REG_INIT`、`REG_TASK_MS`、`REG_INTERRUPT`、`REG_FSM`、`REG_COMM` 等宏接入运行时。

### 3.8 Legacy `code/legacy/`

Legacy 目录保存当前平台构建未引用的产品升级实现和旧 USART 接口，作为兼容与实现参考。当前公共模块和平台工程使用 `code/app/bootloader/`、`code/comm/` 与 `code/interface/` 中的接口。

## 4. 平台工程结构

### 4.1 MCU 平台

MCU 平台位于 `platform/<platform>/`。各平台根据实际工具链包含以下内容：

| 目录 | 职责 |
|---|---|
| `bsp/` | 时钟、GPIO、ADC、PWM、通信和存储驱动 |
| `src/` | 平台入口、中断、SysTick 和系统调用 |
| `inc/` | 平台工程头文件 |
| `gcc/`、`gcc_startup/`、`ldscripts/` | GCC 构建、启动和链接配置 |
| `keil_mdk/`、`mdk/` | Keil 工程和链接配置 |
| `bootloader/` | 平台 Bootloader 入口、FAL cfg 和 Flash 适配 |
| `tools/` | 平台编译、下载和硬件验证脚本 |
| `Firmware/`、`Libraries/`、`cmsis/` | 芯片厂商支持文件 |

平台构建文件通过相对路径引用 `code/` 中的公共模块。链接脚本或 scatter 文件定义启动地址、程序区、运行栈和保留内存。

### 4.2 Zynq-7020 平台

```text
platform/zynq7020/
├─ ps/
│  ├─ bsp/                     PS 外设和 PL MMIO 驱动
│  ├─ src/                     ARM 应用入口与平台服务
│  ├─ srtos/                   Cortex-A9 上下文切换端口
│  └─ bootloader/              Bootloader、FAL cfg 和启动镜像构建
└─ pl/                         Vivado 工程、约束和 IP 打包脚本
```

PS 工程提供裸机与 SRTOS 构建目标。PL 工程引用 `verilog/` 中的可复用 IP，并生成 PS 软件构建所需的硬件描述和 bitstream。

### 4.3 仿真平台

`platform/matlab/` 保存控制器分析、参数设计和时域模型；`platform/plecs/` 按拓扑保存开关级仿真工程。仿真平台复用 `code/ctrl/` 和 `code/lib/` 中的控制实现，使算法验证与嵌入式实现保持一致。

## 5. Flash 与升级架构

### 5.1 FAL

FAL Core 位于 `code/interface/fal/`，负责异步 Flash 请求和分区内地址管理。平台在自身 `fal_cfg.c/.h` 中定义：

- Flash 设备及容量、编程页、擦除块和读取分段参数；
- 每个设备的连续分区表和访问权限；
- 初始化、状态、读取、编程、擦除和同步操作；
- FAL 实例的初始化与周期调度。

`fal_read()`、`fal_write()` 和 `fal_erase()` 提交请求，`fal_process()` 根据设备状态分段推进操作。FAL Core 负责配置、权限、边界、累计容量和整数溢出检查。

### 5.2 Bootloader

Bootloader 的 Flash 依赖链为：

```text
Bootloader Core
    ↓ bootloader_flash_ops_t
平台 bootloader_fal_adapter.c
    ↓ fal_read / fal_write / fal_erase / fal_state
FAL Core
    ↓ 平台 fal_cfg
Flash 驱动
```

`bootloader_flash_ops_t` 提供逻辑区域查询、读、写、擦除和状态操作。平台适配器通过 `switch` 将 Bootloader 逻辑区域映射到本平台 FAL 分区，并把适配函数挂载到 Core。FAL 实例、设备配置和调度归平台管理。

Bootloader Core 管理启动判断、直接升级、暂存升级、镜像校验、冗余元数据、掉电恢复和应用跳转决策。协议层处理 FRAME 的升级信息、就绪、数据和结束命令。IAP 服务处理升级触发、用户准备回调、升级请求保存和系统复位。

## 6. FPGA IP 结构

每个 FPGA IP 使用一致的目录：

```text
verilog/<ip>/
├─ rtl/                        Verilog-2001 可综合 RTL
├─ sim/                        SystemVerilog 自检与综合脚本
└─ doc/                        设计、测试和应用文档
```

| IP | 职责 |
|---|---|
| `iir/` | 3P3Z IIR core 与 AXI4-Lite 外设 |
| `uart_dma/` | UART、同步 FIFO、AXI 控制与 DDR 环形 DMA |
| `oled_dma/` | OLED 帧缓存、DMA、SSD1306 协议与串行 PHY |

`platform/zynq7020/pl/` 中的 TCL 脚本读取各 IP 的 `rtl/` 文件完成 Vivado IP 打包和系统集成。

## 7. 测试结构

主机测试位于 `tests/host/`：

| 测试工程 | 覆盖内容 |
|---|---|
| `fal_core/` | 多设备、分区换算、读写擦分段、权限、失败和多实例 |
| `bootloader_core/` | 启动判断、下载方式、协议、镜像校验、元数据和掉电恢复 |

主机测试使用 MinGW C11 和严格警告编译，并直接编译生产代码。平台构建负责验证工具链、链接布局和平台适配；Verilog `sim/` 负责 RTL 自检。

## 8. 文档结构

```text
docs/
├─ engineering_design.md       当前工程结构与模块职责
├─ DOCUMENT_INDEX.md           文档统一入口
├─ design/                    架构、模块和平台设计
│  └─ DESIGN_INDEX.md         设计文档总纲
├─ application/               接入、构建、下载、移植和使用
│  └─ APPLICATION_INDEX.md    应用文档总纲
├─ tutorial/                  原理、参数整定和仿真教材
│  └─ TUTORIAL_INDEX.md       教材总纲
└─ other/                     厂商资料和辅助说明
   └─ OTHER_INDEX.md          其他文档总纲
```

各类总纲负责定义本类文档的内容边界、阅读顺序和详细文档入口，统一入口为 [文档索引](DOCUMENT_INDEX.md)。FPGA IP 的设计和应用资料保存在对应 `verilog/<ip>/doc/` 中，并由各类总纲统一链接。

## 9. 构建与生成物

构建入口随目标工程维护：MCU 工程使用 Makefile、Keil 工程或批处理脚本，Zynq 工程使用 Makefile、PowerShell 与 Vivado TCL，Verilog 仿真使用各 IP 的 `sim/` 脚本。

生成的对象文件、链接文件、镜像、日志、仿真目录和工具状态由 `.gitignore` 管理。仓库根目录的 `clean.bat` 统一清理 MCU、Zynq、PLECS、Verilog 和主机测试生成物。
