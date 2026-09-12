#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include "esp_err.h"

esp_err_t display_service_init(void);
esp_err_t display_service_send_command(uint8_t cmd_id, const uint8_t *payload, size_t len);

#endif // DISPLAY_SERVICE_H
