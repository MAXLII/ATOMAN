# CTRL 控制模块使用总览

## 1. 接入顺序

控制模块的通用接入顺序：

1. 设置模块 timing。
2. 设置模块 setpoint。
3. 调用 `*_cfg_publish_building()` 发布配置。
4. 解锁 HAL 绑定。
5. 绑定采样变量、PWM 回调、保护回调和状态机回调。
6. 锁定 HAL 绑定。
7. 调用 `section_init()`。
8. 主循环持续调用 `run_task()`。
9. 控制 ISR 中调用 `section_interrupt()`。
10. 通过模块 FSM 下发 start/stop 命令。

## 2. 调度要求

平台需要保证控制 ISR 调用 `section_interrupt()` 前，相关采样变量已经更新。

主循环需要持续调用 `run_task()`，让 `REG_TASK()`、`REG_TASK_MS()` 和 `REG_FSM()` 注册对象运行。

## 3. HAL 绑定

HAL 绑定只在模块 idle 阶段更新。绑定流程固定为：

```c
xxx_hal_unlock_binding();
/* xxx_hal_set_*() */
xxx_hal_lock_binding();
```

启动前通过 `xxx_hal_is_ready()` 检查绑定完整性。

## 4. 运行控制

模块启动通过 FSM 命令完成：

```c
xxx_fsm_set_cmd(xxx_fsm_cmd_start);
```

模块停止通过：

```c
xxx_fsm_set_cmd(xxx_fsm_cmd_stop);
```

保护触发时调用模块 HAL 的 hard protect 接口：

```c
xxx_hal_hard_protect_trip();
```

## 5. 模块文档

| 模块 | 使用文档 |
| --- | --- |
| PFC | [CTRL_PFC_USAGE.md](CTRL_PFC_USAGE.md) |
| INV | [CTRL_INV_USAGE.md](CTRL_INV_USAGE.md) |
| BB | [CTRL_BB_USAGE.md](CTRL_BB_USAGE.md) |
| Buck | [CTRL_BUCK_USAGE.md](CTRL_BUCK_USAGE.md) |
| Boost | [CTRL_BOOST_USAGE.md](CTRL_BOOST_USAGE.md) |
