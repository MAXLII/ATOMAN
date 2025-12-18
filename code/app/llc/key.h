#ifndef __KEY_H
#define __KEY_H

#include "stdint.h"
#include "my_math.h"

#define REG_KEY_STATUS(_up_time, _dn_time, _need_release) { \
    .up_time = _up_time,                                    \
    .dn_time = _dn_time,                                    \
    .flg = 0,                                               \
    .need_release = _need_release,                          \
}

typedef enum
{
    KEY_STA_SHORT_TO_INV_PRESS,
    KEY_STA_LONG_PRESS,
    KEY_STA_LONG_TO_INV_PRESS,
    KEY_STA_MAX,
} KEY_STATUS_E;

typedef struct
{
    uint8_t *p_is_press;
    uint32_t time_cnt;
    uint32_t up_time;
    uint32_t dn_time;
    uint8_t flg;
    uint8_t need_release;
    uint32_t cooling_dn_cnt;
} key_table_t;

uint8_t key_get_status(KEY_STATUS_E key);

#endif
