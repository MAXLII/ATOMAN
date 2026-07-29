# 公共组件接入方法

本文给出仓库中新建或移植公共C组件时的文件职责、接口挂载和运行接入方法。目标是让Core可在不同平台和主机测试中复用，而不是为每个模块机械复制同一套目录。

## 1. 先确定变化边界

开始拆分前列出四类变化：

| 变化 | 放置位置 |
| --- | --- |
| 业务规则、状态转换、算法 | Core |
| Section注册、通信命令、周期调用 | Service |
| 平台类型、ID和错误转换 | Adapter |
| 容量、表项、权限和函数绑定 | Cfg |
| 寄存器、DMA、外设时序 | BSP/Driver |

只有存在独立变化方向时才拆层。一个没有转换职责的Adapter或只调用一个函数的空Service不需要单独建立。

## 2. 定义Core上下文

```c
typedef struct
{
    const module_cfg_t *p_cfg;
    module_state_t state;
    module_result_t result;
    uint32_t progress;
} module_t;
```

运行字段放入上下文，常量配置使用`const`指针引用。避免把状态隐藏在文件级全局变量中，除非模块在整个仓库中明确采用单例并且不存在独立测试实例需求。

## 3. 定义最小ops

```c
typedef struct
{
    module_result_t (*p_read)(uint32_t offset,
                              uint32_t length,
                              uint8_t *p_data);
    uint8_t (*p_is_busy)(void);
    module_result_t (*p_result_get)(void);
} module_storage_ops_t;
```

只放Core实际调用的能力。不要因为下层对象具有初始化、配置、统计等接口，就全部向Core暴露。

初始化函数检查所有必需指针：

```c
module_result_t module_ops_init(module_t *p_module,
                                const module_storage_ops_t *p_ops);
```

未挂载时Core保持未初始化或配置错误状态，不应在第一次业务请求到来后才因空指针失败。

## 4. 编写平台Adapter

Adapter可以固定使用平台唯一实例：

```c
static module_result_t module_read(uint32_t offset,
                                   uint32_t length,
                                   uint8_t *p_data)
{
    return result_convert(platform_storage_read(&g_storage,
                                                PLATFORM_ZONE_DATA_E,
                                                offset,
                                                length,
                                                p_data));
}
```

Adapter负责映射和结果转换，但不初始化`g_storage`。公共存储服务由平台独立初始化时，模块只有使用权。

## 5. 建立静态配置

```c
static uint8_t s_work_buffer[MODULE_WORK_SIZE] = {0};

static const module_cfg_t s_cfg = {
    .period_ms = 1UL,
    .p_buffer = s_work_buffer,
    .buffer_size = (uint32_t)sizeof(s_work_buffer),
};
```

结构体使用指定初始化器。运行缓冲区在声明时初始化，并保证生命周期覆盖所有异步操作。

## 6. 接入Service

```c
static module_t s_module = {0};

static void module_service_init(void)
{
    (void)module_ops_init(&s_module, &s_ops);
    (void)module_init(&s_module, &s_cfg);
}

static void module_service_process(void)
{
    module_process(&s_module);
}

REG_INIT(1, module_service_init)
REG_TASK_MS(1u, module_service_process)
```

如果通信命令需要启动模块动作，回调只验证payload并调用提交接口。等待和重试放在`module_process()`中。

## 7. 构建隔离

- 公共Core加入所有需要该能力的目标。
- Service只加入需要Section或对应协议的目标。
- 平台Adapter和Cfg只加入目标平台。
- IAP、Bootloader等不同固件角色通过构建宏或源文件列表隔离。
- 主机测试使用fake ops，不编译平台Driver。

编译成功不能证明边界正确。应检查Core的include列表，确认没有厂商、BSP、平台cfg和产品区域头文件。

## 8. 接入检查

- Core是否只包含规则和状态？
- 状态和缓冲区所有权是否明确？
- ops是否只有Core真正需要的能力？
- Adapter是否承担真实转换？
- 平台是否独立拥有公共基础服务初始化？
- Service回调是否可能阻塞？
- 结果枚举是否区分参数、配置、busy、下层失败和业务拒绝？
- fake ops能否在不包含平台代码的情况下驱动Core？
- 不同固件目标是否只编译各自需要的Service？

## 9. 关联导航

### 源代码

- [FAL Core接口](../../../code/interface/fal/fal_core.h)
- [Bootloader Core接口](../../../code/app/bootloader/core/bootloader_core.h)
- [HC32F334 Bootloader-FAL适配器](../../../platform/hc32f334/bootloader/bootloader_fal_adapter.c)
- [Perf Core](../../../code/dbg/perf.c)
- [Perf Service](../../../code/dbg/perf_service.c)

### 设计文档

- [公共软件组件模型设计](../../design/framework/component_model.md)
- [FAL分区与异步Flash管理设计](../../design/storage/fal_design.md)
- [Bootloader升级与防变砖设计](../../design/bootloader/bootloader_design.md)
