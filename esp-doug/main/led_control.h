#ifndef LED_CONTROL_H
#define LED_CONTROL_H
#include <stdint.h> 
#include "esp_event.h"

extern int LED_STATE;

void init_led(void);
void set_led_pwm(int duty);

#endif
