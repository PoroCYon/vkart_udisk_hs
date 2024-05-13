
#include <stdint.h>
#include <stdbool.h>

#include "ch32v30x.h"

#include "led_blinker.h"


#define TIMER TIM6 /* BCTM basic timer */

static volatile enum led_interval intv;
static bool state;


void TIM6_IRQHandler(void) {
	asm volatile("call TIM6_IRQHandler_impl; mret");
}
__attribute__ ((used))
void TIM6_IRQHandler_impl(void) {
	NVIC_ClearPendingIRQ(TIM6_IRQn);
	TIMER->INTFR = 0; // ack irq

	// toggle the led state
	GPIOE->BSHR = (1<<2) << (state?16:0);
	state = !state;
}


void led_blinker_set(enum led_interval itv) {
	intv = itv;

	if (itv == led_error) {
		TIMER->CTLR1 = 0;
		GPIOE->BSHR = (1<<2);
		state = true;
	} else {
		TIMER->ATRLR = /*0xffff -*/ intv*16;
		TIMER->CTLR1 = TIM_ARPE | TIM_CEN;
	}
}

void led_blinker_init(void) {
	RCC_ClocksTypeDef clkfreq;

	state = false;

	// GPIO E2 Push-Pull output
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOE;
	GPIOE->CFGLR &= ~((uint32_t)0x0F<<(4*2));
	GPIOE->CFGLR |= (uint32_t)(GPIO_Speed_10MHz | GPIO_Mode_Out_PP)<<(4*2);
	GPIOE->BSHR = (1<<(16+2)); // Turn off GPIO*/

	// enable clock supply
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	RCC_GetClocksFreq(&clkfreq);

	// set up timer
	TIMER->CTLR1 = TIM_ARPE;
	TIMER->CNT = 0;
	TIMER->PSC = clkfreq.PCLK1_Frequency / 1000; // divide down to 1 kHz
	led_blinker_set(led_waiting);

	// set up interrupts
	__disable_irq();
	NVIC_EnableIRQ(TIM6_IRQn);
	TIMER->DMAINTENR = TIM_UIE;
	__enable_irq();

	// start the timer (with autoreload)
	TIMER->CTLR1 = TIM_ARPE | TIM_CEN;
}
void led_blinker_task(void) {
}

