# Bootloader升级接入与运行

本文说明如何把公共Bootloader Core、协议层和IAP触发服务加入一个固件工程，以及运行时如何完成首次升级和IAP内二次升级。新平台的完整驱动、链接和布局工作见Bootloader平台移植文档。

## 1. 两个固件角色

同一产品保留两个构建目标：

| 目标 | 包含内容 | 外部升级命令 |
| --- | --- | --- |
| Bootloader/ISP | Bootloader Core、元数据、完整升级协议、FAL适配器 | `0x08`～`0x0B` |
| IAP | 正常应用、独立IAP升级触发服务 | 仅`0x08` |

IAP不编译完整Bootloader协议和安装状态机。Bootloader也不需要编译IAP内的准备回调服务。

## 2. Bootloader目标接入

加入公共源码：

```text
code/app/bootloader/core/bootloader_core.c
code/app/bootloader/core/bootloader_metadata.c
code/app/bootloader/protocol/bootloader_protocol.c
code/interface/fal/fal_core.c
code/comm/comm.c
```

平台加入：

```text
fal_cfg.c/h
bootloader_fal_adapter.c
Bootloader平台操作实现
Flash驱动
专用通信Link
启动与链接文件
```

FAL由平台服务初始化和周期推进。Bootloader-FAL适配器只映射逻辑区域、转换结果并挂载`bootloader_flash_ops_t`。

## 3. 配置Core资源

Bootloader配置需要：

- 目标模块ID；
- 默认直接或暂存模式；
- 平台镜像首部读取长度；
- 1024字节数据包缓冲区；
- 暂存复制和完整校验共用的块缓冲区；
- CRC16初始化和增量更新函数。

```c
static uint8_t s_packet_buffer[BOOTLOADER_PACKET_DATA_SIZE] = {0};
static uint8_t s_copy_buffer[FLASH_ERASE_BLOCK_SIZE] = {0};

static const bootloader_config_t s_config = {
    .expected_module_id = DEVICE_ADDR,
    .default_mode = BOOTLOADER_UPGRADE_MODE_STAGED_E,
    .image_header_length = 8UL,
    .p_packet_buffer = s_packet_buffer,
    .packet_buffer_size = (uint32_t)sizeof(s_packet_buffer),
    .p_copy_buffer = s_copy_buffer,
    .copy_buffer_size = (uint32_t)sizeof(s_copy_buffer),
    .p_crc16_init = firmware_crc_init,
    .p_crc16_update = firmware_crc_update,
};
```

复制缓冲区必须覆盖平台允许的单次安装块。所有缓冲区具有静态生命周期，因为Core会跨多个周期保存指针和进度。

## 4. 挂载Flash操作

平台适配器实现：

```c
static const bootloader_flash_ops_t s_flash_ops = {
    .p_zone_info_get = bootloader_flash_zone_info_get,
    .p_read = bootloader_flash_read,
    .p_write = bootloader_flash_write,
    .p_erase = bootloader_flash_erase,
    .p_is_busy = bootloader_flash_is_busy,
    .p_result_get = bootloader_flash_result_get,
};
```

区域映射使用显式`switch`：

```text
BOOTLOADER_FLASH_ZONE_IAP_E     → 平台IAP FAL zone
BOOTLOADER_FLASH_ZONE_STAGING_E → 平台暂存 FAL zone
BOOTLOADER_FLASH_ZONE_META_A_E  → 平台元数据A FAL zone
BOOTLOADER_FLASH_ZONE_META_B_E  → 平台元数据B FAL zone
BOOTLOADER_FLASH_ZONE_LAYOUT_E  → 平台布局 FAL zone
```

不要提供Bootloader自身FAL zone的映射分支。

## 5. 初始化顺序

```text
Flash驱动和FAL cfg成立
  → 平台FAL初始化
  → Bootloader适配器挂载flash ops
  → Bootloader挂载platform ops
  → Bootloader Core初始化
  → 协议对象初始化和挂载
  → 开放FRAME链路
```

初始化后，1ms或其他稳定周期同时推进：

