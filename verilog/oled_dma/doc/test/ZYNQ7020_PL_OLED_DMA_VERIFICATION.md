# Zynq-7020 PL OLED DMA 数字验证

## 1. 验证版本

- RTL版本：`0x00010000`
- 目标器件：`xc7z020clg400-2`
- PL时钟：50 MHz
- RTL：Verilog-2001
- testbench：SystemVerilog

## 2. 分层验证

`tb_ssd1306_protocol.sv`使用独立PHY模型验证：

- 初始化、寻址、显示开关、反显和对比度命令。
- CLEAR产生1024字节零。
- 连续1024帧、累计1048576字节，零mismatch。

`tb_axi_oled_dma.sv`使用AXI内存模型验证：

- AXI-Lite AW/W独立到达、读写响应和WSTRB。
- 16个16-beat INCR burst组成一帧。
- DDR到快照RAM和串行输出的字节顺序。
- CLEAR不依赖DDR数据。
- 非对齐地址、W1C IRQ和AXI SLVERR。
- 正常路径IRQ保持为0。

结果：

```text
TB_SSD1306_PROTOCOL result=PASS frames=1024 bytes=1048576
TB_AXI_OLED_DMA result=PASS clear=1 serial_bytes=4096
OLED_DMA_RTL_SIMULATION result=PASS
```

## 3. OOC综合

```text
OLED_DMA_OOC_SYNTH_RESULT status=PASS
actionable_drc=0
ooc_exempt=ZPS7-1
period_ns=20.000
wns_ns=14.674
lut=745
registers=571
bram=0
```

`ZPS7-1`仅由脱离PS单独综合AXI Master产生。1024字节快照RAM映射为分布式RAM。

```text
OLED_DMA_RTL_VERIFICATION result=PASS
```
