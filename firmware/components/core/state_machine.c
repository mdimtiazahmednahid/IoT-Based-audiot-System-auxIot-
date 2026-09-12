#include "state_machine.h"
#include "event_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Services
#include "network_manager.h"
#include "cloud_service.h"
#include "audio_service.h"
#include "display_service.h"

// HAL
#include "button_driver.h"
#include "modem_driver.h"
#include "power_driver.h"
#include "led_driver.h"

// Core
#include "failsafe_manager.h"

static const char *TAG = "state_machine";
static auxiot_system_state_t current_state = STATE_BOOT;

static void system_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    if (base != AUXIOT_SYSTEM_EVENTS) return;

    switch (id) {
        case SYSTEM_EVENT_WIFI_CONNECTED:
            ESP_LOGI(TAG, "Event: Wi-Fi Connected");
            if (current_state == STATE_NETWORK_INIT) {
                current_state = STATE_READY;
                ESP_LOGI(TAG, "Transitioned to STATE_READY");
            }
            break;
        case SYSTEM_EVENT_BATTERY_LOW:
            ESP_LOGW(TAG, "Event: Battery Low");
            current_state = STATE_LOW_BATTERY;
            break;
        case SYSTEM_EVENT_ERROR:
            ESP_LOGE(TAG, "Event: System Error");
            current_state = STATE_ERROR;
            break;
        default:
            ESP_LOGD(TAG, "Unhandled system event: %ld", id);
            break;
    }
}

static void state_machine_task(void *pvParameters) {
    while (1) {
        switch (current_state) {
            case STATE_BOOT:
                ESP_LOGI(TAG, "State: BOOT");
                // Initialize Failsafe Manager early to check for factory reset button
                failsafe_manager_init();
                current_state = STATE_HARDWARE_INIT;
                break;
            case STATE_HARDWARE_INIT:
                ESP_LOGI(TAG, "State: HARDWARE_INIT");
                // Initialize HAL drivers
                button_driver_init();
                modem_driver_init();
                power_driver_init();
                led_driver_init();

                // Initialize Services
                audio_service_init();
                display_service_init();
                network_manager_init();
                cloud_service_init();

                current_state = STATE_SELF_TEST;
                break;
            case STATE_SELF_TEST:
                ESP_LOGI(TAG, "State: SELF_TEST");
                // TODO: Perform self test
                current_state = STATE_NETWORK_INIT;
                break;
            case STATE_NETWORK_INIT:
                // Waiting for network connection
                break;
            case STATE_READY:
                // Idle state, waiting for user interaction or cloud commands
                break;
            case STATE_ERROR:
                ESP_LOGE(TAG, "State: ERROR - Halting");
                vTaskSuspend(NULL);
                break;
            default:
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

esp_err_t state_machine_init(void) {
    current_state = STATE_BOOT;
    esp_event_handler_instance_register(AUXIOT_SYSTEM_EVENTS, ESP_EVENT_ANY_ID, system_event_handler, NULL, NULL);
    return ESP_OK;
}

esp_err_t state_machine_start(void) {
    xTaskCreate(state_machine_task, "state_machine_task", 4096, NULL, 5, NULL);
    return ESP_OK;
}

auxiot_system_state_t state_machine_get_current_state(void) {
    return current_state;
}
