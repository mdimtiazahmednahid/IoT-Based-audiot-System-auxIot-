#ifndef CLOUD_SERVICE_H
#define CLOUD_SERVICE_H

#include "esp_err.h"
#include "mqtt_client.h"

// MyMosque Cloud Configuration
#define MYMOSQUE_MQTT_URI "mqtt://mqtt.mymosque.local:1883" // To be configured by user
#define MYMOSQUE_DEVICE_ID "mymosque_audio_001"

#define TOPIC_TELEMETRY "mymosque/" MYMOSQUE_DEVICE_ID "/telemetry"
#define TOPIC_STATUS    "mymosque/" MYMOSQUE_DEVICE_ID "/status"
#define TOPIC_COMMAND   "mymosque/" MYMOSQUE_DEVICE_ID "/commands"

esp_err_t cloud_service_init(void);
esp_err_t cloud_service_publish_telemetry(const char* payload);
esp_err_t cloud_service_publish_status(const char* status);

#endif // CLOUD_SERVICE_H
