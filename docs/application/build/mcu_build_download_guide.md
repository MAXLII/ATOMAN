# MCU 编译与下载指南

## 1. HC32F334 GCC 工程

工程目录：

```text
platform/hc32f334/gcc/
```

GCC 工程使用 Arm GNU Toolchain、`mingw32-make.exe` 和 `makefile`。编译脚本支持增量编译、全量重编译和清理：

```bat
cd platform\hc32f334\gcc
compile.bat
compile.bat -r
compile.bat -c
```

默认产物位于 `platform/hc32f334/gcc/build/`：

```text
hc32f334_ac.elf
hc32f334_ac.hex
hc32f334_ac.bin
hc32f334_ac.map
```

下载默认 GCC HEX：

```bat
cd platform\hc32f334\gcc
download.bat
```

下载指定 HEX：

```bat
download.bat D:\firmware\hc32f334_ac.hex
```

HC32F334 的 J-Link 通用 `Cortex-M4` 目标只提供内核调试连接，不包含片内 Flash 编程算法。GCC 下载脚本调用 `platform/hc32f334/keil_mdk/download.bat`，由 HDSC Keil Pack 的 `HC32F334_128K.FLM` 擦除、编程并校验 GCC 生成的 HEX。下载日志必须包含：

```text
Erase Done.
Programming Done.
Verify OK.
```

默认下载完成后复位并运行 MCU。设置以下环境变量可跳过下载脚本末尾的额外 J-Link 运行命令：

```bat
set RUN_AFTER_DOWNLOAD=0
download.bat
```

Keil Flash 下载本身会按工程配置处理复位和运行状态；`RUN_AFTER_DOWNLOAD=0` 只控制脚本是否再执行一次 J-Link `go`。

## 2. HC32F334 Keil MDK 工程

工程目录：

```text
platform/hc32f334/keil_mdk/
```

编译当前工程：

```bat
cd platform\hc32f334\keil_mdk
compile.bat
compile.bat -r
```

`compile.bat` 使用 `UV4.exe -b` 或 `UV4.exe -r`，要求构建日志报告 `0 Error(s)`。Keil 进程默认通过隐藏窗口启动，结果输出到当前终端和 `compile.log`。

下载默认 MDK HEX：

```bat
download.bat AC
```

下载指定 HEX：

```bat
download.bat AC D:\firmware\hc32f334_ac.hex
```

下载脚本使用现有 `hc32f334_ac.uvprojx` 和 `hc32f334_ac.uvoptx` 生成临时外部 HEX 下载工程。临时工程保持 Cortex-M4、SWD、J-Link 和 HC32F334 Flash 算法配置，下载完成后自动删除。Keil 和下载后的 J-Link 运行步骤默认隐藏窗口。

下载日志位于：

```text
platform/hc32f334/keil_mdk/download_ac.log
```

## 3. TI C2000 Section Demo

工程目录：

```text
platform/tms320f280049c/
platform/tms320f28p55/
```

两个工程使用 TI C2000 编译器、C2000Ware DriverLib、裸机 Section 运行时和 FRAME 串口服务。F280049C 工程从仓库根目录编译：

```bat
platform\tms320f280049c\compile.bat
```

输出文件位于：

```text
platform/tms320f280049c/build/section_task_demo.out
platform/tms320f280049c/build/section_task_demo.map
```

F280049C 的链接命令文件为 `platform/tms320f280049c/linker/f280049c_ram_lnk.cmd`。程序入口、代码、常量、Section 自动注册段、栈和数据均放置在片内 RAM；该工程生成的是调试 RAM 镜像，断电后不会保留。

连接 LAUNCHXL-F280049C 的板载 XDS110 后下载并运行：

```bat
platform\tms320f280049c\download.bat
```

下载脚本使用 `targetConfigs/TMS320F280049C_LaunchPad.ccxml` 和 CCS `loadti`。当电脑同时连接多块 XDS110 时，应在 CCS 目标配置中选择对应探针序列号，再加载同一个 `.out` 文件。

板载 XDS110 Application/User UART 连接到 SCIA GPIO28/GPIO29，串口参数为 115200、8N1。在 FRAME 工程根目录验证参数、Section 链路及调度记录：

```powershell
.\frame.ps1 param list --port COM9 --baud 115200 --timeout 3
.\frame.ps1 perf dict --port COM9 --baud 115200 --timeout 3
.\frame.ps1 perf sample --port COM9 --baud 115200 --timeout 3 --filter task
```

`COM9` 是示例端口。实际端口通过以下命令识别：

```powershell
.\frame.ps1 serial ports
```

F28P55 使用对应目录下相同名称的脚本：

```bat
platform\tms320f28p55\compile.bat
platform\tms320f28p55\download.bat
```

## 4. 环境要求

HC32F334 GCC 编译与下载链路使用以下工具：

| 工具 | 用途 |
| --- | --- |
| Arm GNU Toolchain | 编译 GCC 固件 |
| `mingw32-make.exe` 或 `make.exe` | 执行 GCC Makefile |
| Keil MDK / `UV4.exe` | 调用 HC32F334 Flash 下载算法 |
| HDSC HC32F334 Keil Pack | 提供 `HC32F334_128K.FLM` |
| SEGGER J-Link | Cortex-M4 SWD 连接与下载传输 |

脚本自动查找常用安装目录和 `Path`。Arm GNU Toolchain 不在 `Path` 中时，可设置：

```bat
set GCC_PATH=C:\ArmGNU\bin
```

TI C2000 工程使用以下工具：

| 工具 | 用途 |
| --- | --- |
| Code Composer Studio 21 | 提供 `gmake`、调试服务器和 `loadti` |
| TI C2000 Compiler 25.11.1.LTS | 编译与链接 C28x EABI 程序 |
| C2000Ware 5.04.00.00 | 提供 F28004x/F28P55x Device Support 和 DriverLib |
| XDS110 | JTAG 下载和 Application/User UART 通信 |

## 5. 下载结果判定

脚本不以 J-Link Commander 的进程退出码单独判断烧录成功。下载必须满足：

1. Keil 下载进程正常结束。
2. 日志中不存在 `*** error`、`Flash Download failed` 或设备连接错误。
3. 日志包含 `Verify OK.`。
4. 默认配置下日志包含 `Application running ...`。

`Cortex-M4` 连接成功只说明调试内核可访问，不能替代片内 Flash 编程与校验结果。

C2000 RAM 下载必须满足：

1. `loadti` 按目标芯片配置连接成功。
2. `.out` 文件加载完成。
3. 日志包含 `Target running...`。
4. FRAME 能读取参数列表和 Perf 字典。
5. Perf 采样中包含 `section_link_task` 和当前平台的 Demo 周期任务。

## 6. 关联导航

- 源码：[HC32F334 GCC 构建脚本](../../../platform/hc32f334/gcc/compile.bat) · [HC32F334 Keil 构建脚本](../../../platform/hc32f334/keil_mdk/compile.bat) · [HC32F334 下载脚本](../../../platform/hc32f334/keil_mdk/download.bat) · [F280049C 构建脚本](../../../platform/tms320f280049c/compile.bat) · [F280049C 下载脚本](../../../platform/tms320f280049c/download.bat)
- 设计：[工程设计](../../engineering_design.md)
