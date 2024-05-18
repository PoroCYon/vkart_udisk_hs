
#ifndef CRITICAL_H_
#define CRITICAL_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "ch32v30x.h" /* IRQn_Type, used in core_riscv.h */
#include "core_riscv.h"

#define NO_ASAN_PRIVATE __attribute__((__no_sanitize_address__))
#define NO_ASAN_PUBLIC __attribute__((__no_sanitize_address__, __externally_visible__, __used__))

#define __get_RA() ({ \
		register uint32_t __ra; \
		asm volatile("mv %0, ra":"=r"(__ra)); \
		(__ra); \
	}) \


inline static bool irq_is_enabled(void) {
	register uint32_t mstatus;
	asm volatile("csrr %0, mstatus":"=r"(mstatus));
	return (mstatus & 8) != 0;
}

#define CRITICAL_SECTION(...) do { \
		/*register bool __a = irq_is_enabled();*/ \
		/*asm volatile("csrrci %0, mstatus, 8":"=r"(__a));*/ \
		__disable_irq(); \
		do { __VA_ARGS__; } while (0); \
		if (/*__a|*/1) __enable_irq(); \
	} while (0) \

void hexdump(const void* src, size_t len);

uint32_t crc32(uint32_t start, const void* addr, uint32_t len);

#endif

