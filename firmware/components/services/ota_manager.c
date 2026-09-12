#include "ota_manager.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ota_manager";

static void ota_task(void *pvParameter) {
    const char *url = (const char *)pvParameter;
    ESP_LOGI(TAG, "Starting OTA from URL: %s", url);

    esp_http_client_config_t config = {
        .url = url,
        .cert_pem = NULL, // In production, provide the server certificate
        .skip_cert_common_name_check = true, // For development only
    };

    esp_err_t ret = esp_https_ota(&config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA Succeed, Rebooting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed...");
    }
    
    // Free the copied URL
    free((void*)url);
    vTaskDelete(NULL);
}

esp_err_t ota_manager_init(void) {
    ESP_LOGI(TAG, "Initializing OTA Manager");
    return ESP_OK;
}

esp_err_t ota_manager_start_update(const char* url) {
    if (url == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy the URL to heap so it survives the task creation
    char *url_copy = strdup(url);
    if (url_copy == NULL) {
        return ESP_ERR_NO_MEM;
    }
    
    xTaskCreate(&ota_task, "ota_task", 8192, (void *)url_copy, 5, NULL);
    return ESP_OK;
}
