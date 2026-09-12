# 三电平 SVPWM 库

源码：`code/lib/svpwm_3level.c/.h`。模块使用 C 标准库和 `platform.h` 的 `FUNC_RAM` 属性，不访问 MCU 寄存器、PWM 驱动或控制器。

`svpwm_3level_cal()` 和较大的内部扇区函数 `calculate_sector()` 使用 `FUNC_RAM`，在支持的平台上进入 `.func_ram` 段；两个短辅助函数使用 `static inline`。目标平台需按现有链接及启动流程将 RAM 段装载到 RAM。该标记不自动迁移数学库函数或只读表，也不表示整个调用链均不访问 Flash。主机 testbench 使用 `PLATFORM_TESTBENCH`，RAM 属性为空。

## 算法与文档的对应

参考 [TI SPRABS6：Center-Aligned SVPWM Realization for 3-Phase 3-Level Inverter](https://www.ti.com/cn/lit/pdf/sprabs6) 的表 3、表 4 和图 9：先按相电压符号选择各相的 O/P 或 N/O 电平对，再做中心对齐的调制。

等压情况下，本实现用等价的 min/max 共模注入计算占空比，省去显式子扇区判定和三角函数；对应虚拟零矢量时间等分。零指令单独输出 OOO。

**不等压扩展属于本库实现，非 TI 文档已验证结论。** 用实测电压构造每相允许的电压区间 `[L_i, U_i]`，解共同偏移 `z`：

```text
z_min = max(L_i - v_i)
z_max = min(U_i - v_i)
z = (z_min + z_max) / 2
O/P: duty_p = (v_i + z) / v_dc_p
N/O: duty_n = -(v_i + z) / v_dc_n
```

公共偏移不会改变 αβ 电压。首选符号扇区不可行时，依固定顺序检查其他电平对组合，最多检查 6 个扇区。此扩展保证平均电压合成，不保证中点电压平衡或最优谐波性能。

## 输入与输出

- `v_alpha/v_beta`：幅值不变 Clarke 坐标的电压指令，单位 V；α 轴沿 A 相，正 β 轴朝 B 相。
- `v_dc_p/v_dc_n`：正母线至中点、中点至负母线的电压正值，单位 V。
- `v_dc_half_min`：每个半母线分别适用的最低有效电压，必须为有限正值。
- 每相 `duty_p/duty_o/duty_n`：完整 PWM 周期中的电平驻留比例，有效时范围为 `[0,1]`，和约等于 1。

允许范围是实际母线对应的六边形：逆 Clarke 三相指令的最大值减最小值不超过 `v_dc_p + v_dc_n`。不裁剪指令、不实现过调制模式。旋转圆形指令若要求整周可实现，幅值应不超过总母线电压除以 `sqrt(3)`；六边形顶点方向的瞬时可实现幅值可以更大。

边界比较允许 `8 * FLT_EPSILON * max(v_dc_p, v_dc_n)` 的电压舍入误差，最终占空比限幅仅用于消除该误差。NaN、Inf、半母线欠压和归一化母线比下溢会报错。构建时不要启用 `-ffast-math` 或 `-ffinite-math-only`，以保留非有限数检查。

## 调用

```c
svpwm_3level_t mod = {0};                    /* 调用方持有的调制实例。 */
const svpwm_3level_cfg_t cfg = {20.0f};      /* 每个半母线最低有效电压，V。 */

if (svpwm_3level_init(&mod, &cfg) == true)
{
    /* 每个 PWM 周期，写入同一次采样对应的完整快照。 */
    mod.input = (svpwm_3level_input_t){300.0f, 100.0f, 350.0f, 350.0f};
    if (svpwm_3level_cal(&mod) == SVPWM_3LEVEL_OK)
    {
        /* 由实际 PWM 适配层消费 mod.output，并同步装载三相结果。 */
    }
    else
    {
        /* 由实际硬件保护路径禁止 PWM；不得继续使用上一周期结果。 */
    }
}
```

初始化失败同样不得使能 PWM。`reset()` 保留配置和输入，清空输出并置 `NOT_READY`；再次计算可恢复有效输出。单个实例不能并发调用，输入快照和输出同步由调用方负责。

## PWM 波形约定

三相共用周期起点，归一化时间为 `t ∈ [0,1)`：

| 电平 | 时间区间 |
|---|---|
| P | `[(1-duty_p)/2, (1+duty_p)/2)` |
| N | `[0,duty_n/2)` 和 `[1-duty_n/2,1)` |
| O | 剩余时间 |

每相一个周期内只使用 O/P 或 N/O；整体序列中心对称、最多 7 段。不能把 P、N 都配置成周期中央的有效脉冲。扇区切换时跨周期的安全换流、比较寄存器同步更新、死区及最小脉宽由驱动负责。

输出不是开关管门极信号。NPC 与 T 型需要各自的门极映射；Vienna 的电流方向约束不在本库范围内。OOO 是中点连接状态，不代表硬件关断。

## 验证与接入范围

在 `platform/testbench/svpwm_3level` 执行 `mingw32-make test`，编译真实库源码并运行独立双精度电压重构、几何矢量驻留时间对照、波形分段积分和异常路径测试。产物位于该目录的 `build/`。

同一命令还会运行 `common/testbench.cpp` 公共 runner。`svpwm_3level_testbench.c` 通过 `TESTBENCH_REGISTER/TESTBENCH_CASE` 注册 DUT 和 4 个场景；按 `before_dut → dut_run → after_dut` 执行，每拍只调用一次真实调制函数。

2026-09-12 主机验证结果：4/4 用例通过，每例 10 kHz 运行 1,000 拍，共 0.1 秒。独立回归同时通过 51,856 个输入点、2,937,471 项检查和 7,155 组等压几何算法对照。

| 注册场景 | 输入变化 | 拒绝拍数 | 最大 αβ 平均电压重构误差 | 周期记录 |
|---|---|---:|---:|---|
| balanced | 350/350 V 母线，300 V、50 Hz 旋转指令 | 0 | 21.30 μV | `build/balanced.csv` |
| unbalanced_step | 第 500 拍从 250/450 V 切换至 450/250 V | 0 | 28.61 μV | `build/unbalanced.csv` |
| range_recovery | 第 200–399 拍指令幅值升至 600 V，随后恢复 | 200 | 21.30 μV | `build/range_recovery.csv` |
| undervoltage_recovery | 第 200–399 拍上半母线降至 10 V，随后恢复 | 200 | 21.30 μV | `build/undervoltage_recovery.csv` |

CSV 包含时间、αβ 给定、半母线电压、状态、三相 P/O/N 占空比及重构电压。错误期间的重构电压和误差记为 NaN，避免将失效输出解释成实际施加的零电压。上述误差是占空比对应的理想平均电压误差，不代表开关纹波或硬件测量精度。

本次未接入平台工程源文件清单或修改 PLECS 模型。接入时需将 `svpwm_3level.c` 加入实际目标，配置 `code/lib`、`code/section` 头文件路径及实际平台宏；需要显式数学库链接的平台应链接 `libm`。主机测试不覆盖目标 ISR 执行时间、实际死区或硬件换流。
