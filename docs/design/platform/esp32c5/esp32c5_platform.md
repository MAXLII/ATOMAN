# ESP32-C5 通信平台

本平台将 T-Display-C5 工程已经验证的 Wi-Fi 和 USB Serial/JTAG 能力回归到 base，并接入 base 当前的 demo。硬件目标是 LILYGO T-Display-C5 / ESP32-C5、16 MB Flash；不初始化显示、触摸、按键或电池器件，也不包含 Shuttle、多镜像、公共 RAM 或升级机制。

本目录保留裸机 SECTION 协作调度。SRTOS 工程已独立到 `platform/esp32c5_srtos`，两者的 BSP、构建脚本、配置及 Interface 分别维护，不相互引用。

## 代码位置与职责

| 位置 | 职责 |
|---|---|
| `platform/esp32c5/bsp/bsp_usart.c/.h` | Type-C USB Serial/JTAG 初始化、非阻塞读取、有限等待发送 |
| `platform/esp32c5/bsp/bsp_wifi.c/.h` | Wi-Fi 初始化、STA/AP 配置、状态查询、STA 重连 |
| `platform/esp32c5/bsp/bsp_wifi_socket.c/.h` | 非阻塞 IPv4 TCP 服务端、UDP 收发、描述符清理 |
| `platform/esp32c5/bsp/bsp_comm.c/.h` | 公共响应队列及 UDP；通过 Wi-Fi BSP 使用控制 TCP，不依赖协议层 |
| `platform/esp32c5/bsp/bsp_wifi_channels.c/.h` | Wi-Fi 模块统一管理 TCP 5000/9001、连接代数、取包、部分发送及断线清理 |
| `code/interface/demo/esp32c5/comm_link.c` | 将 BSP 字节流、发送函数和会话事件接到上层协议回调 |
| `platform/esp32c5/bsp/comm_discovery.c/.h` | 设备发现应答和 UDP 回环服务，由 CommLink 轮询驱动 |
| `code/interface/demo/esp32c5/wifi_interface.c/.h` | Wi-Fi 请求与状态值的桥梁接口，不持有命令状态 |
| `code/business/demo/esp32c5/` | 协议状态机、命令处理、诊断显示、采样缓存及绑定重试 |
| `platform/esp32c5/bsp/bsp_system.c/.h` | SDK 单调时钟、100 us 转换、复位、临界区、静态 RAM 校验及固件标识 |
| `platform/esp32c5/bsp/bsp_timer.c/.h` | 板级固定周期中断，向已绑定的 `section_interrupt()` 分发 |
| `code/section/baremetal/` | 裸机注册表、周期任务协作执行及中断分发 |
| `code/interface/demo/esp32c5/realtime_scope_bindings.c/.h` | 将上层提供的采样存储和通用示波器回调绑定到 BSP 能力 |
| `code/interface/demo/esp32c5/platform_bindings.c/.h` | SECTION 时钟/复位回调与 Perf 注册，调用 BSP，不直接依赖 SDK |
| `platform/esp32c5/src/app_main.c` | 唯一启动入口，初始化后循环调用裸机 `run_task()` |
| `platform/esp32c5/main/` | ESP-IDF 组件和注册段保留 |
| `platform/esp32c5/gcc/compile.ps1` | 使用本机 SDK 的构建/烧录入口及私有联网配置 |
| `code/business/demo/` | 直接编译已有的 `demo_shell.c`、`demo_task.c`、`demo_perf.c` |

沿用其他平台的 BSP 收发接口 → CommLink 协议绑定 → SECTION 注册关系。`bsp_comm` 维护公共响应队列及 UDP，Wi-Fi 模块统一拥有两条 TCP 连接；`bsp_wifi_socket` 提供原始非阻塞 socket 接口。通用 E8/E9/Shell 解析属于上层 demo；BSP 负责设备发现、字节传输和网络资源，Interface 只负责 `REG_LINK` 及上下层回调接线。没有接入硬件 UART；`bsp_usart` 实际硬件为 Type-C USB Serial/JTAG。数据 TCP 9001 和控制 TCP 5000 的会话及缓冲独立。

