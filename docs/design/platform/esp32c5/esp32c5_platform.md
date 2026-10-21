# ESP32-C5 通信平台

本平台将 T-Display-C5 工程已经验证的 Wi-Fi 和 USB Serial/JTAG 能力回归到 base，并接入 base 当前的 demo。硬件目标是 LILYGO T-Display-C5 / ESP32-C5、16 MB Flash；不初始化显示、触摸、按键或电池器件，也不包含 Shuttle、多镜像、公共 RAM 或升级机制。

## 代码位置与职责

| 位置 | 职责 |
|---|---|
| `platform/esp32c5/bsp/bsp_usart.c/.h` | Type-C USB Serial/JTAG 初始化、非阻塞读取、有限等待发送 |
| `platform/esp32c5/bsp/bsp_wifi.c/.h` | Wi-Fi 初始化、STA/AP 配置、状态查询、STA 重连 |
| `platform/esp32c5/bsp/bsp_wifi_socket.c/.h` | 非阻塞 IPv4 TCP 服务端、UDP 收发、描述符清理 |
| `platform/esp32c5/bsp/bsp_comm.c/.h` | 公共响应队列及 UDP；通过 Wi-Fi BSP 使用控制 TCP，不依赖协议层 |
| `platform/esp32c5/bsp/bsp_wifi_channels.c` | Wi-Fi 模块统一管理 TCP 5000/9001、连接代数、取包、部分发送及断线清理 |
| `code/interface/demo/esp32c5/comm_link.c` | USB/TCP 的 `REG_LINK`、独立 E8/E9/Shell 上下文、帧所有权及换会话清理 |
| `code/interface/demo/esp32c5/comm_discovery.c/.h` | FRAME 设备搜索应答和 UDP 回环；通过 BSP 接口收发，不操作 SDK 或描述符 |
| `code/interface/demo/esp32c5/wifi_demo.c` | demo 的 Wi-Fi 启停参数、状态命令和 FRAME 二进制回环 |
| `platform/esp32c5/bsp/bsp_system.c/.h` | SDK 单调时钟、100 us 转换、复位、临界区、静态 RAM 校验及固件标识 |
| `code/interface/demo/esp32c5/realtime_scope_bindings.c` | 在 SECTION 初始化/任务中绑定通用示波器与 BSP 能力 |
| `code/interface/demo/esp32c5/platform_bindings.c` | SECTION 时钟/复位回调与 Perf 注册，调用 BSP，不直接依赖 SDK |
| `platform/esp32c5/src/app_main.c` | 唯一启动入口，按顺序初始化 BSP、Interface 和框架并运行主循环 |
| `platform/esp32c5/main/` | ESP-IDF 组件和注册段保留 |
| `platform/esp32c5/gcc/compile.ps1` | 使用本机 SDK 的构建/烧录入口及私有联网配置 |
| `code/business/demo/` | 直接编译已有的 `demo_shell.c`、`demo_task.c`、`demo_perf.c` |

沿用其他平台的 BSP 收发接口 → CommLink 协议绑定 → SECTION 注册关系。`bsp_comm` 维护公共响应队列及 UDP，Wi-Fi 模块统一拥有两条 TCP 连接；`bsp_wifi_socket` 提供原始非阻塞 socket 接口。BSP 不包含 E8/E9/Shell 解析、FRAME 搜索文本或 `REG_LINK`。没有接入硬件 UART；`bsp_usart` 实际硬件为 Type-C USB Serial/JTAG。数据 TCP 9001 和控制 TCP 5000 的会话及缓冲独立。

## 构建与烧录

目录边界如下。`main/` 保留 ESP-IDF 默认组件和链接配置，`src/` 只保留启动入口；不把协议注册或服务适配继续堆放在 `src/`。

```text
platform/esp32c5/
  bsp/       USB、Wi-Fi/TCP、socket、通信队列、时钟/复位、临界区及内存/标识
  inc/       平台契约 esp32c5_platform.h、默认 Wi-Fi 配置
  src/       app_main.c
  main/      CMakeLists.txt、section.lf
  gcc/       compile.ps1
  tests/     主机故障注入与独立真机验收脚本
  build/     忽略跟踪的构建结果及私有配置
  CMakeLists.txt、sdkconfig.defaults、partitions.csv、compile.bat、download.bat

code/interface/demo/esp32c5/
  comm_link.c/.h、comm_discovery.c/.h、wifi_demo.c、platform_bindings.c
  realtime_scope_bindings.c
```

组件构建按 `BASE_SOURCES`、`BSP_SOURCES`、`INTERFACE_SOURCES` 分组。示波器通用采样、EA 组包和控制服务仍在 `code/dbg`，BSP 仅承接本板的运行资源及绑定。

