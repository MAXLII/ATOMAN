# Buck 控制模块使用

通用接入流程见 [ctrl_usage.md](../ctrl_usage.md)，内部设计见 [ctrl_buck_design.md](../../../design/control/buck/ctrl_buck_design.md)。

## 1. 配置

```c
(void)buck_cfg_set_out_volt_ref(12.0f);
(void)buck_cfg_set_in_volt_lmt(24.0f);
(void)buck_cfg_set_pwr_lmt(1000.0f);
(void)buck_cfg_set_in_curr_lmt(50.0f);
(void)buck_cfg_set_out_curr_lmt(50.0f);
(void)buck_cfg_set_run_request(1u);
```

## 2. HAL 绑定

```c
buck_hal_unlock_binding();
buck_hal_set_v_in_ptr(&v_in_code);
buck_hal_set_v_out_ptr(&v_out_code);
buck_hal_set_i_l_ptr(0u, &i_l0_code);
buck_hal_set_i_l_ptr(1u, &i_l1_code);
buck_hal_set_pwm_setter(0u, buck_pwm0_set);
buck_hal_set_pwm_setter(1u, buck_pwm1_set);
buck_hal_set_pwm_disable(buck_pwm_disable);
buck_hal_lock_binding();
```

## 3. 启停

```c
buck_cfg_set_run_request(1u);
buck_cfg_set_run_request(0u);

buck_run_sta_e run_state = buck_cfg_get_run_state();
```

## 4. 注意事项

- `buck_cfg_t` 只保存输出电压参考、输入电压限制、功率限制、输入/输出电流限制和启停请求。
- `ctrl_ts`、`task_ts` 和 `pwm_cmp_max` 是控制模块的编译期参数，分别来自 `CTRL_TS`、Buck任务周期和 `CTRL_PWM_CMP_MAX`。
- 每个 `buck_cfg_set_*()` 只修改一个应用参数；setter 返回 `0` 表示输入无效。
- HAL 采样指针类型为 `int32_t *`。
- 电感电流通道号小于 `BUCK_CTRL_IND_CURR_CH_NUM`。
- PWM setter 输出 compare 和上下管使能。
- FSM在init状态逐一检查全部HAL数据指针和函数回调；每项失败都会输出对应的`PLECS_LOG`并停留在init。
- init检查通过后，Buck其他路径不再重复执行空指针检查。
- 应用只包含 `buck_cfg.h` 和 `buck_hal.h`，通过 cfg 设置参数与启停请求。
- FSM 持续消费启停请求，独占发布函数并决定完整控制设定值的生效时刻。
- 应用 setter 只修改 building 配置，不直接修改 ISR 使用的 active 配置。
- CTRL 只读FSM提交的 published 快照，不读取应用正在修改的参数。

## 5. 关联导航

- 源码：[Buck 控制](../../../../code/ctrl/buck/buck_ctrl.c) · [Buck HAL](../../../../code/ctrl/buck/buck_hal.c) · [Buck FSM](../../../../code/ctrl/buck/buck_fsm.c)
- 设计：[Buck 控制设计](../../../design/control/buck/ctrl_buck_design.md) · [Buck 整数控制设计](../../../design/control/buck/buck_integer_control_design.md) · [控制 HAL 挂载与生命周期设计](../../../design/control/hal_binding_lifecycle_design.md)
