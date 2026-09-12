#ifndef POWER_DRIVER_H
#define POWER_DRIVER_H

#include "esp_err.h"
#include <stdint.h>

esp_err_t power_driver_init(void);
uint8_t power_driver_get_battery_percentage(void);

#endif // POWER_DRIVER_H
