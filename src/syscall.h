#pragma once
#include <stdint.h>
#include <stddef.h>

#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_ALLOC   5
#define SYS_FREE    6
#define SYS_SLEEP   7
#define SYS_GETPID  8
#define SYS_YIELD   9
#define SYS_TIME    10
#define SYS_DEBUG   11
#define SYS_CREATE  12
#define SYS_MAX     13

void syscall_init(void);
int  syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2,
                      uint64_t arg3, uint64_t arg4, uint64_t arg5);