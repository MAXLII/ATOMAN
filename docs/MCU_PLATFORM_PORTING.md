# MCU 平台接入流程

本文档记录新增 MCU 平台时的工程流程。目标是让新的 MCU 平台接入后，能够达到当前 HC32F558 工程的效果：工程可编译、串口通信可用、perf 时间可信、BSP 能承接接口层、Keil/ARMCC6 工程可直接使用。

## 1. 输入资料

新增 MCU 平台前，需要先准备以下资料。

| 资料 | 用途 |
|---|---|
| 官方 SDK / DDL / HAL 库 | 提供启动文件、CMSIS、外设驱动、工程模板、链接脚本或 scatter 文件 |
| 官方 Keil demo 工程 | 作为工程结构、启动流程、编译选项、芯片型号、Flash/RAM 配置和调试配置的基准 |
| 数据手册 | 确认内核、主频、Flash/RAM、Cache、DMA、ADC、PWM、USART、GPIO 管脚复用能力 |
| 参考手册 | 确认时钟树、外设寄存器、触发链、DMA 请求、AOS/XBAR/事件系统、定时器分频 |
| 板级原理图或管脚表 | 确认 USART、PWM、ADC、GPIO、继电器、调试 IO 的真实管脚 |
| 现有同类平台工程 | 作为代码结构、接口命名、BSP 风格和工程分组的参考 |

官方 demo 和规格书是必要输入。没有这些资料时，只能做目录和接口层面的移植，无法可靠确认以下内容：

- 工程模板中的启动文件、scatter、芯片型号和编译选项
- 主频和总线分频的合法配置
- Cache、Flash wait cycle、SRAM wait cycle 的使能方式
- USART 的 function 编号、DMA 请求源和 AOS 触发通道
- ADC 管脚是否和 PWM、晶振、串口冲突
- HRPWM / Timer / ADC 之间的硬件触发链是否正确
- perf 硬件计数器的真实周期

## 2. 平台目录

新增平台目录按当前仓库结构组织。当前 MCU 工程定位为 demo 工程，重点是验证平台工程、BSP 接口、通信链路、perf 和调试能力是否能完整跑通，不要求管脚语义和最终产品板完全一致。

```text
<mcu>/
├─ ac/
│  ├─ bsp/
│  ├─ inc/
│  ├─ src/
│  └─ keil_mdk/
├─ Firmware/
└─ inc/
```

平台目录只放 MCU 相关内容。共享控制、通信、调试、算法和接口代码继续复用 `code/`。

工程内 BSP 文件保持当前命名方式：

```text
bsp_clk.c/.h
bsp_timer.c/.h
bsp_usart.c/.h
bsp_gpio.c/.h
bsp_adc.c/.h
bsp_pwm.c/.h
```

## 3. 工程骨架

从官方 Keil demo 工程复制或参考以下内容：

- 启动文件
- system 文件
- scatter 文件
- Keil target 配置
- include path
- C11 / ARMCC6 编译选项
- 官方库源文件列表
- Flash 下载算法
- 调试配置

工程只保留一个目标 target。当前约定统一使用 C11 和 ARMCC6，不需要为 GCC、ARMCC5 或其他 shell 环境做兼容。

新增工程后，需要把共享代码按现有工程方式加入：

- `code/section`
- `code/comm`
- `code/dbg`
- `code/interface`
- 当前需要的 `code/app` demo 或业务模块
- 当前需要的 `code/lib`

## 4. 时钟与 Cache

先根据数据手册和参考手册确认 MCU 最高主频，再实现 `bsp_clk`。

`bsp_clk` 需要完成：

- 外部晶振或内部时钟源配置
- PLL 配置
- HCLK / PCLK / 外设总线分频
- Flash wait cycle
- SRAM wait cycle
- SystemCoreClock 更新
- ICache / DCache / Flash cache 等 Cache 使能
- 必要的寄存器写保护解锁和锁回

主频不要只看 SDK demo 的默认值。demo 常用于保守启动，不一定是芯片最高性能配置。

## 5. 定时器与 perf

perf 需要一个硬件递增计数器作为时间基准。平台 BSP 必须注册计数器地址和真实计数周期：

定时器选型优先级：

1. 查数据手册和参考手册中的 timer / counter 章节。
2. 优先选择该 MCU 上最高规格的 32 位定时器。
3. 定时器配置为自由运行递增计数，计数范围尽量覆盖较长时间窗口。
4. 避免使用会被业务逻辑、PWM、通信或系统 tick 占用的定时器。
5. 如果规格书确认没有合适的 32 位定时器，perf 硬件基准可以先放空，不注册 `REG_PERF_BASE_CNT`。

