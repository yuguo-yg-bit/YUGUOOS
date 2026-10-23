#include "sched.h"
#include "proc.h"
#include "timer.h"
#include "irq.h"
#include "idt.h"
#include "hal.h"
#include "serial.h"
#include <stdbool.h>
#include <stddef.h>

proc_t *sched_next_proc = NULL;

static proc_t *ready_queue = NULL;
static bool    sched_enabled = false;

static void sched_yield_isr(regs_t *regs)
{
    (void)regs;
    sched_schedule(regs);
}

static void sched_timer_isr(regs_t *regs)
{
    tick_count++;
    sched_schedule(regs);
}

void sched_init(void)
{
    ready_queue = NULL;
    sched_enabled = false;
    sched_next_proc = NULL;

    isr_register_handler(0x81, sched_yield_isr);

    serial_write("[SCHED] 调度器初始化完成 (Round Robin)\n");
}

void sched_start(void)
{
    sched_enabled = true;

    irq_register_handler(IRQ_TIMER, sched_timer_isr);
    irq_enable(IRQ_TIMER);

    serial_write("[SCHED] 调度器已启动\n");
}

void sched_add_proc(proc_t *p)
{
    if (!p) return;

    p->ready_next = NULL;

    if (!ready_queue) {
        ready_queue = p;
    } else {
        proc_t *cur = ready_queue;
        while (cur->ready_next) {
            cur = cur->ready_next;
        }
        cur->ready_next = p;
    }
}

void sched_remove_proc(proc_t *p)
{
    if (!p || !ready_queue) return;

    if (ready_queue == p) {
        ready_queue = p->ready_next;
        p->ready_next = NULL;
        return;
    }

    proc_t *cur = ready_queue;
    while (cur->ready_next) {
        if (cur->ready_next == p) {
            cur->ready_next = p->ready_next;
            p->ready_next = NULL;
            return;
        }
        cur = cur->ready_next;
    }
}

void sched_tick(void)
{
}

void sched_wake_sleepers(uint64_t now_ticks)
{
    proc_t *p = ready_queue;
    while (p) {
        if (p->state == PROC_SLEEPING && now_ticks >= p->sleep_until_ticks) {
            p->state = PROC_READY;
        }
        p = p->ready_next;
    }

    p = proc_all_head();
    while (p) {
        if (p->state == PROC_SLEEPING && now_ticks >= p->sleep_until_ticks) {
            p->state = PROC_READY;
            sched_add_proc(p);
        }
        p = p->all_next;
    }
}

void sched_schedule(regs_t *regs)
{
    if (!sched_enabled) return;

    proc_t *old = current_proc;
    if (!old) return;

    if (old->state == PROC_RUNNING && old != &idle_proc) {
        old->ctx.r15 = regs->r15;
        old->ctx.r14 = regs->r14;
        old->ctx.r13 = regs->r13;
        old->ctx.r12 = regs->r12;
        old->ctx.r11 = regs->r11;
        old->ctx.r10 = regs->r10;
        old->ctx.r9  = regs->r9;
        old->ctx.r8  = regs->r8;
        old->ctx.rdi = regs->rdi;
        old->ctx.rsi = regs->rsi;
        old->ctx.rbp = regs->rbp;
        old->ctx.rbx = regs->rbx;
        old->ctx.rdx = regs->rdx;
        old->ctx.rcx = regs->rcx;
        old->ctx.rax = regs->rax;
        old->ctx.rip = regs->rip;
        old->ctx.cs  = regs->cs;
        old->ctx.rflags = regs->rflags;

        old->saved_rsp = (uint64_t)regs;
        old->state = PROC_READY;
        sched_add_proc(old);
    }

    sched_wake_sleepers(tick_count);

    proc_t *next = &idle_proc;
    proc_t *ready_cur = ready_queue;

    while (ready_cur) {
        if (ready_cur->state == PROC_READY) {
            next = ready_cur;
            ready_queue = ready_cur->ready_next;
            next->ready_next = NULL;
            break;
        }
        ready_cur = ready_cur->ready_next;
    }

    next->state = PROC_RUNNING;
    current_proc = next;

    if (old == next) {
        sched_next_proc = NULL;
        return;
    }

    uint64_t *dst = (uint64_t *)next->saved_rsp;
    dst[0]  = next->ctx.rax;
    dst[1]  = next->ctx.rcx;
    dst[2]  = next->ctx.rdx;
    dst[3]  = next->ctx.rbx;
    dst[4]  = next->ctx.rbp;
    dst[5]  = next->ctx.rsi;
    dst[6]  = next->ctx.rdi;
    dst[7]  = next->ctx.r8;
    dst[8]  = next->ctx.r9;
    dst[9]  = next->ctx.r10;
    dst[10] = next->ctx.r11;
    dst[11] = next->ctx.r12;
    dst[12] = next->ctx.r13;
    dst[13] = next->ctx.r14;
    dst[14] = next->ctx.r15;
    dst[15] = regs->int_no;
    dst[16] = regs->err_code;
    dst[17] = next->ctx.rip;
    dst[18] = next->ctx.cs;
    dst[19] = next->ctx.rflags;

    sched_next_proc = next;
}