## Interface 边界

Interface 是 BSP 与上层之间的桥梁，只做参数/类型转换、回调绑定、字节及会话事件转接和必要的注册接线。协议状态机、Shell 命令及文本展示、采样缓存和绑定重试生命周期归上层 demo；设备发现、网络队列、SDK 状态和堆查询归 BSP。上层 demo 不直接包含 BSP/SDK 头文件，Interface 不包含 SDK 头文件、不注册业务命令、不持有采样缓存。

`tests/test_platform_isolation.py` 同时检查这些分层依赖和两份平台之间的隔离；只共享通用框架，不通过另一平台的 BSP、Interface 或上层 demo 编译。

绑定声明与实现放在 Interface 同目录的 `platform_bindings.c/.h`，公共 `platform.h` 通过本平台独立的包含目录选择该接口；启动入口单独使用 `inc/app_main.h` 与 `src/app_main.c`。TCP 通道接口和诊断声明属于 `bsp_wifi_channels.c/.h`，无线电配置与状态属于 `bsp_wifi.c/.h`。

## 构建与烧录

目录边界如下。`main/` 保留 ESP-IDF 默认组件和链接配置，`src/` 只保留启动入口；不把协议注册或服务适配继续堆放在 `src/`。

```text
platform/esp32c5/
  bsp/       USB、Wi-Fi/TCP、socket、通信队列、时钟/中断、内存/标识
  inc/       启动入口 app_main.h、默认 Wi-Fi 配置
  src/       app_main.c
  main/      CMakeLists.txt、section.lf
  gcc/       compile.ps1
  tests/     主机故障注入与独立真机验收脚本
  build/     忽略跟踪的构建结果及私有配置
  CMakeLists.txt、sdkconfig.defaults、partitions.csv、compile.bat、download.bat

code/interface/demo/esp32c5/
  comm_link.c/.h、wifi_interface.c/.h、platform_bindings.c/.h
  realtime_scope_bindings.c/.h
```

组件构建按 `BASE_SOURCES`、`BSP_SOURCES`、`INTERFACE_SOURCES` 分组。示波器通用采样、EA 组包和控制服务仍在 `code/dbg`，BSP 仅承接本板的运行资源及绑定。

通信绑定位于 `code/interface/demo/esp32c5/`，由本平台构建选择，不复制或混编 `demo/common/comm_link.c`。Interface 只通过 BSP 接口适配协议；Platform 保留 BSP、启动入口、时钟复位和直接依赖 ESP-IDF 的运行时适配。设备搜索声明与实现位于本平台 BSP 的 `comm_discovery.c/.h`。

本平台选择 `code/section/baremetal/`；SRTOS 调度器和 SDK 中断返回适配仅由独立的 `esp32c5_srtos` 平台选择。公共通信、设备发现及 Wi-Fi 状态刷新使用 `REG_TASK`；示波器配置与链表管理在通用服务中注册，采样由共享 demo 的 `REG_INTERRUPT(255, sample_interrupt)` 接入板级中断。ESP-IDF 仍负责 Wi-Fi、lwIP、系统事件及其 FreeRTOS 运行时。

板级 GPTimer 当前固定为 32 us，即 31250 Hz；上层 demo 在服务绑定后通过 `realtime_scope_service_source_set(actual_hz)` 声明实际频率，上位机查询而不设置采样率。停止示波器不会停止板级中断。移植到已有控制算法的平台时复用其周期中断，通用 scope 不配置定时器。

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

