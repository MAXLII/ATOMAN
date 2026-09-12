# NPC PLECS 5 kHz 占空比输出

## 分层与数据流

```text
PLECS 每 200 μs 调用 DLL
  → platform/plecs/npc/app/app.c：采样输入、调度、保持输出
  → code/interface/npc/common/pwm.c：调用 SVPWM，将 P/O/N 解成每桥臂 2 路占空比
  → platform/plecs/npc/bsp/bsp_pwm.c：发布 6 路占空比和整桥使能
  → PLECS 后级 PWM 调制模块：产生 12 路互补开关波形并插入死区
```

interface 负责 NPC 门极占空比计算；BSP 负责把完整占空比帧和使能写到 PLECS 输出口。DLL 内不生成载波、不比较脉冲边沿、不输出逐点门极电平。

目录保持 `code/interface/npc/common/`、`platform/plecs/npc/bsp/`、`platform/plecs/npc/app/` 分层。NPC 构建只选择本拓扑的 interface 和 BSP，复用 `common/DllHeader.h` ABI；不同时链接 `common/plecs.c`。

## 5 kHz 运行周期

固定控制周期宏为 `PLECS_NPC_CONTROL_PERIOD_S = 0.0002`，即 **200 μs / 5 kHz**。

**PLECS DLL 模块的 Sample time 必须设为 `2e-4 s`。** 当前 DLL ABI 无采样时间配置字段，因此代码不能替 PLECS 设置模块的调用周期；必须同时配置模型。也允许使用 200 μs 的精确整数分频采样时间，DLL 会在中间回调中保持输出。

第一次有效输出回调建立更新时基，此后每 200 μs 内部生成一次 αβ 指令，并采样母线电压、Shell 配置和使能。更新时间由“起点 + 整数更新序号 × 200 μs”计算，避免反复累加浮点周期造成漂移。重复时间戳不重复计算；周期间所有输出（包括使能）保持上一帧。时间比较使用 1 ns 基础容差，并按绝对时间的 double 精度增加舍入余量。

如果宿主未在更新时刻调用 DLL，例如使用 1 ms 采样导致漏掉 200 μs 更新，DLL 关闭使能并设置仿真错误，不静默降低控制频率。仿真时间倒退或非有限时间也报错；需要重新开始仿真恢复。

控制更新率与后级载波频率分别配置。DLL 的 5 kHz 是占空比指令更新率，后级按自己的 PWM 载波进行调制，并在规定边界锁存指令。

## DLL 端口

产物：`platform/plecs/npc/build/bin/plecs_npc.dll`（Windows x64）。

**8 个输入、7 个输出、0 个参数、0 个离散状态端口。** DLL 输入包含上下半母线电压、三相输出电压和三相电感电流；αβ 在内部生成，幅值、频率、使能和欠压阈值由 Shell 配置。接入时输入向量为 8 路，输出向量为 7 路。

| 输入序号（从 1 起） | 信号 |
|---:|---|
| 1 | `PLECS_INPUT_V_DC_P`：正母线到中点的电压正值，V |
| 2 | `PLECS_INPUT_V_DC_N`：中点到负母线的电压正值，V |
| 3 | `PLECS_INPUT_V_OUT_A`：A 相输出电压，相对输出中性点，V |
| 4 | `PLECS_INPUT_V_OUT_B`：B 相输出电压，相对输出中性点，V |
| 5 | `PLECS_INPUT_V_OUT_C`：C 相输出电压，相对输出中性点，V |
| 6 | `PLECS_INPUT_I_L_A`：A 相电感电流，桥臂流向输出为正，A |
| 7 | `PLECS_INPUT_I_L_B`：B 相电感电流，桥臂流向输出为正，A |
| 8 | `PLECS_INPUT_I_L_C`：C 相电感电流，桥臂流向输出为正，A |

输出电压和电感电流为瞬时反馈量，每 200 μs 采样并保持，关闭 PWM 时仍采样。当前接通反馈监视，尚未加入电压环或电流环；不会用测量值替换开环 αβ 指令。任一反馈为 NaN、Inf 或超出 float 可表示范围时，PWM 在更新点关闭，对应监视值为 NaN；反馈恢复有效后按使能恢复调制。此次不修改 `.plecs`，模型侧需自行按上述顺序连接实际测量信号。

### Shell 配置

