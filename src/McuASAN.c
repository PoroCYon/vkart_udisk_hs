/*
 * Copyright (c) 2021, Erich Styger
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "McuASANconfig.h"
#if McuASAN_CONFIG_IS_ENABLED
#include "debug.h"
#include "McuASAN.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>


// defined in linker script
extern char _heap_end[], _eusrstack[];
extern char _data_vma[];
extern char _end[];


// below are the required callbacks needed by ASAN
NO_ASAN_PUBLIC void __asan_report_store1(void *address) {
	__asan_ReportGenericError(__get_RA(), __func__+14);
}
NO_ASAN_PUBLIC void __asan_report_store2(void *address) {
	__asan_ReportGenericError(__get_RA(), __func__+14);
}
NO_ASAN_PUBLIC void __asan_report_store4(void *address) {
	__asan_ReportGenericError(__get_RA(), __func__+14);
}
NO_ASAN_PUBLIC void __asan_report_store_n(void *address, int n) {
	__asan_ReportGenericError(__get_RA(), __func__+14);
}
NO_ASAN_PUBLIC void __asan_report_load1(void *address) {
	__asan_ReportGenericError(__get_RA(), __func__+14);
}
NO_ASAN_PUBLIC void __asan_report_load2(void *address) {
	__asan_ReportGenericError(__get_RA(), __func__+14);
}
NO_ASAN_PUBLIC void __asan_report_load4(void *address) {
	__asan_ReportGenericError(__get_RA(), __func__+14);
}
NO_ASAN_PUBLIC void __asan_report_load_n(void *address, int n) {
	__asan_ReportGenericError(__get_RA(), __func__+14);
}

NO_ASAN_PUBLIC void __asan_stack_malloc_1(size_t size, void *addr) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}
NO_ASAN_PUBLIC void __asan_stack_malloc_2(size_t size, void *addr) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}
NO_ASAN_PUBLIC void __asan_stack_malloc_3(size_t size, void *addr) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}
NO_ASAN_PUBLIC void __asan_stack_malloc_4(size_t size, void *addr) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}
NO_ASAN_PUBLIC void __asan_handle_no_return(void) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}
NO_ASAN_PUBLIC void __asan_option_detect_stack_use_after_return(void) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}

NO_ASAN_PUBLIC void __asan_register_globals(void* a, int b) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}
NO_ASAN_PUBLIC void __asan_unregister_globals(void* a, int b) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}
NO_ASAN_PUBLIC void __asan_version_mismatch_check_v8(void) {
	__asan_NOT_IMPL(__get_RA(), __func__+7);
}

/* see https://github.com/gcc-mirror/gcc/blob/master/libsanitizer/asan/asan_interface_internal.h */
static uint8_t shadow[McuASAN_CONFIG_APP_MEM_SIZE/8]; /* one shadow byte for 8 application memory bytes. A 1 means that the memory address is poisoned */

#if McuASAN_CONFIG_FREE_QUARANTINE_LIST_SIZE > 0
static void *freeQuarantineList[McuASAN_CONFIG_FREE_QUARANTINE_LIST_SIZE];
/*!< list of free'd blocks in quarantine */
static int freeQuarantineListIdx; /* index in list (ring buffer), points to free element in list */
#endif

NO_ASAN_PRIVATE static uint8_t* mem_to_shadow(const void* address) {
	address -= McuASAN_CONFIG_APP_MEM_START;
	return shadow + (((uintptr_t)address) >> 3); /* divided by 8: every byte has a shadow bit */
}
NO_ASAN_PRIVATE static void poison_shadow_byte_addr(const void* addr) {
	if ((uintptr_t)addr >= (uintptr_t)McuASAN_CONFIG_APP_MEM_START
			&& (uintptr_t)addr < (uintptr_t)(McuASAN_CONFIG_APP_MEM_START + McuASAN_CONFIG_APP_MEM_SIZE)) {
		*mem_to_shadow(addr) |= 1<<((uintptr_t)addr&7); /* mark memory in shadow as poisoned with shadow bit */
	}
}

