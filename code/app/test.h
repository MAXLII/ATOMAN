#ifndef __TEST_H
#define __TEST_H

#include "stdint.h"

#pragma pack(push, 1) // 确保结构体按1字节对齐
typedef struct
{
    float fp32;   ///< 测试32位浮点数
    uint8_t u8;   ///< 测试无符号8位整数
    uint16_t u16; ///< 测试无符号16位整数
    int16_t i16;  ///< 测试有符号16位整数
    int32_t i32;  ///< 测试有符号32位整数
    int8_t i8;    ///< 测试有符号8位整数
    uint32_t u32; ///< 测试无符号32位整数
} test_value_t;

#pragma pack(pop) // 恢复默认对齐

#endif
