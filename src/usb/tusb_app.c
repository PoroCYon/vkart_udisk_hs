
#include "ch32v30x.h"
#include "tusb.h"
#include "util.h"


__attribute__((/*__interrupt__("WCH-Interrupt-fast"),*/ __naked__))
void USBHS_IRQHandler(void) {
	asm volatile ("call USBHS_IRQHandler_impl; mret");
}

__attribute__((__used__, __noinline__)) void USBHS_IRQHandler_impl(void) {
  tud_int_handler(0);
}

void tusb_app_init(void) {
	CRITICAL_SECTION({
		RCC_USBCLK48MConfig(RCC_USBCLK48MCLKSource_USBPHY);
		RCC_USBHSPLLCLKConfig(RCC_HSBHSPLLCLKSource_HSE);
		RCC_USBHSConfig(RCC_USBPLL_Div2);
		RCC_USBHSPLLCKREFCLKConfig(RCC_USBHSPLLCKREFCLK_4M);
		RCC_USBHSPHYPLLALIVEcmd(ENABLE);
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_USBHS, ENABLE);
	});

	Delay_Ms(2);

	// init device stack on configured roothub port
	tud_init(BOARD_TUD_RHPORT);
}
void tusb_app_task(void) {
	tud_task();
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(char* file, uint32_t line) {
	iprintf("[USB] Wrong parameters value: file %s on line %d\r\n", file, line);
}
#endif /* USE_FULL_ASSERT */

