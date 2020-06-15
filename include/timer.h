// timer.h -- Defines the interface for all PIT-related functions.
//            Written for JamesM's kernel development tutorials.

#ifndef TIMER_H
#define TIMER_H

#define TICKPERSECOND 50
#include "common.h"

void init_timer(uint32_t frequency);
int get_timer_ticks();
#endif
