#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "esp_err.h"
#include "esp_event.h"

// Define the base event base for AuxIoT
ESP_EVENT_DECLARE_BASE(AUXIOT_SYSTEM_EVENTS);

// System Event IDs
typedef enum {
    SYSTEM_EVENT_BOOT_COMPLETE,
    SYSTEM_EVENT_WIFI_CONNECTED,
    SYSTEM_EVENT_WIFI_DISCONNECTED,
    SYSTEM_EVENT_LTE_CONNECTED,
    SYSTEM_EVENT_LTE_DISCONNECTED,
    SYSTEM_EVENT_BATTERY_LOW,
    SYSTEM_EVENT_BUTTON_PRESSED,
    SYSTEM_EVENT_SHUTDOWN,
    SYSTEM_EVENT_ERROR
} auxiot_system_event_id_t;

/**
 * @brief Initialize the central event bus
 */
esp_err_t event_bus_init(void);

/**
 * @brief Post an event to the system event bus
 */
esp_err_t event_bus_post(int32_t event_id, void* event_data, size_t event_data_size);

#endif // EVENT_BUS_H
