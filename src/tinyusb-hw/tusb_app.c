
#include "tusb.h"
#include "family.h"


void tusb_app_init(void) {
	tusb_family_init();

	// init device stack on configured roothub port
	tud_init(BOARD_TUD_RHPORT);
}
void tusb_app_task(void) {
	tud_task();
}

