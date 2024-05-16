
#ifndef CRITICAL_H_
#define CRITICAL_H_

#include <stdint.h>
#include <stdbool.h>

#include "core_riscv.h"

inline static bool irq_is_enabled(void) {
	register uint32_t mstatus;
	asm volatile("csrr %0, mstatus":"=r"(mstatus));
	return (mstatus & 8) != 0;
}

#define CRITICAL_SECTION(...) do { \
		/*register uint32_t __a = irq_is_enabled();*/ \
		/*asm volatile("csrrci %0, mstatus, 8":"=r"(__a));*/ \
		__disable_irq(); \
		do { __VA_ARGS__; } while (0); \
		if (/*__a|*/1) __enable_irq(); \
	} while (0) \

#endif

