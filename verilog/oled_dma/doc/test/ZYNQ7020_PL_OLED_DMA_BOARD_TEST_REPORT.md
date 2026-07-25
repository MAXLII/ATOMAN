# Zynq-7020 PL OLED DMA 板级测试报告

## 1. 硬件平台

| 项目 | 结果 |
|------|------|
| actionable DRC | 0 |
| vendor RTSTAT-10 | 1 |
| WNS | +9.465 ns |
| Slice LUT | 4034 / 53200 |
| Slice Register | 4419 / 106400 |
| JTAG OLED版本 | `0x00010000` |
| JTAG几何 | `0x00400080` |
| JTAG framebuffer | `0x1FF20000` |
| JTAG frame bytes | 1024 |

硬件产物：

| 产物 | SHA-256 |
|------|---------|
| bitstream | `598B4B76AC6F5208A8330C9CBDFA839DC1769C45D1D1B3131F9637EE60829D5A` |
| HDF | `22CB0181EA067EF74FE60E358B2135E0A014231FD19B05397995A600CBF40C2C` |

## 2. 固件

| 固件 | 大小 | SHA-256 |
|------|-----:|---------|
| baremetal ELF | 218612字节 | `68AEF84DABA6BBCACFA0141F79E9E9D536CD940DB79CEF33D543912986C9B079` |
| A9 SRTOS ELF | 218612字节 | `7108A1951AF847DF8362C052ED4A29153A564F0716E899EB91011ED6BB83E1EA` |

两种固件均以严格警告和 `-Werror` 构建通过。

## 3. FRAME验收

baremetal：

```text
pl_oled_test result=PASS pattern=checkerboard
manual=1003 auto100=5 auto1000=4
irq=0 errors=0/0 stop=00000000
```

SRTOS：

```text
pl_oled_test result=PASS pattern=checkerboard
manual=1003 auto100=5 auto1000=4
irq=0 errors=0/0 stop=00000000

zynq mode=srtos-a9 fault=0 save_fail=0 release_fail=0
pl_oled version=00010000 status=00000023 fb=1FF20000
frame=1056 irq=00000000/0/00000000 errors=0/0 stop=00000000
pl_uart_dma_test result=PASS bytes=4096
rx=4096/4096 tx=4096/4096 irq=0
errors=00000000/00000000 stop=00000000
```

测试期间Zero Player仍按1秒周期运行，因此1000次测试刷新对应的硬件总增量为1003。
OLED与UART DMA共享HP0后均无错误。实体屏幕光学效果需要现场目视确认。

```text
BOARD_PL_OLED_DMA_SELFTEST result=PASS fb=0x1FF20000 bytes=1024 debug_port=COM6 optical=USER_CONFIRM_REQUIRED
```
