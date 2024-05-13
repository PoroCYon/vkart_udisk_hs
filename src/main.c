
#include <stdio.h>

#include "debug.h"
#include "vkart_flash.h"
#include "ch32v30x.h"
#include "led_blinker.h"
#include "tusb_app.h"
#include "tusb.h"

int main(void) {
	SystemCoreClockUpdate();
	Delay_Init();

	USART_Printf_Init(115200);

	//iprintf("SystemClk:%ld\r\n",SystemCoreClock);
	//iprintf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());

	Delay_Ms(10);
	iprintf("USBD Udisk\r\nStorage Medium: VKART Flash\r\n");

	if (!VKART_Init()) {
		iprintf("ERROR: flash detection failed, this is bad!!!\r\n");

		led_blinker_init();
		led_blinker_set(led_error);
		while (1) asm volatile("":::"memory");
	}

	led_blinker_init();
	tusb_app_init();

	while (1) {
		tud_task();//tusb_app_task();
		//led_blinker_task();
	}
}

