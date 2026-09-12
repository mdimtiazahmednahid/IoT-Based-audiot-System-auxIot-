#ifndef FAILSAFE_MANAGER_H
#define FAILSAFE_MANAGER_H

#include "esp_err.h"

/**
 * @brief Initialize the failsafe manager
 * Checks the factory reset button state on boot and initializes the TWDT.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t failsafe_manager_init(void);

/**
 * @brief Manually trigger a factory reset (erases NVS and reboots)
 */
void failsafe_manager_factory_reset(void);

#endif // FAILSAFE_MANAGER_H
