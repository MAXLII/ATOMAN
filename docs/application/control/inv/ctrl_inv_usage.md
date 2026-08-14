# INV 控制模块使用

通用接入流程见 [ctrl_usage.md](../ctrl_usage.md)，内部设计见 [ctrl_inv_design.md](../../../design/control/inv/ctrl_inv_design.md)。

## 1. 配置

```c
inv_ctrl_timing_t timing = {
    .ctrl_ts = 50.0e-6f,
    .ctrl_freq = 20000.0f,
};
inv_cfg_set_timing(&timing);

inv_cfg_set_freq_hz(50.0f);
inv_cfg_set_freq_slew_hzps(10.0f);
inv_cfg_set_rms_ref_v(230.0f);
inv_cfg_set_rms_slew_vps(212.0f);
```

## 2. HAL 绑定

```c
inv_hal_unlock_binding();
inv_hal_set_v_cap_ptr(&v_inv_cap);
inv_hal_set_i_l_ptr(&i_inv_l);
inv_hal_set_v_bus_ptr(&v_bus);
inv_hal_set_pwm_setter(inv_pwm_set);
inv_hal_set_pwm_enable(inv_pwm_enable);
inv_hal_set_pwm_disable(inv_pwm_disable);
inv_hal_set_inv_rly_on_func(inv_rly_on);
inv_hal_set_inv_rly_off_func(inv_rly_off);
inv_hal_lock_binding();
```

## 3. 启停

```c
if (inv_hal_is_ready() != 0u)
{
    inv_fsm_set_cmd(inv_fsm_cmd_start);
}

inv_fsm_set_cmd(inv_fsm_cmd_stop);
```

## 4. 注意事项

- `inv_ctrl_update_feedback()` 负责整理电压、电流和母线采样。
- INV PWM setter 接收 `v_pwm` 和 `v_bus`。
- 频率和电压参考按配置斜率变化。
- 采样更新需要先于 INV 控制 ISR。
- 应用只设置 building 参数并发送 FSM start/stop 命令；FSM 管理配置发布和运行许可。

## 5. 拓扑移植重点

- 电容电压极性与DQ变换约定保持一致，APF输出作为滞后90°的正交电压。
- `ctrl_ts`、`ctrl_freq`、相位生成、APF、电容电流差分和谐振补偿器必须使用同一时间基准。
- 输出电容、串联电感、额定负载或控制频率变化后，按论文式(8)、式(13)和式(15)重新计算参数。
- 当前电容电流反馈由 `C*Δv/Δt` 得到；目标硬件存在独立电容电流采样时，可将该反馈替换为传感器值。
- 电容电流比例内环带宽为4 kHz，电压外环带宽为1.3 kHz。
- 3、5、7次谐振增益分别为0.30、0.20、0.15，并与输出频率同步更新中心频率。
- PWM setter 的 `v_pwm`、`v_bus` 语义和硬件调制极性保持一致。
- 输出继电器命令与闭合条件由 FSM 管理，不能用命令动作替代硬件反馈。
- 启动相位、频率斜率和RMS电压斜率不能产生初始输出跳变。

## 6. 拓扑验收

1. 在最低和最高输出频率验证APF正交相位、幅值及相位生成连续性。
2. 验证D/Q电压PI、电容电流比例内环的方向、限幅和稳态目标。
3. 验证电容电流差分反馈、输出电压前馈及3、5、7次谐振补偿方向。
4. 验证频率、RMS参考斜率及重新启动过程。
5. 检查完整周期内DQ重构与PWM电压命令连续。
6. stop、继电器断开和保护触发均关闭逆变输出。

## 7. 关联导航

- 源码：[INV 控制](../../../../code/ctrl/inv/inv_ctrl.c) · [INV HAL](../../../../code/ctrl/inv/inv_hal.c) · [INV FSM](../../../../code/ctrl/inv/inv_fsm.c)
- 设计：[INV 控制设计](../../../design/control/inv/ctrl_inv_design.md) · [控制 HAL 挂载与生命周期设计](../../../design/control/hal_binding_lifecycle_design.md)
