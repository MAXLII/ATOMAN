# PFC 控制模块使用

通用接入流程见 [ctrl_usage.md](../ctrl_usage.md)，内部设计见 [ctrl_pfc_design.md](../../../design/control/pfc/ctrl_pfc_design.md)。

## 1. 配置

```c
pfc_ctrl_timing_t timing = {
    .ctrl_ts = 50.0e-6f,
};
pfc_cfg_set_timing(&timing);

pfc_cfg_set_vbus_ref_v(400.0f);
pfc_cfg_set_vbus_slew_vps(200.0f);
pfc_cfg_set_run_allowed(0u);
pfc_cfg_publish_building();
```

## 2. HAL 绑定

```c
pfc_hal_unlock_binding();
pfc_hal_set_v_g_ptr(&v_grid);
pfc_hal_set_v_rms_ptr(&v_grid_rms);
pfc_hal_set_i_l_ptr(&i_pfc_l);
pfc_hal_set_v_cap_ptr(&v_pfc_cap);
pfc_hal_set_v_bus_ptr(&v_bus);
pfc_hal_set_main_rly_is_closed_ptr(&main_rly_closed);
pfc_hal_set_vbus_sta_ptr(&vbus_sta);
pfc_hal_set_pwm_setter(pfc_pwm_set);
pfc_hal_set_pwm_enable(pfc_pwm_enable);
pfc_hal_set_pwm_disable(pfc_pwm_disable);
pfc_hal_set_main_rly_on_func(main_rly_on);
pfc_hal_set_main_rly_off_func(main_rly_off);
pfc_hal_lock_binding();
```

## 3. 启停

```c
if (pfc_hal_is_ready() != 0u)
{
    pfc_fsm_set_cmd(pfc_fsm_cmd_start);
}

pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
```

## 4. 注意事项

- `pfc_ctrl_update_feedback()` 负责整理采样方向。
- PFC PWM setter 接收 `v_pwm` 和 `v_bus`。
- 主继电器闭合后 PFC 快速环路才进入运行门控。
- 母线参考和斜率使用 V、V/s。

## 5. 拓扑移植重点

- 电网电压、电感电流和 RMS 信号来自同一相位与极性约定。
- 实际控制周期必须与 SOGI、FLL、notch、PR 和电压环系数一致。
- 母线电容、交流侧电感和电容变化后重新核对环路带宽和限幅。
- 母线状态阈值、预充时间和主继电器反馈符合目标硬件时序。
- PWM setter 的 `v_pwm`、`v_bus` 含义与调制实现一致。
- 主继电器未闭合、run 许可撤销或保护触发时快速控制路径保持关闭。

## 6. 拓扑验收

1. 验证 FSM 按 Idle、Soft Start、Main Relay、Run 顺序运行。
2. 验证 SOGI/FLL 锁定方向、频率范围和失网恢复。
3. 验证母线电压环、电感电流环及 notch 的作用方向。
4. 校验 RMS、母线状态和继电器反馈的边界条件。
5. stop、失网和保护触发均使 PWM 与继电器进入安全状态。

## 7. 关联导航

- 源码：[PFC 控制](../../../../code/ctrl/pfc/pfc_ctrl.c) · [PFC HAL](../../../../code/ctrl/pfc/pfc_hal.c) · [PFC FSM](../../../../code/ctrl/pfc/pfc_fsm.c)
- 设计：[PFC 控制设计](../../../design/control/pfc/ctrl_pfc_design.md) · [控制 HAL 挂载与生命周期设计](../../../design/control/hal_binding_lifecycle_design.md)
