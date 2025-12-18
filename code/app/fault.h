#ifndef __FAULT_H
#define __FAULT_H

#include "stdint.h"
#include "assert.h"
#include "stdbool.h"

typedef enum
{
    FAULT_STA_MPPT_IN_OVP,
    FAUTL_STA_MPPT_L_HARD_OCP,
    FAULT_STA_MPPT_L_SOFT_OCP,
    FAULT_STA_AC_L_OCP,
    FAULT_STA_AC_OUT_OCP,
    FAULT_STA_AC_OUT_ACTOPP,
    FAULT_STA_AC_OUT_TTLOPP,
    FAULT_STA_ALIVE_IS_LOSS,
    FAULT_STA_BUS_GRID_SS_ERR,
    FAULT_STA_BUS_PFC_SS_ERR,
    FAULT_STA_BUS_LLC_SS_ERR,
    FAULT_STA_SAMPLE_ERR,
    FAULT_STA_BUS_ERR,
    FAULT_STA_MAX,
} FAULT_STA_E;

void fault_set_bit(FAULT_STA_E sta);

void fault_clr_bit(FAULT_STA_E sta);

uint8_t fault_get_bit(FAULT_STA_E sta);

uint32_t fault_get_all(void);

uint8_t fault_check(void);

#endif
