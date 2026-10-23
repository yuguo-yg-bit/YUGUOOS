#pragma once
#include "proc.h"
#include "idt.h"

void sched_init(void);
void sched_start(void);
void sched_add_proc(proc_t *p);
void sched_remove_proc(proc_t *p);
void sched_tick(void);
void sched_schedule(regs_t *regs);
void sched_wake_sleepers(uint64_t now_ticks);

extern proc_t *sched_next_proc;