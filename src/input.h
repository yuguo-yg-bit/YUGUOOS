#pragma once
#include "types.h"

typedef enum {
    EV_NONE = 0,
    EV_KEY_DOWN,
    EV_KEY_UP,
    EV_MAX,
} event_type_t;

typedef struct {
    event_type_t type;
    char         key;
    char         raw_char;
} input_event_t;

void input_init(void);
bool input_poll(input_event_t *ev);
bool input_has_event(void);
void input_push_key(char c);