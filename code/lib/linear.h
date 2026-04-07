#ifndef __LINEAR_H
#define __LINEAR_H

#include "stdint.h"
#include "stdbool.h"

typedef struct
{
    float *p_in;         // 输入变量指针
    float (*p_x_y)[2];   // 二维数组 [][2]，每行是 {x,y}
    uint32_t size;       // 数据点数量
    float out;           // 插值输出
    uint32_t last_index; // 上次区间索引
    uint8_t err;         // 错误标志
} linear_t;

bool linear_init(linear_t *p_str, float *p_in, float (*p_x_y)[2], uint32_t size);

void linear_func(linear_t *p_str);

void linear_func_bin(linear_t *p_str);

#endif