NO_ASAN_PRIVATE static void clear_shadow_byte_addr(const void* addr) {
	if ((uintptr_t)addr >= (uintptr_t)McuASAN_CONFIG_APP_MEM_START
			&& (uintptr_t)addr < (uintptr_t)(McuASAN_CONFIG_APP_MEM_START + McuASAN_CONFIG_APP_MEM_SIZE)) {
		*mem_to_shadow(addr) &= ~(1<<((uintptr_t)addr&7)); /* clear shadow bit: it is a valid memory */
	}
}

NO_ASAN_PRIVATE static bool slow_path_check(int8_t shadow_value, const void* address, size_t kAccessSize) {
	/* return true if access to address is poisoned */
	int8_t last_accessed_byte = (((uintptr_t)address) & 7) + kAccessSize - 1;
	return (last_accessed_byte >= shadow_value);
}

NO_ASAN_PRIVATE static void check_shadow(uint32_t ra, const char* typ,
		const void* address, size_t kAccessSize, rw_mode_e mode) {

	if ((uintptr_t)address >= (uintptr_t)_heap_end && (uintptr_t)address <= (uintptr_t)_eusrstack)
		return; // stack address -> fine, prolly (let's hope)

	if ((uintptr_t)address >= (uintptr_t)McuASAN_CONFIG_APP_MEM_START
			&& address < (uintptr_t)(McuASAN_CONFIG_APP_MEM_START + McuASAN_CONFIG_APP_MEM_SIZE)) {
		int8_t *shadow_address;
		int8_t shadow_value;

		shadow_address = (int8_t*)mem_to_shadow(address);
		shadow_value = *shadow_address;
		if (shadow_value == -1) {
			__asan_ReportError(ra, typ, address, kAccessSize, mode);
		} else if (shadow_value != 0) { /* fast check: poisoned! */
			if (slow_path_check(shadow_value, address, kAccessSize)) {
				__asan_ReportError(ra|0x80000000, typ, address, kAccessSize, mode);
			}
		}
	}
}

NO_ASAN_PUBLIC void __asan_load1_noabort(const void* address) {
	CheckShadow(__get_RA(), __func__+7, address, 1, kIsRead);
}
NO_ASAN_PUBLIC void __asan_load2_noabort(const void* address) {
	CheckShadow(__get_RA(), __func__+7, address, 2, kIsRead);
}
NO_ASAN_PUBLIC void __asan_load4_noabort(const void* address) {
	CheckShadow(__get_RA(), __func__+7, address, 4, kIsRead);
}
NO_ASAN_PUBLIC void __asan_load8_noabort(const void* address) {
	CheckShadow(__get_RA(), __func__+7, address, 8, kIsRead);
}
NO_ASAN_PUBLIC void __asan_loadN_noabort(const void* address, size_t size) {
	CheckShadow(__get_RA(), __func__+7, address, size, kIsRead);
}

NO_ASAN_PUBLIC void __asan_store1_noabort(const void* address) {
	CheckShadow(__get_RA(), __func__+7, address, 1, kIsWrite);
}
NO_ASAN_PUBLIC void __asan_store2_noabort(const void* address) {
	CheckShadow(__get_RA(), __func__+7, address, 2, kIsWrite);
}
NO_ASAN_PUBLIC void __asan_store4_noabort(const void* address) {
	CheckShadow(__get_RA(), __func__+7, address, 4, kIsWrite);
}
NO_ASAN_PUBLIC void __asan_store8_noabort(const void* address) {
	CheckShadow(__get_RA(), __func__+7, address, 8, kIsWrite);
}
NO_ASAN_PUBLIC void __asan_storeN_noabort(const void* address, size_t size) {
	CheckShadow(__get_RA(), __func__+7, address, size, kIsWrite);
}


#if McuASAN_CONFIG_CHECK_MALLOC_FREE
/* undo possible defines for malloc and free */
#ifdef malloc
	#undef malloc
	void *malloc(size_t);
