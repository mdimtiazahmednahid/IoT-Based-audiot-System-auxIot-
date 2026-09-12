#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "event_bus.h"
#include "state_machine.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting AuxIoT Smart Audio Firmware");

    // Initialize core systems
    event_bus_init();
    state_machine_init();

    // Start state machine loop
    state_machine_start();

    while (1) {
        // Main loop can yield or sleep, as the state machine and services 
        // will be driven by FreeRTOS tasks/events.
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
