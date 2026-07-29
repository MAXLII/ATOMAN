# FAL平台配置与上层接入

本文说明新平台如何提供Flash设备表和区域表，以及参数管理、日志或升级模块如何使用公共FAL状态机。

## 1. 接入文件

公共代码直接复用：

```text
code/interface/fal/fal_core.c
code/interface/fal/fal_core.h
```

平台准备：

```text
platform/<platform>/.../fal_cfg.c
platform/<platform>/.../fal_cfg.h
```

`fal_cfg`包含平台Flash枚举、区域枚举、设备和区域静态表、驱动函数绑定、共享FAL实例。底层Flash驱动仍放在平台BSP中。

## 2. 定义设备和区域ID

```c
typedef enum
{
    FAL_DEVICE_INTERNAL_E = 1U,
    FAL_DEVICE_EXTERNAL_E
} platform_fal_device_id_t;

typedef enum
{
    FAL_ZONE_BOOT_E = 1U,
    FAL_ZONE_APP_E,
    FAL_ZONE_PARAMETER_E,
    FAL_ZONE_LOG_E
} platform_fal_zone_id_t;
```

ID应在平台cfg中定义，FAL Core不包含业务区域枚举。所有设备的zone ID必须全局唯一。

## 3. 按物理设备建立区域表

```c
#define INTERNAL_BLOCK_SIZE (4UL * 1024UL)
#define EXTERNAL_BLOCK_SIZE (4UL * 1024UL)

static const fal_zone_cfg_t s_internal_zones[] = {
    {
        .zone_id = FAL_ZONE_BOOT_E,
        .size = 4UL * INTERNAL_BLOCK_SIZE,
        .permissions = FAL_ZONE_PERMISSION_READ,
    },
    {
        .zone_id = FAL_ZONE_APP_E,
        .size = 28UL * INTERNAL_BLOCK_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
};

static const fal_zone_cfg_t s_external_zones[] = {
    {
        .zone_id = FAL_ZONE_PARAMETER_E,
        .size = 2UL * EXTERNAL_BLOCK_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
    {
        .zone_id = FAL_ZONE_LOG_E,
        .size = 16UL * EXTERNAL_BLOCK_SIZE,
        .permissions = FAL_ZONE_PERMISSION_ALL,
    },
};
```

同一设备的区域按真实地址从低到高排列。FAL使用前序区域大小累加起点，因此不要给表预留没有区域描述的地址空洞。需要空洞时，应把它定义成具有明确权限的区域。

区域大小和累计起点必须满足设备erase block对齐。容量表达优先使用“块数 × 基本块大小”，使布局边界可以直接审查。

## 4. 适配Flash操作

```c
static fal_result_t external_program(void *p_context,
                                     uint32_t address,
                                     uint32_t length,
                                     const uint8_t *p_data)
{
    (void)p_context;
    return (driver_page_program(address, length, p_data) == DRIVER_OK)
               ? FAL_RESULT_SUCCESS
               : FAL_RESULT_DRIVER_ERROR;
}
```

所有操作接收物理Flash地址，不接收FAL区域ID。适配函数负责：

- 把驱动结果转换成`fal_result_t`；
- 把驱动busy/error转换成`fal_device_state_t`；
- 拒绝底层不支持的长度或对齐；
- 不在函数内部阻塞等待整个擦写过程。

若驱动的读取是同步完成的，可以在`p_read`返回成功后让`p_get_state`保持ready。若写入或擦除由DMA/外设状态推进，`p_get_state`在完成前返回busy。

## 5. 建立设备表