#endif
#ifdef free
	#undef free
	void free(void*);
#endif
/*
 * rrrrrrrr	red zone border (incl. size below)
 * size
 * memory returned
 * rrrrrrrr	red zone boarder
 */

NO_ASAN_PUBLIC void *__asan_malloc(size_t size) {
	/* malloc allocates the requested amount of memory with redzones around it.
	 * The shadow values corresponding to the redzones are poisoned and the shadow values
	 * for the memory region are cleared.
	 */
	void* p = malloc(size+2*McuASAN_CONFIG_MALLOC_RED_ZONE_BORDER); /* add size_t for the size of the block */
	uint8_t* q;

	q = p;
	/* poison red zone at the beginning */
	for (size_t i = 0; i < McuASAN_CONFIG_MALLOC_RED_ZONE_BORDER; ++i, ++q) {
		poison_shadow_byte_addr(q);
	}
	*((size_t*)(q-sizeof(size_t))) = size; /* store memory size, needed for the free() part */

	/* clear valid memory */
	for (size_t i = 0; i < size; ++i, ++q) {
		clear_shadow_byte_addr(q);
	}
	/* poison red zone at the end */
	for (size_t i = 0; i < McuASAN_CONFIG_MALLOC_RED_ZONE_BORDER; ++i, ++q) {
		poison_shadow_byte_addr(q);
	}
	return p + McuASAN_CONFIG_MALLOC_RED_ZONE_BORDER; /* return pointer to valid memory */
}

NO_ASAN_PUBLIC void __asan_free(void *p) {
	/* Poisons shadow values for the entire region and put the chunk of memory into a quarantine queue
	 * (such that this chunk will not be returned again by malloc during some period of time).
	 */
	size_t size = *((size_t*)(p-sizeof(size_t))); /* get size */
	uint8_t *q = p;

	for (size_t i = 0; i < size; ++i, ++q) {
		poison_shadow_byte_addr(q);
	}
	q = p-McuASAN_CONFIG_MALLOC_RED_ZONE_BORDER; /* calculate beginning of malloc()ed block */
#if McuASAN_CONFIG_FREE_QUARANTINE_LIST_SIZE > 0
	/* put the memory block into quarantine */
	freeQuarantineList[freeQuarantineListIdx] = q;
	freeQuarantineListIdx++;
	if (freeQuarantineListIdx >= McuASAN_CONFIG_FREE_QUARANTINE_LIST_SIZE) {
		freeQuarantineListIdx = 0;
	}
	if (freeQuarantineList[freeQuarantineListIdx] != NULL) {
		free(freeQuarantineList[freeQuarantineListIdx]);
		freeQuarantineList[freeQuarantineListIdx] = NULL;
	}
#else
	free(q); /* free block */
#endif
}
#endif /* McuASAN_CONFIG_CHECK_MALLOC_FREE */


NO_ASAN_PUBLIC void McuASAN_Init(void) {
	for (size_t i = 0; i < sizeof(shadow); i++) { /* initialize full shadow map */
		shadow[i] = -1; /* poison everything  */
	}
	// mark .data and .bss as initialized/nonpoisoned
	for (uintptr_t i = (uintptr_t)_data_vma; i < (uintptr_t)_end; ++i) {
		clear_shadow_byte_addr((const void*)i);
	}
	/* because the shadow is part of the memory area: poison the shadow */
	for (size_t i = 0; i < sizeof(shadow); i += 8) {
		poison_shadow_byte_addr(&shadow[i]);
	}
#if McuASAN_CONFIG_FREE_QUARANTINE_LIST_SIZE > 0
	for (size_t i = 0; i < McuASAN_CONFIG_FREE_QUARANTINE_LIST_SIZE; i++) {
		freeQuarantineList[i] = NULL;
	}
	freeQuarantineListIdx = 0;
#endif
}

#endif /* McuASAN_CONFIG_IS_ENABLED */

