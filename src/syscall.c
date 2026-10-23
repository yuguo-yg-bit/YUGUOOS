#include "syscall.h"
#include "idt.h"
#include "proc.h"
#include "sched.h"
#include "vmm.h"
#include "vfs.h"
#include "timer.h"
#include "serial.h"
#include "hal.h"
#include <stdbool.h>

typedef int (*syscall_fn_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

static syscall_fn_t syscall_table[SYS_MAX];

static int sys_exit(uint64_t code, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    proc_exit((int)code);
    return 0;
}

static int sys_write(uint64_t fd, uint64_t buf_ptr, uint64_t count, uint64_t a4, uint64_t a5)
{
    (void)a4; (void)a5;
    if (fd == 1 || fd == 2) {
        const char *str = (const char *)(uintptr_t)buf_ptr;
        for (uint64_t i = 0; i < count && str[i]; i++) {
            serial_putchar(str[i]);
        }
        return (int)count;
    }
    return vfs_write((int)fd, (const void *)(uintptr_t)buf_ptr, (size_t)count);
}

static int sys_read(uint64_t fd, uint64_t buf_ptr, uint64_t count, uint64_t a4, uint64_t a5)
{
    (void)a4; (void)a5;
    return vfs_read((int)fd, (void *)(uintptr_t)buf_ptr, (size_t)count);
}

static int sys_open(uint64_t path_ptr, uint64_t flags, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a3; (void)a4; (void)a5;
    return vfs_open((const char *)(uintptr_t)path_ptr, (int)flags);
}

static int sys_close(uint64_t fd, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    return vfs_close((int)fd);
}

static int sys_alloc(uint64_t size, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    void *ptr = kmalloc((size_t)size);
    return (int)(intptr_t)ptr;
}

static int sys_free(uint64_t ptr_val, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    kfree((void *)(uintptr_t)ptr_val);
    return 0;
}

static int sys_sleep(uint64_t ms, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    proc_sleep_ms((uint32_t)ms);
    return 0;
}

static int sys_getpid(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return (int)proc_get_pid();
}

static int sys_yield(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    proc_yield();
    return 0;
}

static int sys_time(uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return (int)timer_get_uptime_ms();
}

static int sys_debug(uint64_t msg_ptr, uint64_t a2, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;
    serial_write((const char *)(uintptr_t)msg_ptr);
    return 0;
}

static int sys_create(uint64_t entry_ptr, uint64_t name_ptr, uint64_t a3, uint64_t a4, uint64_t a5)
{
    (void)a3; (void)a4; (void)a5;
    proc_t *p = proc_create((void (*)(void))(uintptr_t)entry_ptr,
                            (const char *)(uintptr_t)name_ptr);
    if (!p) return -1;
    sched_add_proc(p);
    return (int)p->pid;
}

static void syscall_handler(regs_t *regs)
{
    uint64_t num  = regs->rax;
    uint64_t arg1 = regs->rdi;
    uint64_t arg2 = regs->rsi;
    uint64_t arg3 = regs->rdx;
    uint64_t arg4 = regs->r10;
    uint64_t arg5 = regs->r8;

    int ret = -1;
    if (num < SYS_MAX && syscall_table[num]) {
        ret = syscall_table[num](arg1, arg2, arg3, arg4, arg5);
    }

    regs->rax = (uint64_t)(int64_t)ret;
}

void syscall_init(void)
{
    for (int i = 0; i < SYS_MAX; i++) {
        syscall_table[i] = NULL;
    }

    syscall_table[SYS_EXIT]   = sys_exit;
    syscall_table[SYS_WRITE]  = sys_write;
    syscall_table[SYS_READ]   = sys_read;
    syscall_table[SYS_OPEN]   = sys_open;
    syscall_table[SYS_CLOSE]  = sys_close;
    syscall_table[SYS_ALLOC]  = sys_alloc;
    syscall_table[SYS_FREE]   = sys_free;
    syscall_table[SYS_SLEEP]  = sys_sleep;
    syscall_table[SYS_GETPID] = sys_getpid;
    syscall_table[SYS_YIELD]  = sys_yield;
    syscall_table[SYS_TIME]   = sys_time;
    syscall_table[SYS_DEBUG]  = sys_debug;
    syscall_table[SYS_CREATE] = sys_create;

    isr_register_handler(0x80, syscall_handler);

    serial_write("[SYSCALL] 系统调用层初始化完成 (");
    serial_write_dec(SYS_MAX);
    serial_write(" 个调用)\n");
}

int syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2,
                      uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    if (num >= SYS_MAX || !syscall_table[num]) return -1;
    return syscall_table[num](arg1, arg2, arg3, arg4, arg5);
}