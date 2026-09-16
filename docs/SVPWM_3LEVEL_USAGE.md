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

公共偏移不会改变 αβ 电压。以下区间组合搜索适用于 `average_balance=false`：关闭中点平衡时，首选符号组合不可行才依固定顺序检查其他电平对组合；启用平衡后比较全部可行组合的中点电流误差，最多检查 6 个组合。当前 NPC 使用 `average_balance=true` 的平均平衡分支，直接在整个可行偏移区间内调节，见下文。两者均不采用显式 36 分区查表。

## 中点电压平衡

输入 `i_a/i_b/i_c`，单位 A，正方向为**桥臂流向交流侧**，采用本拍更新的三相基波电流，不能直接传入载波边界的含开关纹波原始采样。NPC 控制层复用现有 DSOGI，将正序与负序电流相加后逆 Clarke，随电压指令一起交给 PWM 回调；原始采样仍供保护使用。`midpoint_kp` 单位 A/V，设为 0 关闭所选分支的中点平衡；平均分支同时清除积分，保留中心偏移。调用方保证电流为有限值、增益为有限非负值。

传统分支（`average_balance=false`）采用压差比例反馈和可行域内的中点电流分配：

```text
delta_v = v_dc_p - v_dc_n
i_mid_ref = -midpoint_kp * delta_v
i_mid = duty_o_a * i_a + duty_o_b * i_b + duty_o_c * i_c
```

在每个可行电平对组合中，`i_mid` 对 z 是一次函数。计算能逼近目标电流的 z，限制在该组合的可行区间，再比较所有可行组合的电流误差。近似相等时保留先尝试的组合，避免浮点误差引起无意义的组合切换。不会为了平衡而修改目标 αβ 或突破驻留比例边界，也没有积分饱和状态。

输出新增 `midpoint_current_ref`、`midpoint_current`、`common_mode_v`，分别是目标中点电流、按本拍电流预测的平均中点电流和实际选取的 z（V）。`midpoint_current` 是估计值，不是中点传感器测量值。

`sum(duty_o * i)` 是周期内电流近似恒定时的估算。实际中点电荷要积分 `O 状态指示量 × 瞬时电流`；开关纹波、一拍延迟和死区会造成差异。直接用载波边界采样挑选 z，可能把纹波引起的电流偏差当成可利用的充放电能力，反而增大母线压差。参见[本次开关模型复现与修正记录](design/npc_midpoint_ripple_fix.md)。

NPC 自动 CSV 末尾增加 `np_kp/np_i_ref/np_i_est/np_offset` 和 `ia_fundamental/ib_fundamental/ic_fundamental`，与原始 `ia/ib/ic` 分别记录，便于验证调制实际使用的电流。

当前 NPC 选择平均平衡分支（`average_balance=true`）。它将母线压差和三相基波电流绝对值之和作 10 Hz 低通，压差 PI 经有方向的中点调节灵敏度换算为额外共同偏移；额外偏移限幅 ±150 V，并受实际可行区间约束。调节能力不足时暂停积分，偏移限幅时作抗饱和回算。每相仍只使用 O/P 或 N/O。

当前默认 `midpoint_kp=0.06×2π×2≈0.75398 A/V`，积分增益为 `midpoint_ki=midpoint_kp×2π×0.5≈2.3687 A/(V·s)`。60 mF 是增益设计基准，用户当前模型 C4/C5 为 66/60 mF；公式不能解释为保证的实际闭环带宽。复用配置函数为 `pwm_make_modulator_cfg()`；库的平均分支还需设置 `ts`、`midpoint_filter_hz`、`midpoint_ki`、`midpoint_kaw`、`midpoint_current_min`、`midpoint_slope_floor_ratio` 和 `midpoint_offset_max`，不能只打开布尔开关而省略配置。

NPC 保持 6 路占空比加使能。接口层对理想 P/N 占空比施加与模型 **2 µs** 一致的死区补偿；轻载暂停补偿，门极死区仍由 PLECS 生成。库输出的 P/O/N 是补偿前理想驻留比例，不能与补偿后门极指令混用来验证理想电压重构。

| Shell 参数 | 含义 |
|---|---|
| `NP_BAL_KP` | 平衡增益，默认约 0.75398 A/V；0 同时关闭比例和积分 |
| `NP_I_REF` | 目标中点电流，A |
| `NP_I_EST` | 预测中点电流，A |
| `NP_OFFSET` | 所选共同偏移 z，V |
| `NP_DELTA_AVG` | 低通后的母线压差，V |
| `NP_CORRECTION` | 相对中心偏移的额外校正，V |
| `NP_CURRENT_MAG` | 三相基波电流绝对值之和的低通值，A |

