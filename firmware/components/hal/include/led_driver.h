#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include "esp_err.h"
#include <stdint.h>

esp_err_t led_driver_init(void);
void led_driver_set_color(uint8_t r, uint8_t g, uint8_t b);

#endif // LED_DRIVER_H
