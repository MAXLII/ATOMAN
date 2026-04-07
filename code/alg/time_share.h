#ifndef __TIME_SHARE_H
#define __TIME_SHARE_H

#include "stdint.h"

#define TIME_SHARE_REG_FUNC(_func, _period) { \
    .func = _func,                            \
    .period = _period,                        \
    .cnt = 0,                                 \
}

typedef struct
{
    void (*func)(void);
    uint32_t period;
    uint32_t cnt;
} time_share_func_table_t;

typedef struct
{
    time_share_func_table_t *p_func_table;
    uint32_t table_size;
    uint32_t table_index;
    uint32_t idle_dn_cnt;
} time_share_t;

void time_share_init(time_share_t *p_str,
                     time_share_func_table_t *p_func_table,
                     uint32_t table_size);

void time_share_func(time_share_t *p_str);

#endif
