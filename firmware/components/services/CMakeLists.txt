idf_component_register(SRCS "network_manager.c" "cloud_service.c" "audio_service.c" "display_service.c" "ota_manager.c"
                       INCLUDE_DIRS "include"
                       REQUIRES core mqtt wifi_provisioning app_update esp_https_ota json)
