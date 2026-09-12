#include "failsafe_manager.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_ota_ops.h"
#include "board_config.h"

static const char *TAG = "failsafe_manager";

void failsafe_manager_factory_reset(void) {
    ESP_LOGW(TAG, "Factory Reset Triggered! Erasing NVS...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_LOGW(TAG, "NVS Erased. Rebooting system.");
    esp_restart();
}

esp_err_t failsafe_manager_init(void) {
    ESP_LOGI(TAG, "Initializing Failsafe Manager");
    
    // Check if Menu button is held down during boot for Factory Reset
    if (GPIO_BTN_MENU != GPIO_NUM_NC) {
        gpio_config_t btn_cfg = {
            .pin_bit_mask = (1ULL << GPIO_BTN_MENU),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&btn_cfg);
        
        // Wait a small amount of time for level to stabilize
        vTaskDelay(pdMS_TO_TICKS(100));
        
        // Assuming button pulls to GND when pressed
        if (gpio_get_level(GPIO_BTN_MENU) == 0) {
            ESP_LOGW(TAG, "Menu button held at boot. Checking for 5 seconds to trigger reset...");
            int hold_count = 0;
            while(gpio_get_level(GPIO_BTN_MENU) == 0) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                hold_count++;
                ESP_LOGW(TAG, "Holding... %d sec", hold_count);
                if (hold_count >= 5) {
                    failsafe_manager_factory_reset();
                }
            }
            ESP_LOGI(TAG, "Menu button released before factory reset threshold.");
        }
    }
    
    // Mark OTA app as valid so we don't rollback if this boot is healthy
    esp_ota_mark_app_valid_cancel_rollback();
    
    return ESP_OK;
}