通信绑定位于 `code/interface/demo/esp32c5/`，由本平台构建选择，不复制或混编 `demo/common/comm_link.c`。Interface 只通过 BSP 接口适配协议；Platform 保留 BSP、启动入口、时钟复位和直接依赖 ESP-IDF 的运行时适配。设备搜索接口声明放在 Interface 自己的 `comm_discovery.h`，不再由平台头文件声明。

2026-10-22 分层整理后，主循环只调度 `run_task()`，通过 SDK 的 POSIX 等待让出执行时间。公共通信、设备发现及 Wi-Fi 状态刷新使用 SECTION 注册任务；示波器配置和链表管理由 `code/dbg/realtime_scope_service.c` 自身的 `REG_TASK` 处理，采样通过 `REG_INTERRUPT` 接入已有控制 ISR 的 `section_interrupt()`。BSP 在 TCP 接入和断开时更新会话代数；旧 TCP 队列随断开清空，USB 队列独立保留。项目源码没有 FreeRTOS 头文件、任务、队列或示波器专用定时器配置；ESP-IDF 的 Wi-Fi/系统运行时仍由 SDK 提供。

已有控制中断的初始化代码在服务绑定后调用 `realtime_scope_service_source_set(actual_hz)`，声明该中断的实际频率，停止示波器不会停止控制中断。当前 ESP32 demo 没有另一套控制中断：移除原专用 GPTimer 后，未绑定采样源时返回 0 Hz、拒绝开始采样。历史 31.25 kHz 结果仅对应旧版已烧录固件。

本次整理通过 ESP-IDF 完整编译、源码格式检查及主机故障注入测试，覆盖复制发送、部分发送、满队列、TCP 会话切换、读写错误、USB 隔离、UDP 搜索/回环/重试/断网清理。主机结果：`platform/esp32c5/build/host-drivers/result.json`。2026-10-22 已将默认 `build/` 产物通过 COM7 烧录并校验，实机确认 USB E8 参数枚举、TCP 5000 参数读取和能力应答正常，TCP 9001 可连接；设备能力中的 ELF SHA256 与本地文件一致。实机记录：`platform/esp32c5/build/section-scope-flash-smoke/result.json`；当前 ELF SHA256：`26b7dde4e0fe43c320e0e17e72da0fa1f3d105c53952d0534e8f4909c8e8aa6c`。采样源未接入，设备返回 0 Hz，本次没有进行波形采样或吞吐验收。

已验证 ESP-IDF v6.1、RISC-V GCC 15.2、C11。SDK 与工具使用本机安装，不复制厂商库到仓库，也不更改全局 SDK 选择。从 base 根目录运行：

```powershell
.\platform\esp32c5\compile.bat
```

默认联网配置为空；全新构建的固件保留 USB 通信、停止 Wi-Fi。要启用 STA，提供本地私有 JSON，字段为 `ssid`、`password`、`channel`。`channel: 0` 扫描 2.4/5 GHz；指定通道则限定对应频段。示例使用占位内容：

```json
{"ssid":"YOUR_SSID","password":"YOUR_PASSWORD","channel":0}
```

```powershell
.\platform\esp32c5\gcc\compile.ps1 -WifiConfigPath 'D:\private\wifi.json'
.\platform\esp32c5\download.bat COM7
```

脚本只把配置写到被 Git 忽略的 `platform/esp32c5/build/private/wifi_config.h`，后续同一构建目录会继续使用这份配置。删除该明确文件后重新构建，可恢复不联网的默认配置。BIN/ELF 同样包含本地配置，按私有构建产物管理；账号密码不进入跟踪源码。驱动关闭 Wi-Fi NVS 配置存储，不擦除 NVS。

其他 SDK 位置可通过 `-IdfPath`、`-IdfToolsPath`、`-PythonPath` 指定。已经激活 ESP-IDF 环境时，也可以进入 `platform/esp32c5` 执行 `idf.py build` 和 `idf.py -p COM7 flash`。烧录使用 SDK 生成的 `flash_args`，替换开发板的启动、分区表和应用。

## 运行约定

