#ifndef __ALARM_H__
#define __ALARM_H__

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ALARM_FAULT_MPPT_IN_OVP = 0,
    ALARM_FAULT_MPPT_L_HARD_OCP,
    ALARM_FAULT_MPPT_L_SOFT_OCP,
    ALARM_FAULT_AC_L_OCP,
    ALARM_FAULT_AC_OUT_OCP,
    ALARM_FAULT_AC_OUT_ACTOPP,
    ALARM_FAULT_AC_OUT_TTLOPP,
    ALARM_FAULT_ALIVE_IS_LOSS,
    ALARM_FAULT_BUS_GRID_SS_ERR,
    ALARM_FAULT_BUS_PFC_SS_ERR,
    ALARM_FAULT_BUS_LLC_SS_ERR,
    ALARM_FAULT_SAMPLE_ERR,
    ALARM_FAULT_BUS_ERR,
    ALARM_FAULT_MAX,
} alarm_fault_e;

typedef enum
{
    ALARM_WARN_MAX = 0,
} alarm_warn_e;

void alarm_fault_set(alarm_fault_e alarm);
void alarm_fault_clr(alarm_fault_e alarm);
void alarm_fault_clr_all(void);
uint8_t alarm_fault_get(alarm_fault_e alarm);
uint32_t alarm_fault_get_all(void);
uint32_t alarm_fault_get_record_all(void);
void alarm_fault_record_clr_all(void);
uint8_t alarm_fault_check(void);

void alarm_warn_set(alarm_warn_e alarm);
void alarm_warn_clr(alarm_warn_e alarm);
void alarm_warn_clr_all(void);
uint8_t alarm_warn_get(alarm_warn_e alarm);
uint32_t alarm_warn_get_all(void);
uint32_t alarm_warn_get_record_all(void);
void alarm_warn_record_clr_all(void);
uint8_t alarm_warn_check(void);

#endif
