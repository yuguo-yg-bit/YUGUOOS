#include "timer.h"
#include "hal.h"
#include "serial.h"

#define PIT_CHANNEL0  0x40
#define PIT_CHANNEL1  0x41
#define PIT_CHANNEL2  0x42
#define PIT_COMMAND   0x43

volatile uint64_t tick_count = 0;
static uint32_t   ms_per_tick = 0;
static uint32_t   frequency = 0;

void timer_init(uint32_t freq_hz)
{
    frequency = freq_hz;

    if (freq_hz == 0) {
        freq_hz = 100;
        frequency = 100;
    }

    uint32_t divisor = PIT_BASE_FREQ / freq_hz;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;

    ms_per_tick = 1000 / freq_hz;
    if (ms_per_tick == 0) ms_per_tick = 1;

    outb(PIT_COMMAND, 0x36);
    io_wait();
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    io_wait();
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    tick_count = 0;

    serial_write("[TIMER] PIT 定时器初始化完成 (");
    serial_write_dec(freq_hz);
    serial_write(" Hz, divisor=");
    serial_write_dec(divisor);
    serial_write(")\n");
}

uint64_t timer_get_ticks(void)
{
    return tick_count;
}

uint64_t timer_get_uptime_ms(void)
{
    return tick_count * ms_per_tick;
}

void timer_sleep_ms(uint32_t ms)
{
    uint64_t target = tick_count + (ms + ms_per_tick - 1) / ms_per_tick;
    while (tick_count < target) {
        hlt();
    }
}