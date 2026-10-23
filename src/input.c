#include "input.h"
#include "vmm.h"

#define INPUT_MAX_EVENTS 64

static input_event_t event_queue[INPUT_MAX_EVENTS];
static uint32_t ev_head = 0;
static uint32_t ev_tail = 0;

void input_init(void)
{
    ev_head = 0;
    ev_tail = 0;
    for (int i = 0; i < INPUT_MAX_EVENTS; i++) {
        event_queue[i].type = EV_NONE;
        event_queue[i].key = 0;
        event_queue[i].raw_char = 0;
    }
}

bool input_poll(input_event_t *ev)
{
    if (ev_head == ev_tail) return false;
    *ev = event_queue[ev_tail];
    ev_tail = (ev_tail + 1) % INPUT_MAX_EVENTS;
    return true;
}

bool input_has_event(void)
{
    return ev_head != ev_tail;
}

void input_push_key(char c)
{
    uint32_t next = (ev_head + 1) % INPUT_MAX_EVENTS;
    if (next == ev_tail) return;

    event_queue[ev_head].type = EV_KEY_DOWN;
    event_queue[ev_head].key = c;
    event_queue[ev_head].raw_char = c;
    ev_head = next;
}