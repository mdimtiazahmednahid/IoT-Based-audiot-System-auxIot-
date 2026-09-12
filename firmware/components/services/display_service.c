#include "display_service.h"
#include "esp_log.h"
#include "board_config.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include <string.h>

static const char *TAG = "display_service";
#define IPC_UART_PORT UART_NUM_1
#define IPC_BUF_SIZE  1024

static void display_rx_task(void *arg) {
    uint8_t *data = (uint8_t *) malloc(IPC_BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(IPC_UART_PORT, data, IPC_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "RX from P4: %s", (char *)data);
            // TODO: Parse incoming commands from ESP32-P4
        }
    }
    free(data);
}

esp_err_t display_service_init(void) {
    ESP_LOGI(TAG, "Initializing Display Service IPC (UART)");

    if (GPIO_IPC_UART_TX == GPIO_NUM_NC || GPIO_IPC_UART_RX == GPIO_NUM_NC) {
        ESP_LOGW(TAG, "IPC UART pins not configured. Bypassing Display IPC init.");
        return ESP_OK;
    }

    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(IPC_UART_PORT, IPC_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(IPC_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(IPC_UART_PORT, GPIO_IPC_UART_TX, GPIO_IPC_UART_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    xTaskCreate(display_rx_task, "display_rx_task", 4096, NULL, 5, NULL);

    return ESP_OK;
}

esp_err_t display_service_send_command(uint8_t cmd_id, const uint8_t *payload, size_t len) {
    if (GPIO_IPC_UART_TX == GPIO_NUM_NC) return ESP_FAIL;

    // We'll wrap the raw payload in a JSON envelope for the UI
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "cmd", cmd_id);
    
    // In a full implementation, you would convert the payload to hex or structured fields
    // Here we just send a generic command event for demonstration
    cJSON_AddStringToObject(root, "msg", "UI Update");

    char *json_str = cJSON_PrintUnformatted(root);
    
    // Send string + newline
    uart_write_bytes(IPC_UART_PORT, json_str, strlen(json_str));
    uart_write_bytes(IPC_UART_PORT, "\n", 1);
    
    ESP_LOGI(TAG, "TX to P4: %s", json_str);
    
    free(json_str);
    cJSON_Delete(root);

    return ESP_OK;
}
