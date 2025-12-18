#ifndef __FW_INFO_H
#define __FW_INFO_H

#include "stdint.h"

#pragma pack(push, 1)
typedef struct
{
    uint32_t unix_time;    // 4字节
    uint8_t fw_type;       // 1字节 0:ISP,1:IAP
    uint32_t version;      // 4字节
    uint32_t file_size;    // 4字节 - 新增：原始文件大小
    uint8_t commit_id[16]; // 16字节
    uint8_t module_id;     // 1字节
    uint32_t crc32;        // 4字节
} footer_t;
#pragma pack(pop)

// 硬件版本 (高8位)
#define HARD_VER 1 // 硬件版本 1

// 设备商代码 (中8位)
#define DEVICE_VENDOR 2 // 设备商代码

// 发布版本 (低8位中的高4位)
#define RELEASE_VER 0 // 发布版本 2

// 调试版本 (低8位中的低4位)
#define DEBUG_VER 13 // 调试版本 3
// 模块主机类型
#ifdef IS_LLC
#define HOST_ADDR 0x02
#endif
#ifdef IS_PFC
#define HOST_ADDR 0x03
#endif

#define MODULE_HOST HOST_ADDR // 模块主机标识

// 版本信息组合宏
#define COMPOSE_VERSION(hard, vendor, release, debug) \
    (((uint32_t)(hard) << 24) |                       \
     ((uint32_t)(vendor) << 16) |                     \
     ((uint32_t)(release) << 8) |                     \
     ((uint32_t)(debug)))

// 固件类型枚举
typedef enum
{
    FW_TYPE_ISP = 0, // ISP固件
    FW_TYPE_IAP = 1  // IAP固件（可升级）
} firmware_type_t;

// 根据宏定义设置固件类型
#ifdef FIRMWARE_TYPE_IAP
#define DEFAULT_FW_TYPE FW_TYPE_IAP
#elif defined(FIRMWARE_TYPE_ISP)
#define DEFAULT_FW_TYPE FW_TYPE_ISP
#else
#define DEFAULT_FW_TYPE FW_TYPE_IAP // 默认
#endif

#define FW_TYPE DEFAULT_FW_TYPE

#endif
