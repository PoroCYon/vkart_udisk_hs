/********************************** (C) COPYRIGHT *******************************
* File Name          : main.c
* Author             : WCH
* Version            : V1.0.0
* Date               : 2021/06/06
* Description        : Main program body.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/

/* @Note
 * UDisk Example:
 * This program provides examples of UDisk.Supports external SPI Flash and internal
 * Flash, selected by STORAGE_MEDIUM at SW_UDISK.h.
 *  */

#include <stdio.h>

#include "debug.h"
#include "vkart_flash.h"
#include "ch32v30x.h"
#include "led_blinker.h"
#include "tusb_app.h"

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
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

		/*uint16_t rv[512];
		vkart_read_data(0x4000, rv, 512);
		for (size_t i = 0; i < 512; ++i) {
			iprintf("%04x: %04x\r\n", i, rv[i]);
		}*/

		led_blinker_init();
		led_blinker_set(led_error);
		while (1) asm volatile("":::"memory");
	}

	led_blinker_init();
	//tusb_app_init();

	while(1) {
		//tusb_app_task();
		led_blinker_task();
	}
}

