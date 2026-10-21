# ESP32-C5 通信平台

本平台将 T-Display-C5 工程已经验证的 Wi-Fi 和 USB Serial/JTAG 能力回归到 base，并接入 base 当前的 demo。硬件目标是 LILYGO T-Display-C5 / ESP32-C5、16 MB Flash；不初始化显示、触摸、按键或电池器件，也不包含 Shuttle、多镜像、公共 RAM 或升级机制。

## 代码位置与职责

| 位置 | 职责 |
|---|---|
| `platform/esp32c5/bsp/bsp_usart.c/.h` | Type-C USB Serial/JTAG 初始化、非阻塞读取、有限等待发送 |
| `platform/esp32c5/bsp/bsp_wifi.c/.h` | Wi-Fi 初始化、STA/AP 配置、状态查询、STA 重连 |
| `platform/esp32c5/bsp/bsp_wifi_socket.c/.h` | 非阻塞 IPv4 TCP 服务端、UDP 收发、描述符清理 |
| `platform/esp32c5/src/comm_link.c` | demo 的 USB/TCP Link 绑定、独立解析上下文、私有发送队列和 UDP 回环 |
| `platform/esp32c5/src/platform_demo.c` | demo 的 Wi-Fi 启停参数、状态命令和 FRAME 二进制回环 |
| `platform/esp32c5/src/platform_port.c` | SECTION 的 100 us 时间基准、微秒 Perf 影子计数和复位适配 |
| `platform/esp32c5/main/` | ESP-IDF 组件和注册段保留 |
| `platform/esp32c5/gcc/compile.ps1` | 使用本机 SDK 的构建/烧录入口及私有联网配置 |
| `code/business/demo/` | 直接编译已有的 `demo_shell.c`、`demo_task.c`、`demo_perf.c` |

驱动只负责联网和字节传输。协议、FRAME 参数、队列与服务端端口策略位于 demo 适配，不是 BSP 的固定组成部分。没有接入硬件 UART；文件沿用 base 的 `bsp_usart` 命名，实际硬件为 Type-C USB Serial/JTAG。

## 构建与烧录

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
- USB 单次最多 512 字节；RX 不等待，TX 最多等待 20 ms。返回实际接收/接受字节数，调用者保留未发送尾部。
- Socket 单次最多 2048 字节，均非阻塞。TCP 的 0 表示 EOF，`-1/EAGAIN` 表示暂时没有数据/空间；UDP 接收检查 `MSG_TRUNC`，超长包返回 `EMSGSIZE`。
- Socket 描述符属于调用者，Wi-Fi 驱动不隐式替用户关闭；demo 在失去 IP、Wi-Fi 停止、TCP EOF/错误时关闭并重建对应资源。
- USB 和 TCP 的 E8、E9、Shell 上下文独立。每个物理流按完整二进制帧/文本行选择解析所有者，不把二进制 payload 再交给另一个解析器。
- demo 私有发送队列每条链路最多 16 帧、每帧 512 字节；回调先复制字节，再由主循环处理部分发送。满队列拒绝整帧；TCP 暂停接收新的请求。
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
