#include "proc.h"
#include "vmm.h"
#include "pmm.h"
#include "serial.h"
#include "hal.h"
#include "timer.h"

static proc_t *proc_all_list = NULL;
proc_t *current_proc = NULL;
proc_t  idle_proc;
static uint32_t next_pid = 1;

static uint8_t idle_stack[PROC_STACK_SIZE] __attribute__((aligned(16)));

static void idle_thread(void)
{
    for (;;) {
        hlt();
    }
}

void proc_init(void)
{
    idle_proc.pid       = 0;
    idle_proc.state     = PROC_RUNNING;
    idle_proc.priority  = 0;
    idle_proc.ready_next = NULL;
    idle_proc.all_next  = NULL;
    idle_proc.page_table = NULL;
    idle_proc.kernel_stack    = idle_stack;
    idle_proc.kernel_stack_top = idle_stack + PROC_STACK_SIZE;
    idle_proc.exit_code = 0;
    idle_proc.sleep_until_ticks = 0;

    for (int i = 0; i < PROC_NAME_MAX; i++) {
        idle_proc.name[i] = 0;
    }
    idle_proc.name[0] = 'i';
    idle_proc.name[1] = 'd';
    idle_proc.name[2] = 'l';
    idle_proc.name[3] = 'e';

    uint64_t *sp = (uint64_t *)(idle_stack + PROC_STACK_SIZE);
    *(--sp) = (uint64_t)idle_thread;
    *(--sp) = 0x08;
    *(--sp) = 0x202;
    *(--sp) = 0;
    *(--sp) = 0;
    for (int i = 0; i < 15; i++) {
        *(--sp) = 0;
    }
    idle_proc.saved_rsp = (uint64_t)sp;
    for (int i = 0; i < 15; i++) {
        ((uint64_t *)&idle_proc.ctx)[i] = 0;
    }
    idle_proc.ctx.rip    = (uint64_t)idle_thread;
    idle_proc.ctx.cs     = 0x08;
    idle_proc.ctx.rflags = 0x202;

    current_proc = &idle_proc;

    serial_write("[PROC] 进程管理初始化完成\n");
}

proc_t *proc_all_head(void)
{
    return proc_all_list;
}

proc_t *proc_create(void (*entry)(void), const char *name)
{
    proc_t *p = (proc_t *)kmalloc(sizeof(proc_t));
    if (!p) return NULL;

    p->kernel_stack = (uint8_t *)kmalloc(PROC_STACK_SIZE);
    if (!p->kernel_stack) {
        kfree(p);
        return NULL;
    }

    p->pid       = next_pid++;
    p->state     = PROC_READY;
    p->priority  = 1;
    p->page_table = NULL;
    p->kernel_stack_top = p->kernel_stack + PROC_STACK_SIZE;
    p->exit_code = 0;
    p->sleep_until_ticks = 0;
    p->ready_next = NULL;

    for (int i = 0; i < PROC_NAME_MAX; i++) {
        p->name[i] = 0;
    }
    if (name) {
        int i = 0;
        while (i < PROC_NAME_MAX - 1 && name[i]) {
            p->name[i] = name[i];
            i++;
        }
    }

    uint64_t *sp = (uint64_t *)(p->kernel_stack + PROC_STACK_SIZE);

    *(--sp) = (uint64_t)entry;
    *(--sp) = 0x08;
    *(--sp) = 0x202;

    *(--sp) = 0;
    *(--sp) = 0x81;

    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;
    *(--sp) = 0;

    p->saved_rsp = (uint64_t)sp;

    p->ctx.rax    = 0;
    p->ctx.rbx    = 0;
    p->ctx.rcx    = 0;
    p->ctx.rdx    = 0;
    p->ctx.rsi    = 0;
    p->ctx.rdi    = 0;
    p->ctx.rbp    = 0;
    p->ctx.r8     = 0;
    p->ctx.r9     = 0;
    p->ctx.r10    = 0;
    p->ctx.r11    = 0;
    p->ctx.r12    = 0;
    p->ctx.r13    = 0;
    p->ctx.r14    = 0;
    p->ctx.r15    = 0;
    p->ctx.rip    = (uint64_t)entry;
    p->ctx.cs     = 0x08;
    p->ctx.rflags = 0x202;

    p->all_next = proc_all_list;
    proc_all_list = p;

    return p;
}

void proc_exit(int code)
{
    if (!current_proc || current_proc == &idle_proc) return;

    current_proc->state = PROC_ZOMBIE;
    current_proc->exit_code = code;

    serial_write("[PROC] 进程 ");
    serial_write(current_proc->name);
    serial_write(" (PID=");
    serial_write_dec(current_proc->pid);
    serial_write(") 退出, 代码=");
    serial_write_dec(code);
    serial_write("\n");

    for (;;) {
        proc_yield();
    }
}

proc_t *proc_get_current(void)
{
    return current_proc;
}

proc_t *proc_get_by_pid(uint32_t pid)
{
    proc_t *p = proc_all_list;
    while (p) {
        if (p->pid == pid) return p;
        p = p->all_next;
    }
    if (idle_proc.pid == pid) return &idle_proc;
    return NULL;
}

uint32_t proc_get_pid(void)
{
    if (current_proc) return current_proc->pid;
    return 0;
}

void proc_block(void)
{
    if (!current_proc || current_proc == &idle_proc) return;
    current_proc->state = PROC_BLOCKED;
    proc_yield();
}

void proc_unblock(proc_t *p)
{
    if (!p) return;
    if (p->state == PROC_BLOCKED || p->state == PROC_SLEEPING) {
        p->state = PROC_READY;
    }
}

void proc_sleep_ms(uint32_t ms)
{
    if (!current_proc || current_proc == &idle_proc) return;

    current_proc->state = PROC_SLEEPING;
    current_proc->sleep_until_ticks = tick_count + ms / 10;
    if (ms < 10) current_proc->sleep_until_ticks++;

    proc_yield();
}

void proc_yield(void)
{
    asm volatile("int $0x81");
}

void proc_print_list(void)
{
    serial_write("[PROC] 进程列表:\n");
    serial_write("  PID=0 name=idle state=running\n");

    proc_t *p = proc_all_list;
    while (p) {
        serial_write("  PID=");
        serial_write_dec(p->pid);
        serial_write(" name=");
        serial_write(p->name);
        serial_write(" state=");
        switch (p->state) {
        case PROC_READY:    serial_write("ready"); break;
        case PROC_RUNNING:  serial_write("running"); break;
        case PROC_BLOCKED:  serial_write("blocked"); break;
        case PROC_SLEEPING: serial_write("sleeping"); break;
        case PROC_ZOMBIE:   serial_write("zombie"); break;
        case PROC_DEAD:     serial_write("dead"); break;
        }
        serial_write(" pri=");
        serial_write_dec(p->priority);
        serial_write("\n");
        p = p->all_next;
    }
}