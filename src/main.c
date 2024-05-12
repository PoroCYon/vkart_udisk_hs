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

#include "debug.h"
#include "vkart_flash.h"

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
	SystemCoreClockUpdate( );
	Delay_Init( );

	USART_Printf_Init( 115200 );

	printf( "SystemClk:%ld\r\n",SystemCoreClock );
	//printf( "ChipID:%08x\r\n", DBGMCU_GetCHIPID() );
	printf( "This program is a Simulate UDisk\r\n" );

	Delay_Ms(10);
	printf("USBD Udisk\r\nStorage Medium: VKART Flash\r\n");
	VKART_Init();

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOE;

	// GPIO E2 Push-Pull
	GPIOE->CFGLR &= ~((uint32_t)0x0F<<(4*2));
	GPIOE->CFGLR |= (uint32_t)(GPIO_Speed_10MHz | GPIO_Mode_Out_PP)<<(4*2);

	while(1) {
		GPIOE->BSHR = (1<<2);    // Turn on GPIO
		Delay_Ms( 1000 );
		GPIOE->BSHR = (1<<(16+2)); // Turn off GPIO
		Delay_Ms( 1000 );
	}
}

