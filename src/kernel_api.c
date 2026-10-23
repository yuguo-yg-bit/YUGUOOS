#include "kernel_api.h"

int kproc_create(void (*entry)(void), const char *name)
{
    proc_t *p = proc_create(entry, name);
    if (!p) return -1;
    sched_add_proc(p);
    return (int)p->pid;
}

void kproc_exit(int code)
{
    proc_exit(code);
}

int kproc_get_pid(void)
{
    return (int)proc_get_pid();
}

void kproc_yield(void)
{
    proc_yield();
}

void kproc_sleep_ms(uint32_t ms)
{
    proc_sleep_ms(ms);
}

void kproc_print_list(void)
{
    proc_print_list();
}

int kopen(const char *path, int flags)
{
    return vfs_open(path, flags);
}

int kclose(int fd)
{
    return vfs_close(fd);
}

int kread(int fd, void *buf, size_t count)
{
    return vfs_read(fd, buf, count);
}

int kwrite(int fd, const void *buf, size_t count)
{
    return vfs_write(fd, buf, count);
}

uint64_t ktime_ms(void)
{
    return timer_get_uptime_ms();
}

void ksleep_ms(uint32_t ms)
{
    timer_sleep_ms(ms);
}

void kdebug(const char *msg)
{
    serial_write(msg);
}

void kdebug_hex(uint64_t value)
{
    serial_write_hex(value);
}

void kdebug_dec(uint64_t value)
{
    serial_write_dec(value);
}