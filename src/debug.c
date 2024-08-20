/********************************** (C) COPYRIGHT  *******************************
* File Name          : debug.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : This file contains all the functions prototypes for UART
*                      Printf , Delay functions.
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* SPDX-License-Identifier: Apache-2.0
*******************************************************************************/
#include <stdbool.h>
#include <stdio.h>
#include "ch32v30x.h"
#include "McuASAN.h"
#include "util.h"

#include "debug.h"

#if DEBUG == DEBUG_UART1
#define USART_DBG USART1
#elif DEBUG == DEBUG_UART2
#define USART_DBG USART2
#elif DEUBG == DEBUG_UART3
#define USART_DBG USART3
#endif

#ifdef DEBUG
NO_ASAN_PUBLIC void uart_init_dbg(void) {
#if DEBUG == DEBUG_UART1
	// pin A9
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1;
	GPIOA->CFGHR &= ~(0xf<<(4*1));
	GPIOA->CFGHR |= (GPIO_Speed_10MHz | GPIO_Mode_AF_PP)<<(4*1);
#elif DEBUG == DEBUG_UART2
	// pin A2
	RCC->APB1PCENR |= RCC_APB1Periph_USART2;
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;
	GPIOA->CFGLR &= ~(0xf<<(4*2));
	GPIOA->CFGLR |= (GPIO_Speed_10MHz | GPIO_Mode_AF_PP)<<(4*2);
#elif DEUBG == DEBUG_UART3
	// pin B10
	RCC->APB1PCENR |= RCC_APB1Periph_USART3;
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;
	GPIOB->CFGHR &= ~(0xf<<(4*2));
	GPIOB->CFGHR |= (GPIO_Speed_10MHz | GPIO_Mode_AF_PP)<<(4*2);
#endif

	USART_DBG->CTLR1 = USART_WordLength_8b | USART_Parity_No | USART_Mode_Tx;
	USART_DBG->CTLR2 = USART_StopBits_1;
	USART_DBG->CTLR3 = USART_HardwareFlowControl_None;
	USART_DBG->BRR = ((SystemCoreClock + DEBUG_UART_BAUDRATE/2) / DEBUG_UART_BAUDRATE);
	USART_DBG->CTLR1 |= 1<<13;//USART_UE;
}

NO_ASAN_PRIVATE static void uart_writebuf(const void* src_, size_t t) {
	const uint8_t* src = src_;
	for (size_t i = 0; i < t; ++i) {
		while (!(USART_DBG->STATR & USART_FLAG_TC))
			;
		USART_DBG->DATAR = src[i];
	}
}
NO_ASAN_PRIVATE static void uart_writestr(const void* src_) {
	for (const uint8_t* src = src_; *src; ++src) {
		while (!(USART_DBG->STATR & USART_FLAG_TC))
			;
		USART_DBG->DATAR = *src;
	}
}
NO_ASAN_PRIVATE static char nyb2hex(uint8_t v) {
	if (v >= 0xa) return v + 'a' - 0xa;
	else return v + '0';
}
NO_ASAN_PRIVATE static void uart_writehex(uint32_t v) {
	char buf[8];
	for (size_t i = 0; i < 8; ++i) {
		buf[7^i] = nyb2hex((v >> 4*i) & 0xf);
	}
	uart_writebuf(buf, 8);
}

#define HANG() do{__builtin_trap();for(;;);__builtin_unreachable();}while(0)

