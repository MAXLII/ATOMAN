# ATOMAN 与 FRAME 配合使用

本文说明 ATOMAN（本地工程名为 base）与 FRAME 的职责关系，以及从获取工程、准备下位机、建立连接到使用调试功能的完整流程。

## 1. 工程地址与职责

两个工程分别维护，使用时通过统一 FRAME 协议和通信链路协作。

| 工程 | GitHub 地址 | 职责 |
|---|---|---|
| ATOMAN（base） | [https://github.com/MAXLII/ATOMAN](https://github.com/MAXLII/ATOMAN) | 下位机公共代码、MCU/Zynq 平台、PLECS 仿真、协议服务和调试数据源 |
| FRAME | [https://github.com/MAXLII/FRAME](https://github.com/MAXLII/FRAME) | Windows GUI、交互式终端、CLI、数据展示、控制与导出 |

ATOMAN 在 GitHub 上的仓库名是 `ATOMAN`，本文中的“base”指该仓库在本地使用的工程名。

配合使用时的数据路径为：

```text
FRAME GUI / CLI
        │
        │ 串口 / CAN / Ethernet TCP
        ▼
ATOMAN 平台通信链路
        │
        ▼
code/comm FRAME 解析与命令分发
        │
        ├─ Shell / 参数与实时波形
        ├─ Scope / SFRA / Perf / Trace
        ├─ Section 链表与 Black Box
        └─ Bootloader / 固件升级
```

FRAME 不编译进 ATOMAN，ATOMAN 也不依赖 FRAME 的 Python 代码。两者只通过协议帧交互，可以分别升级和维护。

## 2. 获取 ATOMAN 与 FRAME

只使用 FRAME 图形界面时，克隆 ATOMAN 后安装 FRAME Release 即可，不需要克隆 FRAME 源码：

```powershell
git clone https://github.com/MAXLII/ATOMAN.git base
```

需要开发 FRAME 或使用源码中的 CLI 时，在同一父目录下克隆两个仓库：

```powershell
git clone https://github.com/MAXLII/ATOMAN.git base
git clone https://github.com/MAXLII/FRAME.git FRAME
```

克隆后的目录关系为：

```text
workspace/
├─ base/                       ATOMAN 下位机与仿真工程
└─ FRAME/                      Windows 上位机
```

如果不需要把 ATOMAN 的本地目录命名为 `base`，也可以省略克隆命令末尾的 `base`，Git 会创建 `ATOMAN/` 目录。

网页入口：

- 浏览 ATOMAN 源码：<https://github.com/MAXLII/ATOMAN>
- 浏览 FRAME 源码：<https://github.com/MAXLII/FRAME>
- 下载 FRAME Release：<https://github.com/MAXLII/FRAME/releases/latest>

## 3. 安装并启动 FRAME

FRAME 在 Windows 10/11 上运行，可以直接安装 GitHub Release，也可以从源码运行。

### 3.1 使用 Release 安装版

普通使用者不需要配置 Python 环境，也不需要克隆 FRAME 仓库。打开 [FRAME 最新 Release](https://github.com/MAXLII/FRAME/releases/latest)，在 Assets 中下载：

```text
FRAME-Setup-<version>.exe
```

运行安装程序完成安装，然后从 Windows 开始菜单或桌面快捷方式启动 FRAME。Release 页面同时提供对应的 `.sha256` 文件，可用于校验安装包完整性。

全部历史版本见 [FRAME Releases](https://github.com/MAXLII/FRAME/releases)。应优先使用最新 Release，以获得与 ATOMAN 当前协议和 PLECS 联调功能匹配的版本。

### 3.2 从源码运行

需要开发 FRAME、使用 CLI 或跟踪最新源码时，进入 FRAME 仓库：

```powershell
Set-Location .\FRAME
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
.\frame.ps1 gui
```

查看 CLI 支持的功能：

```powershell
.\frame.ps1 --help
```

启动交互式终端：

```powershell
.\frame.ps1 shell
```

### 3.3 FRAME 文档

FRAME 的通用安装、页面和命令说明在 FRAME 仓库维护：

- [FRAME Releases](https://github.com/MAXLII/FRAME/releases)
- [FRAME README](https://github.com/MAXLII/FRAME/blob/master/README.md)
- [FRAME CLI 命令](https://github.com/MAXLII/FRAME/blob/master/docs/CLI_COMMANDS.md)
- [FRAME J-Link 使用方法](https://github.com/MAXLII/FRAME/blob/master/docs/JLINK_USAGE.md)

## 4. ATOMAN 下位机需要提供的能力

选择 MCU、Zynq 或 PLECS 目标后，目标工程需要完成以下接入，FRAME 才能访问对应功能：

1. 运行 Section 初始化和周期调度。
2. 注册至少一条通信 Link，并绑定串口、CAN 或 Ethernet 的收发接口。
3. 为每条 FRAME 链路提供独立的 `comm_ctx_t` 和 payload 缓冲区。
4. 使用 `comm_run()` 解析链路收到的字节。
5. 编译需要的调试核心和对应 `*_service.c`。
6. 注册需要暴露给上位机的变量、Scope、SFRA、Perf 或 Trace 数据。
7. 设置本机静态地址，并在 FRAME 中填写相同的目标地址。

FRAME 协议接入的代码入口为：

```text
code/comm/comm.c
code/comm/comm.h
```

底层接入细节见：

- [FRAME 通信接入](frame_usage.md)
- [通信命令开发方法](command_development_usage.md)
- [Section 使用方法](../framework/section/section_usage.md)

各上位机页面与 ATOMAN 模块的对应关系：

| FRAME 功能 | ATOMAN 侧主要模块 | ATOMAN 使用文档 |
|---|---|---|
| 参数读写、参数波形 | `shell.c`、`shell_service.c` | [Shell 使用](../debug/shell/shell_usage.md) |
| Scope | `scope.c`、`scope_service.c` | [Scope 使用](../debug/scope/scope_usage.md) |
| SFRA | `sfra.c`、`sfra_service.c` | [SFRA 使用](../debug/sfra/sfra_usage.md) |
| Perf | `perf.c`、`perf_service.c` | [Perf 使用](../debug/perf/perf_usage.md) |
| Trace | `trace.c`、`trace_service.c` | [Trace 使用](../debug/trace/trace_usage.md) |
| Section 链表 | `section_list_service.c` | [Section 链表协议](../../SECTION_LIST_PROTOCOL.md) |
| 固件升级 | `code/app/bootloader/` | [Bootloader 升级](../bootloader/bootloader_upgrade_usage.md) |

只编译核心模块而没有编译对应服务模块时，本地代码可以运行，但 FRAME 无法通过二进制协议访问该功能。

## 5. MCU 与 FRAME 联调

### 5.1 构建并运行目标工程

根据硬件选择 ATOMAN 平台，例如：

```bat
base\platform\gd32g553c\compile.bat
base\platform\gd32e507\compile.bat
base\platform\hc32f334\gcc\compile.bat
base\platform\apm32\compile.bat
```

将生成的固件下载到目标板，连接目标工程实际配置的串口、CAN 或 Ethernet 接口。

### 5.2 核对通信参数

FRAME 与目标工程必须使用一致的参数：

| 项目 | FRAME 侧 | ATOMAN 侧 |
|---|---|---|
| Transport | Serial、CAN 或 Ethernet | 目标平台已绑定的 Link 类型 |
| 串口号 | Windows 实际 COM 口 | 板卡连接的 USART |
| 波特率 | GUI 或 `--baud` | BSP USART 配置 |
| Target Address | 页面目标地址或 `--dst` | `DECLARE_COMM_CTX()` 的本机地址 |
| Dynamic Address | 页面动态地址或 `--d-dst` | 目标工程的动态地址配置 |
| TCP Host/Port | 设备 IP 与监听端口 | Ethernet 服务配置 |

目标地址不匹配时，帧可能被当作其他节点数据或路由帧处理，因此“串口能收到字节”不代表命令一定会进入本机回调。

### 5.3 使用 GUI 验证

1. 在 FRAME 顶部通信配置中选择 Transport。
2. 填写 COM 口与波特率，或 Ethernet Host/Port。
3. 点击连接。
4. 在功能页面填写目标地址和动态地址。
5. 先使用参数页“读取列表”或 Perf 页“读取信息”验证请求/ACK 链路。
6. 基础通信正常后，再使用 Scope、SFRA、Trace 或升级功能。

### 5.4 使用 CLI 验证串口链路

先枚举串口：

```powershell
Set-Location .\FRAME
.\frame.ps1 serial ports
```

以下示例假设端口为 `COM8`、波特率为 `921600`、目标地址为 `0x02`、动态地址为 `0x00`：

```powershell
.\frame.ps1 param list --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00
.\frame.ps1 scope list --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00
.\frame.ps1 perf summary --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00
```

示例值只用于展示命令格式。实际 COM 口、波特率和地址必须以所选 ATOMAN 平台工程为准。

## 6. PLECS 与 FRAME 联调

ATOMAN 提供可直接验证两个工程协作关系的 PLECS FRAME Bridge：

```text
base/platform/plecs/frame_bridge/
```

### 6.1 编译 PLECS DLL

```bat
cd /d workspace\base\platform\plecs\frame_bridge
compile.bat
```

输出文件为：

```text
build/bin/libplecs.dll
build/bin/plecs.map
```

### 6.2 启动仿真

使用 PLECS 打开：

```text
base/platform/plecs/frame_bridge/frame_bridge.plecs
```

启动仿真后，DLL 在 `0.0.0.0:5000` 监听 FRAME 协议 TCP 连接。本机访问使用 `127.0.0.1:5000`。

### 6.3 配置 FRAME

FRAME 顶部通信配置填写：

| 字段 | 值 |
|---|---|
| Transport | `Ethernet` |
| Host | `127.0.0.1` |
| TCP Port | `5000` |

功能页面的协议地址填写：

| 字段 | 值 |
|---|---|
| Target Address | `0x02` |
| Dynamic Address | `0x00` |

点击连接后按以下顺序验证：

1. 参数页点击“读取列表”，应看到 `SIM_TIME_S`、`SIM_GAIN`、`SIM_OFFSET` 等变量。
2. 修改 `SIM_GAIN` 或 `SIM_OFFSET`，PLECS 输出从下一个采样点开始变化。
3. 波形页选择参数并启动实时上报。
4. Scope 页刷新列表并操作 `frame_simulation`。
5. SFRA 页刷新实例并操作 `frame_gain`。
6. Perf 页读取任务与服务执行时间。
7. Trace 页启动上报后修改仿真参数，查看执行记录。
8. Section 链表页刷新 DLL 中的注册目录与节点。

PLECS Bridge 的完整参数表、功能范围和日志说明见 [PLECS 与 FRAME 调试通信](plecs_frame_bridge.md)。

## 7. 可运行 Demo

以下文档使用仓库中已经存在的平台工程、Demo 模块和 FRAME 命令，适合按顺序验证两个工程的配合关系：

| Demo | 适用场景 | 验证内容 |
|---|---|---|
| [GD32G553 串口与 FRAME 联调](../examples/frame_gd32g553_serial_demo.md) | 有 GD32G553 目标板和 USB 转串口 | 串口链路、协议回环、参数读写、实时波形和 Scope |
| [PLECS 与 FRAME TCP 联调](../examples/frame_plecs_tcp_demo.md) | 有 PLECS，不需要目标板 | Ethernet TCP、仿真参数、波形、Scope、SFRA、Perf、Trace 和 Section 链表 |
| [FRAME CLI 批处理](../examples/frame_cli_batch_demo.md) | 需要脚本化回归和结果导出 | 参数、回环命令、Scope、Perf 的命令行检查与 JSON/CSV 输出 |

第一次接触两个工程时，先运行 PLECS TCP Demo，可以排除板卡、串口线和固件下载因素；需要验证真实 MCU 链路时再运行 GD32G553 串口 Demo；通信稳定后使用 CLI Demo 固化重复检查。

## 8. 常用联调顺序

对新的 ATOMAN 目标工程，按以下顺序接入可以缩小问题范围：

1. 用 FRAME 原始收发或目标平台日志确认物理链路工作。
2. 用参数列表或简单自定义命令确认 FRAME 帧解析和直接 ACK。
3. 验证 Shell 参数读写与实时波形。
4. 验证 Section 链表，确认调试服务确实被链接和注册。
5. 逐项验证 Scope、Perf 和 Trace。
6. SFRA 从小注入幅值、窄频段开始。
7. 固件升级最后验证，并先确认 Bootloader、分区表和掉电恢复流程。

## 9. 常见问题

### FRAME 可以连接，但页面读取超时

- 检查 Target Address 和 Dynamic Address。
- 检查对应 `*_service.c` 是否参与构建。
- 检查 Section 注册段是否被链接脚本保留。
- 检查命令回调的 ACK 是否使用相同 `cmd_set / cmd_word`。
- 检查接收 payload 容量是否覆盖当前命令。

### 串口有数据，但协议无法识别

- 检查波特率、数据位、校验位和停止位。
- 检查 FRAME 帧是否从 `0xE8` 开始并通过 CRC。
- 同一链路同时运行字符串 Shell 和 FRAME 时，应使用独立解析上下文，由 Link 扇出收到的字节。

### PLECS 无法连接端口 5000

- 确认仿真已经启动，而不是只打开模型。
- 查看 `platform/plecs/frame_bridge/build/bin/plecs_log.txt`。
- 检查端口 5000 是否被其他进程占用。
- 跨计算机访问时使用 PLECS 主机局域网 IPv4 地址，并检查 Windows 防火墙。

### Scope、SFRA 或 Perf 列表为空

- 检查目标实例是否通过注册宏创建。
- 检查核心模块、服务模块和周期任务是否全部接入。
- 检查链接脚本是否保留相应 Section。

## 10. 关联导航

### GitHub

- [ATOMAN 仓库](https://github.com/MAXLII/ATOMAN)
- [FRAME 仓库](https://github.com/MAXLII/FRAME)
- [FRAME 在线 README](https://github.com/MAXLII/FRAME/blob/master/README.md)

### ATOMAN 文档

- [GD32G553 串口与 FRAME 联调](../examples/frame_gd32g553_serial_demo.md)
- [PLECS 与 FRAME TCP 联调](../examples/frame_plecs_tcp_demo.md)
- [FRAME CLI 批处理](../examples/frame_cli_batch_demo.md)
- [FRAME 通信接入](frame_usage.md)
- [PLECS 与 FRAME 调试通信](plecs_frame_bridge.md)
- [通信命令开发方法](command_development_usage.md)
- [公共功能接入总览](../guides/feature_usage_guide.md)
