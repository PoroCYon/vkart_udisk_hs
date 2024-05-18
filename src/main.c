
#include <stdio.h>

#include "debug.h"
#include "vkart_flash.h"
#include "ch32v30x.h"
#include "led_blinker.h"
#include "tusb_app.h"
#include "tusb.h"

#include "McuASAN.h"

int main(void) {
	SystemCoreClockUpdate();
	Delay_Init();

	led_blinker_init();
	led_blinker_set(led_error);

	uart_init_dbg();
	Delay_Ms(10);

	iprintf("USBD Udisk\r\nStorage Medium: VKART Flash\r\n");
	//iprintf("SystemClk:%ld\r\n",SystemCoreClock);
	//iprintf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());

#if McuASAN_CONFIG_IS_ENABLED == 1
	McuASAN_Init();
	iprintf("inited ASAN\r\n");
#endif

	if (!vkart_init()) {
		iprintf("ERROR: flash detection failed, this is bad!!!\r\n");

		while (1) asm volatile("":::"memory");
	}

	led_blinker_set(led_waiting);
	tusb_app_init();

	while (1) {
		tusb_app_task();
	}
}