// hooks if using -fsanitize=address -fasan-shadow-offset=number -fsanitize=kernel-address
__attribute__((__noreturn__))
NO_ASAN_PUBLIC void __asan_ReportGenericError(uint32_t ra, const char* typ) {
	uart_writestr("[ASAN] failure, ra=0x");
	/*uart_writehex(ra);
	uart_writestr(" ");
	uart_writestr(typ);*/
	uart_writestr("\r\n");

	HANG();
}
__attribute__((__noreturn__))
NO_ASAN_PUBLIC void __asan_NOT_IMPL(uint32_t ra, const char* typ) {
	uart_writestr("[ASAN] SHOULD NOT HAPPEN; ra=0x");
	/*uart_writehex(ra);
	uart_writestr(" ");
	uart_writestr(typ);*/
	uart_writestr("\r\n");

	HANG();
}
__attribute__((__noreturn__))
NO_ASAN_PUBLIC void __asan_ReportError(uint32_t ra, const char* typ, void *address, size_t size, rw_mode_e mode) {
	uart_writestr("[ASAN] ptr failure: ra=0x");
	uart_writehex(ra);
	uart_writestr(" ptraddr 0x");
	uart_writehex((uint32_t)address);
	/*uart_writestr((mode==kIsRead)?", read, size: ":", write, size: ");
	uart_writehex(size);
	uart_writestr(" ");
	uart_writestr(typ);*/
	uart_writestr("\r\n");

	HANG();
}

NO_ASAN_PUBLIC __attribute__((used)) int _write(int fd, char *buf, int size) {
	uart_writebuf(buf, size);

	return size;
}
#else
#define uart_writebuf(src_, t) do{}while(0)
#define uart_writestr(src_) do{}while(0)
#define uart_writehex(v) do{}while(0)

NO_ASAN_PUBLIC void __asan_ReportGenericError(uint32_t ra, const char* typ) {}
NO_ASAN_PUBLIC void __asan_NOT_IMPL(uint32_t ra, const char* typ) {}
NO_ASAN_PUBLIC void __asan_ReportError(uint32_t ra, const char* typ, void *address, size_t size, rw_mode_e mode) {}

NO_ASAN_PUBLIC __attribute__((used)) int _write(int fd, char *buf, int size) {
	return size;
}
#endif

static uint8_t  p_us = 0;
static uint16_t p_ms = 0;

NO_ASAN_PUBLIC void Delay_Init(void) {
	p_us = SystemCoreClock / 8000000;
	p_ms = (uint16_t)p_us * 1000;
}

NO_ASAN_PUBLIC void Delay_Us(uint32_t n) {
	SysTick->SR &= ~(1 << 0);
	uint32_t i = (uint32_t)n * p_us;

	SysTick->CMP = i;
	SysTick->CTLR |= (1 << 4) | (1 << 5) | (1 << 0);

	while((SysTick->SR & (1 << 0)) != (1 << 0))
		;
	SysTick->CTLR &= ~(1 << 0);
}

NO_ASAN_PUBLIC void Delay_Ms(uint32_t n) {
	SysTick->SR &= ~(1 << 0);
	uint32_t i = (uint32_t)n * p_ms;

	SysTick->CMP = i;
	SysTick->CTLR |= (1 << 4) | (1 << 5) | (1 << 0);

	while((SysTick->SR & (1 << 0)) != (1 << 0))
		;
	SysTick->CTLR &= ~(1 << 0);
}

NO_ASAN_PUBLIC void *_sbrk(ptrdiff_t incr) {
	extern char _end[];
	extern char _heap_end[];
	static char *curbrk = _end;

	if ((curbrk + incr < _end) || (curbrk + incr > _heap_end))
	return NULL - 1;

	curbrk += incr;
	return curbrk - incr;
}

__attribute__((__used__, __noinline__))
NO_ASAN_PUBLIC void hardfault_impl(void) {
	uint32_t v_mepc, v_mcause, v_mtval;

	uart_init_dbg();

	v_mepc = __get_MEPC();
	v_mcause = __get_MCAUSE();
	v_mtval = __get_MTVAL();

	uart_writestr("hardfault\r\n");

	uart_writestr("mepc: ");
	uart_writehex(v_mepc);
	uart_writestr("\r\nmcause: ");
	uart_writehex(v_mcause);
	uart_writestr("\r\nmtval: ");
	uart_writehex(v_mtval);
	uart_writestr("\r\n");

	while(1);
}

__attribute__((/*__section__(".highcode"),*/ __naked__))
NO_ASAN_PUBLIC void HardFault_Handler(void) {
	asm volatile("call hardfault_impl; mret");
}