```c
static const fal_device_cfg_t s_devices[] = {
    {
        .device_id = FAL_DEVICE_INTERNAL_E,
        .capacity = INTERNAL_FLASH_CAPACITY,
        .program_page_size = INTERNAL_PROGRAM_SIZE,
        .erase_block_size = INTERNAL_BLOCK_SIZE,
        .max_read_size = 1024UL,
        .p_zones = s_internal_zones,
        .zone_count = (uint16_t)(sizeof(s_internal_zones) /
                                sizeof(s_internal_zones[0])),
        .ops = {
            .p_context = NULL,
            .p_init = internal_init,
            .p_get_state = internal_state_get,
            .p_read = internal_read,
            .p_program = internal_program,
            .p_erase = internal_erase,
            .p_sync = NULL,
        },
    },
    {
        .device_id = FAL_DEVICE_EXTERNAL_E,
        .capacity = EXTERNAL_FLASH_CAPACITY,
        .program_page_size = EXTERNAL_PROGRAM_PAGE_SIZE,
        .erase_block_size = EXTERNAL_BLOCK_SIZE,
        .max_read_size = 1024UL,
        .p_zones = s_external_zones,
        .zone_count = (uint16_t)(sizeof(s_external_zones) /
                                sizeof(s_external_zones[0])),
        .ops = {
            .p_context = NULL,
            .p_init = external_init,
            .p_get_state = external_state_get,
            .p_read = external_read,
            .p_program = external_program,
            .p_erase = external_erase,
            .p_sync = NULL,
        },
    },
};

const fal_cfg_t g_platform_fal_cfg = {
    .p_devices = s_devices,
    .device_count = (uint16_t)(sizeof(s_devices) / sizeof(s_devices[0])),
};

fal_t g_platform_fal = {0};
```

program page描述单次编程不能跨越的页边界；erase block描述最小擦除单位；`max_read_size == 0`表示FAL不限制单次读取。

## 6. 初始化和周期推进

平台拥有FAL初始化和调度：

```c
static void fal_service_init(void)
{
    (void)fal_init(&g_platform_fal, &g_platform_fal_cfg);
}

static void fal_service_process(void)
{
    fal_process(&g_platform_fal);
}

REG_INIT(0, fal_service_init)
REG_TASK_MS(1u, fal_service_process)
```

上层模块只有使用权，不应重复调用`fal_init()`或替平台推进同一个共享实例。若未来拆分独立`fal_service.c`，配置对象和运行服务可以分文件，但FAL所有权仍归平台。

## 7. 上层提交请求

```c
static uint8_t s_read_buffer[64] = {0};

fal_result_t result = fal_read(&g_platform_fal,
                               FAL_ZONE_PARAMETER_E,
                               0UL,
                               sizeof(s_read_buffer),
                               s_read_buffer);

if (result == FAL_RESULT_IN_PROGRESS)
{
    /* 等待周期任务推进。 */
}
```

完成条件：

```c
if (fal_is_busy(&g_platform_fal) == 0u)
{
    result = fal_result_get(&g_platform_fal);
}
```

读写缓冲区必须保持到`fal_is_busy()`返回0。忙时不要复用缓冲区，也不要提交另一个请求覆盖当前事务。

## 8. 擦除范围

```c
(void)fal_erase(&g_platform_fal,
                FAL_ZONE_LOG_E,
                record_offset,
                record_length);
```

FAL会把范围扩展到完整擦除块。调用者必须确认同一擦除块内没有需要保留的其他数据。多个独立小对象若共享一个擦除块，上层需要采用读改写、日志结构或双区事务，不能把FAL当作字节级可擦存储。

## 9. 接入检查

- 每个设备的独立读、页编程、块擦除和状态查询已验证。
- 区域表顺序与链接/持久化布局一致。
- Bootloader等保护区只有读权限。
- 区域总容量不超过物理设备容量。
- 页大小、块大小和最大读取长度来自真实器件。
- 驱动busy期间FAL周期任务不会重复发起同一操作。
- 上层正确处理busy、权限错误、越界和驱动错误。
- 零长度请求和区域首尾边界行为符合预期。

## 10. 关联导航

### 源代码

- [FAL公共接口](../../../code/interface/fal/fal_core.h)
- [FAL状态机实现](../../../code/interface/fal/fal_core.c)
- [HC32F334配置参考](../../../platform/hc32f334/bootloader/fal_cfg.c)
- [Zynq-7020配置参考](../../../platform/zynq7020/ps/bootloader/fal_cfg.c)

### 设计文档

- [FAL分区与异步Flash管理设计](../../design/storage/fal_design.md)
- [公共软件组件模型设计](../../design/framework/component_model.md)