增益变更在下一次 5 kHz 更新重新配置调制器。停波、reset 会清空调制诊断和平均分支积分。传统分支零参考输出 OOO；平均分支零参考仍可能有共同偏移，不能假定必为 OOO。零电流、区间收缩或可用中点电流不足时，不能保证压差立即消除。输入电流方向必须正确。正式 2 µs DLL 的三负载实际 PLECS 结果和验证边界见[修正记录第 12 节](design/npc_midpoint_ripple_fix.md#12-用户确认-2-µs-后的正式交付)。

## 输入与输出

- `v_alpha/v_beta`：幅值不变 Clarke 坐标的电压指令，单位 V；α 轴沿 A 相，正 β 轴朝 B 相。
- `v_dc_p/v_dc_n`：正母线至中点、中点至负母线的电压正值，单位 V。
- `v_dc_half_min`：每个半母线分别适用的最低有效电压，必须为有限正值。
- 每相 `duty_p/duty_o/duty_n`：完整 PWM 周期中的电平驻留比例，有效时范围为 `[0,1]`，和约等于 1。

允许范围是实际母线对应的六边形：逆 Clarke 三相指令的最大值减最小值不超过 `v_dc_p + v_dc_n`。超范围时按可用母线与所需三相电压跨度之比等比例缩小指令，保持 αβ 方向，不进入过调制。旋转圆形指令若要求整周可实现，幅值应不超过总母线电压除以 `sqrt(3)`；六边形顶点方向的瞬时可实现幅值可以更大。

边界比较允许浮点舍入误差，最终占空比限制在 `[0,1]`。库不再检查空指针、配置、NaN/Inf 或欠压，也不返回状态；调用方保证对象及配置有效、输入有限且两侧母线严格为正，并在应用保护层处理欠压。

## 调用

```c
svpwm_3level_t mod = {0};                    /* 调用方持有的调制实例。 */
const svpwm_3level_cfg_t cfg = {            /* 基准调用示例，关闭中点平衡。 */
    .v_dc_half_min = 20.0f,
    .midpoint_kp = 0.0f
};

svpwm_3level_init(&mod, &cfg);
/* 应用先完成输入与保护判定，再写入同拍快照。 */
mod.input = (svpwm_3level_input_t){
    .v_alpha = 300.0f, .v_beta = 100.0f,
    .v_dc_p = 350.0f, .v_dc_n = 350.0f,
    .i_a = 0.0f, .i_b = 0.0f, .i_c = 0.0f
};
svpwm_3level_cal(&mod);
/* 由实际 PWM 适配层消费 mod.output，并同步装载三相结果。 */
```

`init()`、`cal()`、`reset()` 均返回 `void`，输出不再含状态字段。`reset()` 保留配置和输入，清空输出与平衡历史；停机必须另行禁止硬件 PWM。单个实例不能并发调用，输入快照和输出同步由调用方负责。

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

初版验证时尚未接入平台工程源文件清单。移植到其他目标时，需将 `svpwm_3level.c` 加入实际目标，配置 `code/lib`、`code/section` 头文件路径及实际平台宏；需要显式数学库链接的平台应链接 `libm`。主机测试不覆盖目标 ISR 执行时间、实际死区或硬件换流。当前 NPC 接入及新增平衡验证见下节。

### 2026-09-13 中点平衡增量验证

以下是传统分支的历史验证，不能当作新增平均分支的测试结果。`mingw32-make test` 执行 `test_midpoint_balance.c`：720 个角度/正反电流工况与独立的 513 点偏移扫描比较，检查电压重构、占空比约束和中点电流；用双精度电容电荷守恒模型运行 5 个闭环工况，每个 3 s、5 kHz。

60 mF 等值电容、固定 1330 V 总母线下，初始 ±100 V 压差在 ±100 A 正反功率方向，以及 3000 A 工况中，最后一个工频周期的最大压差均小于 0.001 V。此数值是理想平均模型结果，不是硬件精度或 PLECS 开关仿真的承诺。新测试共 1,666,602 项检查通过；原有 2,940,069 项检查和 4 个 runner 工况也通过。

原有极端母线比、零参考用例已与此前删除下溢检查的行为对齐：此时返回有效 OOO，不再期待已删除分支的错误码。新结构体尾部增加了字段，源码调用需重新编译；测试初始化已改为指定成员，避免遗漏字段产生警告。

## 关联导航

- [从物理原理到源码实现](design/svpwm_3level_walkthrough.md)
- [中点平衡测试](../platform/testbench/svpwm_3level/test_midpoint_balance.c)
- [Imperix TN129：中点电压平衡与共同偏移](https://imperix.com/doc/implementation/balancing-of-npc-converters)介绍了利用冗余状态改变中点充放电、并保持平均线电压的原理。本文采用基于三相电流的可行区间求解，并非直接照搬其增益或控制公式。