- 应用使用 `code/section/baremetal`，周期任务在 `run_task()` 中协作执行，不链接 RV32 SRTOS 或中断返回包装。
- 本平台的 `bsp_system_runtime_enter()/exit()` 为空操作；SDK 事务不会被另一应用任务抢占，采样硬件中断仍可运行。
- 本目录和 `code/interface/demo/esp32c5/` 仅引用本平台文件；裸机核心与 SRTOS 核心分别编译。
- SDK 负责时钟、FreeRTOS、Wi-Fi 事件与 DHCP。Wi-Fi/IP 事件通过无锁原子状态发布关联、取得 IP、丢失 IP 和断线；事件回调不进入射频查询。`bsp_wifi_poll()` 只读取该状态及 netif IP 缓存，断开时至多每 5 s 提交 1 次重连，不等待联网。
- Wi-Fi 初始化、启停和首次射频校准是 SDK 同步操作，可能耗时；不作为实时控制中断的一部分。
- USB 单次最多 512 字节；RX/TX 均不等待，返回实际接收/接受字节数，调用者保留未发送尾部。
- Socket 原始调用上限为 8192 字节；本平台上传暂存为 3072 字节，公共控制响应仍使用每帧 512 字节的原队列。TCP 的 0 表示 EOF，`-1/EAGAIN` 表示暂时没有数据/空间；UDP 接收检查 `MSG_TRUNC`，超长包返回 `EMSGSIZE`。
- 原始 socket 描述符属于调用者；TCP 由 `bsp_wifi_channels` 在失去 IP、Wi-Fi 停止、EOF/错误时关闭并重建。数据端点拒绝上位机应用数据，有界取完整包，每轮最多推进 16 批。发送优先取 1440 字节 MSS 的整数倍，3072 字节暂存可一次提交 2880 字节；不足 MSS 的尾部最多主动聚合 1 ms。部分发送和背压保留尾部，后续追加不延长旧数据的期限。控制和上传两端的接收探测均限频 1 ms，直接收发硬错误立即处理；实际发送间隔以诊断计数为准。
- USB 和 TCP 的 E8、E9、Shell 上下文独立。每个物理流按完整二进制帧/文本行选择解析所有者，不把二进制 payload 再交给另一个解析器。
- BSP 私有发送队列每条链路最多 16 帧、每帧 512 字节；回调先复制字节，再由主循环处理部分发送。满队列拒绝整帧；TCP 暂停接收新的请求。
- 控制 TCP 通过 256 字节暂存批量接收，再逐字节供给 `REG_LINK`；含两个计数共增加 264 字节。未消费尾部可跨 Link 轮次保留，EOF、发送错误、断网或会话变化立即清理，避免旧数据进入新会话。
- USB 专用于二进制/文本协议，不与 SDK 日志/stdio 并发输出。ROM 复位阶段输出不属于应用协议。
- SECTION 使用 100 us 计时单位，实际任务时机受协作循环和 SDK 等待影响；本平台没有 SRTOS 时间片。Perf 是回调经过时间，不是 ESP32 全系统 CPU 占用率。

## 示波器内存与 Wi-Fi 发送

本平台使用 16 KiB 内部采样队列、3072 B 上传暂存，关闭 PSRAM。`BASE_SCOPE_QUEUE_BYTES`、`BASE_WIFI_UPLOAD_BYTES` 和 `BASE_SCOPE_BYTE_BUDGET=1700000` 属于本平台自己的 CMake 配置。此前 SRTOS 实验不能代替恢复裸机后的实板验收。

发送按 MSS 聚合，短尾最多主动等待 1 ms；部分发送及 EAGAIN 保留尾部。TCP 发送窗口动态上限为 41760 B，接收窗口为 8760 B，接收 mailbox 为 8。lwIP 两项 IRAM 优化均启用；其代码占用需与缓冲数组分别统计。

SRTOS 内存、实测吞吐及失败边界见 [独立 SRTOS 平台](../esp32c5_srtos/esp32c5_srtos_platform.md)。该链接仅为说明，不构成代码或构建依赖。

