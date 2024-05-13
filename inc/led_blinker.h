
#ifndef LED_BLINKER_H_
#define LED_BLINKER_H_

enum led_interval {
	led_waiting = 2000,
	led_reading = 1000,
	led_writing =  500, // 2 Hz is used as the limit, rule of thumb is
						// that faster may trigger seizures in photosensive people
	led_error = 0, // always on
};

void led_blinker_set(enum led_interval itv);

void led_blinker_init(void);

#endif

