#include "button_driver.h"
#include "esp_log.h"
#include "event_bus.h"
#include "driver/gpio.h"
#include "board_config.h"

static const char *TAG = "button_driver";

esp_err_t button_driver_init(void) {
    ESP_LOGI(TAG, "Initializing Button Driver");
    
    if (GPIO_BTN_POWER != GPIO_NUM_NC) {
        // gpio_config_t for POWER button
    } else {
        ESP_LOGW(TAG, "GPIO_BTN_POWER not configured (GPIO_NUM_NC). Bypassing initialization.");
    }
    
    return ESP_OK;
}
