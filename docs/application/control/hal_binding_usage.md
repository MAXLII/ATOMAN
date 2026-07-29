# 控制 HAL 平台挂载方法

## 1. 接入目标

平台通过目标拓扑的 `*_hal_set_*()` 接口，把实时采样、PWM、继电器和保护状态挂载给公共控制模块。挂载只建立依赖关系，不初始化 ADC、PWM 或保护外设；底层硬件必须先进入安全状态。

## 2. 接入前准备

先列出目标拓扑头文件要求的所有绑定，并逐项确定：

| 项目 | 需要确认的内容 |
|---|---|
| 测量指针 | 数据类型、单位、极性、更新时间和生命周期 |
| PWM setter | 输入域、通道对应、限幅和执行时间 |
| PWM enable | 初始比较值是否安全、启用哪些桥臂 |
| PWM disable | 是否立即关闭全部相关输出、能否重复调用 |
| FSM 状态 | 继电器反馈、母线状态和保护锁存的有效值 |
| timing | 实际控制周期、任务周期和 PWM 周期 |

平台变量应使用静态或全局存储期。不得把局部变量地址挂入 HAL。

## 3. 推荐初始化顺序

```text
初始化时钟、ADC、PWM和保护
  → 保持PWM关闭
  → 设置控制timing
  → 准备building参数并publish
  → 等待FSM进入Idle
  → unlock HAL
  → 设置全部指针和回调
  → is_ready检查
  → ready后lock
  → 允许应用提交start
```

若 ready 失败，保持功率级关闭，在 Idle 中补齐挂载。不要为了通过检查而绑定无意义的临时变量或空操作回调。

## 4. Buck 挂载示例

```c
static int32_t s_v_in_code = 0;
static int32_t s_i_in_code = 0;
static int32_t s_v_out_code = 0;
static int32_t s_i_out_code = 0;
static int32_t s_i_l_code[BUCK_CTRL_IND_CURR_CH_NUM] = {0};
static const buck_pwm_setter_t s_pwm_setters[] = {
    platform_pwm_ch0_set,
    platform_pwm_ch1_set,
};

_Static_assert((sizeof(s_pwm_setters) / sizeof(s_pwm_setters[0])) ==
                   BUCK_CTRL_IND_CURR_CH_NUM,
               "PWM setter count mismatch");

static uint8_t platform_bind_buck_hal(void)
{
    uint32_t ch = 0U;

    buck_hal_unlock_binding();
    buck_hal_set_v_in_ptr(&s_v_in_code);
    buck_hal_set_i_in_ptr(&s_i_in_code);
    buck_hal_set_v_out_ptr(&s_v_out_code);
    buck_hal_set_i_out_ptr(&s_i_out_code);
    buck_hal_set_pwm_disable(platform_pwm_disable);

    for (ch = 0U; ch < BUCK_CTRL_IND_CURR_CH_NUM; ch++)
    {
        buck_hal_set_i_l_ptr(ch, &s_i_l_code[ch]);
        buck_hal_set_pwm_setter(ch, s_pwm_setters[ch]);
    }

    if (buck_hal_is_ready() == 0U)
    {
        platform_pwm_disable();
        return 0U;
    }

    buck_hal_lock_binding();
    return 1U;
}
```

示例按当前默认的两个 Buck 通道列出回调。通道数量变化时，静态断言会要求平台同步修改映射。控制 ISR 通过已经冻结的 HAL 数组直接调用回调，不在运行时查找硬件。

## 5. 采样更新

HAL 保存的是实时变量地址，不负责触发采样。平台应在控制计算之前更新这些变量：

```c
static void platform_feedback_isr(void)
{
    s_v_in_code = bsp_adc_get_v_in_code();
    s_v_out_code = bsp_adc_get_v_out_code();
    s_i_l_code[0] = bsp_adc_get_i_l0_code();
    s_i_l_code[1] = bsp_adc_get_i_l1_code();
}

REG_INTERRUPT(0, platform_feedback_isr)
```

采样注册优先级必须早于控制 ISR。多通道数据若来自不同 ADC 或 DMA 批次，平台需要先形成同一采样时刻的快照。

## 6. PWM 回调

PWM setter 应只做确定时长的数值限幅和寄存器更新。不得在回调中：

- 等待外设状态；
- 打印日志或发送通信；
- 动态分配内存；
- 修改控制 setpoint；
- 执行与当前功率拓扑无关的业务状态机。

`p_pwm_disable` 是最重要的安全回调。它应直接关闭当前拓扑的全部相关 PWM 输出，并允许 stop、初始化失败和保护中断重复调用。

## 7. FSM 资源

PFC、INV 等拓扑还需要继电器与状态绑定。继电器命令回调只负责发出硬件动作，FSM 根据独立反馈决定动作是否完成。不能用“已经调用 on 回调”替代“继电器已经闭合”的反馈。

CLLC 的 enable 与 modulation setter 带方向参数。平台必须把正向和反向映射到明确桥臂，并保证 disable 同时关闭两种方向。

## 8. 保护接入

硬件比较器、PWM brake 或驱动器保护应先独立形成快速关断链。软件保护入口用于同步控制状态：

```c
void platform_power_fault_isr(void)
{
    platform_pwm_disable();
    buck_hal_hard_protect_trip();
}
```

如果 `buck_hal_hard_protect_trip()` 已调用同一个 disable，额外的直接调用必须仍然安全。故障源未消失时不得调用 clear，也不得自动提交 start。

## 9. 启动和停止

应用只在以下条件均满足后提交 start：

- timing 和 setpoint 已准备；
- HAL ready；
- FSM 位于 Idle；
- 硬件保护和软件锁存均未激活；
- 平台电源条件满足目标拓扑要求。

停止由 FSM 统一推进，平台不要一边直接启停 PWM、一边让 FSM 维持 Run 状态。紧急故障除外：先关 PWM，再让软件状态收敛到 Fault 或 Idle。

## 10. 常见问题定位

| 现象 | 检查项 |
|---|---|
| setter 调用后仍未 ready | 是否仍处于 locked、是否缺少通道或回调 |
| start 被拒绝 | HAL、cfg、setpoint、保护锁存和 FSM 状态 |
| 输出通道错位 | 通道索引、极性、桥臂和 PWM setter 映射 |
| 启动瞬间跳变 | 进入运行前是否准备控制状态和安全比较值 |
| stop 后仍有脉冲 | disable 是否覆盖全部输出及互补通道 |
| 重绑后数据混杂 | 是否在同一轮完整覆盖旧绑定并重新 ready |

## 11. 移植验收

1. 故意缺失每一类绑定，确认 start 均被拒绝。
2. 锁定后尝试 setter，确认活动绑定不变。
3. 用固定输入检查采样量纲、极性和通道。
4. 低压条件下验证 enable、setter、stop 的顺序。
5. 分别触发软件 stop 与硬件 fault，确认 PWM 安全关闭。
6. 保持故障源有效，确认 clear/start 不能绕过保护。
7. 测量控制 ISR 与所有回调的最坏执行时间。

## 12. 关联导航

- 源码：[Buck HAL](../../../code/ctrl/buck/buck_hal.c) · [PFC HAL](../../../code/ctrl/pfc/pfc_hal.c) · [CLLC HAL](../../../code/ctrl/cllc/cllc_hal.c)
- 设计：[控制 HAL 挂载与生命周期设计](../../design/control/hal_binding_lifecycle_design.md) · [控制模块总设计](../../design/control/ctrl_design.md)
