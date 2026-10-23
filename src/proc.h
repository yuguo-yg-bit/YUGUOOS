#pragma once
#include <stdint.h>

#define PROC_NAME_MAX  32
#define PROC_MAX       256
#define PROC_STACK_SIZE 8192

typedef enum {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_SLEEPING,
    PROC_ZOMBIE,
    PROC_DEAD,
} proc_state_t;

typedef struct context {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;
    uint64_t rip, cs, rflags;
} context_t;

typedef struct proc {
    uint32_t        pid;
    char            name[PROC_NAME_MAX];
    proc_state_t    state;
    context_t       ctx;
    uint64_t        saved_rsp;
    uint64_t       *page_table;
    uint8_t        *kernel_stack;
    uint8_t        *kernel_stack_top;
    struct proc    *ready_next;
    struct proc    *all_next;
    uint32_t        priority;
    uint64_t        sleep_until_ticks;
    int             exit_code;
} proc_t;

void     proc_init(void);
proc_t  *proc_create(void (*entry)(void), const char *name);
void     proc_exit(int code);
proc_t  *proc_get_current(void);
proc_t  *proc_get_by_pid(uint32_t pid);
uint32_t proc_get_pid(void);
void     proc_block(void);
void     proc_unblock(proc_t *p);
void     proc_sleep_ms(uint32_t ms);
void     proc_yield(void);
void     proc_print_list(void);

extern proc_t *current_proc;
extern proc_t  idle_proc;
proc_t *proc_all_head(void);