`app/app.c` 使用现有 `REG_SHELL_VAR` 保留以下变量：

| 名称 | 默认值 | 范围 | 含义 |
|---|---:|---|---|
| `V_ALPHA` | 0 | 只读 float | 最近一次 5 kHz 更新生成的 α 实际瞬时值，V |
| `V_BETA` | 0 | 只读 float | 最近一次 5 kHz 更新生成的 β 实际瞬时值，V |
| `V_OUT_A` / `V_OUT_B` / `V_OUT_C` | 0 | 只读 float | 最近一次采样的三相输出电压，V |
| `I_L_A` / `I_L_B` / `I_L_C` | 0 | 只读 float | 最近一次采样的三相电感电流，A |
| `V_ALPHA_AMP` | 0 | 0～1000000 | α 轴峰值幅值，V |
| `V_BETA_AMP` | 0 | 0～1000000 | β 轴峰值幅值，V |
| `FREQ_HZ` | 50 | 0～1000 | αβ 共用电角频率，Hz；0 保持相位 |
| `RUN_ENABLE` | 0 | 0～1 | 运行使能；1 允许调制 |
| `V_DC_HALF_MIN` | 20 | 0.001～1000000 | 每半母线最低有效电压，V |

每次开始仿真恢复以上默认值，相位从 0 开始，输出默认关闭。内部按 `v_alpha = V_ALPHA_AMP × cos(θ)`、`v_beta = V_BETA_AMP × sin(θ)` 生成，首次更新使用 θ=0，再按 `2π × FREQ_HZ × 200 μs` 累加共用相位并周期回绕。幅值相等时为圆形旋转矢量，不相等时为椭圆。

Shell 的 `V_ALPHA`、`V_BETA` 监视实际生成指令，不是功率级测得的输出电压，不能写入覆盖。关闭使能时发生器仍运行并更新监视值。可写配置在下一次 5 kHz 更新生效；改变频率保持相位连续，改变幅值可能产生指令跳变；欠压阈值改变时重新配置调制器。实际可实现指令由当前母线电压决定，超范围指令关闭输出。

已接入 `common/dbg/shell`、Section 注册和公共 FRAME TCP 服务；仿真开始后监听 TCP 5000，并开启公共 UDP 发现。通过 FRAME 的参数列表读写变量；该 TCP 端口使用 FRAME 协议封包，不是裸文本终端。Shell 文本命令通过相应协议服务发送，例如：

```text
V_ALPHA_AMP:300
V_BETA_AMP:300
FREQ_HZ:50
RUN_ENABLE:1
V_ALPHA
V_BETA
```

上面的命令配置两轴峰值 300 V、频率 50 Hz，开启调制后读取实际生成值。停止命令为 `RUN_ENABLE:0`。文本命令按行结束。DLL 回调与协议处理共用 dispatch 锁，避免 Shell 访问和控制采样并发。日志输出到宿主标准错误流。

| 输出序号（从 1 起） | 信号 |
|---:|---|
| 1 | `PLECS_OUTPUT_A_POSITIVE_DUTY`：A 相 P 占空比 |
| 2 | `PLECS_OUTPUT_A_NEGATIVE_DUTY`：A 相 N 占空比 |
| 3 | `PLECS_OUTPUT_B_POSITIVE_DUTY`：B 相 P 占空比 |
| 4 | `PLECS_OUTPUT_B_NEGATIVE_DUTY`：B 相 N 占空比 |
| 5 | `PLECS_OUTPUT_C_POSITIVE_DUTY`：C 相 P 占空比 |
| 6 | `PLECS_OUTPUT_C_NEGATIVE_DUTY`：C 相 N 占空比 |
| 7 | `PLECS_OUTPUT_PWM_ENABLE`：整桥使能；0 要求后级禁止全部门极 |

Signal Selector 的 Input width 改为 `7`；A/B/C 相分别取 `[1 2]`、`[3 4]`、`[5 6]`，使能取 `7`。

占空比是高电平有效的导通时间比例，范围为 `[0,1]`，并非比较寄存器值或当前时刻的 0/1 门极信号。

## P/O/N 到每桥臂 2 路占空比

每相管号从正母线到负母线依次为 Q1、Q2、Q3、Q4。P 状态导通 Q1/Q2，O 状态导通 Q2/Q3，N 状态导通 Q3/Q4：

