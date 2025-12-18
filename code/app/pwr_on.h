#ifndef __PWR_ON_H
#define __PWR_ON_H

#include "stdint.h"

typedef enum
{
    PWR_STA_INIT,
    PWR_STA_PV_ON,
    PWR_STA_AC_ON,
    PWR_STA_KEY_ON,
    PWR_STA_OFF,
} PWR_ON_STA_E;

uint8_t pwr_on_get_aux_is_on(void);

#endif
