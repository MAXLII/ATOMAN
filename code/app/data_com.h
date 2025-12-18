#ifndef __DATA_COM_H
#define __DATA_COM_H

#include "stdint.h"
#include "device_info.h"
#include "my_math.h"

typedef struct
{
    uint8_t *p_alive;
    uint32_t dn_cnt;
    uint32_t reload;
} data_com_alive_t;

#define DATA_COM_REG_ALIVE(name, _reload, _p_alive) \
    data_com_alive_t data_com_alive_##name = {      \
        .reload = _reload,                          \
        .p_alive = &_p_alive,                       \
    };

#define DATA_COM_ALIVE_RELOAD(name) \
    data_com_alive_##name.dn_cnt = data_com_alive_##name.reload;

#define DATA_COM_ALIVE_DN_CNT(name)                                                \
    do                                                                             \
    {                                                                              \
        DN_CNT(data_com_alive_##name.dn_cnt);                                      \
        if (data_com_alive_##name.p_alive)                                         \
        {                                                                          \
            *data_com_alive_##name.p_alive = data_com_alive_##name.dn_cnt ? 1 : 0; \
        }                                                                          \
    } while (0)

#pragma pack(1)

#define CMD_SET_PFC_TO_LLC_INFO 0x03
#define CMD_WORD_PFC_TO_LLC_INFO 0x01

typedef struct
{
    uint8_t bus_is_ok; // 母线电压OK
    uint8_t ac_is_ok;  // 有市电输入
    uint8_t is_dsg;    // 正在放电
} pfc_to_llc_info_t;

#define CMD_SET_LLC_TO_PFC_INFO 0x02
#define CMD_WORD_LLC_TO_PFC_INFO 0x01

typedef struct
{
    float v_bat;       // 电池电压
    uint8_t aux_is_on; // 辅源开启
} llc_to_pfc_info_t;

typedef struct
{
    uint8_t allow_ac_chg; // 允许AC充电
    uint8_t allow_ac_dsg; // 允许AC放电
    uint8_t allow_pv_chg; // 允许PV充电
} bms_to_llc_info_t;

typedef struct
{
    uint8_t pv_is_ok; // 有PV输入
    uint8_t ac_is_ok; // 有市电输入
} llc_to_bms_info_t;

#pragma pack()

uint8_t data_com_get_llc_is_alive(void);

float data_com_get_v_bat(void);

uint8_t data_com_get_bus_is_ok(void);

uint8_t data_com_get_ac_is_ok(void);

uint8_t data_com_get_pfc_is_dsg(void);

uint8_t data_com_get_pfc_is_alive(void);

#endif
