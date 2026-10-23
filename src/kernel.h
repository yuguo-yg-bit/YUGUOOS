#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define YUGUOOS_VERSION "0.3.0"

void kernel_main(uint32_t magic, uint32_t multiboot_addr);