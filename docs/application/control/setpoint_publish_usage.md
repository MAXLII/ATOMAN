# 控制参数发布使用方法

## 1. 适用场景

控制参数发布接口用于把后台任务、通信命令或状态机产生的一组新参数，作为一个版本交给实时控制路径。当前 Buck、Boost、LLC、CLLC 和 PFC 配置模块均采用 `building` 与 `active` 两份参数对象。

## 2. 基本接入顺序

1. 在模块初始化阶段准备调用方持有的 `building` 对象。
2. 调用对应的 `*_cfg_set_p_building()` 绑定对象。
3. 后台只通过 `*_cfg_set_*()` 修改 `building`。
4. 一组相关字段写完后调用 `*_cfg_publish_building()`。
5. 控制中断在一次控制计算开始前调用 `*_cfg_sync_building_to_active()`。
6. 控制算法在本周期内只读取 `*_cfg_get_p_active()` 返回的对象。

```c
static buck_ctrl_setpoint_t s_buck_building = {0};

void app_control_cfg_init(void)
{
    buck_cfg_set_p_building(&s_buck_building);
    buck_cfg_set_out_volt_ref(12.0f);
    buck_cfg_set_in_curr_lmt(20.0f);
    buck_cfg_set_run_allowed(0U);
    buck_cfg_publish_building();
}

void app_control_isr(void)
{
    buck_cfg_sync_building_to_active_fast();
    /* 本周期控制计算只使用 active 参数。 */
}
```

## 3. 发布边界

多个有关联的字段必须连续写入 `building` 后再发布，不能每修改一个字段就发布一次。发布函数表达“这一组参数已经完整”，而不是单个赋值动作。

当前实现要求同一个配置对象只有一个后台写入者，且发布与实时同步不会并发复制同一对象。若一个平台存在多个写入者，应先在应用层串行化，不能依靠 `volatile` 解决一致性。

## 4. 各模块接口

| 模块 | 后台绑定与发布 | 实时同步 |
|---|---|---|
| Buck | `buck_cfg_set_p_building()`、`buck_cfg_publish_building()` | `buck_cfg_sync_building_to_active()` / `buck_cfg_sync_building_to_active_fast()` |
| Boost | `boost_cfg_set_p_building()`、`boost_cfg_publish_building()` | `boost_cfg_sync_building_to_active()` / 快速同步接口 |
| LLC | `llc_cfg_set_p_building()`、`llc_cfg_publish_building()` | `llc_cfg_sync_building_to_active()` |
| CLLC | `cllc_cfg_set_p_building()`、`cllc_cfg_publish_building()` | `cllc_cfg_sync_building_to_active()` |
| PFC | `pfc_cfg_set_p_building()`、`pfc_cfg_publish_building()` | `pfc_cfg_sync_building_to_active()` |

调用前应查看目标模块头文件，以该模块实际暴露的字段设置函数为准。

## 5. CLLC 方向参数

CLLC 的方向会改变控制结构和能量流向。应用应在空闲状态调用 `cllc_cfg_unlock_direction()`，设置方向并发布；离开空闲状态前调用 `cllc_cfg_lock_direction()`。运行期间不得用普通参数更新路径改变方向。

## 6. 参数对象生命周期

传给 `*_cfg_set_p_building()` 的对象由调用方持有，在模块整个运行期间必须保持有效。不得绑定函数栈上的临时对象，也不得在控制运行期间释放或复用这段内存。

## 7. 关联导航

- 源码：[Buck 配置](../../../code/ctrl/buck/buck_cfg.c) · [PFC 配置](../../../code/ctrl/pfc/pfc_cfg.c) · [CLLC 配置](../../../code/ctrl/cllc/cllc_cfg.c)
- 设计：[控制参数构建与发布设计](../../design/control/setpoint_publish_design.md) · [控制模块总设计](../../design/control/ctrl_design.md)
