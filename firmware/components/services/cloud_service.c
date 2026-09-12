#include "cloud_service.h"
#include "esp_log.h"
#include "network_manager.h"
#include "mqtt_client.h"
#include "ota_manager.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "cloud_service";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            mqtt_connected = true;
            esp_mqtt_client_subscribe(mqtt_client, TOPIC_COMMAND, 0);
            cloud_service_publish_status("online");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            mqtt_connected = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
            
            // Check if it's the command topic
            if (strncmp(event->topic, TOPIC_COMMAND, event->topic_len) == 0) {
                // Parse JSON payload
                cJSON *root = cJSON_ParseWithLength(event->data, event->data_len);
                if (root != NULL) {
                    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
                    if (cmd != NULL && cJSON_IsString(cmd)) {
                        if (strcmp(cmd->valuestring, "ota") == 0) {
                            cJSON *url = cJSON_GetObjectItem(root, "url");
                            if (url != NULL && cJSON_IsString(url)) {
                                ESP_LOGI(TAG, "Received OTA command with URL: %s", url->valuestring);
                                ota_manager_start_update(url->valuestring);
                            } else {
                                ESP_LOGW(TAG, "OTA command missing URL");
                            }
                        }
                    }
                    cJSON_Delete(root);
                }
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

esp_err_t cloud_service_init(void) {
    ESP_LOGI(TAG, "Initializing Cloud Service (MyMosque MQTT)");
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MYMOSQUE_MQTT_URI,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    return ESP_OK;
}

esp_err_t cloud_service_publish_telemetry(const char* payload) {
    if (!mqtt_connected) {
        ESP_LOGW(TAG, "Cannot publish telemetry: MQTT not connected");
        return ESP_FAIL;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, TOPIC_TELEMETRY, payload, 0, 1, 0);
    ESP_LOGI(TAG, "Publishing Telemetry, msg_id=%d", msg_id);
    return ESP_OK;
}

esp_err_t cloud_service_publish_status(const char* status) {
    if (!mqtt_connected) {
        return ESP_FAIL;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, TOPIC_STATUS, status, 0, 1, 1); // Retained
    ESP_LOGI(TAG, "Publishing Status, msg_id=%d", msg_id);
    return ESP_OK;
}