```c
REG_PERF_BASE_CNT(timer_cnt, period_s)
```

`period_s` 是一个计数 tick 的实际秒数。例如 0.1 us 写成：

```c
0.1e-6f
```

计数周期必须根据实际时钟树和实际分频计算，不能按期望值填写。比如 Timer6 使用 PCLK0、16 分频，则：

```text
period_s = 1 / (PCLK0 / 16)
```

接入后需要做一次外部校验：

- 写一个 100 ms 周期任务
- 用 GPIO 翻转确认物理周期
- 在 perf 中记录同一段周期
- 两者一致后，才能认为 perf 时间可信

验证完成后删除临时 GPIO 翻转和临时 perf 调试点。

## 6. 串口通信

串口 BSP 需要满足 `code/interface` 和 `comm_link` 的接口需求。

当前目标效果：

- USART 初始化
- RX DMA 环形接收
- TX DMA 环形发送
- `bsp_usart_dbg_tx_dma`
- `bsp_usart_dbg_tx`
- `bsp_usart_dbg_printf`
- `bsp_usart_dbg_rx_get_byte`
- 注册到 interface 的 `comm_link`

TX 发送原则：

- buffer 空间足够时，只把数据装入 ring buffer，不等待 DMA 完成
- buffer 空间不足时，等待 DMA 发送释放空间
- 等到空间足够后，再一次性装入待发送数据
- 不因为每一帧发送而阻塞等待整帧 DMA 完成

串口接入需要从规格书和官方库确认：

- USART 外设号
- TX/RX GPIO 管脚
- 管脚复用 function
- RX DMA 请求源
- TX DMA 请求源
- DMA 通道和 AOS 触发选择
- 中断或轮询维护 TX DMA 的方式

## 7. BSP GPIO

`bsp_gpio` 负责平台逻辑 IO 到真实管脚的映射。接口层不直接访问 MCU GPIO。

GPIO 设计优先采用数组注册或表驱动形式，把逻辑 IO、端口、管脚、模式、默认电平等信息集中到配置表里。实际初始化函数不要强行套固定模板，应根据官方库提供的 GPIO 初始化接口决定注册和遍历方式：如果官方库支持结构体初始化，就用结构体表；如果官方库按端口、管脚、功能分别配置，就在遍历表时按官方接口拆开调用。

需要提供：

- GPIO 枚举
- GPIO 参数表
- `bsp_gpio_init`
- `bsp_gpio_set_bit`
- `bsp_gpio_get_bit`

继电器、LED、测试 IO 等按逻辑名映射。demo 工程阶段不在意具体哪个脚最终对应什么硬件功能，管脚只需要满足规格书合法、和当前 demo 使用的外设不冲突，并能支撑接口链路验证。后续进入真实板级工程时，再按原理图改成最终硬件映射。

## 8. BSP ADC

`bsp_adc` 需要把接口层需要的采样量映射到 MCU ADC 外设、通道和管脚。

ADC 设计优先采用数组注册或表驱动形式，把采样信号、ADC 外设、通道、GPIO、采样时间、触发类型等信息集中到配置表里。实际实现要看官方库提供的 ADC 接口：有的库按 ADC unit 配置通道，有的库按 sequence 配置，有的库需要单独配置 channel mux、sample time、trigger 或同步模式。BSP 的注册表用于描述硬件映射，初始化流程按目标 MCU 官方库的真实接口组织。

当前 AC 接口层需要的宏包括：

```c
BSP_ADC_V_G
BSP_ADC_V_AC_IN
BSP_ADC_V_CAP
BSP_ADC_V_AC_CAP
BSP_ADC_I_L
BSP_ADC_V_BUS
BSP_ADC_I_OUT
BSP_ADC_I_AC_OUT
BSP_ADC_V_OUT
BSP_ADC_V_AC_OUT
```

ADC 接入流程：

1. 从规格书确认每个 ADC 通道对应的 GPIO。
2. 避开 USART、PWM、晶振、调试接口已占用管脚。
3. 区分同步采样信号和异步采样信号。
4. 同步信号接入硬件触发 ADC。
5. 异步信号可使用周期任务软件触发。
6. 使用表驱动初始化 GPIO analog、ADC channel、sample time。
7. 通过数据寄存器宏直接提供最新采样值。

