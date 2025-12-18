#include "key.h"
#include "gpio.h"
#include "section.h"
#include "my_math.h"
#include "stdint.h"

key_table_t key_table[KEY_STA_MAX] = {
    [KEY_STA_SHORT_TO_INV_PRESS] = REG_KEY_STATUS(TIME_CNT_1S_IN_1MS, TIME_CNT_200MS_IN_1MS, 1),
    [KEY_STA_LONG_PRESS] = REG_KEY_STATUS(TIME_CNT_10S_IN_1MS, TIME_CNT_3S_IN_1MS, 0),
    [KEY_STA_LONG_TO_INV_PRESS] = REG_KEY_STATUS(TIME_CNT_10S_IN_1MS, TIME_CNT_3S_IN_1MS, 0),
};

static uint8_t is_press;

uint8_t key_get_status(KEY_STATUS_E key)
{
    uint8_t key_status = key_table[key].flg;
    key_table[key].flg = 0;
    return key_status;
}

static void key_func(key_table_t *p_str)
{
    if (p_str->cooling_dn_cnt)
    {
        if (*p_str->p_is_press == 0)
        {
            p_str->cooling_dn_cnt--;
        }
        return;
    }
    p_str->flg = 0;
    if (*p_str->p_is_press == 1)
    {
        p_str->time_cnt++;
    }
    if ((*p_str->p_is_press == 0) &&
        (p_str->need_release == 1))
    {
        if ((p_str->time_cnt >= p_str->dn_time) &&
            (p_str->time_cnt <= p_str->up_time))
        {
            p_str->cooling_dn_cnt = TIME_CNT_200MS_IN_1MS;
            p_str->flg = 1;
            p_str->time_cnt = 0;
        }
    }
    if (p_str->need_release == 0)
    {
        if (p_str->time_cnt >= p_str->dn_time)
        {
            p_str->cooling_dn_cnt = TIME_CNT_200MS_IN_1MS;
            p_str->flg = 1;
            p_str->time_cnt = 0;
        }
    }
}

static void key_init(void)
{
    for (KEY_STATUS_E i = 0; i < KEY_STA_MAX; i++)
    {
        key_table[i].p_is_press = &is_press;
    }
}

REG_INIT(2, key_init)

static void key_task(void)
{
    is_press = (gpio_get_s_key() == 0) ? 1 : 0;
    for (KEY_STATUS_E i = 0; i < KEY_STA_MAX; i++)
    {
        key_func(&key_table[i]);
    }
}

REG_TASK_MS(1, key_task)
