# Zynq-7020 PL OLED DMA 设计

## 1. 外设定位

PL OLED DMA 外设从 DDR 中读取 128x64 单色 framebuffer，生成
SSD1306 兼容的 4 线串行时序。控制寄存器基地址为 `0x41220000`，
默认 framebuffer 地址为 `0x1FF20000`。

framebuffer 固定为 1024 字节，按 SSD1306 page 格式组织：

```text
offset = (y / 8) * 128 + x
bit    = y % 8
```

## 2. 分层边界

| 层次 | 模块 | 职责 |
|------|------|------|
| 数据控制 | `axi_oled_dma`、`oled_frame_dma`、`oled_frame_ram` | AXI寄存器、DDR DMA、帧快照、调度和错误 |
| 控制器协议 | `ssd1306_protocol` | 初始化、寻址、显示控制和帧协议 |
| 串行PHY | `oled_serial_phy` | 命令/数据字节到D0、D1、DC时序 |

数据控制层通过 operation valid/ready 和 frame data valid/ready 接口连接协议层。
协议层通过 byte valid/ready 接口连接PHY。SSD1306命令只存在于协议层，物理引脚
时序只存在于PHY。

## 3. DMA与快照

- 显存地址要求1024字节对齐。
- DMA使用32位AXI4读通道。
- 每帧执行16个16-beat INCR burst。
- AXI Master只允许一个outstanding事务。
- 完整帧先写入256x32位快照RAM，再交给协议层。
- DMA完成后PS可以修改DDR显存，不影响正在发送的快照。
- SLVERR、DECERR和RLAST错误停止DMA并锁存停止原因。

## 4. 寄存器

| 偏移 | 名称 | 功能 |
|---:|------|------|
| `0x00` | CONTROL | enable、soft reset、reinit、present、clear、display on、invert、auto refresh |
| `0x04` | STATUS | 初始化、DMA、协议、显示和错误状态 |
| `0x08` | FB_BASE | framebuffer基地址 |
| `0x0C` | SPI_DIV | 串行半周期分频 |
| `0x10` | REFRESH_PERIOD | 自动刷新周期，单位为50 MHz时钟 |
| `0x14` | CONTRAST | 低8位对比度 |
| `0x18` | IRQ_STATUS | W1C错误状态 |
| `0x1C` | IRQ_ENABLE | 错误中断使能 |
| `0x20` | FRAME_COUNT | 成功帧数 |
| `0x24` | CLEAR_COUNT | 清屏次数 |
| `0x28` | AXI_ERROR_COUNT | AXI错误次数 |
| `0x2C` | COMMAND_ERROR_COUNT | 配置、命令和协议错误次数 |
| `0x30` | DMA_STOP_REASON | DMA停止原因 |
| `0x34` | VERSION | `0x00010000` |
| `0x38` | GEOMETRY | `0x00400080` |
| `0x3C` | FRAME_BYTES | `1024` |

CONTROL的bit0、5、6、7为保持位。bit1 soft reset、bit2 reinit、bit3 present、
bit4 clear为写1脉冲。正常刷新和清屏不产生中断。

## 5. 显示行为

- enable上升沿自动执行控制器初始化。
- PRESENT先DMA快照，再发送寻址命令和1024字节数据。
- CLEAR直接发送1024字节零，不读取或修改DDR显存。
- 自动刷新复位后关闭；到期遇到忙状态时只保留一个待处理刷新。
- 显示开关、反显和对比度操作经过通用operation接口提交给协议层。
