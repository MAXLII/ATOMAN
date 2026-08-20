# 控制参数发布使用方法

## 1. 应用层职责

控制参数由配置层保存。应用层只负责：

1. 按模块需要绑定参数对象和 HAL。
2. Buck 通过字段 setter 更新统一配置；其他模块通过各自 setter 修改候选参数。
3. 通过模块对外提供的启停接口设置运行请求。

除 Buck 外，应用层不得调用 `*_cfg_publish_building()`，也不得直接设置 `run_allowed`；这些接口位于对应模块的 `*_cfg_fsm.h`，仅供同模块 FSM 使用。Buck 的发布函数只存在于 `buck_fsm.c` 内部，cfg仅保存参数与启停请求，应用层和其他模块无法调用发布操作。

```c
void app_control_cfg_init(void)
{
    (void)buck_cfg_set_out_volt_ref(12.0f);
    (void)buck_cfg_set_in_volt_lmt(24.0f);
    (void)buck_cfg_set_pwr_lmt(1000.0f);
    (void)buck_cfg_set_in_curr_lmt(20.0f);
    (void)buck_cfg_set_out_curr_lmt(20.0f);
}

void app_control_start(void)
{
    buck_cfg_set_run_request(1U);
}

void app_control_stop(void)
{
    buck_cfg_set_run_request(0U);
}
```

## 2. FSM 发布边界

FSM 收到 start 后先检查配置、HAL 绑定和保护状态。启动条件全部满足时，Buck FSM 发布完整 building 配置，其他模块 FSM 发布完整 building 快照，然后才调用进入运行的 HAL 回调。

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

- Buck 每个字段 setter 只修改 building 中的对应参数；其他模块的多次 setter 调用也只修改 building。
- FSM 接受 start 时，Buck 和其他模块都发布当时完整的 building。
- Buck 在run状态的FSM执行点发布新快照；其他模块的修改内容保留到下一次被FSM接受的start。
- stop 和硬保护不依赖 app 发布，FSM 会统一撤销运行许可。
- Buck控制入口只读FSM独占提交的published快照；其他模块通过各自的sync接口消费已发布版本。

## 4. 各模块应用接口

| 模块 | 应用侧配置 | 启动命令 |
|---|---|---|
| Buck | `buck_cfg_set_*()` | `buck_cfg_set_run_request(1U)` |
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
