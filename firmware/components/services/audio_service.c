#include "audio_service.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "audio_service";
static bool mic_muted = false;
static uint8_t current_volume = 50;

esp_err_t audio_service_init(void) {
    ESP_LOGI(TAG, "Initializing Audio Service (I2S Stub)");
    // TODO: Initialize I2S based on Audio Codec datasheet.
    return ESP_OK;
}

void audio_service_toggle_mic(void) {
    mic_muted = !mic_muted;
    ESP_LOGI(TAG, "Microphone is now %s", mic_muted ? "MUTED" : "UNMUTED");
}

void audio_service_set_volume(uint8_t vol) {
    current_volume = vol > 100 ? 100 : vol;
    ESP_LOGI(TAG, "Volume set to %d", current_volume);
}
