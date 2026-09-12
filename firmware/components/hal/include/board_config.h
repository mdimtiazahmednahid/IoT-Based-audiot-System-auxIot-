#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "hal/gpio_types.h"

/**
 * @brief Board Configuration for MyMosque Firmware
 * 
 * NOTE: Since the hardware schematic is pending, all GPIOs are defaulted
 * to GPIO_NUM_NC (-1) to safely bypass initialization without crashing.
 * Update these macros once the schematic is finalized.
 */

// --- Buttons ---
#define GPIO_BTN_POWER       (GPIO_NUM_NC)
#define GPIO_BTN_MENU        (GPIO_NUM_NC)
#define GPIO_BTN_VOL_UP      (GPIO_NUM_NC)
#define GPIO_BTN_VOL_DOWN    (GPIO_NUM_NC)
#define GPIO_BTN_MIC_MUTE    (GPIO_NUM_NC)

// --- RGB Status LED ---
#define GPIO_LED_STATUS_R    (GPIO_NUM_NC)
#define GPIO_LED_STATUS_G    (GPIO_NUM_NC)
#define GPIO_LED_STATUS_B    (GPIO_NUM_NC)

// --- Modem (Quectel EG916Q-GL) ---
#define GPIO_MODEM_PWRKEY    (GPIO_NUM_NC)
#define GPIO_MODEM_RESET     (GPIO_NUM_NC)
#define GPIO_MODEM_STATUS    (GPIO_NUM_NC)
#define GPIO_MODEM_TX        (GPIO_NUM_NC)
#define GPIO_MODEM_RX        (GPIO_NUM_NC)

// --- Audio ---
#define GPIO_AUDIO_I2S_BCLK  (GPIO_NUM_NC)
#define GPIO_AUDIO_I2S_WS    (GPIO_NUM_NC)
#define GPIO_AUDIO_I2S_DOUT  (GPIO_NUM_NC)
#define GPIO_AUDIO_I2S_DIN   (GPIO_NUM_NC)

// --- Power / Battery Gauge ---
#define GPIO_I2C_SDA         (GPIO_NUM_NC)
#define GPIO_I2C_SCL         (GPIO_NUM_NC)

// --- IPC Serial (ESP32-S31 to ESP32-P4) ---
#define GPIO_IPC_UART_TX     (GPIO_NUM_NC)
#define GPIO_IPC_UART_RX     (GPIO_NUM_NC)

#endif // BOARD_CONFIG_H