demo 工程阶段不要求 ADC 管脚对应真实采样网络，只要求所选管脚和通道在规格书中合法，能验证 ADC 初始化、触发、数据读取宏和接口层编译链路。如果 MCU 支持 ADC 同步模式，应按参考手册确认同步单元、同步模式和触发源。不能把其他平台的触发事件名直接照搬。

## 9. BSP PWM

`bsp_pwm` 负责将接口层的 duty 命令落到 MCU PWM/HRPWM 外设。

当前接口需要：

```c
void bsp_pwm_enable(void);
void bsp_pwm_disable(void);
void bsp_pwm_set_duty(float duty_fast,
                      float duty_slow,
                      uint8_t up_en_fast,
                      uint8_t dn_en_fast,
                      uint8_t up_en_slow,
                      uint8_t dn_en_slow);
```

PWM 接入流程：

1. 根据规格书确认 PWM/HRPWM 单元数量和输出管脚。
2. 选择一个 master 单元作为系统节拍和 ADC 触发源。
3. 选择输出单元连接功率管。
4. 配置周期、计数模式、死区、输出极性、buffer 更新点。
5. 配置安全 idle 状态。
6. 配置 ADC 触发点。
7. 配置 PWM 中断，调用 `section_interrupt()`。

硬件触发链必须用目标 MCU 的官方 demo 和参考手册确认。例如 HC32F558 中，HRPWM 的 ADC 触发不是直接照搬旧平台的 `EVT_SRC_HRPWM_x_SCMP_A`，而是通过 HRPWM AOS event 输出，再由 ADC 硬触发选择对应事件。

## 10. interface 接入

BSP 完成后，需要确认 `code/interface` 能在新平台编译通过。

重点检查：

- `interface/ac/gpio.c` 中继电器逻辑名是否存在
- `interface/ac/adc.h` 中 ADC 宏是否完整
- `interface/ac/pwm.c` 中 PWM API 是否完整
- `interface/ac/comm_link.c` 中串口 TX/RX 是否注册到正确链路
- `HOST_ADDR` 是否符合模块地址约定

当前 HC32F558 默认模块地址为 `0x02`。

## 11. perf 与通信验证

平台接入不是只看能编译，还要看调试链是否可信。

验证顺序：

1. 串口能连接。
2. comm_link 能收发协议帧。
3. perf 字典能查询。
4. perf 任务时间非零且数量正确。
5. perf 计数单位和物理时间一致。
6. 大包发送时 TX DMA 不长时间阻塞任务。
7. `perf_opt_poll_task` 的耗时主要来自数据构造和 ring buffer 写入，而不是等待 DMA 完成。

必要时可以临时给关键路径加 perf record，定位时间分布。验证完成后删除临时 debug record。

## 12. 工程验证

每次平台接入完成后至少执行：

```text
Keil uVision rebuild
```

当前约定使用 ARMCC6。编译结果必须达到：

```text
0 Error(s), 0 Warning(s)
```

如果涉及实际硬件，还需要验证：

- 串口收发
- perf 时间
- GPIO 输出
- PWM 波形
- ADC 触发和采样值
- 中断调度频率

## 13. 提交拆分

新增 MCU 平台的提交按职责拆分：

1. 工程目录骨架
2. 构建系统和 Keil 工程接入
3. BSP 驱动接入
4. 通信和 perf 接入
5. 文档和配置同步
6. 版本号更新

如果同一轮工作里还修改了共享通信、perf 或其他平台 timer 注册，需要单独提交，不和新平台 BSP 混在一起。

## 14. HC32F558 接入经验

HC32F558 本次接入形成了以下经验：

- 先根据官方 demo 建工程，再按仓库已有平台结构整理目录。
- 官方库中用到的 LL 源文件要加入 Keil 工程。
- 时钟要按规格书配置到目标最高主频，并打开 Cache。
- perf 的计数周期要按真实分频注册，不能写死平台宏。
- 串口 TX DMA 应使用 ring buffer，空间足够时装载后立即返回。
- ADC 管脚要和 PWM、USART、晶振避让。
- HRPWM/ADC 触发链要查目标 MCU 官方示例，不能照搬旧平台事件名。
- 临时 debug perf 和 GPIO 翻转只用于验证，验证完要删除。

新增平台时，如果能提供官方 demo、数据手册、参考手册和板级管脚信息，就可以按本文档把平台工程接到和 HC32F558 类似的完成度。
