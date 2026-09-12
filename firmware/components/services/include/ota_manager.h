#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "esp_err.h"

/**
 * @brief Initialize the OTA manager
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ota_manager_init(void);

/**
 * @brief Start an OTA update from a given URL
 * 
 * @param url The HTTPS URL of the firmware bin
 * @return esp_err_t ESP_OK if started successfully
 */
esp_err_t ota_manager_start_update(const char* url);

#endif // OTA_MANAGER_H
