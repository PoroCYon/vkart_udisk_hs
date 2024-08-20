
#include <stdio.h>

#include "debug.h"
#include "util.h"

void hexdump(const void* src_, size_t len) {
	const uint16_t* src = src_;
	for (size_t i = 0; i < len; i += 16) {
		iprintf("%08x:", i);
		size_t todo = len & 15;
		if (len && todo == 0) todo = 16;
		for (size_t j = 0; j < todo; j += 2) {
			iprintf(" %04x", src[(j+i)>>1]);
		}
		iprintf("%s","\r\n");
	}
}

static uint32_t crctable[256];

static uint32_t reflect(uint32_t refl, uint8_t b) {
	uint32_t value = 0;

	for (size_t i = 1; i < (b + 1u); i++) {
		if (refl & 1)
			value |= 1u << (b - i);
		refl >>= 1;
	}

	return value;
}

static void inittable(void) {
	const uint32_t polynomial = 0x04C11DB7;

	for (size_t i = 0; i < 0x100; i++) {
		crctable[i] = reflect(i, 8) << 24;

		for (size_t j = 0; j < 8; j++)
			crctable[i] = (crctable[i] << 1) ^ (crctable[i] & (1u << 31) ? polynomial : 0);

		crctable[i] = reflect(crctable[i],  32);
	}
}

uint32_t crc32(uint32_t start, const void* addr, uint32_t len) {
	static bool inited = false;
	if (!inited) {
		inittable();
		inited = true;
	}

	const uint8_t* data = addr;
	uint32_t crc = start ^ 0xFFFFFFFFu;
	for (; len; --len) {
		crc = (crc >> 8) ^ crctable[(crc & 0xFF) ^ *data];
		++data;
	}

	return (crc ^ 0xFFFFFFFFu);
}

