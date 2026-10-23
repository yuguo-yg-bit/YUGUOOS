#pragma once
#include <stdint.h>
#include <stddef.h>

void  vmm_init(void);
void *kmalloc(size_t size);
void *krealloc(void *ptr, size_t new_size);
void  kfree(void *ptr);

void  heap_print_stats(void);