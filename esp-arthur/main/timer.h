#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "driver/timer.h"

void stop_timer();

void reset_timer_counter();

void restart_timer();

void init_timer(timer_isr_t callback, uint64_t tempo);

#endif // TIMER_H