#pragma once
#include <stdint.h>

#define PIT_BASE_FREQ 1193182

extern volatile uint64_t tick_count;

void     timer_init(uint32_t freq_hz);
uint64_t timer_get_ticks(void);
uint64_t timer_get_uptime_ms(void);
void     timer_sleep_ms(uint32_t ms);