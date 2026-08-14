# 控制参数发布使用方法

## 1. 应用层职责

控制参数使用 `building` 与 `active` 两份对象。应用层只负责：

1. 在初始化阶段设置 timing，并按模块需要绑定 building 对象。
2. 通过 `*_cfg_set_*()` 修改候选参数。
3. 参数准备完成后调用 `*_fsm_set_cmd(...start)` 请求启动。
4. 通过 `*_fsm_set_cmd(...stop)` 请求停止。

应用层不得调用 `*_cfg_publish_building()`，也不得直接设置 `run_allowed`。这两个接口位于 `*_cfg_fsm.h`，仅供同模块 FSM 使用。

```c
static buck_ctrl_setpoint_t buck_building = {0};

void app_control_cfg_init(void)
{
    buck_cfg_set_p_building(&buck_building);
    buck_cfg_set_out_volt_ref(12.0f);
    buck_cfg_set_in_curr_lmt(20.0f);
}

void app_control_start(void)
{
    buck_fsm_set_cmd(buck_fsm_cmd_start);
}
```

## 2. FSM 发布边界

FSM 收到 start 后先检查配置、HAL 绑定和保护状态。启动条件全部满足时，FSM 设置运行许可并发布完整 building 快照，然后才调用进入运行的 HAL 回调。

停止时序固定为：

```text
stop / hard protect
  → 立即关闭 PWM 或结束功率传输
  → FSM 撤销 run_allowed
  → FSM 发布停止配置
  → 返回 idle 或进入 fault
```

带继电器或预充状态的模块可以在接受 start 时先发布 `run_allowed = 0` 的参数快照，在真正进入 run 前再由 FSM 发布 `run_allowed = 1`。这样参数能够在启动序列中锁存，同时不会提前开放控制输出。

## 3. 参数生效规则

- app 的多次 setter 调用只修改 building，不会逐字段影响 active。
- FSM 接受 start 时发布当时完整的 building。
- run 状态继续修改 building 时，修改内容保留到下一次被 FSM 接受的 start。
- stop 和硬保护不依赖 app 发布，FSM 会统一撤销运行许可。
- 控制入口通过 `*_cfg_sync_building_to_active()` 或 fast sync 消费已发布版本。

## 4. 各模块应用接口

| 模块 | 应用侧配置 | 启动命令 |
|---|---|---|
| Buck | `buck_cfg_set_p_building()`、`buck_cfg_set_*()` | `buck_fsm_set_cmd(buck_fsm_cmd_start)` |
| Boost | `boost_cfg_set_p_building()`、`boost_cfg_set_*()` | `boost_fsm_set_cmd(boost_fsm_cmd_start)` |
| BB | `bb_cfg_set_p_building()`、`bb_cfg_set_*()` | `bb_fsm_set_cmd(bb_fsm_cmd_start)` |
| LLC | `llc_cfg_set_p_building()`、`llc_cfg_set_*()` | `llc_fsm_set_cmd(llc_fsm_cmd_start)` |
| CLLC | `cllc_cfg_set_p_building()`、`cllc_cfg_set_*()` | `cllc_fsm_set_cmd(CLLC_FSM_CMD_START)` |
| PFC | `pfc_cfg_set_p_building()`、`pfc_cfg_set_*()` | `pfc_fsm_set_cmd(pfc_fsm_cmd_start)` |
| INV | `inv_cfg_set_p_building()`、`inv_cfg_set_*()` | `inv_fsm_set_cmd(inv_fsm_cmd_start)` |

调用前应查看目标模块头文件，以该模块实际暴露的字段设置函数为准。

## 5. CLLC 方向参数

CLLC 方向会改变控制结构和能量流向。应用在 idle 状态设置方向并发送 start。FSM 接受 start 时统一发布配置并锁存方向；startup、run 和 fault 期间拒绝方向修改。停止回到 idle 后才能设置并启动另一个方向。

## 6. 参数对象生命周期

传给 `*_cfg_set_p_building()` 的对象由调用方持有，在模块整个运行期间必须保持有效。不得绑定函数栈上的临时对象，也不得在控制运行期间释放或复用这段内存。

## 7. 关联导航

- 源码：[Buck FSM](../../../code/ctrl/buck/buck_fsm.c) · [PFC FSM](../../../code/ctrl/pfc/pfc_fsm.c) · [CLLC FSM](../../../code/ctrl/cllc/cllc_fsm.c)
- 设计：[控制参数构建与发布设计](../../design/control/setpoint_publish_design.md) · [控制模块总设计](../../design/control/ctrl_design.md)
