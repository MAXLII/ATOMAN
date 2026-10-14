# STM32F103C8T6 平台

本平台沿用仓库的 `Firmware / bsp / inc / src / gcc` 分层，公共 SECTION、Shell、COMM、COMM v1 和 Perf 从 `code/` 编译引用。BSP 只操作硬件，`src/platform_init.c` 和 `src/comm_link.c` 负责框架注册与接口绑定。平台不复制公共调度器，也不接入功率控制业务。

## 板级配置

- 原理图：仓库 `references/MINI-STM32F103C8T6原理图.pdf`。
- 芯片：Cortex-M3，64 KB Flash、20 KB RAM；编译使用 `STM32F10X_MD`。
- 时钟：HSE 8 MHz，PLL x9，HCLK/PCLK2 72 MHz、PCLK1 36 MHz；启动后检查实际时钟。
- 串口：USART1 默认映射，PA9 TX、PA10 RX，115200、8N1；RX 中断队列 512 字节，TX DMA1 通道 4 队列 1024 字节。
- 原理图的 J10 `TX1/RX1` 实际接 PB6/PB7。本工程按用户的 PA9/PA10 接线，不使能 USART1 重映射。
- LED：PB3，低电平点亮。禁用 JTAG、保留 PA13/PA14 SWD，释放 PB3。
- SECTION 时基：SysTick 100 us；TIM2 每 1 ms 调用注册中断。
- Perf 时基：DWT CYCCNT，72 MHz，1 个计数为 1/72 us。SysTick 恢复调试器断开时可能清除的 DWT 门控；测试同时对照主机时间和 SysTick。

## 构建和下载

在仓库根目录运行，ARM GCC 与 `mingw32-make` 需在 PATH 中：

```bat
platform\stm32f103c8t6\compile.bat baremetal
platform\stm32f103c8t6\compile.bat srtos_m
platform\stm32f103c8t6\download.bat srtos_m
```

不带参数时选择 `srtos_m`。编译第二个参数支持 `-b`、`-r`、`-c`。下载脚本用 J-Link SWD，并执行 Flash 验证；可以通过环境变量 `JLINK_EXE` 指定工具路径。两个运行时分别输出到 `build_baremetal` 和 `build_srtos_m`，不会混用对象文件。

串口发送 `board`、`time`、`help`，以 CRLF 或 LF 结束。设备地址为 2；兼容 E8 与 E9 的 `cmd_set=0x01 / cmd_word=0x17` 原样载荷回环。Link 适配层按整帧选择解析器，并排空被拒绝的帧，避免二进制载荷误入其他协议或 Shell。

SRTOS 使用仓库 `code/section/srtos_m`，由 PendSV 保存和恢复 R4–R11、EXC_RETURN。Cortex-M3 无 FPU。PSP 运行栈 2 KB，上下文保存池 2 KB；超出池容量时沿用公共调度器的故障策略。GCC 链接脚本预留 512 字节 Newlib 堆和至少 1 KB MSP 栈，并检查 SRAM 上限。

## 实板复验

安装 Python `pyserial`，或安装到本平台忽略的 `build_tools` 目录：

```bat
python -m pip install --target platform/stm32f103c8t6/build_tools pyserial
mingw32-make -C platform/stm32f103c8t6/gcc -j8 SECTION_RUNTIME=srtos_m BOARD_TEST=1 all
python platform/stm32f103c8t6/tests/run_board.py --port COM3 --runtime srtos_m --stress --image platform/stm32f103c8t6/build_srtos_m_test/stm32f103c8t6_demo.bin
```

裸机复验将上述 `srtos_m` 改为 `baremetal`。`--image` 明确执行烧录、读回和 SHA-256 比对；不带它只测试当前固件。默认压力测试 20 秒，检查 1 ms 任务、4 ms 长任务的局部栈值、实际抢占、TIM2 回调、队列错误、两种协议的随机载荷回环、损坏帧拒绝及超时恢复。测试记录保存在相应构建目录的 `acceptance.json`、`flash.log`、`readback.bin`。

普通固件不启用长任务压力负载。复验后用普通构建目录的下载脚本切回普通固件。

## 厂商源码与本地适配

`Firmware` 来自已下载的 ST 标准外设库 V3.6.0，保留包许可证与各源文件版权。CMSIS 1.30 的 `core_cm3.h/.c` 有局部 GCC 适配：独占访问参数保留 `volatile`，STREX 输出使用 early-clobber 约束，独占访问和中断/内存屏障增加编译器 memory clobber。这些改动用于 GCC 14 和公共通信原子锁，未修改 `references` 原始库。

GCC 启动文件基于官方中容量设备模板，增加 `.func_ram` 的装载复制；链接脚本隔离并 `KEEP` 注册段，提供 `__section_start / __section_end`。共享代码仅在 `code/section/platform.h` 新增平台选择、时基、性能计数单位及 Cortex-M3 调度契约。
