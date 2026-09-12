#include "event_bus.h"
#include "esp_log.h"

static const char *TAG = "event_bus";

ESP_EVENT_DEFINE_BASE(AUXIOT_SYSTEM_EVENTS);

esp_err_t event_bus_init(void) {
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Event bus initialized");
    return ESP_OK;
}

esp_err_t event_bus_post(int32_t event_id, void* event_data, size_t event_data_size) {
    return esp_event_post(AUXIOT_SYSTEM_EVENTS, event_id, event_data, event_data_size, portMAX_DELAY);
}
