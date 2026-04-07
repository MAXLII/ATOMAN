#ifndef VOLT_MT_H
#define VOLT_MT_H

#include <stdint.h>

// 电压监测状态枚举
typedef enum
{
    ST_INVALID = 0,  // 电压无效
    ST_VALID = 1,    // 电压有效
    ST_ENTERING = 2, // 正在进入有效范围
    ST_EXITING = 3   // 正在退出有效范围
} volt_st_t;

// 配置参数结构体
typedef struct
{
    float min_vld;      // 有效电压下限 (V)
    float max_vld;      // 有效电压上限 (V)
    float min_exit;     // 退出有效范围下限 (V)
    float max_exit;     // 退出有效范围上限 (V)
    float min_inst;     // 立即退出下限 (V)
    float max_inst;     // 立即退出上限 (V)
    uint32_t enter_dly; // 进入有效范围延时 (ms)
    uint32_t exit_dly;  // 退出有效范围延时 (ms)
} volt_cfg_t;

// 输入参数结构体
typedef struct
{
    const float *volt; // 输入电压指针
    uint32_t time;     // 当前时间 (ms)
} volt_in_t;

// 输出参数结构体
typedef struct
{
    volt_st_t st;     // 当前状态
    uint8_t is_vld;      // 当前是否有效
    uint32_t vld_dur; // 有效状态持续时间 (ms)
    uint32_t st_dur;  // 当前状态持续时间 (ms)
} volt_out_t;

// 中间变量结构体
typedef struct
{
    uint32_t st_start; // 状态开始时间 (ms)
    uint32_t last_upd; // 最后更新时间 (ms)
    float last_volt;   // 上次电压值
} volt_int_t;

// 电压监测主结构体
typedef struct
{
    volt_cfg_t cfg;   // 配置参数
    volt_in_t in;     // 输入参数
    volt_out_t out;   // 输出参数
    volt_int_t inter; // 中间变量
} volt_mt_t;

// 函数声明 - 统一以volt_mt开头
void volt_mt_init(volt_mt_t *mt, const volt_cfg_t *cfg, const float *volt_ptr);
void volt_mt_upd(volt_mt_t *mt, uint32_t curr_time);
void volt_mt_set_ptr(volt_mt_t *mt, const float *volt_ptr);

// 获取输出信息的快捷函数 - 也统一以volt_mt开头
static inline volt_st_t volt_mt_st(const volt_mt_t *mt)
{
    return mt ? mt->out.st : ST_INVALID;
}

static inline uint8_t volt_mt_is_vld(const volt_mt_t *mt)
{
    return mt ? mt->out.is_vld : 0;
}

static inline uint32_t volt_mt_vld_dur(const volt_mt_t *mt)
{
    return mt ? mt->out.vld_dur : 0;
}

static inline uint32_t volt_mt_st_dur(const volt_mt_t *mt)
{
    return mt ? mt->out.st_dur : 0;
}

#endif // VOLT_MT_H
