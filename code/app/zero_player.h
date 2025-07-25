#ifndef __ZERO_PLAYER_H
#define __ZERO_PLAYER_H

#include "section.h"

#define ROWS 37
#define COLS 41

void zero_player_init(const int init[ROWS][COLS]);
void zero_player_step(void);

void zero_player_add(DEC_MY_PRINTF);

#endif