`wifi` 只读取缓存，RSSI 初始为 0；需要刷新信号强度时显式执行 `wifi_rssi`。该命令调用 `bsp_wifi_refresh_rssi()` 并保持 SDK guard，可能推迟应用任务及正在运行的采集发送。当前 SDK 的 `esp_wifi_sta_get_ap_info()` 会投递 Wi-Fi 任务消息并等待完成信号，并非本地字段读取，已从每 100 ms 周期路径移除。`esp_netif_get_ip_info()` 在当前 SDK 中直接读取 netif 缓存，不经过该同步 RPC。

## FRAME 接入

两条链路访问同一个 base demo，均为静态地址 **2**、动态地址 **0**、E8；串口侧使用 **COM7 / 115200 / 8N1**。USB 虚拟串口的波特率设置不代表物理 UART 传输速率。

Wi-Fi 加入路由器后，TCP 和 UDP 监听 **5000**。USB Shell 输入 `wifi` 加换行可读取 IP、就绪状态和错误。`WIFI_ENABLE` 参数的 0/1 控制 radio 停止/重新联网，不通过命令提交账号密码。TCP 接入共享 E8/Shell 服务；UDP 为原始字节回环，不解析 E8。

```powershell
D:\OneDrive\LWX\FRAME\build\app\frame.exe param list --port COM7 --baud 115200 --dst 2 --wire e8 --json
D:\OneDrive\LWX\FRAME\build\app\frame.exe param list --transport tcp --host 192.168.1.17 --tcp-port 5000 --dst 2 --wire e8 --json
```

IP 是 DHCP 动态分配，上述地址仅为本次实测。demo 暴露原有 `DEMO_SHELL_COUNTER`、`DEMO_SHELL_GAIN`、`DEMO_PERF_LOOP` 和 Perf 参数，也提供 `DEMO_SHELL_PING`、`DEMO_PERF_RUN`、`wifi` 等 Shell 命令。目录包含变量和命令；命令条目没有数值存储，序列化为零值和零限值。

## 验证

裸机与 SRTOS 拆分后分别构建。`tests/test_platform_isolation.py` 检查实际编译源和包含目录，再检查最终 ELF：裸机工程不得包含 `section_riscv_run` 或 `__wrap_rtos_int_enter`，事务边界须为本平台弱空操作。

2026-10-23 本平台构建及 BSP 主机测试通过，785 个编译单元的实际输入和链接输入未发现另一平台引用，ELF 符号检查通过。ELF SHA-256 为 `d3733bdfc8fdcf1770d03113597aa7e4aa4baec5863cf3621364570d9b1c9c07`；静态 RAM 为 93524 B，IRAM 含尾部对齐为 115072 B，SRAM 起点至堆起点保留 208608 B。SDK 动态堆另计。因当前 ESP32 USB 串口未枚举，本次未重新烧录裸机固件。

拆分前 SRTOS 的低内存和 Wi-Fi 实测单独记入独立平台文档；以下 2026-10-21 记录属于早期裸机固件，不代替本次恢复裸机后的运行验证。

主机测试编译真实 BSP，只替换 SDK 调用；上传暂存按 2500、3072 字节分别编译运行，覆盖 MSS 分段、尾部追加、部分发送、背压、计时回绕及逐字节完整性。控制接收覆盖 256 字节批读、跨轮消费、EOF/EAGAIN、换会话尾部丢弃和两端 1 ms 探测。目录回归直接编译真实 `shell_service.c`：

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

早期 2026-10-22 分层整理的主机故障注入结果位于 `platform/esp32c5/build/host-drivers/result.json`；随后通过 COM7 烧录的旧固件确认了 USB E8 参数枚举、TCP 5000 参数读取和 TCP 9001 接入，记录位于 `platform/esp32c5/build/section-scope-flash-smoke/result.json`。该次 ELF SHA256 为 `26b7dde4e0fe43c320e0e17e72da0fa1f3d105c53952d0534e8f4909c8e8aa6c`，当时采样源未绑定、返回 0 Hz，没有进行波形采集或吞吐验收。
