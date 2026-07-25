# Zynq-7020 PL UART DMA 板级验收报告

## 1. 版本与环境

测试日期为 2026-07-25。开发板为 ZYNQ MINI RevB，
器件为 `xc7z020clg400-2`，工具为 Vivado/SDK 2018.3 和 FRAME CLI。

| 产物 | SHA-256 |
|------|---------|
| `zynq7020_platform.bit` | `44F3CA1846076E023F2FA0E396B87EE498C50309E457249B09A32D0720E28387` |
| `zynq7020_platform.hdf` | `1E1B8BACD3E0AD32A62DA5C051D086B787BB5BC1C8EDF58CCEFFB7B61EB3A9A1` |
| baremetal ELF | `3A51922EEB6B1B0FCCDCA8CA55CF9C2EADD589F67D43DE59C1EB2679A0D30B42` |
| A9 SRTOS ELF | `F2BFBFA7FA2C1678D4C695A2EB571004959067FE5068B34689CC79B4ED801BD8` |

COM6 为 PS UART1，COM7 为 PL UART DMA，两端均使用 921600 8N1。JTAG、
COM6 和 COM7 同时连接。

## 2. PS+PL 平台门禁

完整平台实现结果：

| 项目 | 结果 |
|------|------|
| block design | PASS |
| actionable DRC | 0 |
| WNS | +9.405 ns |
| TNS | 0.000 ns |
| Slice LUT | 2606 / 53200 |
| Slice Register | 3257 / 106400 |
| bitstream/HDF | 已生成 |
| JTAG UART DMA VERSION | `0x00010000` |

实现 DRC 报告只包含 Xilinx 2018.3 对 PS7 HP0 内部路径产生的
`RTSTAT-10`，构建脚本将该厂商内部规则从 actionable DRC 中排除。GPIO、
IIR、DDR 和 UART DMA JTAG 回归均通过。

## 3. 固件与内部回环

baremetal 和 A9 SRTOS 均在全部严格警告选项及 `-Werror` 下零警告构建。
两者链接末地址均为 `0x001320AB`，低于 DMA 保留区 `0x1FF00000`。

COM6 执行 4096 字节内部数字回环：

```text
pl_uart_dma_test result=PASS bytes=4096 rx=4096/4096 tx=4096/4096
irq=0 errors=00000000/00000000 stop=00000000
```

A9 SRTOS 在压测前后的状态均满足：

```text
fault=0 save_fail=0 release_fail=0
```

## 4. FRAME 验收

`board_pl_uart_dma_acceptance.ps1` 启动两个独立进程：COM6 进程每 2 秒执行
`PL_UART_STATUS`，COM7 进程使用 FRAME 的构帧和解析器执行协议压测。同一
串口始终只有一个进程打开。

COM7 使用 `0x01/0x17` 回环命令，每帧 payload 为目标最大值 497 字节，
完整帧长 512 字节。payload 前 4 字节为序号，其余字节由序号生成确定性测试
数据。每个 ACK 均检查：

- 命令号和 `is_ack`
- 源、动态源、目的和动态目的地址
- FRAME CRC 和 EOP
- 序号
- 497 字节完整 payload

baremetal 结果：

```text
FRAME_COM7_STRESS result=PASS frames=11000 payload=497
tx_bytes=5632000 rx_bytes=5632000 total_bytes=11264000 elapsed_s=152.125
pl_uart version=00010000 irq=00000000/0/00000000
rx=6656535/6656535 tx=6656535/6656535
uart_err=00000000 dma_err=00000000 stop=00000000
```

A9 SRTOS 结果：

```text
FRAME_COM7_STRESS result=PASS frames=11000 payload=497
tx_bytes=5632000 rx_bytes=5632000 total_bytes=11264000 elapsed_s=151.953
pl_uart version=00010000 irq=00000000/0/00000000
rx=5632000/5632000 tx=5632000/5632000
uart_err=00000000 dma_err=00000000 stop=00000000
```

每轮均包含前 1000 个最大有效回环帧验收，且总双向传输量为
11,264,000 字节，大于 10 MiB。最终 produced 与 consumed 一致。

FRAME 原始十六进制测试在 COM7 发送：

```text
74 69 6D 65 0D 0A
```

目标返回 `time = ...ms\r\n` 的完整十六进制数据，baremetal 和 SRTOS 均通过。
COM6 Shell、COM7 Shell 和两个 comm context 在并发运行中保持独立。

## 5. 结论

baremetal 与 A9 SRTOS 两轮验收均满足正常收发 IRQ 为 0、ring 已排空、
overflow/parity/frame/AXI 错误为 0，SRTOS 无调度故障。

```text
BOARD_PL_UART_DMA_SELFTEST result=PASS debug_port=COM6 pl_port=COM7 baud=921600
```