- IDF main task 串行调用驱动和 baremetal SECTION；所有 BSP 公共 API 都是任务上下文接口，不允许多任务同时改配置或直接在 ISR 调用。
- SDK 负责时钟、FreeRTOS、Wi-Fi 事件与 DHCP。应用定期调用 `bsp_wifi_poll()`；断开时至多每 5 s 提交 1 次重连，不阻塞等待联网。
- Wi-Fi 初始化、启停和首次射频校准是 SDK 同步操作，可能耗时；不作为实时控制中断的一部分。
- USB 单次最多 512 字节；RX/TX 均不等待，返回实际接收/接受字节数，调用者保留未发送尾部。
- Socket 单次最多 2048 字节，均非阻塞。TCP 的 0 表示 EOF，`-1/EAGAIN` 表示暂时没有数据/空间；UDP 接收检查 `MSG_TRUNC`，超长包返回 `EMSGSIZE`。
- 原始 socket 描述符属于调用者；TCP 由 `bsp_wifi_channels` 在失去 IP、Wi-Fi 停止、EOF/错误时关闭并重建。数据端点拒绝上位机应用数据，有界取完整包，每轮最多发送 16 个 2048 字节块，未发送尾部保留到下轮。
- USB 和 TCP 的 E8、E9、Shell 上下文独立。每个物理流按完整二进制帧/文本行选择解析所有者，不把二进制 payload 再交给另一个解析器。
- BSP 私有发送队列每条链路最多 16 帧、每帧 512 字节；回调先复制字节，再由主循环处理部分发送。满队列拒绝整帧；TCP 暂停接收新的请求。
- USB 专用于二进制/文本协议，不与 SDK 日志/stdio 并发输出。ROM 复位阶段输出不属于应用协议。
- SECTION 使用 100 us 计时单位，实际协作分发约为 1 ms 或更长。Perf 是回调经过时间，包含阻塞与 SDK 抢占；不是 ESP32 全系统 CPU 占用率。

## FRAME 接入

两条链路访问同一个 base demo，均为静态地址 **2**、动态地址 **0**、E8；串口侧使用 **COM7 / 115200 / 8N1**。USB 虚拟串口的波特率设置不代表物理 UART 传输速率。

Wi-Fi 加入路由器后，TCP 和 UDP 监听 **5000**。USB Shell 输入 `wifi` 加换行可读取 IP、就绪状态和错误。`WIFI_ENABLE` 参数的 0/1 控制 radio 停止/重新联网，不通过命令提交账号密码。TCP 接入共享 E8/Shell 服务；UDP 为原始字节回环，不解析 E8。

```powershell
D:\OneDrive\LWX\FRAME\build\app\frame.exe param list --port COM7 --baud 115200 --dst 2 --wire e8 --json
D:\OneDrive\LWX\FRAME\build\app\frame.exe param list --transport tcp --host 192.168.1.17 --tcp-port 5000 --dst 2 --wire e8 --json
```

IP 是 DHCP 动态分配，上述地址仅为本次实测。demo 暴露原有 `DEMO_SHELL_COUNTER`、`DEMO_SHELL_GAIN`、`DEMO_PERF_LOOP` 和 Perf 参数，也提供 `DEMO_SHELL_PING`、`DEMO_PERF_RUN`、`wifi` 等 Shell 命令。目录包含变量和命令；命令条目没有数值存储，序列化为零值和零限值。

## 验证

主机测试编译真实 BSP，只替换 SDK 调用；目录回归直接编译真实 `shell_service.c`：

```powershell
python platform/esp32c5/tests/test_drivers.py
python platform/esp32c5/tests/frame_acceptance.py --output platform/esp32c5/build/frame-acceptance-new
```

第二条必须在开发板已烧录、已配置联网且 COM7 已释放时运行。它开启 2 个独立 FRAME CLI 会话，使用真实 USB/TCP 连接；结果保存在指定的新目录，避免覆盖旧证据。

2026-10-21 实测记录：

- 严格 GCC C11 编译通过；Flash 写入由 esptool 校验哈希。
- 主机测试通过：初始化失败回收、重复初始化、输入验证、2.4/5 GHz/自动频段选择、5 s 重连间隔、断线状态、停止失败、USB 驱动交接/边界/部分发送，以及命令条目 NULL 存储的目录回归。
- 双 FRAME 枚举实际 demo 目录；USB→TCP、TCP→USB 各 16 次整型参数交叉读写，以及浮点参数交叉读写。
- USB/TCP 各完成包含全部 256 个字节值的 E8 回环、CRC/地址错误拒绝、嵌套帧隔离，以及 Perf info/summary/dictionary/samples 查询。
- UDP 512 字节完整回环、1024 字节超长数据包拒绝与后续恢复；TCP 半帧断开后新会话解析恢复。
- Wi-Fi 停止期间 USB 保持通信、TCP 监听关闭；重新开启后加入路由器、FRAME 重连成功。

最终硬件证据位于本地 `platform/esp32c5/build/frame-acceptance-fixed/result.json`；该目录不进入 Git。AP 模式与自动重连时间逻辑有主机验证；本次实板网络验收使用 STA/路由器，没有声称 AP 射频实测。
