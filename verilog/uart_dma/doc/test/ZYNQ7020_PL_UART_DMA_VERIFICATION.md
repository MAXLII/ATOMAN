# Zynq-7020 PL UART DMA 数字验证报告

## 1. 验证版本

测试日期为 2026-07-25，目标器件为 `xc7z020clg400-2`，时钟为 50 MHz，
RTL 版本寄存器为 `0x00010000`。

| 文件 | SHA-256 |
|------|---------|
| `axi_uart_dma.v` | `A87AF8BF3EC0C472014C435EE9F55B39B7902696914BABE30672D35D7D4C328E` |
| `uart_serial_core.v` | `0172E05C898734151A637F0BDE6BB8858F0A627A8430D321D2B919EEF7BE15D2` |
| `uart_sync_fifo.v` | `2D4ABB4239F49338D46282B6078E5F508939D449891ED58EDE34BE358FEB0FCE` |
| `tb_axi_uart_dma.sv` | `E84CD3590E0D7F725373ADC405809D6E4293363664D7CFC70DE63966C329A705` |
| `tb_uart_serial_core.sv` | `534F7196F165F2E43A9C97EF685A75A85A2AC2A7E3FD468831B8B406D2372984` |

## 2. SystemVerilog 自检

执行：

```powershell
.\verilog\uart_dma\sim\run_sim.ps1
```

结果：

```text
UART_CORE_SIM_RESULT PASS vectors=25 failures=0
UART_DMA_AXI_SIM_RESULT PASS bytes=1048576 wraps=4096 normal_irq=0 error_irq=PASS failures=0
UART_DMA_RTL_SIMULATION result=PASS
UART_DMA_RTL_VERIFICATION result=PASS
```

自检覆盖：

- 5～8 数据位、无/奇/偶校验和 1/2 停止位
- 1200、默认 921600 和 1000000 baud
- 连续帧、起始位毛刺、采样偏移、parity、frame 和 break
- FIFO 满空、TX/RX 背压和内部数字回环
- RX/TX ring 正常运行、多次回卷、跨尾部连续数据、满和空
- 32 位计数器回卷
- AXI AW/W 独立到达、`WSTRB`、读写延迟和背压
- SLVERR、DECERR、reset、disable 和错误恢复
- 正常路径 IRQ 计数为 0，错误 IRQ 可 W1C 清除
- 1 MiB PRBS 内部回环，mismatch 为 0

## 3. Vivado OOC 综合

执行：

```powershell
.\verilog\uart_dma\sim\run_synth.ps1
```

结果：

| 项目 | 结果 |
|------|------|
| 约束周期 | 20.000 ns |
| WNS | +14.811 ns |
| TNS | 0.000 ns |
| Slice LUT | 765 |
| 综合后 LUT cell 计数 | 833 |
| Slice Register | 735 |
| Block RAM Tile | 0 |
| actionable DRC | 0 |

独立 OOC 顶层仅有预期的 `ZPS7-1`，表示 Zynq 器件设计中没有实例化 PS7；
脚本只豁免这一条，其他 DRC 均会导致失败。

## 4. Gate 2 结论

RTL、testbench 和 OOC 报告来自上表同一源码版本。全部自检通过、1 MiB PRBS
零 mismatch、正常 IRQ 为 0、错误 IRQ 用例全部通过、actionable DRC 为 0、
WNS 非负。

```text
UART_DMA_RTL_VERIFICATION result=PASS
```