```text
fal_process()
bootloader_process()
通信Link任务
看门狗维护
```

Core的等待状态依赖FAL继续运行。只调用`bootloader_process()`而没有推进FAL会使Flash请求永久保持busy。

## 6. IAP目标接入

IAP构建只加入：

```text
code/app/bootloader/iap/iap_update_service.c
code/app/bootloader/common/bootloader_update_request.h
code/app/bootloader/protocol/bootloader_protocol_types.h
```

构建脚本通过`IS_IAP`选择IAP源码、中断向量和链接起点。保留请求段`.noinit.boot_request`必须位于复位后不被初始化清零的SRAM区域。

## 7. 实现准备回调

应用覆盖弱定义函数：

```c
iap_update_prepare_result_t
iap_update_prepare(const iap_update_info_t *p_info)
{
    if (p_info == NULL)
    {
        return IAP_UPDATE_PREPARE_FAILED_E;
    }

    power_stage_disable();

    if (parameter_save_failed() != 0u)
    {
        return IAP_UPDATE_PREPARE_FAILED_E;
    }
    if (parameter_save_busy() != 0u)
    {
        return IAP_UPDATE_PREPARE_IN_PROGRESS_E;
    }

    return IAP_UPDATE_PREPARE_READY_E;
}
```

该函数由周期process重复调用：

- 首次调用可以发起封波、日志落盘或参数保存；
- 未完成返回`IN_PROGRESS`；
- 确认失败返回`FAILED`，不会复位；
- 所有准备动作完成后返回`READY`。

不要在回调中阻塞等待Flash或发送完成。

## 8. 首次升级

空IAP时设备应保持Bootloader通信可用：

1. FRAME发送`0x08`升级信息。
2. 轮询`0x09`直到Bootloader完成目标区域擦除。
3. 按offset顺序发送`0x0A`数据包。
4. 每包等待同命令ACK。
5. 发送`0x0B`及整包CRC。
6. Bootloader完成暂存安装或直接目标校验。
7. 元数据提交为有效后跳转IAP。

主机超时重发时只能重发上一offset。不能跳过尚未确认的数据包。

## 9. IAP内二次升级

1. FRAME向IAP发送`0x08`。
2. IAP直接返回接收ACK。
3. IAP周期执行用户准备回调。
4. 准备完成后写入保留SRAM请求并复位。
5. Bootloader读取请求并驻留。
6. FRAME重新建立Bootloader升级会话，继续`0x09`～`0x0B`。

IAP的`0x08` ACK只表示升级请求已接受，不表示已经复位或固件已经写入。

## 10. 运行异常处理

- `0x08`被拒绝：检查模块ID、文件大小、升级模式和区域容量。
- `0x09`长期未就绪：检查FAL是否周期推进及Flash状态函数是否退出busy。
- `0x0A`失败：检查offset、数据长度、包CRC和上一个写请求是否完成。
- 复位后仍进入IAP：检查保留SRAM段、magic/checksum和Bootloader启动原因读取。
- 暂存下载完成但不跳转：检查复制读回、完整CRC、footer及平台镜像首部。
- 失败后设备驻留Bootloader属于安全行为，应重新发送完整镜像，不要强制跳转。

## 11. 关联导航

### 源代码

- [Bootloader Core接口](../../../code/app/bootloader/core/bootloader_core.h)
- [升级元数据接口](../../../code/app/bootloader/core/bootloader_metadata.h)
- [FRAME升级协议接口](../../../code/app/bootloader/protocol/bootloader_protocol.h)
- [IAP升级触发服务](../../../code/app/bootloader/iap/iap_update_service.c)
- [HC32F334适配参考](../../../platform/hc32f334/bootloader/bootloader_fal_adapter.c)
- [Zynq-7020适配参考](../../../platform/zynq7020/ps/bootloader/bootloader_fal_adapter.c)

### 设计文档

- [Bootloader升级与防变砖设计](../../design/bootloader/bootloader_design.md)
- [FAL分区与异步Flash管理设计](../../design/storage/fal_design.md)
- [公共软件组件模型设计](../../design/framework/component_model.md)