| BSP 字段 | 占空比 | PLECS 中的用途 |
|---|---|---|
| `positive_duty` | P | 产生 Q1 主波形，互补波形用于 Q3 |
| `negative_duty` | N | 产生 Q4 主波形，互补波形用于 Q2 |

命名对应桥臂状态：`positive_duty` 表示正电平 P 的占空比，`negative_duty` 表示负电平 N 的占空比；两者数值均为 `[0,1]`，负占空比不是负数。`bsp_pwm_phase_duty_t` 表达一个桥臂的这两个字段；interface 按 A/B/C 顺序传入 3 个结构体。BSP 验证两路有限且在 `[0,1]` 内、`positive_duty + negative_duty ≤ 1` 后统一发布并使能，O 的占空比为 `1−P−N`。初始化、关闭或错误状态输出 6 个零占空比和使能 0。BSP 不生成互补波形，也不处理死区。

零电压指令对应有效 OOO，各相输出占空比对为 `0,0`，PLECS 生成的理想四管状态为 `0,1,1,0`，整桥使能仍为 1。关闭状态的占空比也为零，但整桥使能为 0。

后级在 `.plecs` 中按同一同步载波调制：Q1 使用正占空比 P，脉冲位于周期中央；Q4 使用负占空比 N，脉冲分布在周期两端。两路不能都采用同极性的中央脉冲，否则 P/N 区间会重叠。Q3 由 Q1 互补生成，Q2 由 Q4 互补生成；两组分别在 PLECS 中插入死区并处理最小脉宽。输出使能为 0 时，后级必须覆盖互补逻辑并禁止全部门极。原先接到 Q2 调制器的第二路输出现在是 Q4 的 N 占空比，模型需要同步修改，不能沿用旧的调制接法。

## 调用接口

- `pwm_init(v_dc_half_min)`：初始化，输出保持关闭。
- `pwm_update(&input)`：一次完整 SVPWM 计算、6 路占空比转换和 BSP 发布。
- `pwm_disable()`：关闭输出并使调制结果失效。
- `bsp_pwm_set_duty(duty)`：接收 A/B/C 三相的 `bsp_pwm_phase_duty_t` 数组，验证后输出并使能。
- `bsp_pwm_disable()`：清空占空比、撤销使能。

已移除 `pwm_output(phase)`、脉冲窗口结构和 BSP 载波比较接口。无效电压、欠压、超范围指令或输入使能撤销会在下一次 5 kHz 更新关闭输出；本 DLL 不承担异步硬件保护。

## 构建与验证

一键脚本为 `platform/plecs/npc/compile.bat`，自动定位工程目录，使用 `C:\mingw64\bin` 下的 x64 GCC 和 MinGW Make。默认增量编译 Release DLL；传入 `test` 同时编译并运行测试。CMake/CTest 需在 PATH 中，失败返回非零退出码。

```powershell
.\platform\plecs\npc\compile.bat
.\platform\plecs\npc\compile.bat test
```

仓库根目录执行：

```powershell
cmake -S platform/plecs/npc -B platform/plecs/npc/build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe -DCMAKE_MAKE_PROGRAM=C:/mingw64/bin/mingw32-make.exe -DCMAKE_BUILD_TYPE=Release
cmake --build platform/plecs/npc/build -j 4
ctest --test-dir platform/plecs/npc/build --output-on-failure
```

测试与 DLL 共用同一套 `npc_core` 编译对象，运行 2,000 个调制更新点及 60 秒长时间调度回归。通过真实 Shell 解析器设置独立幅值、频率、使能和阈值，检查内部正交生成、变频相位连续、只读监视不可覆盖、关闭使能时发生器继续运行及重启相位复位。同时检查 6 路占空比关系、由 P/N 重构 αβ 电压、8/7 端口尺寸、周期间保持、故障及恢复，以及 BSP 字段发布顺序、P+N 超限、NaN、越界和空指针处理。反馈测试覆盖 6 路电压/电流的有符号采样、只读注册、关闭时监视、采样保持及无效输入恢复。使用 `compile.bat test` 验证；未进行 FRAME GUI 连接验收。

当前仅支持每个 DLL 模块一套 NPC 桥状态，不支持同一 DLL 内多个独立实例并发。原有 `npc.plecs` 和自动保存文件未改动，尚未在 PLECS GUI 中设置采样时间和连接后级调制模块；主机测试不等同于实际开关波形或硬件换流验证。
