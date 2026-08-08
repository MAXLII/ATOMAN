# GD32E507 以太网通信使用方法

## 1. 功能

`platform/gd32e507/` 使用 GD32E507Z-EVAL 板载 DP83848 PHY 和 LwIP 2.2.1，提供以下 IPv4 通信能力：

- ICMP Ping
- TCP FRAME 协议，端口 `5000`
- UDP Echo，端口 `5000`

网络协议栈运行在 Section 注册的 1 ms 后台任务中。ENET DMA 使用轮询方式接收报文，LwIP Raw API 回调不在中断中运行。

## 2. 硬件连接

板卡以太网接口使用 RMII 和板载 50 MHz 振荡器，PHY 地址为 `1`。

以下跳线连接 MCU 与 PHY：

| 跳线 | 位置 | 信号 |
|---|---:|---|
| JP14 | 2-3 | PB13 / RMII_TXD1 |
| JP15 | 2-3 | PB12 / RMII_TXD0 |
| JP16 | 2-3 | PA7 / RMII_CRS_DV |

JP10 对应 PHY 中断信号。当前固件轮询 ENET DMA，不依赖 PHY 中断。

## 3. 网络配置

板卡参数在 `platform/gd32e507/enet/inc/enet_config.h` 中定义：

| 参数 | 值 |
|---|---|
| IPv4 地址 | `192.168.1.101` |
| 子网掩码 | `255.255.255.0` |
| 网关 | `0.0.0.0` |
| MAC 地址 | `02:00:00:E5:07:01` |
| TCP FRAME 协议端口 | `5000` |
| UDP Echo 端口 | `5000` |

电脑直连板卡时，可将有线网卡配置为 `192.168.1.100/24`。

## 4. 编译与下载

```bat
cd platform\gd32e507
compile.bat
download.bat
```

固件启动后会发送 Gratuitous ARP，使电脑更新板卡 IP 与 MAC 的映射。

## 5. 通信验证

### 5.1 Ping

```powershell
ping 192.168.1.101
```

### 5.2 FRAME TCP

FRAME 上位机的串口栏选择 `Ethernet`，Host 填 `192.168.1.101`，TCP Port 填 `5000`，然后连接。TCP 字节流直接承载与调试串口相同的 `0xE8` 二进制协议，可使用参数、Scope、SFRA、Perf 和 Trace 等协议页面。

### 5.3 UDP Echo

```powershell
python -c "import socket; s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); s.settimeout(2); p=b'gd32e507 udp'; s.sendto(p,('192.168.1.101',5000)); print(s.recvfrom(1024)); s.close()"
```

## 6. 诊断变量

固件提供以下 J-Link 全局变量：

| 变量 | 含义 |
|---|---|
| `g_bsp_enet_init_attempt_count` | PHY/MAC 初始化次数 |
| `g_bsp_enet_init_error_count` | PHY/MAC 初始化失败次数 |
| `g_bsp_enet_phy_read_error_count` | PHY 状态读取失败次数 |
| `g_enet_rx_frame_count` | LwIP 已接收帧数 |
| `g_enet_rx_error_count` | 接收处理错误数 |
| `g_enet_tcp_connection_count` | TCP 连接数 |
| `g_enet_tcp_rx_byte_count` | TCP 接收字节数 |
| `g_enet_tcp_rx_drop_count` | TCP 接收缓冲区溢出并断开连接的次数 |
| `g_enet_tcp_tx_byte_count` | 已进入 TCP 发送缓冲区的协议字节数 |
| `g_enet_tcp_tx_drop_count` | TCP 未连接或发送缓冲区不足时丢弃的完整协议帧数 |
| `g_enet_tcp_tx_error_count` | TCP 发送错误数 |
| `g_enet_udp_datagram_count` | UDP 接收报文数 |
| `g_enet_udp_rx_byte_count` | UDP 接收字节数 |
| `g_enet_udp_tx_error_count` | UDP 发送错误数 |

## 7. 当前验证结果

GD32E507Z-EVAL 与电脑有线网卡直连验证结果：

- Ping：4/4 成功，0% 丢包
- TCP FRAME：Scope 列表查询收到 `0x01/0x18`、`ACK=1`，返回 `demo_scope_wave`
- UDP Echo：26 字节报文成功，10 组 512 字节报文内容完全一致
- TCP、UDP 和 ENET 接收错误计数均为 `0`

## 关联导航

- [GD32E507 平台工程](../../../platform/gd32e507/)
- [MCU 编译与下载](../build/mcu_build_download_guide.md)
- [工程设计](../../ENGINEERING_DESIGN.md)
